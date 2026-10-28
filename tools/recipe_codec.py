"""KRC1 recipe container: 16-byte Flash index and bounded heatshrink bodies."""
import binascii
import copy
import json
import struct
import re

from asset_codec import fnv1a
from check_recipes import ROOT, asset_rows, read_json
from heatshrink_codec import decode as hs_decode, encode as hs_encode
from jsonschema import Draft202012Validator

HEADER = struct.Struct('<4sHH10I')
PAYLOAD = struct.Struct('<HHI')
MAX_DECODED = 2048
CODEC = 8 | (4 << 8)
RECIPE_VALIDATOR = Draft202012Validator(read_json(ROOT / 'data/schema/recipe.schema.json'))


def canonical(value):
    return json.dumps(value, ensure_ascii=False, sort_keys=True, separators=(',', ':'),
                      allow_nan=False).encode('utf-8')


def make_dictionary(recipes, root=ROOT):
    schema = read_json(root / 'data/schema/recipe.schema.json')['properties']
    assets = asset_rows(root)
    return {
        'ids': [r['id'] for r in recipes],
        'ingredients': sorted(x['id'] for x in read_json(root / 'data/ingredients.json')['ingredients']),
        'seasonings': sorted(x['id'] for x in read_json(root / 'data/seasonings.json')['seasonings']),
        'assets': sorted(k for k, r in assets.items() if r['category'].startswith('dish-')),
        'sauces': sorted({k[6:].split('__')[0] for k, r in assets.items() if r['category'] == 'dish-L1'}),
        'methods': schema['steps']['items']['properties']['method']['enum'],
        'heats': schema['steps']['items']['properties']['heat']['enum'],
        'vessels': schema['dish']['properties']['vessel']['enum'],
        'garnishes': schema['dish']['properties']['garnish']['items']['enum'],
        'courses': schema['course']['items']['enum'],
        'tags': schema['tags']['items']['enum'],
        'cuisines': schema['cuisine']['enum'],
        'sources': schema['source']['enum'],
        # Flash-side labels and filtering metadata do not require body decompression.
        'metadata': [{'name': r['name'], 'aliases': r.get('aliases', []),
                      'season_mask': sum(1 << (m-1) for m in r.get('season', [])),
                      'servings': r['servings'], 'version': r['version']} for r in recipes],
    }


def translate(recipe, dictionary, decode=False):
    result = copy.deepcopy(recipe)

    def value(group, item):
        table = dictionary[group]
        if decode:
            if type(item) is not int or not 0 <= item < len(table):
                raise ValueError(f'KRC1 invalid {group} reference: {item}')
            return table[item]
        try:
            return table.index(item)
        except ValueError as error:
            raise ValueError(f'unknown {group} value: {item}') from error

    result['id'] = value('ids', result['id'])
    for key, group in [('course', 'courses'), ('tags', 'tags')]:
        if key in result:
            result[key] = [value(group, x) for x in result[key]]
    for key, group in [('cuisine', 'cuisines'), ('source', 'sources')]:
        if key in result:
            result[key] = value(group, result[key])
    for item in result['ingredients']:
        item['id'] = value('ingredients', item['id'])
    for item in result['seasonings']:
        item['id'] = value('seasonings', item['id'])
    for step in result['steps']:
        step['method'] = value('methods', step['method'])
        step['heat'] = value('heats', step['heat'])
    dish = result['dish']
    dish['vessel'] = value('vessels', dish['vessel'])
    if dish['sauce'] is not None:
        dish['sauce'] = value('sauces', dish['sauce'])
    dish['layers'] = [value('assets', x) for x in dish['layers']]
    if 'garnish' in dish:
        dish['garnish'] = [value('garnishes', x) for x in dish['garnish']]
    return result


def metadata_fields(recipe, dictionary):
    courses = sum(1 << dictionary['courses'].index(x) for x in recipe['course'])
    tags = sum(1 << dictionary['tags'].index(x) for x in recipe.get('tags', []))
    bitmap = bytearray((len(dictionary['ingredients']) + 7) // 8)
    for item in recipe['ingredients']:
        if item.get('main'):
            number = dictionary['ingredients'].index(item['id'])
            bitmap[number // 8] |= 1 << (number % 8)
    return courses, tags, bytes(bitmap)


def pack_recipes(recipes, root=ROOT):
    recipes = sorted(recipes, key=lambda x: x['id'])
    ids = [r['id'] for r in recipes]
    if not recipes or len(ids) != len(set(ids)) or len({fnv1a(x) for x in ids}) != len(ids):
        raise ValueError('empty/duplicate recipe set or ID hash collision')
    dictionary = make_dictionary(recipes, root)
    encoded_dictionary = canonical(dictionary)
    bitmap_width = (len(dictionary['ingredients']) + 7) // 8
    index_offset = HEADER.size
    dictionary_offset = index_offset + len(recipes) * 16
    main_offset = dictionary_offset + len(encoded_dictionary)
    payload_offset = main_offset + len(recipes) * bitmap_width
    index, main, payload = bytearray(), bytearray(), bytearray()
    for number, recipe in enumerate(recipes):
        raw = canonical(translate(recipe, dictionary))
        if len(raw) > MAX_DECODED:
            raise ValueError(f"{recipe['id']}: decoded numbered payload {len(raw)} exceeds 2048 bytes")
        compressed = hs_encode(raw)
        if len(compressed) > 65535 or len(main) > 0xffffff:
            raise ValueError('KRC1 relative offset/length overflow')
        courses, tags, bitmap = metadata_fields(recipe, dictionary)
        record = (struct.pack('<IBHBB', fnv1a(recipe['id']), courses, tags,
                              recipe['minutes'], recipe['difficulty'])
                  + len(main).to_bytes(3, 'little') + struct.pack('<I', len(payload)))
        if len(record) != 16:
            raise AssertionError('KRC1 record size')
        index.extend(record)
        main.extend(bitmap)
        payload.extend(PAYLOAD.pack(len(compressed), len(raw), binascii.crc32(raw)) + compressed)
    body = bytes(index) + encoded_dictionary + bytes(main + payload)
    return HEADER.pack(b'KRC1', 1, HEADER.size, HEADER.size + len(body), len(recipes), index_offset,
                       dictionary_offset, len(encoded_dictionary), main_offset, bitmap_width,
                       payload_offset, CODEC, binascii.crc32(body)) + body


def unpack_recipes(data):
    data = bytes(data)
    if len(data) < HEADER.size:
        raise ValueError('truncated KRC1 header')
    magic, version, header, size, count, io, do, dl, mo, bw, po, codec, crc = HEADER.unpack_from(data)
    if magic != b'KRC1' or version != 1 or header != HEADER.size or size != len(data) or codec != CODEC:
        raise ValueError('invalid KRC1 header/version/codec/size')
    if not 0 < count <= 65535 or io != header or do != io + count * 16:
        raise ValueError('KRC1 index bounds invalid')
    if do + dl != mo or mo + count * bw != po or not do <= mo <= po <= size:
        raise ValueError('KRC1 dictionary/bitmap/payload bounds invalid')
    if binascii.crc32(data[header:]) != crc:
        raise ValueError('KRC1 body CRC mismatch')
    try:
        dictionary = json.loads(data[do:mo])
    except (ValueError, UnicodeError) as error:
        raise ValueError('KRC1 invalid dictionary JSON') from error
    expected = {'ids', 'ingredients', 'seasonings', 'assets', 'sauces', 'methods', 'heats',
                'vessels', 'garnishes', 'courses', 'tags', 'cuisines', 'sources', 'metadata'}
    if not isinstance(dictionary, dict) or set(dictionary) != expected:
        raise ValueError('KRC1 invalid dictionary fields')
    for name in expected - {'metadata'}:
        values = dictionary[name]
        if not isinstance(values, list) or any(not isinstance(x, str) or not x for x in values) or len(values) != len(set(values)):
            raise ValueError('KRC1 invalid/duplicate dictionary values')
    if (len(dictionary['ids']) != count or not isinstance(dictionary['metadata'], list)
            or len(dictionary['metadata']) != count or bw != (len(dictionary['ingredients']) + 7) // 8
            or any(not re.fullmatch(r'[a-z0-9]+(?:-[a-z0-9]+)*', ident) or len(ident) > 40
                   for ident in dictionary['ids'])):
        raise ValueError('KRC1 dictionary counts invalid')
    recipes, records, seen, position = [], [], set(), po
    for number in range(count):
        off = io + number * 16
        hashed, courses, tags, minutes, difficulty = struct.unpack_from('<IBHBB', data, off)
        bo = int.from_bytes(data[off+9:off+12], 'little')
        relative = struct.unpack_from('<I', data, off+12)[0]
        if hashed in seen or hashed != fnv1a(dictionary['ids'][number]) or bo != number * bw or po + relative != position:
            raise ValueError('KRC1 ID hash/record offsets invalid')
        if position + PAYLOAD.size > size:
            raise ValueError('KRC1 truncated body prefix')
        clen, rlen, checksum = PAYLOAD.unpack_from(data, position)
        end = position + PAYLOAD.size + clen
        if end > size or not 0 < rlen <= MAX_DECODED:
            raise ValueError('KRC1 compressed/decoded body bounds invalid')
        raw = hs_decode(data[position+PAYLOAD.size:end], rlen)
        if binascii.crc32(raw) != checksum:
            raise ValueError('KRC1 decoded body CRC mismatch')
        try:
            recipe = translate(json.loads(raw), dictionary, decode=True)
            if not RECIPE_VALIDATOR.is_valid(recipe):
                raise ValueError('decoded object violates recipe schema')
            expected_courses, expected_tags, bitmap = metadata_fields(recipe, dictionary)
        except (ValueError, KeyError, TypeError, IndexError) as error:
            raise ValueError('KRC1 malformed numbered body/reference') from error
        if (recipe['id'] != dictionary['ids'][number] or courses != expected_courses or tags != expected_tags
                or minutes != recipe['minutes'] or difficulty != recipe['difficulty']
                or bitmap != data[mo+bo:mo+bo+bw]):
            raise ValueError('KRC1 Flash index/bitmap differs from decoded body')
        expected_meta = {'name': recipe['name'], 'aliases': recipe.get('aliases', []),
                         'season_mask': sum(1 << (m-1) for m in recipe.get('season', [])),
                         'servings': recipe['servings'], 'version': recipe['version']}
        if dictionary['metadata'][number] != expected_meta:
            raise ValueError('KRC1 Flash labels/metadata differ from decoded body')
        recipes.append(recipe)
        records.append({'id': recipe['id'], 'id_hash': hashed, 'class_bits': courses, 'tag_bits': tags,
                        'minutes': minutes, 'difficulty': difficulty, 'main_bitmap_offset': mo+bo,
                        'payload_offset': position, 'compressed_bytes': clen, 'decoded_bytes': rlen})
        seen.add(hashed)
        position = end
    if position != size:
        raise ValueError('KRC1 trailing/unclaimed bytes')
    return recipes, {'dictionary': dictionary, 'records': records, 'index_record_bytes': 16,
                     'bitmap_width': bw, 'dictionary_bytes': dl, 'body_codec': 'heatshrink-8/4'}
