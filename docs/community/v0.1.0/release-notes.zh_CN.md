<p align="right"><strong>简体中文</strong> · <a href="release-notes.md">English</a></p>

# NABO 深夜厨房 v0.1.0

今晚吃什么？让戴白帽的 NABO 陪你想一想。选好人数、时间和菜品组合，颠勺抽一道家常菜；不合口味就再换一道。从准备食材，到照着步骤出锅，离线也能慢慢做好一顿饭。

- 144 道家常菜谱草案，每道配有完整菜图。
- 58 种冰箱食材，含 13 种海鲜，支持冰箱推荐和采购清单。
- 用料、步骤跟做、收藏、做菜记录，以及最多三个可暂停计时器。
- 开场动画、本地欢迎语和已选定的 NABO C 厨房造型。

这是**首版体验版（预发布）**。菜谱和插画仍在审校，尚未逐道实厨验收。近期做过的菜只降低权重，七天内仍可能再次抽到；日期手动确认，断电中断的计时恢复为暂停。本版不含配网、云对话或语音识别。

## 下载和安装

| 文件 | 用途 |
| --- | --- |
| `nabo-kitchen-v0.1.0-full.bin` | 首次安装，写入 `0x0`；替换现有固件并可能重置记录。 |
| `nabo-kitchen-v0.1.0.zip` | 分段镜像、中英安装说明、清单、校验值和许可。 |
| `manifest.json` | 应用、资源与完整镜像的哈希、地址、来源及验证边界。 |
| `SHA256SUMS.txt` | 核对前三个下载文件；发布后也会独立回下载此校验文件。 |

已有完全兼容布局的厨房，如需保留 NVS/PHY 和 files，只写应用 `0x10000`、资源 `0x200000`。刷写前阅读[安装说明](https://github.com/Liutupi/nabo-kitchen/blob/v0.1.0/docs/community/v0.1.0/installation.zh_CN.md)。

应用沿用 SHA256 `a6f4a167bf1d79d11bd7dbbf2a46d56eef5362446bbcd41313ace56effcb1202`，资源沿用 SHA256 `f1c0f215a460b6c4e7c8805527a6077ff2b8473626eb55205a77f9bda26d2931`，均与已上机文件一致。新完整镜像为 5,853,214 字节，SHA256 `9a7ade79bb7ebf4aa91332368a7dc4910af7cb86eb3aeec38775bc8380838a1c`。镜像由已验证构建产物合成，所有空隙均为擦除填充值，不包含设备读回备份或凭据。

Build：PASS；Host tests：PASS；有限设备检查：PASS，包括资源读回、三次启动、六张完整采集画面及 100,800 个一致菜图像素。更新后面板/听感、全量实厨、全部美术、长期运行和本次打包 bootloader 的首次安装仍未完成验收。新构建仅作源码验证，单独归档，没有替换已接受的应用。[完整验证溯源](https://github.com/Liutupi/nabo-kitchen/blob/v0.1.0/docs/community/v0.1.0/release-validation.json)。

封面为宣传插画，图集注明生产代码绘制及设备帧缓冲来源，不是屏幕实拍照片。源码沿用原有 MIT 许可；NABO 形象、美术、动画、语音和菜谱文案不包含在 MIT 中，随包字体另遵其许可。

![宣传封面](https://raw.githubusercontent.com/Liutupi/nabo-kitchen/v0.1.0/assets/community/v0.1.0/01-cover.webp)
