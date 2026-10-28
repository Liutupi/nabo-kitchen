#include "krc_stream.h"
#include <string.h>

/* Only traversal, not a general JSON validator: KRC1's packer emits canonical
 * JSON. The dictionary and each returned ID are bounds checked, and its ID must
 * match the corresponding KRC index hash. */
static const uint8_t *space(const uint8_t *p, const uint8_t *end) {
  while (p < end && (*p == ' ' || *p == '\n' || *p == '\r' || *p == '\t'))
    ++p;
  return p;
}
static const uint8_t *skip_string(const uint8_t *p, const uint8_t *end) {
  if (p == end || *p++ != '"')
    return NULL;
  while (p < end) {
    uint8_t c = *p++;
    if (c == '"')
      return p;
    if (c < 0x20)
      return NULL;
    if (c == '\\') {
      if (p == end)
        return NULL;
      ++p;
    }
  }
  return NULL;
}
static const uint8_t *skip_value(const uint8_t *p, const uint8_t *end) {
  p = space(p, end);
  if (p == end)
    return NULL;
  if (*p == '"')
    return skip_string(p, end);
  if (*p == '{' || *p == '[') {
    uint8_t stack[32];
    unsigned depth = 0;
    do {
      if (p == end)
        return NULL;
      if (*p == '"') {
        p = skip_string(p, end);
        if (!p)
          return NULL;
        continue;
      }
      if (*p == '{' || *p == '[') {
        if (depth == sizeof(stack))
          return NULL;
        stack[depth++] = *p == '{' ? '}' : ']';
      } else if (*p == '}' || *p == ']') {
        if (!depth || *p != stack[--depth])
          return NULL;
      }
      ++p;
    } while (depth);
    return p;
  }
  const uint8_t *start = p;
  while (p < end && *p != ',' && *p != '}' && *p != ']' && *p != ' ' &&
         *p != '\n' && *p != '\r' && *p != '\t')
    ++p;
  return p == start ? NULL : p;
}
static const uint8_t *find_ids(const uint8_t *p, const uint8_t *end) {
  p = space(p, end);
  if (p == end || *p++ != '{')
    return NULL;
  for (;;) {
    p = space(p, end);
    if (p == end || *p != '"')
      return NULL;
    const uint8_t *key = p;
    p = skip_string(p, end);
    if (!p)
      return NULL;
    int ids = p - key == 5 && !memcmp(key, "\"ids\"", 5);
    p = space(p, end);
    if (p == end || *p++ != ':')
      return NULL;
    p = space(p, end);
    if (ids)
      return p < end && *p == '[' ? p + 1 : NULL;
    p = skip_value(p, end);
    if (!p)
      return NULL;
    p = space(p, end);
    if (p == end || *p++ != ',')
      return NULL;
  }
}
static uint32_t hash_id(const char *id) {
  uint32_t h = UINT32_C(2166136261);
  while (*id) {
    h ^= (uint8_t)*id++;
    h *= UINT32_C(16777619);
  }
  return h;
}
int ksc_open(ksc_cursor *c, const uint8_t *data, size_t size) {
  if (!c)
    return 0;
  memset(c, 0, sizeof(*c));
  if (!krc_open(&c->pack, data, size))
    return 0;
  const uint8_t *start = data + c->pack.dictionary_offset;
  c->dictionary_end = start + c->pack.dictionary_length;
  c->next_id = find_ids(start, c->dictionary_end);
  if (!c->next_id) {
    memset(c, 0, sizeof(*c));
    return 0;
  }
  return 1;
}
int ksc_next(ksc_cursor *c, ksc_row *row) {
  if (!c || !row || !c->pack.data || c->failed)
    return -1;
  if (c->ordinal == c->pack.count)
    return 0;
  const uint8_t *p = space(c->next_id, c->dictionary_end);
  if (p == c->dictionary_end || *p++ != '"')
    goto fail;
  ksc_row r = {0};
  size_t n = 0;
  while (p < c->dictionary_end && *p != '"') {
    uint8_t ch = *p++;
    if (n == sizeof(r.id) - 1 ||
        !((ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') || ch == '-'))
      goto fail;
    if (ch == '-' && (!n || r.id[n - 1] == '-'))
      goto fail;
    r.id[n++] = (char)ch;
  }
  if (!n || r.id[n - 1] == '-' || p == c->dictionary_end)
    goto fail;
  ++p;
  p = space(p, c->dictionary_end);
  uint8_t separator = c->ordinal + 1 == c->pack.count ? ']' : ',';
  if (p == c->dictionary_end || *p++ != separator)
    goto fail;
  krc_index idx;
  if (!krc_index_at(&c->pack, c->ordinal, &idx) || hash_id(r.id) != idx.id_hash)
    goto fail;
  r.ordinal = c->ordinal;
  r.id_hash = idx.id_hash;
  r.tags = idx.tags;
  r.courses = idx.courses;
  r.minutes = idx.minutes;
  r.difficulty = idx.difficulty;
  c->next_id = p;
  ++c->ordinal;
  *row = r;
  return 1;
fail:
  c->failed = 1;
  return -1;
}
int ksc_page(ksc_cursor *c, ksc_filter filter, ksc_row *rows, size_t capacity,
             size_t *count, int *finished) {
  if (!c || !rows || !capacity || !count || !finished ||
      filter.main_ingredient < -1 ||
      (filter.main_ingredient >= 0 &&
       (uint64_t)filter.main_ingredient >= (uint64_t)c->pack.bitmap_width * 8u))
    return -1;
  *count = 0;
  *finished = 0;
  while (*count < capacity) {
    ksc_row row;
    int result = ksc_next(c, &row);
    if (result < 0)
      return -1;
    if (!result) {
      *finished = 1;
      break;
    }
    if (filter.any_course && !(filter.any_course & row.courses))
      continue;
    if (filter.main_ingredient >= 0) {
      krc_index idx;
      if (!krc_index_at(&c->pack, row.ordinal, &idx))
        return -1;
      unsigned bit = (unsigned)filter.main_ingredient;
      if (!(idx.main_bitmap[bit / 8] & (1u << (bit % 8))))
        continue;
    }
    rows[(*count)++] = row;
  }
  if (c->ordinal == c->pack.count)
    *finished = 1;
  return 0;
}
int ksc_find_id(const uint8_t *data, size_t size, const char *id,
                ksc_row *row) {
  if (!id || !row || !*id || strlen(id) > 40)
    return -1;
  uint32_t target = hash_id(id);
  ksc_cursor c;
  if (!ksc_open(&c, data, size))
    return -1;
  int matches = 0, result;
  ksc_row item, found = {0};
  while ((result = ksc_next(&c, &item)) > 0) {
    if (item.id_hash == target) {
      if (strcmp(item.id, id))
        return -1;
      found = item;
      ++matches;
    }
  }
  if (result < 0 || matches > 1)
    return -1;
  if (matches)
    *row = found;
  return matches;
}
int ksc_body(const ksc_cursor *c, uint32_t ordinal, uint8_t *out, size_t cap) {
  return c ? krc_read_body(&c->pack, ordinal, out, cap) : -1;
}
