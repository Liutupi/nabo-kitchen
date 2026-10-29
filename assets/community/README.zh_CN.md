<p align="right"><strong>简体中文</strong> · <a href="README.md">English</a></p>

# 社区发布图片

`v0.1.0/community-published.jpg` 记录版本 4068 通过后的实际公开页面，是发布凭证，不是上传图集图片。[匿名公开核验](../../docs/community/v0.1.0/community-publication-verification.json)确认公开固件与 GitHub 一致，五张公开图与先前审看的保存版本一致。

新增的 `v0.1.0/submission-review-status.jpg` 是创作者后台回执，不是上传图集的图片，记录项目 1755 / 版本 4068 审核中。网站保存后的五张 WebP 已另外回下载并逐张查看；服务端重新压缩导致字节不同，尺寸和内容保留，[两套哈希及像素比较](../../docs/community/v0.1.0/community-verification.json)分别记录。原 media manifest 保持为上传原文件的快照。

`v0.1.0/` 保存首版 NABO 深夜厨房体验版的 900 × 1200 WebP 封面和四张介绍图。每个上传文件均在转换后逐张打开检查。原始封面保存在 `originals/`；`cover-generation.json` 记录完整提示词、权威参考图与转换方式，`media-manifest.json` 记录文件哈希。

封面为生成的宣传插画，保持已选 C 造型、NABO 身份及非镜像帽标。菜品图集使用项目原有交付 PNG；界面介绍图使用未改生产 C 代码绘制及六张完整设备采集画面，保留 TEST 标记。设备画面是帧缓冲采集，不是实拍，也不代表全部菜谱已通过实体或实厨验收。

可编辑 SVG 中保留嵌入原图、转曲文字及原界面画面；`source-art/` 和 `source-ui/` 保留输入文件。复现命令为 `tools/render_kitchen_release_media.py --content-root <内容仓库> --font <有使用许可的字体文件> --sharp-root <Node-sharp目录>`，并准备本机 `CC`、`CJSON_DIR` 和脚本注明的 Python 依赖。不分发本机字体二进制，只记录其哈希。界面未经过 AI 重绘。

NABO 形象、插画、菜谱文案及语音不在 MIT 授权内，遵循[厨房素材许可](../kitchen/NOTICE.zh_CN.md)。SVG 排版和渲染代码遵循源码许可，不因此单独授予嵌入 NABO 美术的使用权。
