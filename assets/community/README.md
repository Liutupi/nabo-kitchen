<p align="right"><a href="README.zh_CN.md">简体中文</a> · <strong>English</strong></p>

# Community release media

The additional `v0.1.0/submission-review-status.jpg` is a creator-workbench receipt, not an upload gallery image. It shows project 1755 / revision 4068 under review. The server-returned gallery WebPs were separately downloaded and viewed: the website recompresses their bytes while retaining dimensions and content; [both hashes and pixel checks](../../docs/community/v0.1.0/community-verification.json) are recorded. The original media manifest remains the upload-source snapshot.

The `v0.1.0/` directory contains the finished 900 x 1200 WebP cover and four gallery posters for the first NABO Midnight Kitchen experience release. Every upload file was viewed after conversion. The original cover is retained under `originals/`; `cover-generation.json` records its exact prompt, authoritative project references and conversion. `media-manifest.json` records file hashes.

The cover is generated promotional illustration and preserves the approved C costume, character identity and unmirrored cap mark. The dish gallery uses the project's unchanged final PNGs. Interface posters combine the unchanged production C renderer with six complete device frame captures. TEST labels are retained. Device captures are framebuffers, not photographs or a claim that every recipe has passed physical/cooking acceptance.

Editable SVG layouts retain embedded source art, outlined display text and exact interface frames. `source-art/` and `source-ui/` preserve those inputs. Render again with `tools/render_kitchen_release_media.py --content-root <content-checkout> --font <licensed-font-file> --sharp-root <node-sharp-directory>`, with native `CC`, `CJSON_DIR` and the documented Python dependencies available. The machine's font binary is not redistributed; its hash is recorded. No AI editing is applied to interface frames.

NABO character, illustrations, recipe text and voice are excluded from MIT, following [the kitchen notices](../kitchen/NOTICE.md). SVG layouts and rendering code follow the source-code license; that does not grant independent reuse of embedded NABO art.
