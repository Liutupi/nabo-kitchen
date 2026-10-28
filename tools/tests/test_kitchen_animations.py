"""Tests for real bounds, transparent erasure, temporal selection and corrupt data."""
import binascii
from pathlib import Path
import struct
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

import _bootstrap  # noqa: F401
from PIL import Image
from animation_codec import (ACTION, FRAME, HEADER, DELTA_RAW, DELTA_RLE, KEY_RAW,
                             KEY_RLE, REPEAT, pack_animations, unpack_animations)
from asset_codec import OUTLINE, rgb565
from pack_animations import animation_profile, key_and_prepare, parse_selections, sampling_indices


class AnimationCodecTests(unittest.TestCase):
    def setUp(self):
        self.size = (32, 24)
        self.palette = [0, rgb565(OUTLINE), rgb565((255, 200, 60)), rgb565((240, 80, 60))]
        empty = bytes(32 * 24)
        one = bytearray(empty)
        one[10 * 32 + 12:10 * 32 + 16] = bytes([2]) * 4
        moved = bytearray(empty)
        moved[11 * 32 + 13:11 * 32 + 17] = bytes([3]) * 4
        self.frames = [empty, bytes(one), bytes(one), bytes(moved), empty]
        self.actions = [{'id': 'idle', 'loop': True, 'frames': self.frames}]

    def pack(self, actions=None):
        return pack_animations(actions or self.actions, self.palette, self.size, (16, 23), 12)

    def repaired(self, data):
        data = bytearray(data)
        header = list(HEADER.unpack_from(data))
        for number in range(header[10]):
            at = header[11] + number * ACTION.size
            action = list(ACTION.unpack_from(data, at))
            offset, length = action[3:5]
            action[5] = binascii.crc32(data[offset:offset + length])
            ACTION.pack_into(data, at, *action)
        header[-1] = binascii.crc32(data[HEADER.size:])
        HEADER.pack_into(data, 0, *header)
        return bytes(data)

    def frame_offsets(self, data):
        header = HEADER.unpack_from(data)
        entry = ACTION.unpack_from(data, header[11])
        offset = entry[3]
        result = []
        for _ in range(entry[1]):
            result.append(offset)
            frame = FRAME.unpack_from(data, offset)
            offset += FRAME.size + frame[-2]
        return result

    def test_roundtrip_movement_erases_prior_transparent_pixels(self):
        decoded = unpack_animations(self.pack())
        self.assertEqual(decoded.actions['idle']['frames'], self.frames)
        self.assertEqual(decoded.size, self.size)
        self.assertEqual(decoded.anchor, (16, 23))
        self.assertEqual(decoded.fps, 12)
        self.assertTrue(decoded.actions['idle']['loop'])

    def test_delta_and_repeat_are_used_and_reconstruct_exactly(self):
        records = unpack_animations(self.pack()).actions['idle']['records']
        self.assertIn(records[0]['mode'], (KEY_RLE, KEY_RAW))
        self.assertIn(records[1]['mode'], (DELTA_RLE, DELTA_RAW))
        self.assertEqual(records[2]['mode'], REPEAT)
        self.assertEqual(records[2]['payload_bytes'], 0)
        self.assertEqual(records[1]['rectangle'], (12, 10, 4, 1))

    def test_output_deterministic_and_action_order_canonical(self):
        other = {'id': 'toss', 'loop': False, 'frames': self.frames[:2]}
        self.assertEqual(self.pack([other, *self.actions]), self.pack([*self.actions, other]))
        self.assertEqual(self.pack(), self.pack())

    def test_noisy_keyframes_have_raw_fallback(self):
        frame = bytes(i % len(self.palette) for i in range(32 * 24))
        decoded = unpack_animations(self.pack([{'id': 'noise', 'frames': [frame]}]))
        self.assertEqual(decoded.actions['noise']['frames'][0], frame)
        self.assertEqual(decoded.actions['noise']['records'][0]['mode'], KEY_RAW)

    def test_encoder_rejects_invalid_inputs(self):
        for actions in ([{'id': 'bad-name', 'frames': self.frames}], self.actions * 2,
                        [{'id': 'idle', 'frames': []}],
                        [{'id': 'idle', 'frames': [b'short']}],
                        [{'id': 'idle', 'frames': [bytes([7]) * (32 * 24)]}]):
            with self.subTest(actions=actions[0].get('id')), self.assertRaises(ValueError):
                self.pack(actions)
        for palette in ([0], [0, 1], [0, rgb565(OUTLINE)] + list(range(63))):
            with self.assertRaises(ValueError):
                pack_animations(self.actions, palette, self.size, (16, 23), 12)

    def test_truncation_and_body_corruption_rejected(self):
        packed = self.pack()
        for data in (b'', packed[:HEADER.size - 1], packed[:-1], packed + b'junk'):
            with self.assertRaises(ValueError):
                unpack_animations(data)
        corrupt = bytearray(packed)
        corrupt[-1] ^= 1
        with self.assertRaisesRegex(ValueError, 'CRC'):
            unpack_animations(corrupt)

    def test_header_dimensions_anchor_fps_and_offsets_rejected(self):
        for field, value in ((4, 0), (5, 2048), (6, 32), (7, 24), (8, 0),
                             (9, 65), (10, 257), (11, 0), (12, 0)):
            corrupt = bytearray(self.pack())
            header = list(HEADER.unpack_from(corrupt))
            header[field] = value
            HEADER.pack_into(corrupt, 0, *header)
            with self.subTest(field=field), self.assertRaises(ValueError):
                unpack_animations(corrupt)

    def test_action_name_and_bounds_rejected(self):
        for field, value in ((0, b'x' * 16), (1, 0), (2, 2), (3, 0), (4, 1), (6, 1)):
            corrupt = bytearray(self.pack())
            header = HEADER.unpack_from(corrupt)
            action = list(ACTION.unpack_from(corrupt, header[11]))
            action[field] = value
            ACTION.pack_into(corrupt, header[11], *action)
            # Only repair bodyCRC so malformed offsets aren't dereferenced by test helper.
            header = list(header)
            header[-1] = binascii.crc32(corrupt[HEADER.size:])
            HEADER.pack_into(corrupt, 0, *header)
            with self.subTest(field=field), self.assertRaises(ValueError):
                unpack_animations(corrupt)

    def test_firstframe_delta_rejected_after_allCRCs_repaired(self):
        corrupt = bytearray(self.pack())
        corrupt[self.frame_offsets(corrupt)[0]] = DELTA_RLE
        with self.assertRaisesRegex(ValueError, 'previous'):
            unpack_animations(self.repaired(corrupt))

    def test_out_of_bounds_delta_rejected_after_CRC_repair(self):
        corrupt = bytearray(self.pack())
        at = self.frame_offsets(corrupt)[1]
        frame = list(FRAME.unpack_from(corrupt, at))
        frame[4] = 32
        FRAME.pack_into(corrupt, at, *frame)
        with self.assertRaisesRegex(ValueError, 'rectangle'):
            unpack_animations(self.repaired(corrupt))

    def test_repeat_payload_metadata_rejected(self):
        corrupt = bytearray(self.pack())
        at = self.frame_offsets(corrupt)[2]
        frame = list(FRAME.unpack_from(corrupt, at))
        frame[2] = 1
        FRAME.pack_into(corrupt, at, *frame)
        with self.assertRaisesRegex(ValueError, 'repeat'):
            unpack_animations(self.repaired(corrupt))

    def test_fullframe_crc_detects_wrong_reconstructed_pixels(self):
        corrupt = bytearray(self.pack())
        at = self.frame_offsets(corrupt)[1]
        frame = list(FRAME.unpack_from(corrupt, at))
        frame[-1] ^= 1
        FRAME.pack_into(corrupt, at, *frame)
        with self.assertRaisesRegex(ValueError, 'reconstructed'):
            unpack_animations(self.repaired(corrupt))

    def test_24fps_profile_preserves_action_durations(self):
        standard, smooth = animation_profile(12), animation_profile(24)
        for ident in standard:
            old_count, old_loop, old_pose = standard[ident]
            count, loop, pose = smooth[ident]
            self.assertEqual(count, old_count * 2)
            self.assertEqual(count / 24, old_count / 12)
            self.assertEqual((loop, pose), (old_loop, old_pose))
        for invalid in (0, 1, 25):
            with self.assertRaises(ValueError):
                animation_profile(invalid)
        decoded = unpack_animations(pack_animations(self.actions, self.palette, self.size, (16, 23), 24))
        self.assertEqual(decoded.fps, 24)
        self.assertEqual(decoded.actions['idle']['frames'], self.frames)

    def test_source_selection_is_integer_deterministic_and_preserves_ends(self):
        self.assertEqual(sampling_indices(49, 8), [0, 7, 14, 21, 27, 34, 41, 48])
        self.assertEqual(sampling_indices(4, 3), [0, 2, 3])
        with self.assertRaises(ValueError):
            sampling_indices(5, 10)
        self.assertEqual(parse_selections(['serve=0,8,14,21,27,33,41,48'])['serve'][5], 33)
        for text in ('unknown=0,1', 'idle=notanumber', 'idle'):
            with self.assertRaises(ValueError):
                parse_selections([text])

    def test_green_key_does_not_erase_blue_clothing_and_despills_edge(self):
        image = Image.new('RGB', (720, 720), (0, 255, 0))
        for x in range(250, 470):
            for y in range(100, 720):
                image.putpixel((x, y), (35, 55, 110))
        for y in range(100, 720):
            image.putpixel((249, y), (55, 70, 58))
        prepared = key_and_prepare(image)
        self.assertEqual(prepared.mask.size, (192, 192))
        self.assertEqual(prepared.mask.getpixel((0, 0)), 0)
        self.assertEqual(prepared.mask.getpixel((96, 100)), 255)
        self.assertGreater(prepared.stats['source_green_pixels_keyed'], 0)
        self.assertGreater(prepared.stats['source_edge_pixels_despilled'], 0)
        self.assertTrue(prepared.stats['character_canonicalization_disabled'])


if __name__ == '__main__':
    unittest.main()
