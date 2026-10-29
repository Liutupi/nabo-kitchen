<p align="right"><a href="installation.zh_CN.md">简体中文</a> · <strong>English</strong></p>

# Install NABO Midnight Kitchen v0.1.1

This is the first experience release for FoloToy AI Passport. It replaces the current application with an independent offline cooking companion. It does not provide cloud conversation, speech recognition or network setup.

Download the files only from [the v0.1.1 release](https://github.com/Liutupi/nabo-kitchen/releases/tag/v0.1.1). Verify downloads using `SHA256SUMS.txt`. The ZIP includes the four separate images, manifests, instructions and license notices. It excludes device backups, credentials, serial logs and debugging binaries.

## First installation

Use `nabo-kitchen-v0.1.1-full.bin` in the community's firmware installer at address `0x0`. Choose the correct connected AI Passport before writing. Installation replaces the existing firmware and the merged image overwrites the NVS/PHY gap; existing settings and kitchen records may be reset. A complete-chip erase is unnecessary. Do not install it on other hardware or devices with unknown Flash encryption / secure-boot requirements.

With an already installed esptool 4.x, the equivalent command is:

```sh
python -m esptool --chip esp32c3 --port PORT --baud 460800 write_flash --flash_mode dio --flash_freq 80m --flash_size 8MB 0x0 nabo-kitchen-v0.1.1-full.bin
```

Replace `PORT` with the port you have just identified. Running this command writes firmware; it is not a discovery command.

## Upgrade an existing compatible kitchen

First verify the existing partition table exactly matches the manifest: NVS `0x9000/0x6000`, PHY `0xF000/0x1000`, app `0x10000/0x1F0000`, kitchen resources `0x200000/0x500000`, files `0x700000/0x100000`. Do not use this upgrade recipe for a different or unknown layout. From the extracted ZIP, write only:

```sh
python -m esptool --chip esp32c3 --port PORT --baud 460800 write_flash --flash_mode dio --flash_freq 80m --flash_size 8MB 0x10000 app.bin 0x200000 kitchen_bundle.bin
```

This operation preserves bootloader, partition table, NVS/PHY and files on that compatible layout. Do not add `erase_flash`, a full image or extra segments to this preservation path. The separate bootloader and table are supplied for inspection or a deliberately planned clean installation, not as routine upgrade writes.

## Start cooking

1. Wait for the opening animation, then briefly press OK to enter the kitchen. An outstanding timer notice may need acknowledgement first.
2. UP/DOWN moves the selection; OK opens or changes it. Double UP goes back. Long UP returns to the opening area; a normal press opens the kitchen again.
3. Choose the number of people, time and meal combination, then draw. DOWN rerolls without a quota; OK opens ingredients and then the steps. Long OK on a result toggles its favorite.
4. In the fridge, choose a category, toggle ingredients with OK, and long OK opens recommendations. In the ingredients page, DOWN adds the dish's ingredients to shopping; long DOWN scrolls the paper.
5. On the timer page, UP/DOWN selects a timer, long OK pauses/resumes and long DOWN cancels it. Timers interrupted by power loss recover paused; they do not claim to have kept time while powered off.
6. Confirm the local date after restart and when the day changes. The date is manually maintained. Recent dishes receive a lower draw weight; they can appear again within seven days.

Recipe quantities, times and ingredient exclusions are household reference material. They are not an allergen or cross-contact filter; check the recipe and ingredients yourself. All 144 recipe and illustration drafts remain under review. Adjust heat, doneness and food handling to the actual ingredients and equipment.

## What was checked

The clean-capture application is 514,880 bytes, SHA256 `52e249206800ad4a27cfed4080f2b4664a17e66b15dfdc595d32dd9f6c842afd`. The unchanged resource bundle is 3,756,062 bytes, SHA256 `f1c0f215a460b6c4e7c8805527a6077ff2b8473626eb55205a77f9bda26d2931`. The package deliberately reuses the build-output bootloader whose v0.1.0 community first installation passed. It does not use a device Flash dump.

The full build/host gate passed. The application-only update was independently read back, protected regions remained unchanged, the subsequent bounded boot was clean, and three complete presentation frames were retained. The user confirmed that the updated opening screen and kitchen entry work. The earlier community full-image installation and one recipe workflow passed separately. This v0.1.1 merged image has not itself been reinstalled through the community before publication. See [release provenance](release-validation.json).

All-recipe real cooking, art acceptance, long-term use, power-loss/full-storage scenarios and separate speaker listening remain unverified. Source code follows its existing MIT notices. NABO character, artwork, animation, voice and recipe text are excluded from MIT; bundled font licenses remain applicable.
