<p align="right"><a href="offline-kitchen-handoff.zh_CN.md">简体中文</a> · <strong>English</strong></p>

# Offline kitchen handoff — 2026-10-29

## Task 2 passed: community first installation

Work now follows only `main` for content and `firmware` for firmware, in the order specified by [the main handoff](https://github.com/Liutupi/nabo-kitchen/blob/main/HANDOFF.md). The earlier branch names below are historical.

After a verified private 8 MiB backup, the user installed the exact v0.1.0 full image through the community page's Install latest button in Edge. The page showed installation success. Independent readback matched the bootloader, partition table, application and resources byte for byte. A subsequent 28-second controlled boot matched the accepted ELF prefix, validated the resource bundle at 2,277 ms and showed no watchdog, panic or storage fault. The serial observer is closed.

The user confirmed kitchen entry, a recipe workflow through completion, and physical timer pause/cancel after receiving the long-press instructions. This is a workflow check, not real cooking or separate listening acceptance. The packaged bootloader works in this test, so task 2 does not require a bootloader replacement or v0.1.1 repack. See the [sanitized report](offline-first-install-validation-20261029.json) and [community success proof](../../assets/community/v0.1.0/first-install-success.jpg).

Next is task 3, clean promotional captures. Record clearer timer-control help as a usability follow-up; the agreed controls remain unchanged. D2 resources and v0.2 are not installed by this test, and the original v0.1.0 tag and Release assets remain unchanged.

## Community publication verified: project 1755, revision 4068

The server approved revision 4068 at `2026-10-29T09:26:31Z`. The creator workbench now says published and its actual copied public URL is [the community page](https://ai-passport.folotoy.cn/plays/1755/?v=4068-2). The public page exposes the Install latest button, five images, the introduction and source link. Anonymous page and public API requests return HTTP 200, with project 1755 / revision 4068 / published. The earlier pending sections below are historical.

The public-page resource inventory exposed `/api/plays/id/1755`; its anonymously fetched `downloadUrl` and `firmware.url` both identify `/api/download/community/nabo-6`. Anonymous download returns 5,853,214 bytes, SHA256 `9a7ade79bb7ebf4aa91332368a7dc4910af7cb86eb3aeec38775bc8380838a1c`, exactly matching the accepted GitHub full image. All five anonymous public gallery downloads match the previously reviewed server-returned images. See [public verification](../community/v0.1.0/community-publication-verification.json), [current state](../community/v0.1.0/submission-status.json) and [public-page proof](../../assets/community/v0.1.0/community-published.jpg).

The Install latest entry requested device selection; no device was selected and the page was reloaded to cancel. No hardware write, rebuild, resubmission, new authorization, tag change or Release asset replacement occurred. Community approval is not recipe, art or hardware acceptance: all 144 illustrations remain review. Continue the existing quiet GitHub/content/public-availability monitor without repeating unchanged publication notices.

## Community submission: project 1755, revision 4068

After the user explicitly confirmed the website rights/rules statement and submission, the official publisher 1.13.0 created exactly one new firmware project: `1755`, revision `4068`, slug `nabo-6`. Both the official API receipt and creator workbench show **pending / under review**. There is no published revision or confirmed public community page yet. The earlier authorization wait below is historical and resolved. Do not authorize or submit again.

All eight bilingual title, description, instructions and changelog fields were read back exactly. The saved firmware link `/api/me/revisions/4068/firmware` was downloaded through the authenticated browser: 5,853,214 bytes, SHA256 `9a7ade79bb7ebf4aa91332368a7dc4910af7cb86eb3aeec38775bc8380838a1c`, byte-identical to the GitHub release. Anonymous access to that private saved-version endpoint returns HTTP 401; CDN redirects were not separately observed. This is not evidence of public community availability.

All five saved images were downloaded in order and individually viewed. The returned WebP bytes differ from uploads, consistent with server recompression; dimensions remain 900 x 1200, layouts/text are intact, and pixel PSNR is 37.98-43.35 dB. Both original and returned hashes are recorded. See [community verification](../community/v0.1.0/community-verification.json), [submission state](../community/v0.1.0/submission-status.json), and [the review receipt screenshot](../../assets/community/v0.1.0/submission-review-status.jpg). No fresh build or hardware write occurred in this submission. Continue the existing quiet monitor; notify on a real moderation result, failure or required decision, and do not send unchanged pending reminders.

## Release preparation: v0.1.0 experience release

GitHub publication is now complete: [v0.1.0](https://github.com/Liutupi/nabo-kitchen/releases/tag/v0.1.0), a public prerelease targeting `77ef2a926281ef7e5c4376ecd7158af1e915b06a`. All four public assets, including the checksum file, were independently downloaded without authentication and match their prepared byte counts and hashes. ZIP members/checksums and merged erased gaps also passed. See [publication verification](../community/v0.1.0/publication-verification.json) and [submission status](../community/v0.1.0/submission-status.json). Community materials are finished and preview-validated; submission is waiting for the user's action-time confirmation of the website rights/rules statement. Do not repeat the authorization or submission after a receipt exists. A pending receipt is not public community availability. No new hardware write was performed for publication.

The first release reuses the accepted app and installed 144-hero resource bundle. The new merged image is `9a7ade79bb7ebf4aa91332368a7dc4910af7cb86eb3aeec38775bc8380838a1c`, 5,853,214 bytes. The full build/host gate and strict archive, segmented-package and erased-gap checks passed. The fresh validation build was retained separately; it is not the shipped application. See the [release record](../community/v0.1.0/release-validation.json), [installation](../community/v0.1.0/installation.md) and [community media](../../assets/community/README.md). The release branch includes all 144 heroes; the older runtime branch and its accepted 17-hero archive are unchanged. Tag-triggered automatic publication is disabled to protect the accepted binaries. GitHub upload verification and the community receipt will be added after their actual outcomes. Community authorization requires confirming the live website rights/rules declaration; no submission or public community availability is claimed yet. No new hardware writes are part of publishing.

## Latest device session: 144 complete dish illustrations installed

The user explicitly requested flashing and validation of the completed art. After verifying the same previously approved board, its 8 MiB Flash/security state, installed application, partition table and old assets, only the resource bundle was written at `0x200000`. The 3,756,062-byte bundle and its complete device readback both have SHA256 `f1c0f215a460b6c4e7c8805527a6077ff2b8473626eb55205a77f9bda26d2931`. The application remains the accepted `a6f4a167bf1d79d11bd7dbbf2a46d56eef5362446bbcd41313ace56effcb1202` image; no new application build, whole-chip erase or Release was performed. Application, partition table, bootloader, NVS/PHY and files preservation was verified.

Three controlled boots matched ELF prefix `698af7057`, validated the new bundle at approximately 2,278 ms, and showed no watchdog/panic. Six complete 240x320 device captures cover egg custard, corn/rib soup, cabbage/tofu, tomato/egg, green-pepper/razor-clam soup and steamed bass, including catalog indices 142 and 143. Their 100,800 dish-area RGB565 pixels exactly match the unchanged production C renderer. Final free heap is 198,336 bytes, largest block 114,688, and minimum UI stack headroom after this update 6,968. TEST mode was exited and the serial port closed. The original paused timer and sound/70% volume were restored; journal sequence remained 151.

See the [sanitized device report](offline-art-device-validation-20261029.json) and [six device captures with pixel checks](https://github.com/Liutupi/nabo-kitchen/tree/codex/kitchen-model/game-art/kitchen/dish-hero/batches/2026-10-29/device-captures). Eleven capture requests yielded six retained frames; one complete wrong-page capture and four incomplete/unterminated transfers were excluded. A complete fried-rice USB frame was not obtained and is not counted as passed. Existing USB capture framing/flow control remains an open diagnostic-tool limitation. The user has not yet seen the panel or heard the greeting. These observations remain unverified, with no failure reported; consumed voice data and zero audio-write errors do not replace human observation.

The editable art and exact bundle are on `codex/kitchen-model`, initially delivered at `116ee8ea2ed72e5e5094805c397b8f1449d08b75`. All art remains `review`. This firmware checkout's default `assets/kitchen` and accepted full-image archive deliberately retain the older 17-hero fixture; they were not overwritten or rebuilt for this resource-only device session. Reuse the art branch's exact `resource-candidate/kitchen_bundle.bin` and report when continuing, and do not describe the old merged-image hash as the updated device's full-Flash identity. Raw logs, device identity and protected-region copies remain local under ignored `build/device-qa/`.

```text
Build: NOT RUN (accepted application and debug archive verified and reused)
Host tests: PASS (candidate checks and six device/C pixel comparisons)
Device tests: PASS (resource readback, three boots, six captures, state restoration)
Unverified: actual panel/audio observation, all-recipe visual/cooking acceptance,
long-term use, physical power/full-NVS cases and capture transport robustness
```

## Latest continuation: Windows runtime fixes

This section supersedes the earlier stop checkpoint below. Work continued from
`f799a2f3196e0d28a1777663a7cb65fcd8d84c4d` on
`codex/runtime-fixes-20261029`, in a separate checkout. The editable-content
branch remains `codex/kitchen-model`. Recipes, artwork, PCM, partition layout,
and KST1 bytes were not changed.

- Bundle checksums now cooperate after each at-most-4096-byte slice; catalog
  loading cooperates after each validated recipe. The device worker blocks for
  one positive scheduler tick. CRC uses a small nibble table with the same IEEE
  result, independently checked against zlib, including seeded/chunked inputs.
  Watchdog checks remain enabled.
- A real clock-warning acknowledgement marks the recovered paused state dirty
  and saves immediately in the UI worker. The previous 1.5-second deferred-save
  window is removed. Running timers still intentionally recover paused when no
  trusted clock exists; a future interrupted running timer may warn again.
- Read-only status adds sound/volume/mixer gain, audio availability, greeting
  requests/starts/sample consumption, write errors, and timer-state counts.
  The actual device had saved sound disabled: no greeting was requested. The
  user enabled sound through Settings and confirmed normal audible speech after
  reboot. No driver or greeting PCM was replaced and mute is still respected.
- Windows host checks now unload test DLLs before cleanup, link real COFF
  dependencies, trap unexpected demo-screen calls, provide missing host libc
  support for the unmodified IDF NVS engine, and report paths consistently.
  Symlink-dependent tests probe capability rather than require elevation.

### Validation and installed candidate

The complete `./tools/validate.sh --all` gate passed using ESP-IDF 5.5.3 and
native Windows build tools; 21 symlink-capability checks were skipped because
this host cannot create directory symlinks. Real-resource rendering has 10
passing cases. The NVS engine still passes 100 maximum-state saves and 484
simulated power-cut boundaries. See the [sanitized validation record](offline-runtime-validation-20261029.json).

- Installed app: **514,704 bytes**, SHA256
  `a6f4a167bf1d79d11bd7dbbf2a46d56eef5362446bbcd41313ace56effcb1202`.
- Matching ELF:
  `698af70572f6700387fa72f9af029fdede66b95dfcc2302f6ab01eeef1b9793f`.
  The compiled working-tree version is `f799a2f-dirty`; the validation record
  hashes the exact production inputs. Do not substitute a later rebuild.
- Full candidate archive:
  `build/firmware/ea2bae8053a427827bc912aff570aa2fa9b84ce1da8e95ca2e19dd3d421c2f4b/`.
  Segmented package: `build/candidates/offline-runtime-ea2bae8053a4/`.
  These generated files remain local and ignored.
- Following explicit approval and device clarification, the live partition
  table and resources matched before writing. Only the application at `0x10000`
  was written and independently verified. Bootloader, partition table, resources,
  NVS/PHY/files were not flashed. Resource SHA256 remains
  `7556d051d6adb1868b452c5f341dd1592ff3415ac551b1ec6a077ca3cad339ec`.
  The full-candidate hash is not an identity for the mixed retained device Flash.

Six controlled boots captured the matching ELF prefix without watchdog/panic
messages; bundle validation completed at about 1,655 ms. Each boot requested one
greeting, consumed 31,200 PCM samples, and recorded zero audio write errors. The
user confirmed actual sound. The acknowledged clock warning did not recur and
the original paused timer survived. Isolated device checks covered four-dish
results and four additional completed rerolls, both seafood pages, ingredients
to shopping, and timer start/pause/resume/cancel. Only test-mode injection was
used for automation; this does not certify physical long-press feel.

The stored sequence remained unchanged throughout isolation. TEST was exited,
the original paused timer/preferences were restored, and the serial port was
closed. Final free heap was 198,336 bytes, largest block 114,688 bytes; minimum
observed UI stack headroom during the tests was 5,664 bytes. Valid screenshots
contain all 320 rows. Occasional truncated USB capture streams were rejected
and retried; capture transport framing/flow control remains a diagnostic-tool
improvement, not a claim that every capture attempt succeeded.

```text
Build: PASS
Host tests: PASS (21 host symlink-capability skips)
Device tests: PASS (bounded startup, greeting, recovery and isolated flows above)
Unverified: long-term kitchen use, physical power-cut/full-NVS behavior,
all recipe content and physical timer-button feel
```

Continue from the runtime-fixes branch and this validation identity. Do not
flash the full merged image over existing user data. The 144 recipes remain
drafts; no formal release or complete hardware certification is claimed.

## Earlier macOS checkpoint

The user requested a cross-computer handoff. Development and device tests are stopped for this checkpoint, and the serial port is closed. The installed 144-recipe app is not hardware-accepted: a later reboot reproduced the watchdog warning, and the user confirmed that the greeting is still inaudible. The earlier successful startup window was insufficient evidence of a complete repair. This is not a release. Do not flash or erase the device merely because this branch exists.

## Where the work lives

- Firmware: `Liutupi/nabo-kitchen`, branch `codex/offline-kitchen`; local checkout `/Volumes/NABO-Build-20261004/ai-passport-nabo-kitchen-offline`.
- Editable recipes, artwork, packing tools and browser prototype: the same GitHub repository, branch `codex/kitchen-model`; local checkout `/Volumes/NABO-Build-20261004/nabo-kitchen`.
- Firmware base: FoloToy AI Passport template commit `a06235e`. Target: ESP32-C3, 8 MiB Flash, no PSRAM, ESP-IDF 5.5.3. The original workspaces were preserved.

## Firmware currently on the device

The approved **144-recipe / 58-ingredient / 17-complete-illustration** candidate was written and independently verified, app SHA256 `5b77ba87157481f4be8e5e70540f0984d1f2131e8cfc3117a60503c47203ed15`, resource SHA256 `7556d051d6adb1868b452c5f341dd1592ff3415ac551b1ec6a077ca3cad339ec`. Identity, 8 MiB capacity, disabled Secure Boot/encryption, the retained bootloader and unchanged partition table were checked before writing. Only app/resources were written; NVS, PHY and files were preserved.

The installed bootloader came from the earlier compatible offline candidate (SHA256 `c20c3a4bd3d38e6670bda9ed1a67d6883b7290122e40a3a31371e2a84aa95bd7`). The partition table SHA256 is `23e0ea7332832e5612b11dd15cfd0b1682a05e1369a86308f89cbedadb11e499`. Later updates wrote only the approved app and resource segments. This mixed installed state must not be identified by a newer complete-image hash.

The user confirmed the opening animation and welcome voice, the opening screen holding until OK, and physical fridge category/ingredient selection. A 128-version isolated test reached the seafood page, selected ingredients, captured all 320 display rows, and restored normal mode. The earlier 104-version isolated flow passed home → conditions → draw → ingredients → cooking. These observations do not certify every timer, power-loss case, animation frame rate or household recipe.

The earlier four-dish test was refused with `pending-save`. New read-only status confirms the cause: a restored timer with an unacknowledged clock-loss warning (`dirty=1`, `clock_lost=1`, `storage_fault=0`, journal sequence 140). The user confirmed that pressing OK cleared the warning; no records were cleared. After the next port reopen/reboot, the warning returned with journal sequence 143 (`dirty=1`, `clock_lost=1`, `storage_fault=0`); acknowledgement persistence and timer status still need investigation. Reopening the native USB port produced another fresh boot log, so acknowledgement and isolated testing must share one persistent serial connection to avoid resetting the restored timers again. Initial 144-version status reports 194,900 free heap bytes, largest block 114,688 bytes, and 8,672 minimum free UI stack bytes. These precede the four-dish stress test.

Full startup logs also contain an IDLE-task watchdog warning at 5,437 ms while `kitchen_ui` was running. Matching ELF `17c0f929a...` decodes the sampled PC/RA to a FreeRTOS critical-section exit, not a complete caller trace. Resource validation completed at 2,970 ms; uninterrupted higher-priority initialization delayed the idle task until startup completed. The repair yields between bundle validation, resource parsing and catalog initialization without disabling the watchdog. The approved app initially had a clean startup window, but the next reboot reproduced the warning at 5,437 ms with the same ELF prefix `2650225e8`. Sampled MEPC `0x4200e5e6` resolves to `crc32` in `main/kitchen/kitchen_krc.c:15`, inlined into `krc_read_body` at line 99; RA `0x4200bc0a` resolves to `load_item` in `main/kitchen/kitchen_catalog.c:55`. This points to long synchronous catalog validation; the two-millisecond delays between broad stages are not a verified fix. Keep the watchdog enabled. The latest startup had 198,368 free heap bytes and at least 8,656 UI stack bytes remaining before stress testing.

The user explicitly confirmed that the clock warning can be dismissed, but the repaired app still produces no audible greeting. The audio driver initializes without an error, and the unchanged PCM is non-silent (62,400 bytes, 1.95 seconds; SHA256 `b47e9a9e8845a130495896d8628a153f7d17339442e7e9d6f7ef3a61c81d4a22`). This does not prove mixer events, saved sound/volume settings or speaker output. No NVS dump or record deletion was performed. All monitors were closed; raw logs and target identity remain private.

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
Device tests: FAIL (watchdog reproduced; greeting still inaudible)
Unverified: reliable startup, audio repair, clock-warning persistence, device seafood/icon appearance, unlimited redraw, timer cancellation,
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
- The intended port is `/dev/cu.usbmodem212401`. The first app/resource update was approved and completed. The repaired app-only update was explicitly approved and completed; installed resources, bootloader, table and user data were retained. The user confirmed that OK dismissed the warning, but a subsequent reboot restored it. The current app still has no audible greeting according to the user. Isolated feature testing was not started or forced past the pending-save protection.

GitHub CLI device authorization was renewed for this handoff after expired credentials blocked earlier pushes. Push the firmware and design checkpoints to the two branches above, verify their remote heads, and do not force-push or upload private logs, NVS, device identity or debug bundles.

## Resume steps

1. Pull both branches, inspect status, and read this handoff and the design branch's `HANDOFF.md`. Check `assets/kitchen/checkpoint.json` against the source packs.
2. The complete `./tools/validate.sh` gate passed for the candidate above. Repeat it only after relevant changes; the actual-resource, migration and NVS tests are now included.
3. Verify the content-addressed debug archive, then build a segmented package with `tools/pack_offline_release.py`. Confirm the exact new artifact and intended device/write scope before flashing.
4. Preserve NVS at `0x9000..0xefff`, PHY at `0xf000..0xffff` and files at `0x700000..0x7fffff`. The compatible layout uses app `0x10000..0x1fffff` and kitchen assets `0x200000..0x6fffff`. A merged image fills gaps and must not be blindly written when preserving data.
5. First repair bounded catalog initialization: inspect the `kc_load` loop and `krc_read_body` CRC work. Yield within bounded batches or use a validated equivalent CRC implementation; delays only between broad phases have reproduced a watchdog. Add relevant host coverage and retest several physical starts.
6. Diagnose missing audio with read-only status for `sound`, `volume`, `audio_ready`, the boot-voice trigger and mixer consumption. Respect saved settings; trace ES8311/I2S playback before claiming sound is fixed. PCM validity and successful API returns do not establish audibility.
7. Check why clock-loss state returned after the user's acknowledgement and newer save sequence. Running timers with no trusted wall clock deliberately restore paused; confirm the resumed/paused state and saved record without deleting timers to bypass a guard.
8. Then run isolated boot/home, both seafood pages, more than three four-dish redraws, ingredient-to-shopping flow, timer pause/cancel and non-restart checks. Record heap/stack, verify the journal sequence did not change during isolated tests, restore normal mode and close the port.

On another computer, use separate checkouts for the two branches. Inspect `git status --short --branch` before pulling or switching. If no clone exists, clone the same repository once with `--branch codex/offline-kitchen` for firmware and once with `--branch codex/kitchen-model` for editable content. Read `AGENTS.md` and this handoff before editing; configure that computer's ESP-IDF 5.5.3 through the maintained setup instructions. Local absolute paths and private test scripts from this Mac are not available on the other computer.

The 1,000/1,500-entry experiment in `experiments/krc_stream/` is a synthetic indexing proof of concept only. It has not created a thousand real recipes and is not connected to production UI, recommendation or storage. Read its paired README before extending it.
