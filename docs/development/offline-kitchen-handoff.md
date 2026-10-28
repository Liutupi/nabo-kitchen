<p align="right"><a href="offline-kitchen-handoff.zh_CN.md">简体中文</a> · <strong>English</strong></p>

# Offline kitchen handoff — 2026-10-29

Work resumed at the user's request. The combined 144-recipe candidate now passes the complete gate and is packaged locally; device acceptance is pending. This is not a release. Do not flash or erase the device merely because this branch exists.

## Where the work lives

- Firmware: `Liutupi/nabo-kitchen`, branch `codex/offline-kitchen`; local checkout `/Volumes/NABO-Build-20261004/ai-passport-nabo-kitchen-offline`.
- Editable recipes, artwork, packing tools and browser prototype: the same GitHub repository, branch `codex/kitchen-model`; local checkout `/Volumes/NABO-Build-20261004/nabo-kitchen`.
- Firmware base: FoloToy AI Passport template commit `a06235e`. Target: ESP32-C3, 8 MiB Flash, no PSRAM, ESP-IDF 5.5.3. The original workspaces were preserved.

## Firmware currently on the device

The installed application is the verified **128-recipe / 50-ingredient / nine-complete-illustration** version. Application SHA256: `8ccfcbdfa7f8ef0ee3e9987a0a9da2167eb91974414ac74ed4d09e15e4569562`. Installed resource SHA256: `21ee0798fbcc8abf69cbf243517d26e50a87f0c1fee20b5d7a27d9897ded51e0`.

The installed bootloader came from the earlier compatible offline candidate (SHA256 `c20c3a4bd3d38e6670bda9ed1a67d6883b7290122e40a3a31371e2a84aa95bd7`). The partition table SHA256 is `23e0ea7332832e5612b11dd15cfd0b1682a05e1369a86308f89cbedadb11e499`. Later updates wrote only the approved app and resource segments. This mixed installed state must not be identified by a newer complete-image hash.

The user confirmed the opening animation and welcome voice, the opening screen holding until OK, and physical fridge category/ingredient selection. A 128-version isolated test reached the seafood page, selected ingredients, captured all 320 display rows, and restored normal mode. The earlier 104-version isolated flow passed home → conditions → draw → ingredients → cooking. These observations do not certify every timer, power-loss case, animation frame rate or household recipe.

The last four-dish device test was refused with `pending-save`; no record or timer was cleared to bypass it. The port was closed. Private device logs remain outside GitHub. Do not automatically open/reset the port during the next session before establishing the user's intended test scope.

## Latest source checkpoint: not yet flashed

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
Device tests: NOT RUN for the latest combined source
Unverified: device seafood/icon appearance, unlimited redraw, timer cancellation,
worker stack, occupied-device NVS capacity and physical power-loss behavior
```

Static checks, all nine actual-resource rendering tests and real 128-recipe/50-ingredient KST1 migration passed. Component tests cover 300 redraws including the 255 boundary, timer cancellation and save restoration. Actual rendering/migration and the new NVS stress test are now part of the unified gate.

The 192-recipe fully populated save is 8,231 bytes; its buffer is 9,216 bytes. The old NVS replacement failed on the third maximum-state save: atomic replacement needed both journal slots plus a third blob. On an out-of-space result, the adapter now reclaims only the inactive destination and retries, keeping the active slot until read-back verification succeeds. KST1 bytes and stable-ID compatibility are unchanged.

Using unmodified ESP-IDF 5.5.3 NVS storage/page/GC sources with synthetic NOR I/O, 100 maximum-state saves and reboots passed in 24 KiB alongside 2 KiB of unrelated settings. All 484 flash-operation cut boundaries in a replacement recovered either the preceding or new complete save. A fully occupied partition reports failure while preserving the active save and unrelated namespace. These are host results; physical timing, torn writes within an operation and the actual device's occupied capacity remain unverified. Never erase settings or change partitions as an implicit repair.

## Verified local candidate

- Segmented package: `/Users/tupi/Library/Caches/nabo-kitchen-fw/offline-144-nvs-0019ff996826`.
- Debug archive: `build/firmware/0019ff996826c0fcf8b278a7724886983d9a61e5428838ff9ce4e11865020a5f/`.
- Full image SHA256: `0019ff996826c0fcf8b278a7724886983d9a61e5428838ff9ce4e11865020a5f`.
- App: 513,136 bytes; SHA256 `9e21fad27eff0fc63efb6aeb26dcf01f2e29cfb8a722d3108ec7cf62a059579d`.
- Matching ELF SHA256: `17c0f929a080ff52757b07958144a818317deca0fb3322bfd300244250606713`; embedded version `f67c7ee-dirty` identifies the tested working tree before its source checkpoint commit.
- Resources retain SHA256 `7556d051d6adb1868b452c5f341dd1592ff3415ac551b1ec6a077ca3cad339ec`. Archive and segmented-package verification both passed. Generated firmware/debug files remain local.
- USB enumeration found the prior `/dev/cu.usbmodem212401`; no port was opened or reset. Approval was requested for this candidate's app/resource-only update; consent and hardware acceptance are still pending.

## Resume steps

1. Pull both branches, inspect status, and read this handoff and the design branch's `HANDOFF.md`. Check `assets/kitchen/checkpoint.json` against the source packs.
2. The complete `./tools/validate.sh` gate passed for the candidate above. Repeat it only after relevant changes; the actual-resource, migration and NVS tests are now included.
3. Verify the content-addressed debug archive, then build a segmented package with `tools/pack_offline_release.py`. Confirm the exact new artifact and intended device/write scope before flashing.
4. Preserve NVS at `0x9000..0xefff`, PHY at `0xf000..0xffff` and files at `0x700000..0x7fffff`. The compatible layout uses app `0x10000..0x1fffff` and kitchen assets `0x200000..0x6fffff`. A merged image fills gaps and must not be blindly written when preserving data.
5. Observe startup and the new behavior using bounded serial tests. Record heap/stack after repeated four-dish draws and check storage without deleting existing records. Restore normal mode and close the port.

The 1,000/1,500-entry experiment in `experiments/krc_stream/` is a synthetic indexing proof of concept only. It has not created a thousand real recipes and is not connected to production UI, recommendation or storage. Read its paired README before extending it.
