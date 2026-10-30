<p align="right"><a href="README.zh_CN.md">简体中文</a> · <strong>English</strong></p>

# NABO Kitchen for AI Passport

**v0.2.0 — experience prerelease:** [downloads and checksums](https://github.com/Liutupi/nabo-kitchen/releases/tag/v0.2.0) · [installation and controls](docs/community/v0.2.0/installation.md) · [release provenance](docs/community/v0.2.0/release-validation.json).

The [community project](https://ai-passport.folotoy.cn/plays/1755/) now publicly serves v0.2.0 revision 4104. Its anonymously downloaded firmware matches GitHub byte-for-byte; bilingual copy and all five images are verified. See [public verification](docs/community/v0.2.0/community-publication-verification.json).

<img src="assets/community/v0.1.0/01-cover.webp" width="450" alt="NABO Midnight Kitchen promotional illustration">

For the saved state, installed device version and remaining work, read the [offline kitchen handoff](docs/development/offline-kitchen-handoff.md).

A standalone offline kitchen companion for the FoloToy AI Passport (ESP32-C3, 8 MiB flash, no PSRAM). It starts with a NABO animation and a locally stored greeting, then stays on the opening screen until a short press of OK opens the kitchen. Network access, cloud conversation and voice recognition are not part of this variant.

The release contains 145 D2 recipes with complete dish illustrations, an animated character, 58 pantry ingredients including 13 seafood types, shopping, favorites, cooking progress and up to three independent timers. Recipes and artwork remain under review; three actual cooking sessions have not yet been completed. The creator explicitly requested an experience release with this scope disclosed. The accepted application and resources are retained byte-for-byte.

The model supports up to 192 recipes. For catalogs over 64 recipes, multi-dish draws use a seeded, bounded candidate pool for responsiveness; their weighting and soft-constraint relaxation may differ from an exhaustive search. Ingredient exclusions do not screen seasonings, allergens or cross-contact. Full 192-recipe records pass repeated saves and simulated power cuts using ESP-IDF's NVS engine in 24 KiB, alongside 2 KiB of unrelated settings. Actual device persistence and occupied-partition limits still need measurement; storage exhaustion reports an error while preserving the active save and other namespaces.

## Controls and time

On the opening screen, briefly press OK to enter; a pending timer alarm opens first. Use UP/DOWN to move, OK to select, double UP to go back and long UP to return directly home. Long OK on home draws a meal. No date setup is needed. Selected cards turn warm orange with dark labels.

On a draw result, DOWN draws again without a limit. The permanent Timers choice at the bottom of home reopens timers after leaving a recipe. In Timers, UP/DOWN selects, long OK pauses/resumes, long DOWN cancels the selected timer and double UP returns. Canceling does not restart that step timer or delete step progress.

In the fridge, use UP/DOWN to select meat, seafood, egg and tofu, vegetables or staples, then press OK to enter that category. Use OK to toggle an ingredient. Double UP returns to the categories; double UP again returns to the previous page. Long OK opens fridge recommendations. Complete main-ingredient matches appear first; a one-missing recommendation must share at least one main ingredient with the fridge.

Recency follows completed cooking sessions rather than days; the menu and draw count persist across reboot. Levels use unique dishes and total completions. Compatible old records migrate to KST2 and retired references are removed; old v0.1 firmware cannot directly read KST2. Running timers recover paused after power loss. The greeting, alarms and animations play locally.

## Build and verification

Activate ESP-IDF 5.5.3, then run `./tools/validate.sh`. The full gate includes repository and host tests, an isolated target build, merged-image validation and a matching debug archive. After changing any of the six source assets in `assets/kitchen`, regenerate the deterministic bundle using `python3 tools/pack_kitchen_assets.py` before the build. The recipe and art source project is maintained separately.

This candidate uses a product-specific partition table: NVS and PHY retain their original addresses, the factory application occupies `0x10000..0x1fffff`, the read-only kitchen resource partition occupies `0x200000..0x6fffff`, and the pre-existing `files` region remains `0x700000..0x7fffff`. A partition-table or bootloader change must be considered explicitly before flashing. The merged image fills gaps and can overwrite NVS; a user-approved segmented install is required when preserving existing data. No complete chip erase or backup of the old firmware is required.

A successful build is not hardware acceptance. The display, buttons, speech, audio alarm next to the extractor, three concurrent timers, power-loss recovery and actual animation speed require device testing. Hardware and development references are indexed in [docs/README.md](docs/README.md).
