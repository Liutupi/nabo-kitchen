<p align="right"><a href="installation.zh_CN.md">简体中文</a> · <strong>English</strong></p>

# Install NABO Midnight Kitchen v0.2.0

This standalone offline experience includes 145 D2 recipes and complete dish illustrations, without cloud conversation or network setup. The creator confirmed that acceptance through three actual cooking sessions is incomplete and explicitly requested an experience release.

Download from the [v0.2.0 release](https://github.com/Liutupi/nabo-kitchen/releases/tag/v0.2.0) and verify SHA256SUMS.txt. The ZIP contains separate images, bilingual instructions, manifests and licenses, without device backups, credentials, logs or debug binaries.

## First installation or complete replacement

The community installer writes nabo-kitchen-v0.2.0-full.bin at 0x0. This replaces current firmware and fills NVS/PHY gaps, which may reset settings and kitchen records. Confirm records may be reset; a whole-chip erase is unnecessary. Never write app.bin at 0x0.

```sh
python -m esptool --chip esp32c3 --port PORT --baud 460800 write_flash --flash_mode dio --flash_freq 80m --flash_size 8MB 0x0 nabo-kitchen-v0.2.0-full.bin
```

This uses esptool 4.x command spelling. Replace PORT with the freshly confirmed target port; the command writes Flash. Other hardware or devices with unknown secure-boot/encryption requirements are outside this procedure.

## Preserve records on an existing compatible kitchen installation

First verify an exact layout match: NVS 0x9000/0x6000, PHY 0xF000/0x1000, application 0x10000/0x1F0000, kitchen assets 0x200000/0x500000 and files 0x700000/0x100000. Do not apply this procedure to different or unknown layouts. Extract the ZIP and write only:

```sh
python -m esptool --chip esp32c3 --port PORT --baud 460800 write_flash --flash_mode dio --flash_freq 80m --flash_size 8MB 0x10000 app.bin 0x200000 kitchen_bundle.bin
```

This preserves the bootloader, table, NVS/PHY and files. Do not add erase_flash or the complete image. Compatible v0.1 records migrate to KST2 and known retired dish references are filtered. v0.1 cannot directly read KST2; direct downgrade compatibility is not promised. Interrupted running timers recover paused after reboot.

## Use

Short OK enters after the opening without a date prompt. UP/DOWN selects, short OK opens, double UP returns and hold UP goes directly home. Hold OK on home draws using remembered conditions; DOWN on a result redraws without a limit. In a timed recipe step, the first OK starts timing and the next advances; short DOWN scrolls long text, then opens timers at the bottom.

The permanent Timers entry is at the bottom of home. Select it and short OK to open; hold OK pauses/resumes, hold DOWN cancels the selected item and double UP returns. Cooking is counted only after all steps are completed. See [usage instructions](instructions.en.txt).

## Validation and licenses

Application: 513,456 bytes; resources: 3,768,770 bytes. The exact accepted application, matching ELF and D2 resources are retained, with a build-output bootloader already validated by v0.1.0 first installation. No device readbacks were used to construct the package. The prototype passed 33 checks, production C rendering 13 checks and the full gate passed. Limited device write/readback, boots, navigation and a user-confirmed physical home entry passed. This new complete merged image has not undergone another first installation. Three actual cooking sessions, all-recipe/art acceptance and full physical control/audio acceptance remain incomplete. See [release evidence](release-validation.json).

Adjust quantities, time and doneness to actual ingredients and equipment; ingredient exclusions do not establish allergy or cross-contact safety. Source follows the included MIT notices. NABO character, artwork, animation, voice and recipe text are excluded from MIT; individual font licenses apply.
