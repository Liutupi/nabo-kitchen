<p align="right"><a href="release-notes.zh_CN.md">简体中文</a> · <strong>English</strong></p>

# NABO Midnight Kitchen v0.1.0

Your little offline companion for deciding what to cook tonight. Choose the number of people, available time and meal combination, draw a home-style dish, and keep drawing until one feels right. NABO stays with you from ingredients through the final step.

- 144 recipe drafts, each with a complete dish illustration.
- 58 pantry ingredients, including 13 seafood types; fridge recommendations and shopping.
- Ingredients, cooking steps, favorites, local cooking records and up to three pausable timers.
- Opening animation, local greeting and the approved C kitchen character.

This is a **first experience prerelease**. Recipes and illustrations remain under review; they have not all been tested by cooking. Recent dishes receive a lower weight and can reappear within seven days. Date is manually confirmed; interrupted timers recover paused. No network setup, cloud conversation or speech recognition is included.

## Downloads

| File | Use |
| --- | --- |
| `nabo-kitchen-v0.1.0-full.bin` | First installation at `0x0`; replaces current firmware and may reset records. |
| `nabo-kitchen-v0.1.0.zip` | Separate images, bilingual installation instructions, manifests, checksums and licenses. |
| `manifest.json` | Exact app/resource/full-image hashes, offsets, source provenance and validation limits. |
| `SHA256SUMS.txt` | Verify the three files above; the published checksum file is independently re-downloaded too. |

For an existing kitchen with the exact compatible layout, use only app at `0x10000` and resources at `0x200000` to preserve NVS/PHY and files. Follow the [installation guide](https://github.com/Liutupi/nabo-kitchen/blob/v0.1.0/docs/community/v0.1.0/installation.md) before writing.

The shipped app is unchanged SHA256 `a6f4a167bf1d79d11bd7dbbf2a46d56eef5362446bbcd41313ace56effcb1202`; the resource bundle is unchanged SHA256 `f1c0f215a460b6c4e7c8805527a6077ff2b8473626eb55205a77f9bda26d2931`. The new merged download is 5,853,214 bytes, SHA256 `9a7ade79bb7ebf4aa91332368a7dc4910af7cb86eb3aeec38775bc8380838a1c`. It is assembled from verified build outputs, with erased fill in every gap and no device dumps or credentials.

Build: PASS. Host tests: PASS. Limited device checks: PASS for resource readback, three controlled boots and six complete frame captures with 100,800 matching dish pixels. Physical observation of the updated panel/speaker, all-recipe cooking, full-art acceptance, long-term use and first installation of the packaged bootloader remain unverified. A fresh validation build is retained separately and is not substituted for the accepted app. [Detailed provenance](https://github.com/Liutupi/nabo-kitchen/blob/v0.1.0/docs/community/v0.1.0/release-validation.json).

The cover is promotional art; gallery captions identify code-rendered interfaces and device framebuffer captures. These are not physical screen photographs. Code follows its existing MIT notices; NABO character, art, animation, voice and recipe text are excluded from MIT. Bundled font licenses apply.

![Promotional cover](https://raw.githubusercontent.com/Liutupi/nabo-kitchen/v0.1.0/assets/community/v0.1.0/01-cover.webp)
