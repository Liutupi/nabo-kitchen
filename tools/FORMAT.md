# T1 host packing formats, version 1

These are concrete host-generated formats for later C-reader review. They do not
change `docs/ARCHITECTURE.md`, certify firmware integration, or prove device RAM,
Flash layout, animation acceptance or font coverage. Native PNG/MP4/SVG/JSON
sources are never rewritten by the packers. All integers below are **little
endian**, unsigned unless stated otherwise; all absolute offsets start at byte0.
CRC means IEEE CRC-32 (`binascii.crc32`), not an authentication mechanism.

## Reproduce

Python3.12 with pinned `tools/requirements.txt`; the desktop's bundled Python
already supplies Pillow12.3.0. Dependencies may be installed in a normal venv or
the ignored task-local directory:

```powershell
python -m pip install --target private/t1/vendor -r tools/requirements.txt
python tools/check_recipes.py
python tools/pack_assets.py pack --strict-budget
python tools/pack_assets.py check build/kitchen/kitchen_dish.bin
python tools/pack_assets.py unpack build/kitchen/kitchen_dish.bin --out build/kitchen/unpacked_dish
python tools/pack_assets.py composite
python tools/pack_recipes.py pack
python tools/pack_recipes.py check build/kitchen/kitchen_recipes.krc
python tools/pack_recipes.py unpack build/kitchen/kitchen_recipes.krc --out build/kitchen/unpacked_recipes
python -m unittest discover -s tools -p test_kitchen_assets.py -v
python -m unittest discover -s tools -p test_kitchen_recipes.py -v
```

`build/` is ignored. `static_report.json` and `kitchen_recipes.json` expose a
`packages` mapping with actual bytes, SHA-256, budget and pass/fail for merging
with the separate animation/font reports. The recipe CLI discovers every current
`data/recipes/curated/*.json`; it never treats CANDIDATES.md as accepted content.
It rejects missing images, including vessel/shadow/sauce/garnish foreign keys.
Use `pack --font-chars <UTF-8 repertoire>` to enforce font coverage. Without it,
the KRC report explicitly says font coverage is pending; `.chars.txt` exports the
requested repertoire for the font pipeline.

## Static source profiles and scope

| CSV family | Display frame | Runtime pool / palette group |
|---|---:|---|
| all dish-L0..L3 |160x120|dish / one shared palette per L0,L1,L2,L3|
| optional hero |160x120|dish / one palette per complete recipe illustration|
| ingredient |40x40|icons / ingredient|
| face |40x40|icons / face|
| heat,method |24x24|icons / method-heat|
| badge |40x40|icons / badge|
| prop |32x32|icons / prop|
| lantern-dot,timer,star,stamp |12x16,16x16,12x12,56x56|icons / ui-glyph|
| wordmark,lantern |220x90,26x26|ui / startup|
| costume A/B/C |150x150|review archive only / costume|
| K1/K2 |192x192|review archive only / grok-start|
| style board |240x320|review archive only / style|

The166 static CSV rows present when T1 began are all covered. Newly registered
dish layers use the same explicit family profile. Review-only source poses and
the layout board go in `review_static.ksp`; they are excluded from runtime pool
budgets and are **not fabricated animation frames**. `costume_final.png` is an
unchanged selected duplicate outside CSV; it is not counted twice.

The conservative decimal budgets are dish200,000B, icons120,000B, ui40,000B.
`--strict-budget` exits2 if a pool exceeds its budget. A successful static build
alone never claims the architecture's approximately1.84MB total is complete:
animation, fonts and recipes have separate budgets. Registered hero art is included
in the dish pool and its reported budget. No device partition is written.

The offline kitchen candidate deliberately exceeds the original dish pool budget
when the first nine complete illustrations are included. Its separate 5 MiB
resource partition has a larger overall budget; report the actual packed size
and remaining partition capacity instead of treating the original budget as met.

Optional complete dish illustrations are registered in `ASSET_LIST.csv` with
category `hero`, ID `hero_<stable-recipe-id>`, path `dish-hero/<ID>.png`, and a
640×480 RGBA source canvas containing transparent pixels. Each complete image
receives its own palette. The browser and C model select that image when present
and otherwise retain the existing layers. This naming convention adds no field to
the recipe JSON, KRC dictionary or saved-state format; adding art does require a
new sprite bundle and a firmware build with hero rendering. Keep the original
image and provenance beside the deliverable and review the decoded 160×120 result.

## Pixel preparation

1. Resize the **whole source canvas** to the listed display frame with Lanczos.
   Do not recenter dish content. A640x480 dish source becomes160x120, so the
   source center(320,270) remains(80,67.5) before integer pixel coverage.
2. For review-only greenscreen poses, remove pixels where g>120 and g>1.5r and
   g>1.5b before resize. Actual animation keying is separately recorded.
3. Alpha<16 becomes transparent noise; all other pixels become opaque indices.
   Before dropping alpha, preblend RGB against exact `#3B2730` using the source
   coverage. This includes half-alpha edge pixels; runtime does no alpha blend.
4. For **non-character** art only, canonicalize a dark warm pixel within32 per
   RGB channel of(59,39,48), r>g and r>=b-12, only on the one-display-pixel
   silhouette ring `mask - erode(mask,3x3)`. Interior food pigment is not globally
   recolored. Faces, hair, garments, costume poses and review boards bypass this
   rewrite. The per-image canonical count is reported. Palette quantization is
   separately lossy; this criterion does not claim every dark pixel is outline.
5. Build one deterministic median-cut palette per group, up to62 learned colors
   plus reserved index0 and1. Convert/deduplicate learned colors to RGB565. Index0
   is transparent; index1 is the exact RGB565 encoding of `#3B2730` (0x3926).
   RGB565 necessarily expands to a nearby RGB888 color, not the original24bit
   triple. No dithering. Opaque pixels can never map to transparent index0.
6. Crop the resulting nonzero index rectangle; preserve its full-frame x/y
   offset and width/height. This is reversible back to the exact indexed frame.
   Source-to-palette reduction is intentionally lossy, original sources remain.

Every group has **at most64 entries including transparent0**, shared by all of
that group's assets. `prepare_pil`, `prepare_image`, `build_palette`,
`index_image`, `rgb565`, `expand565` and the PackBits functions in
`asset_codec.py` are public host helpers for the animation pipeline.
`Prepared.rgb` is a PIL RGB image; `Prepared.mask` is PIL L with values0/255;
`Prepared.stats` records the preprocessing decisions. Both images retain the
whole display frame.

## KSP1 sprite container

Header is40 bytes (`<4sHH8I`):

| Offset | Bytes | Field |
|---:|---:|---|
|0|4|ASCII KSP1|
|4|2|version=1|
|6|2|header bytes=40|
|8|4|total file bytes|
|12|4|palette-group count|
|16|4|sprite count|
|20|4|group-directory absolute offset (=40)|
|24|4|sprite-index absolute offset|
|28|4|UTF-8 string-table absolute offset|
|32|4|pixel-payload absolute offset|
|36|4|CRC of all bytes after header|

Layout: header, group directory, sprite index, RGB565 palettes, NUL-terminated
UTF-8 strings, pixel payloads. Offsets/lengths are checked before reading. Names
are restricted to ASCII letters/digits/underscore/hyphen, length1..128, so the
unpack CLI cannot write outside its output directory.

Each group record is32 bytes (`<IHHII16s`): string-table-relative name offset u32,
palette count u16, flags u16=0, absolute palette offset u32, palette byte count
u32 (=count*2), reserved16 zero bytes. Palettes are contiguous; each RGB565 color
is u16. The first two entries are0 and0x3926.

Each sprite record is36 bytes (`<II8HIII`):

| Offset | Bytes | Field |
|---:|---:|---|
|0|4|FNV-1a32 hash of UTF-8 ID|
|4|4|string-table-relative ID offset|
|8|2|zero-based group number|
|10|2|codec flags:0=raw index bytes,1=PackBits|
|12|2|whole display-frame width|
|14|2|whole display-frame height|
|16|2|crop x offset|
|18|2|crop y offset|
|20|2|crop width|
|22|2|crop height|
|24|4|absolute pixel-payload offset|
|28|4|encoded pixel byte count|
|32|4|CRC of decoded cropped index bytes|

Pixels are top-to-bottom row-major. PackBits may cross row boundaries; the
decoded length must be exactly cropWidth*cropHeight. Flags choose the smaller
of raw and PackBits. An empty image has a0x0 crop and0byte payload. Decoding
creates a transparent whole frame, then pastes the crop at its recorded offset.

PackBits here has a deliberately explicit dialect: a control byte0..127 means
copy the next(control+1) literal bytes;128..255 means repeat the next single byte
((control&127)+1) times. There is no special NOP128. Decoder rejects truncated
packets, overrun and underrun. Frame maximum is1024x1024; index values must be
less than their group's palette count. Unknown flags/reserved fields, duplicate
IDs/hashes, non-contiguous payloads, invalid sections and trailing bytes fail.

`pack_assets.py composite` uses only decoded package layers and actual recipe
`dish` fields in shadow -> vessel -> sauce -> up to4 cooked layers -> garnish
order, saving160x120 PNGs and contacts on both specified backgrounds.

## KRC1 recipe container

Header is48 bytes (`<4sHH10I`):

| Offset | Bytes | Field |
|---:|---:|---|
|0|4|ASCII KRC1|
|4|2|version=1|
|6|2|header bytes=48|
|8|4|total file bytes|
|12|4|recipe count|
|16|4|index absolute offset (=48)|
|20|4|dictionary absolute offset|
|24|4|dictionary byte length|
|28|4|main-ingredient bitmap section absolute offset|
|32|4|bitmap bytes per recipe,ceil(ingredientCount/8)|
|36|4|compressed body section absolute offset|
|40|4|codec0x00000408: window bits8,lookahead bits4|
|44|4|CRC of all bytes after header|

Layout follows the architecture: header,16byte recipe index, Flash-side JSON
dictionary/labels, main bitmaps, compressed bodies. Dictionary JSON is canonical
UTF-8 without whitespace, keys sorted. It includes ordered lists of stable
recipe IDs, ingredient/seasoning/dish-asset IDs and schema enums, plus names,
aliases, season masks, servings and content versions. These lists are metadata
in Flash, not copied into every decompressed recipe. Recipe order is ID-sorted.
Internal ordinal numbers may change between builds; persistent identity remains
the original string ID and its FNV-1a hash. Hash collisions fail packing.

The index is exactly16 bytes per recipe, with no alignment padding:

| Offset | Bytes | Field |
|---:|---:|---|
|0|4|ID FNV-1a32|
|4|1|course/class bit mask, schema enum order|
|5|2|tag mask, schema enum order|
|7|1|minutes (schema3..240)|
|8|1|difficulty (schema1..3)|
|9|3|main bitmap offset relative to main section|
|12|4|body offset relative to compressed body section|

Main bitmaps are fixed-width, little-bit-order: ingredient ordinal i uses byte
i//8, bit i%8. Only `main:true` sets a bit. Filter/class/tag/minutes/difficulty
and fridge scans need only this index and bitmap, no body decompression.

Each body has an8byte prefix: compressed byte count u16, decoded byte count u16,
CRC32 of decoded bytes u32. Then exactly that many **heatshrink** bytes follow.
The decoded object is canonical UTF-8 JSON with schema keys preserved and all
ingredient/seasoning/recipe/asset IDs plus course/tag/cuisine/source/method/heat/
vessel/sauce/garnish enums replaced by dictionary ordinals. Optional fields and
values are retained; reversing ordinals reconstructs the original JSON object.
The decoded numbered JSON, including all text, must fit **2048 bytes**, checked
both before compression and before allocation/decoding. This cap is not relaxed
when a recipe's compressed bytes happen to be small.

The encoder uses pinned `heatshrink2==0.14.0`, a binding to the
[official heatshrink implementation](https://github.com/atomicobject/heatshrink).
It is not zlib or a substitute codec. Tokens are MSB-first:1 then8bits literal;
0 then8bits(distance-1) then4bits(count-1), with zero-filled initial history and
zero padding only to the final byte boundary. The independent bounded reader
rejects decoded overruns, truncation, >7 trailing bits and nonzero padding, then
checks the body CRC. Tests cross-check its output against the C binding.

Reader validation also checks dictionary/reference bounds,16byte record offsets,
all body extents, duplicate IDs/hashes, full-section CRC, index/bitmap equality
to the decoded object, Flash label metadata equality and unclaimed/trailing
bytes. Schema and all actual PNG foreign keys are checked before packing.

## Verification boundary

Roundtrip tests compare **indexed output**, crop offsets and reconstructed recipe
objects; they do not pretend lossy quantization reproduces original PNG pixels.
Source hashes in reports establish input provenance. Corruption tests reseal
container CRCs before exercising structural bounds, so checks are not merely
testing checksum failure. Determinism is tested independently of timestamps.
No C reader, boot resource registration, real Flash installation or real-device
rendering is claimed by these host tools.
