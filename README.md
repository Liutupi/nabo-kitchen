<p align="right"><a href="README.zh_CN.md">简体中文</a> · <strong>English</strong></p>

# NABO Kitchen for AI Passport

For the saved state, installed device version and remaining work, read the [offline kitchen handoff](docs/development/offline-kitchen-handoff.md).

A standalone offline kitchen companion for the FoloToy AI Passport (ESP32-C3, 8 MiB flash, no PSRAM). It starts with a NABO animation and a locally stored greeting, then stays on the opening screen until a short press of OK opens the kitchen. Network access, cloud conversation and voice recognition are not part of this variant.

The current candidate contains 144 family-style recipe drafts, complete dish illustration drafts, layered dish illustrations, an animated character, a pantry and shopping list, favorites, cooking progress and up to three independent timers. Recipe instructions remain pending actual household cooking review. Some dishes reuse approximate illustrations; newly added dishes without complete art display a placeholder. The new icons, dish art and animation variants remain visual-review candidates.

The model supports up to 192 recipes. For catalogs over 64 recipes, multi-dish draws use a seeded, bounded candidate pool for responsiveness; their weighting and soft-constraint relaxation may differ from an exhaustive search. Ingredient exclusions do not screen seasonings, allergens or cross-contact. Full 192-recipe records pass repeated saves and simulated power cuts using ESP-IDF's NVS engine in 24 KiB, alongside 2 KiB of unrelated settings. Actual device persistence and occupied-partition limits still need measurement; storage exhaustion reports an error while preserving the active save and other namespaces.

## Controls and time

On the opening screen, briefly press OK to enter; if a timer alarm is pending, OK opens that alarm first. Elsewhere, use UP and DOWN to move, OK to select, double UP to go back, and long UP to return to the kitchen home. Long OK on the home screen draws a meal; long DOWN on the home screen opens the manual date editor. In the editor, UP/DOWN change the highlighted field, OK advances through year, month, day and confirmation, and double UP cancels.

On a draw result, press DOWN to draw again as often as desired. On the timer page, UP/DOWN selects a timer, long OK pauses or resumes it, and long DOWN cancels the selected timer. Canceling a step timer does not restart it when returning to that step.

In the fridge, use UP/DOWN to select meat, seafood, egg and tofu, vegetables or staples, then press OK to enter that category. Use OK to toggle an ingredient. Double UP returns to the categories; double UP again returns to the previous page. Long OK opens fridge recommendations. Complete main-ingredient matches appear first; a one-missing recommendation must share at least one main ingredient with the fridge.

The date is not an automatically updated clock. Confirm it again after a restart and update it when the day changes. While the date is unconfirmed, the application avoids pretending that seasonal weighting, recent-day history or streaks are accurate; cooking still counts toward total dishes. If power is lost during a running timer, it is restored paused and the user is notified. The startup greeting, timer alarm and animations play locally.

## Build and verification

Activate ESP-IDF 5.5.3, then run `./tools/validate.sh`. The full gate includes repository and host tests, an isolated target build, merged-image validation and a matching debug archive. After changing any of the six source assets in `assets/kitchen`, regenerate the deterministic bundle using `python3 tools/pack_kitchen_assets.py` before the build. The recipe and art source project is maintained separately.

This candidate uses a product-specific partition table: NVS and PHY retain their original addresses, the factory application occupies `0x10000..0x1fffff`, the read-only kitchen resource partition occupies `0x200000..0x6fffff`, and the pre-existing `files` region remains `0x700000..0x7fffff`. A partition-table or bootloader change must be considered explicitly before flashing. The merged image fills gaps and can overwrite NVS; a user-approved segmented install is required when preserving existing data. No complete chip erase or backup of the old firmware is required.

A successful build is not hardware acceptance. The display, buttons, speech, audio alarm next to the extractor, three concurrent timers, power-loss recovery and actual animation speed require device testing. Hardware and development references are indexed in [docs/README.md](docs/README.md).
