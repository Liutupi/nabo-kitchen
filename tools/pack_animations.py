"""Extract selected real video frames and build a silent KAN1 animation package."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys

import _bootstrap  # noqa: F401
import numpy as np
from PIL import Image, ImageChops, ImageDraw, ImageFilter

from animation_codec import pack_animations, unpack_animations
from asset_codec import Prepared, build_palette, index_image, prepare_pil

ROOT = Path(__file__).resolve().parent.parent
CORE = {'idle': (10, True, 'K1'), 'toss': (14, False, 'K2'),
        'serve': (8, False, 'K1'), 'tend': (10, True, 'K2')}
FPS, SIZE, ANCHOR = 12, (192, 192), (96, 191)
SUPPORTED_FPS = (12, 24)
BUDGET = 700 * 1024


def sha256(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def executable(value, name):
    found = value or shutil.which(name)
    if not found or not Path(found).is_file():
        raise ValueError(f'{name} unavailable; pass --{name} with an existing executable')
    return str(Path(found).resolve())


def probe_video(path, ffprobe):
    result = subprocess.run([ffprobe, '-v', 'error', '-show_streams', '-show_format',
                             '-of', 'json', str(path)], check=True, capture_output=True, text=True)
    metadata = json.loads(result.stdout)
    videos = [stream for stream in metadata['streams']
              if stream.get('codec_type') == 'video' and not stream.get('disposition', {}).get('attached_pic')]
    if len(videos) != 1:
        raise ValueError('expected exactly one real video stream, excluding attached posters')
    video = videos[0]
    if video['width'] <= 0 or video['height'] <= 0 or video['width'] > 4096 or video['height'] > 4096:
        raise ValueError('source dimensions outside supported bounds')
    return metadata, video


def extract_frames(source, video, ffmpeg, work, fps=FPS):
    if fps not in SUPPORTED_FPS:
        raise ValueError('unsupported animation frame rate')
    digest = sha256(source)
    cache = work / f'{source.stem}-{digest[:16]}-{fps}fps'
    cache.mkdir(parents=True, exist_ok=True)
    stamp = cache / 'extraction.json'
    if stamp.exists():
        saved = json.loads(stamp.read_text(encoding='utf-8'))
        paths = sorted(cache.glob('[0-9][0-9][0-9][0-9][0-9].png'))
        if saved['source_sha256'] == digest and saved['count'] == len(paths) and saved.get('fps') == fps and paths:
            return paths, saved
        raise ValueError('incomplete/stale frame cache; use a fresh --output-dir')
    subprocess.run([ffmpeg, '-hide_banner', '-loglevel', 'error', '-y', '-i', str(source),
                    '-map', f"0:{video['index']}", '-an', '-sn', '-dn',
                    '-vf', f'fps={fps}', str(cache / '%05d.png')],
                   check=True, capture_output=True)
    paths = sorted(cache.glob('[0-9][0-9][0-9][0-9][0-9].png'))
    if not paths or len(paths) > 50 * fps:
        raise ValueError('source extraction empty or longer than 50 seconds')
    saved = {'source_sha256': digest, 'count': len(paths), 'fps': fps,
             'mapped_video_stream': video['index'], 'audio_mapped': False,
             'attached_posters_mapped': False}
    stamp.write_text(json.dumps(saved, indent=2), encoding='utf-8')
    return paths, saved


def sampling_indices(available, count):
    if count < 2 or available < count:
        raise ValueError('insufficient distinct source frames for target count')
    # Half-up integer rounding; no host-language bankers-round differences.
    return [(i * (available - 1) * 2 + count - 1) // (2 * (count - 1)) for i in range(count)]


def key_and_prepare(image):
    """Key only dominant green, suppress green chroma at surviving edge pixels."""
    rgb = image.convert('RGB')
    pixels = np.asarray(rgb, dtype=np.int16)
    r, g, b = pixels[:, :, 0], pixels[:, :, 1], pixels[:, :, 2]
    maximum = np.maximum(r, b)
    key = (g > 80) & (g - maximum > 28) & (g * 100 > r * 125) & (g * 100 > b * 125)
    background = Image.fromarray((key.astype(np.uint8) * 255))
    adjacent = np.asarray(background.filter(ImageFilter.MaxFilter(3))) > 0
    spill = adjacent & ~key & (g > maximum + 6)
    clean = pixels.copy()
    clean[:, :, 1][spill] = maximum[spill]
    rgba = Image.fromarray(clean.astype(np.uint8)).convert('RGBA')
    rgba.putalpha(Image.fromarray((~key).astype(np.uint8) * 255))
    prepared = prepare_pil(rgba, SIZE, character=True, green_key=False)
    prepared.stats.update({'source_green_pixels_keyed': int(key.sum()),
                           'source_edge_pixels_despilled': int(spill.sum()),
                           'source_size': list(rgb.size),
                           'placement': 'uniform full-canvas resize; bottom edge fixed; no per-frame crop/translation'})
    return prepared


def indexed_png(indices, palette, size=SIZE):
    from asset_codec import expand565
    image = Image.new('RGBA', size)
    image.putdata([(0, 0, 0, 0) if value == 0 else (*expand565(palette[value]), 255)
                   for value in indices])
    return image


def frame_metrics(first, last):
    a, b = np.asarray(first.convert('RGBA'), dtype=np.int16), np.asarray(last.convert('RGBA'), dtype=np.int16)
    union = (a[:, :, 3] > 0) | (b[:, :, 3] > 0)
    changed = np.any(a != b, axis=2)
    return {'changed_pixels': int(changed.sum()), 'frame_pixels': int(changed.size),
            'foreground_rgb_mae': float(np.abs(a[:, :, :3] - b[:, :, :3])[union].mean()) if union.any() else 0.0}


def contact_sheet(actions, destination, background):
    width = max(len(images) for images in actions.values()) * 160
    sheet = Image.new('RGB', (width, len(actions) * 188), background)
    draw = ImageDraw.Draw(sheet)
    for row, (ident, images) in enumerate(actions.items()):
        for column, image in enumerate(images):
            # 160px full canvas produces about150px tall waist-anchored character.
            tile = image.resize((160, 160), Image.Resampling.LANCZOS)
            x, y = column * 160, row * 188
            sheet.paste(tile, (x, y), tile)
            draw.text((x + 3, y + 163), f'{ident} {column + 1}', fill='#FBF1E3' if background == '#1D1A30' else '#15122A')
    sheet.save(destination)


def parse_selections(values):
    result = {}
    for value in values:
        try:
            ident, numbers = value.split('=', 1)
            indices = [int(number) for number in numbers.split(',')]
        except ValueError as error:
            raise ValueError('--select must be ID=0,1,2,... source frame indices at the selected fps') from error
        if ident not in CORE or ident in result:
            raise ValueError('unknown/duplicate selection action')
        result[ident] = indices
    return result


def animation_profile(fps):
    """Double temporal samples at 24 fps while keeping each action duration unchanged."""
    if fps not in SUPPORTED_FPS:
        raise ValueError('unsupported animation frame rate')
    factor = fps // FPS
    return {ident: (count * factor, loop, start) for ident, (count, loop, start) in CORE.items()}


def build(args):
    fps = args.fps
    profile = animation_profile(fps)
    ffmpeg, ffprobe = executable(args.ffmpeg, 'ffmpeg'), executable(args.ffprobe, 'ffprobe')
    source_dir, output = Path(args.input_dir).resolve(), Path(args.output_dir).resolve()
    output.mkdir(parents=True, exist_ok=True)
    selections = parse_selections(args.select)
    qa = json.loads(Path(args.qa_review).read_text(encoding='utf-8')) if args.qa_review else {'actions': {}}
    prepared, sources, report = {}, {}, {}
    for ident, (count, loop, start) in profile.items():
        source = source_dir / f'{ident}.mp4'
        original_hash = sha256(source)
        metadata, video = probe_video(source, ffprobe)
        paths, extraction = extract_frames(source, video, ffmpeg, output / 'work', fps)
        selected = selections.get(ident, sampling_indices(len(paths), count))
        if len(selected) != count or selected[0] != 0 or selected[-1] != len(paths) - 1 or any(i < 0 or i >= len(paths) for i in selected) or any(a >= b for a, b in zip(selected, selected[1:])):
            raise ValueError('selection must contain targetcount increasing indices including actualfirst+last')
        review = qa.get('actions', {}).get(ident, {})
        if review and review.get('verdict') not in ('engineering_visual_pass', 'review', 'blocked'):
            raise ValueError('unknown visual review verdict')
        if review and review.get('source_sha256') != original_hash:
            raise ValueError('visual review refers to a different source file')
        if review.get('verdict') == 'blocked':
            raise ValueError(f'{ident}: source visual review blocked, refusing production package')
        if args.close_loop_endpoints and loop and not review.get('checks', {}).get('return_pose_visually_matches'):
            raise ValueError('loop endpoint selection requires SHA-bound source return-pose visual review')
        actual_selected = selected[:-1] + [selected[0]] if args.close_loop_endpoints and loop else selected
        frames = []
        for index in actual_selected:
            with Image.open(paths[index]) as image:
                frames.append(key_and_prepare(image))
        if sha256(source) != original_hash:
            raise ValueError('source changed during extraction')
        prepared[ident], sources[ident] = frames, original_hash
        audio = [stream for stream in metadata['streams'] if stream.get('codec_type') == 'audio']
        report[ident] = {'source': str(source), 'source_sha256': original_hash,
                         'source_video': {key: video.get(key) for key in ['width', 'height', 'avg_frame_rate', 'duration', 'nb_frames', 'codec_name']},
                         'source_audio_streams': [{key: stream.get(key) for key in ['codec_name', 'sample_rate', 'channels']} for stream in audio],
                         'audio_extracted_or_packaged': False, 'extraction': extraction,
                         f'selected_{fps}fps_indices': selected, f'actual_{fps}fps_indices': actual_selected,
                         'selected_source_seconds': [i / fps for i in actual_selected],
                         'loop_endpoint_selection': 'lastslotusesactualfirstsourceframeafterverifiedreturnpose; exactquantizedloopseam' if actual_selected != selected else 'actualsourceendretained',
                         'target_frames': count, 'loop': loop, 'start_pose': start, 'target_fps': fps,
                         'target_duration_seconds': count / fps,
                         'temporal_processing': f'select actual {fps}fps grid frames including endpoints; retime to required frame count at {fps}fps; no interpolation or generated frames',
                         'visual_review': review or {'verdict': 'not supplied; codec success doesnotimply source acceptance'},
                         'preparation': [frame.stats for frame in frames]}
    palette = build_palette([frame for action in prepared.values() for frame in action])
    actions = [{'id': ident, 'loop': CORE[ident][1], 'frames': [index_image(frame, palette) for frame in frames]}
               for ident, frames in prepared.items()]
    packed = pack_animations(actions, palette, SIZE, ANCHOR, fps)
    decoded = unpack_animations(packed)
    decoded_images = {}
    for action in actions:
        ident = action['id']
        decoded_frames = decoded.actions[ident]['frames']
        if decoded_frames != action['frames']:
            raise AssertionError('decoded animation differs from prepared paletteindices')
        folder = output / 'decoded' / ident
        folder.mkdir(parents=True, exist_ok=True)
        images, quantization_errors = [], []
        for index, indices in enumerate(decoded_frames):
            image = indexed_png(indices, palette)
            image.save(folder / f'{index:02d}.png')
            expected_rgb = np.asarray(prepared[ident][index].rgb, dtype=np.int16)
            actual_rgb = np.asarray(image.convert('RGB'), dtype=np.int16)
            opaque = np.asarray(prepared[ident][index].mask) > 0
            difference = np.abs(expected_rgb - actual_rgb)[opaque]
            quantization_errors.append({'foreground_rgb_mae': float(difference.mean()) if difference.size else 0.0,
                                        'max_channel_error': int(difference.max()) if difference.size else 0})
            images.append(image)
        decoded_images[ident] = images
        report[ident]['encoded_bytes'] = decoded.actions[ident]['encoded_bytes']
        report[ident]['frame_records'] = decoded.actions[ident]['records']
        report[ident]['first_last_quantized'] = frame_metrics(images[0], images[-1])
        report[ident]['quantization_errors'] = quantization_errors
        report[ident]['roundtrip_pixel_exact'] = True
    package = output / 'kitchen_nabo.bin'
    package.write_bytes(packed)
    contact_sheet(decoded_images, output / 'animation-contact-dark.png', '#1D1A30')
    contact_sheet(decoded_images, output / 'animation-contact-paper.png', '#FBF1E3')
    summary = {'format': 'KAN1', 'version': 1, 'frame_size': list(SIZE), 'anchor': list(ANCHOR), 'fps': fps,
               'palette_rgb565': palette, 'palette_count': len(palette), 'transparent_index': 0,
               'package': str(package), 'package_bytes': len(packed), 'package_sha256': sha256(package),
               'budget_bytes': BUDGET, 'within700KiB_budget': len(packed) <= BUDGET,
               'source_files_unchanged': all(sha256(source_dir / f'{ident}.mp4') == digest for ident, digest in sources.items()),
               'roundtrip_pixel_exact': True, 'runtime_audio': False,
               'source_qa_is_separate_from_codec': True,
               'tool_versions': {
                   'ffmpeg': subprocess.check_output([ffmpeg, '-version'], text=True).splitlines()[0],
                   'ffprobe': subprocess.check_output([ffprobe, '-version'], text=True).splitlines()[0],
                   'python': sys.version.split()[0], 'numpy': np.__version__,
                   'pillow': __import__('PIL').__version__,
               },
               'visual_source_acceptance': 'per-source supplied review only; user acceptance not inferred',
               'reproduction': {'ffmpeg': ffmpeg, 'ffprobe': ffprobe, 'input_dir': str(source_dir),
                                'close_loop_endpoints': args.close_loop_endpoints, 'fps': fps,
                                'select': [f"{ident}=" + ','.join(str(i) for i in item[f'selected_{fps}fps_indices']) for ident, item in report.items()]},
               'actions': report}
    (output / 'animations-report.json').write_text(json.dumps(summary, ensure_ascii=False, indent=2), encoding='utf-8')
    if not summary['source_files_unchanged']:
        raise AssertionError('native source hash changed')
    return summary


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fps', type=int, choices=SUPPORTED_FPS, default=FPS, help='12 fps default or 24 fps higher-fidelity candidate; preserves action durations')
    parser.add_argument('--input-dir', default=str(ROOT / 'game-art/kitchen/grok-clips'))
    parser.add_argument('--output-dir', default=str(ROOT / 'private/t1/animations'))
    parser.add_argument('--ffmpeg')
    parser.add_argument('--ffprobe')
    parser.add_argument('--qa-review', help='manual source reviewJSON, bound to sourceSHA256')
    parser.add_argument('--close-loop-endpoints', action='store_true', help='after SHA-bound visual return-pose review, select actualfirstframe for finalloopslot')
    parser.add_argument('--select', action='append', default=[], help='ID=comma-separated source frame indices at the selected --fps')
    args = parser.parse_args()
    summary = build(args)
    print(json.dumps({key: summary[key] for key in ['package', 'package_bytes', 'package_sha256',
                                                   'palette_count', 'within700KiB_budget', 'roundtrip_pixel_exact']}, indent=2))


if __name__ == '__main__':
    main()
