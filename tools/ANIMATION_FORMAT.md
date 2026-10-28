# KAN1 animation package

This documents the implemented T1 codec. It does not alter the finalized art or gameplay rules.

## Inputs and processing

- Native `game-art/kitchen/grok-clips/{idle,toss,serve,tend}.mp4` remain byte-for-byte unchanged.
- FFprobe chooses the single real video stream and excludes attached JPEG poster streams. FFmpeg explicitly maps that video and uses `-an -sn -dn`. No audio is played or encoded in KAN1.
- Sources are decoded onto a genuine 12 fps source timeline. Target source-frame indices are selected by integer half-up rounding, or explicit `--select ID=i,j,...`. Selection is from actual decoded frames, including source start/end; no generated or interpolated frames.
- Output is 192×192 at 12 fps, bottom-center anchor (96,191), using a uniform full-canvas resize. No per-frame crop, mirror or movement correction is applied to these masters. Their waist remains cut by the source bottom edge.
- Green key: G>80, G−max(R,B)>28, and G>1.25×R/B. Remaining green chroma on a one-source-pixel adjacent edge is clamped to max(R,B). Shared `asset_codec.prepare_pil` then resizes, thresholds alpha at16 and mattes partial coverage to the dark outline. Character face/hair/garment pigments are not automatically recolored.
- One RGB565 palette is shared by all four actions, at most64 entries including transparent index0. Index1 reserves RGB565(#3B2730). Palette reduction is lossy and assessed separately from the lossless binary round trip.
- Exact target frame counts: idle10 loop, toss14 nonloop, serve8 nonloop, tend10 loop. At12 fps these run0.8333/1.1667/0.6667/0.8333 seconds. The3–5second masters are temporally retimed to those explicitly specified game-frame counts; their native duration is not claimed preserved.
- `--close-loop-endpoints` is optional and requires a SHA-bound visual review confirming the actual source returns to its starting pose. It selects the actual first source frame for the final loop slot, giving an exact pixel seam after quantization. The nominal end index, actual reused source index, original return-pose review and all timestamps remain in the report. It never repairs an invalid action or rewrites an MP4.
- The inspected idle selection retains its actual blink frame; serve retains the isolated wink at source12fps zero-based index33. Explicit indices are recorded for reproduction.

## Binary layout

Little endian. All fields and offsets are integers; the future target decoder needs no floating point.

### Header: 52 bytes, `<4sHH11I`

| Field | Type | Meaning |
|---|---|---|
| magic | char[4] | KAN1 |
| version | uint16 | 1 |
| header_size | uint16 | 52 |
| total_size | uint32 | Exact file bytes |
| width, height | uint32 each | Full indexed frame dimensions |
| anchor_x, anchor_y | uint32 each | Bottom anchor inside frame |
| fps | uint32 | Output12 |
| palette_count | uint32 | 2–64 |
| action_count | uint32 | 1–256 |
| index_offset | uint32 | Immediately after RGB565 palette |
| data_offset | uint32 | Immediately after action directory |
| body_crc32 | uint32 | CRC32 of all bytes after header |

The palette immediately follows the header: `palette_count` uint16 RGB565 words. Index0 is transparent regardless of its stored zero word. Opaque black can use another index with RGB565 word0.

### Action directory: 36 bytes each, `<16sHH4I`

ASCII ID[16] (NUL terminated and padded); frame_count uint16; loop_flags uint16 (0/1); stream_offset uint32; stream_bytes uint32; stream_crc32 uint32; reserved uint32=0.

IDs use `[a-z][a-z0-9_]{0,14}`. Actions are sorted by ID for deterministic output. Streams are contiguous and no unclaimed bytes are accepted.

### Frame record: 18 bytes, `<BB4H2I`

mode uint8; reserved uint8=0; x/y/w/h uint16; payload_bytes uint32; reconstructed_full_frame_crc32 uint32. Payload follows immediately.

| Mode | Payload |
|---|---|
| 0 KEY_RLE | Full frame, PackBits |
| 1 DELTA_RLE | Current pixels in changed bounding rectangle, PackBits |
| 2 REPEAT | No payload; repeat prior complete frame |
| 3 KEY_RAW | Full frame, raw palette indices |
| 4 DELTA_RAW | Current changed rectangle, raw palette indices |

A delta replaces its rectangle in the prior frame, including transparent zeros that erase old pixels. The encoder compares key/raw/RLE and delta/raw/RLE lengths and uses the smallest payload; repeat is used for identical adjacent frames. First frame must be a key frame. Repeat has x=y=w=h=payload_bytes=0.

PackBits follows shared `asset_codec`: control low7bits +1 gives count1–128. High bit set means repeat the next byte; otherwise copy that many literal bytes. Run length≥3 is encoded as a repeat packet.

### Bounds and corruption

Decoder checks exact magic/version/file length, dimensions≤1024 peredge, anchor, fps, palette cardinality/index validity, action IDs/counts/flags/reserved fields, monotonically contiguous offsets, rectangles inside the frame, no delta/repeat before a key, all byte lengths, full-frame CRC, stream CRC, body CRC and no trailing bytes.

Host full-decode reference caps action frame counts at4096 and total decoded pixels at64MiB. Target streaming can keep only one192×192 index buffer (36,864 bytes), one≤128byte palette and an8row RGB565 output stripe. This is a storage plan, not measured C3 memory/performance proof.

## Reproduction

Requires Python with Pillow/NumPy and existing FFmpeg/FFprobe executables. Tools do not install or download anything.

```powershell
python tools/pack_animations.py --ffmpeg <existing-ffmpeg.exe> --ffprobe <existing-ffprobe.exe> --qa-review <SHA-bound-visual-review.json> --close-loop-endpoints --select idle=0,5,11,16,21,27,34,38,43,48 --select serve=0,8,14,21,27,33,41,48
python -m unittest discover -s tools/tests -p test_kitchen_animations.py -v
```

Default output is ignored `private/t1/animations/`: `kitchen_nabo.bin`, `animations-report.json`, decoded PNGs and dark/paper contact sheets. Source extraction cache is keyed by sourceSHA and12fps.

The report separates source visual review from codec success, records source AAC presence and exclusion, sourceSHA before/after, source/output timing, all frame selections, key/despill statistics, RGB565 palette, quantization errors, every frame mode/rectangle, first/last metrics, exact roundtrip, bytes and700KiB budget result. Source visual review can be `engineering_visual_pass`, `review` or `blocked`; blocked sources are refused and review is SHA-bound. No user or device acceptance is inferred.

The palette-index sequence is compared pixel-exact before and after decode. Original continuous-color video cannot be pixel-exact after green extraction/resize/quantization; that distinct quality check is recorded and inspected on both prescribed backgrounds.
