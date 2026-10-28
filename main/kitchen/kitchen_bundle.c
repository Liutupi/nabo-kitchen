#include "kitchen_bundle.h"
#include <string.h>
#define KB_HEADER 16u
#define KB_ENTRY 16u
#define KB_COUNT 6u
static uint16_t u16(const uint8_t *p) {
  return (uint16_t)(p[0] | (uint16_t)p[1] << 8);
}
static uint32_t u32(const uint8_t *p) {
  return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 |
         (uint32_t)p[3] << 24;
}
static uint32_t crc32(const uint8_t *p, size_t size) {
  uint32_t crc = UINT32_MAX;
  for (size_t i = 0; i < size; ++i) {
    crc ^= p[i];
    for (unsigned bit = 0; bit < 8; ++bit)
      crc = (crc >> 1) ^ (UINT32_C(0xedb88320) & (uint32_t)-(int32_t)(crc & 1));
  }
  return ~crc;
}
bool kb_open(const void *data, size_t size, kb_assets *out) {
  if (!out)
    return false;
  memset(out, 0, sizeof(*out));
  if (!data || size < KB_HEADER + KB_COUNT * KB_ENTRY || size > UINT32_MAX)
    return false;
  const uint8_t *bytes = data;
  if (memcmp(bytes, "KAB1", 4) || u16(bytes + 4) != 1 ||
      u16(bytes + 6) != KB_COUNT || u32(bytes + 8) != size ||
      crc32(bytes + KB_HEADER, size - KB_HEADER) != u32(bytes + 12))
    return false;
  kb_blob blobs[KB_COUNT] = {0};
  size_t end = KB_HEADER + KB_COUNT * KB_ENTRY;
  for (unsigned i = 0; i < KB_COUNT; ++i) {
    const uint8_t *entry = bytes + KB_HEADER + i * KB_ENTRY;
    uint32_t id = u32(entry), offset = u32(entry + 4), length = u32(entry + 8);
    if (id != i + 1 || (offset & 3u) || offset < end || !length ||
        offset > size || length > size - offset ||
        crc32(bytes + offset, length) != u32(entry + 12))
      return false;
    blobs[i] = (kb_blob){bytes + offset, length};
    end = (size_t)offset + length;
  }
  *out =
      (kb_assets){blobs[0], blobs[1], blobs[2], blobs[3], blobs[4], blobs[5]};
  return true;
}
