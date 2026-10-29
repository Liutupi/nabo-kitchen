<p align="right"><a href="offline-kitchen-handoff.zh_CN.md">简体中文</a> · <strong>English</strong></p>

# Offline kitchen handoff — 2026-10-29

Work resumed at the user's request. The first 144-recipe startup watchdog was repaired. With the user's explicit approval, the app-only repair was flashed and verified; its startup retest has no watchdog warning. Clock-warning acknowledgement, welcome audio and the isolated feature flow remain pending. This is not a release. Do not flash or erase the device merely because this branch exists.

## Where the work lives

- Firmware: `Liutupi/nabo-kitchen`, branch `codex/offline-kitchen`; local checkout `/Volumes/NABO-Build-20261004/ai-passport-nabo-kitchen-offline`.
- Editable recipes, artwork, packing tools and browser prototype: the same GitHub repository, branch `codex/kitchen-model`; local checkout `/Volumes/NABO-Build-20261004/nabo-kitchen`.
- Firmware base: FoloToy AI Passport template commit `a06235e`. Target: ESP32-C3, 8 MiB Flash, no PSRAM, ESP-IDF 5.5.3. The original workspaces were preserved.

## Firmware currently on the device

The approved **144-recipe / 58-ingredient / 17-complete-illustration** candidate was written and independently verified, app SHA256 `5b77ba87157481f4be8e5e70540f0984d1f2131e8cfc3117a60503c47203ed15`, resource SHA256 `7556d051d6adb1868b452c5f341dd1592ff3415ac551b1ec6a077ca3cad339ec`. Identity, 8 MiB capacity, disabled Secure Boot/encryption, the retained bootloader and unchanged partition table were checked before writing. Only app/resources were written; NVS, PHY and files were preserved.

The installed bootloader came from the earlier compatible offline candidate (SHA256 `c20c3a4bd3d38e6670bda9ed1a67d6883b7290122e40a3a31371e2a84aa95bd7`). The partition table SHA256 is `23e0ea7332832e5612b11dd15cfd0b1682a05e1369a86308f89cbedadb11e499`. Later updates wrote only the approved app and resource segments. This mixed installed state must not be identified by a newer complete-image hash.

The user confirmed the opening animation and welcome voice, the opening screen holding until OK, and physical fridge category/ingredient selection. A 128-version isolated test reached the seafood page, selected ingredients, captured all 320 display rows, and restored normal mode. The earlier 104-version isolated flow passed home → conditions → draw → ingredients → cooking. These observations do not certify every timer, power-loss case, animation frame rate or household recipe.

The earlier four-dish test was refused with `pending-save`. New read-only status confirms the cause: a restored timer with an unacknowledged clock-loss warning (`dirty=1`, `clock_lost=1`, `storage_fault=0`, journal sequence 140). The user was asked to press OK twice; no records were cleared. Reopening the native USB port produced another fresh boot log, so acknowledgement and isolated testing must share one persistent serial connection to avoid resetting the restored timers again. Initial 144-version status reports 194,900 free heap bytes, largest block 114,688 bytes, and 8,672 minimum free UI stack bytes. These precede the four-dish stress test.

Full startup logs also contain an IDLE-task watchdog warning at 5,437 ms while `kitchen_ui` was running. Matching ELF `17c0f929a...` decodes the sampled PC/RA to a FreeRTOS critical-section exit, not a complete caller trace. Resource validation completed at 2,970 ms; uninterrupted higher-priority initialization delayed the idle task until startup completed. The repair yields between bundle validation, resource parsing and catalog initialization without disabling the watchdog. The approved repair passed a physical startup retest: embedded ELF prefix `2650225e8` matches the archived ELF, and no watchdog or audio-driver error occurred in the bounded startup window. Post-boot free heap was 198,368 bytes and minimum UI stack remaining was 8,656 bytes. The user reported no welcome sound on the earlier app; actual audible playback on the repair is not yet confirmed. The PCM is non-silent (62,400 bytes, 1.95 seconds), but this does not establish speaker output. All monitors were closed; raw logs and target identity remain private.

## Latest implementation

- 144 recipe drafts, 58 ingredients, 13 seafood types and 17 complete dish illustrations. Remaining dishes use existing layers or an explicit art-pending placeholder. Recipes and new art remain subject to household cooking and visual review.
- Model capacity increased to 192 recipes; stable-ID KST1 migration preserves older recipe and ingredient selections. Multi-dish draws above 64 recipes use bounded candidate pools and are not equivalent to exhaustive weighted search.
- Unlimited redraws, removal of the obsolete daily-redraw setting, saturation-safe redraw state, active cancellation of the selected timer with long DOWN, and updated NABO opening/home greeting text (exact wording in the Simplified Chinese handoff). Welcome audio remains unchanged.
- The UI worker stack is 12 KiB, and read-only status includes `stack_free_min` for the next device measurement.
- Read-only status also reports recipe/ingredient counts, dirty state, clock-loss warning, storage fault and journal sequence, so a refused test can be diagnosed without clearing records.
- Current resource bundle: 2,480,194 bytes, SHA256 `7556d051d6adb1868b452c5f341dd1592ff3415ac551b1ec6a077ca3cad339ec`; the 5 MiB asset partition has 2,762,686 bytes free. Font and dish packs deliberately exceed the old design's individual budgets; that does not mean those older budgets were met.

## Validation at this checkpoint

```text
Build: PASS for the complete ESP-IDF 5.5.3 gate
Host tests: PASS for static, actual 144-recipe rendering/migration and NVS stress
Device tests: PASS for repaired startup only; audio and feature acceptance pending
Unverified: physical clock-warning acknowledgement, welcome audio, device seafood/icon appearance, unlimited redraw, timer cancellation,
worker stack, occupied-device NVS capacity and physical power-loss behavior
```

Static checks, all nine actual-resource rendering tests and real 128-recipe/50-ingredient KST1 migration passed. Component tests cover 300 redraws including the 255 boundary, timer cancellation and save restoration. Actual rendering/migration and the new NVS stress test are now part of the unified gate.

The 192-recipe fully populated save is 8,231 bytes; its buffer is 9,216 bytes. The old NVS replacement failed on the third maximum-state save: atomic replacement needed both journal slots plus a third blob. On an out-of-space result, the adapter now reclaims only the inactive destination and retries, keeping the active slot until read-back verification succeeds. KST1 bytes and stable-ID compatibility are unchanged.

Using unmodified ESP-IDF 5.5.3 NVS storage/page/GC sources with synthetic NOR I/O, 100 maximum-state saves and reboots passed in 24 KiB alongside 2 KiB of unrelated settings. All 484 flash-operation cut boundaries in a replacement recovered either the preceding or new complete save. A fully occupied partition reports failure while preserving the active save and unrelated namespace. These are host results; physical timing, torn writes within an operation and the actual device's occupied capacity remain unverified. Never erase settings or change partitions as an implicit repair.

## Verified local candidate

- Segmented package: `/Users/tupi/Library/Caches/nabo-kitchen-fw/offline-144-yield-907e959e29ff`.
- Debug archive: `build/firmware/907e959e29ffe0a5c2754c820317db24953315a32c44d90647a813d6491f9922/`.
- Full image SHA256: `907e959e29ffe0a5c2754c820317db24953315a32c44d90647a813d6491f9922`.
- App: 513,184 bytes; SHA256 `5b77ba87157481f4be8e5e70540f0984d1f2131e8cfc3117a60503c47203ed15`.
- Matching ELF SHA256: `2650225e83603a557c0153efa7cc1f932aa111b937a70264019c894fe936c7d5`; embedded version `697a44a-dirty` identifies the tested working tree before its source checkpoint commit.
- Resources retain SHA256 `7556d051d6adb1868b452c5f341dd1592ff3415ac551b1ec6a077ca3cad339ec`. Archive and segmented-package verification both passed. Generated firmware/debug files remain local.
- The intended port is `/dev/cu.usbmodem212401`. The first app/resource update was approved and completed. The repaired app-only update was explicitly approved and completed; installed resources, bootloader, table and user data were retained. The clock-loss warning still requires a physical OK acknowledgement before isolated tests. The latest user feedback on the earlier app was that the clock warning remained and no greeting was heard; do not mark these accepted.

GitHub synchronization is blocked: CLI authentication for Liutupi is invalid, the connected GitHub integration rejects tree writes with HTTP 403, and SSH access is unavailable. The source repair commit `697a44a` is local; remote `codex/offline-kitchen` remains `f67c7ee`. Do not claim the new work was pushed. Preserve local commits and resume pushing only after GitHub access is restored.

## Resume steps

1. Pull both branches, inspect status, and read this handoff and the design branch's `HANDOFF.md`. Check `assets/kitchen/checkpoint.json` against the source packs.
2. The complete `./tools/validate.sh` gate passed for the candidate above. Repeat it only after relevant changes; the actual-resource, migration and NVS tests are now included.
3. Verify the content-addressed debug archive, then build a segmented package with `tools/pack_offline_release.py`. Confirm the exact new artifact and intended device/write scope before flashing.
4. Preserve NVS at `0x9000..0xefff`, PHY at `0xf000..0xffff` and files at `0x700000..0x7fffff`. The compatible layout uses app `0x10000..0x1fffff` and kitchen assets `0x200000..0x6fffff`. A merged image fills gaps and must not be blindly written when preserving data.
5. Observe startup and the new behavior using bounded serial tests. Record heap/stack after repeated four-dish draws and check storage without deleting existing records. Restore normal mode and close the port.

The 1,000/1,500-entry experiment in `experiments/krc_stream/` is a synthetic indexing proof of concept only. It has not created a thousand real recipes and is not connected to production UI, recommendation or storage. Read its paired README before extending it.
