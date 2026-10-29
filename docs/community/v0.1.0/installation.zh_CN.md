<p align="right"><strong>简体中文</strong> · <a href="installation.md">English</a></p>

# 安装 NABO 深夜厨房 v0.1.0

这是适用于 FoloToy AI Passport 的首版体验固件，会将现有应用替换为独立离线厨房助手。本版不含云对话、语音识别或配网功能。

请从 [v0.1.0 发布页](https://github.com/Liutupi/nabo-kitchen/releases/tag/v0.1.0) 下载，并用 `SHA256SUMS.txt` 核对文件。ZIP 包含四个分段镜像、清单、安装说明和许可，不含设备备份、凭据、串口日志及调试二进制。

## 首次安装

在社区固件安装工具中选择 `nabo-kitchen-v0.1.0-full.bin`，写入地址 `0x0`。写入前确认连接的是要更新的 AI Passport。安装会替换原固件，合并镜像会覆盖 NVS/PHY 空隙，原设置和厨房记录可能重置；不需要整片擦除。不要用于其他硬件，或安全启动、Flash 加密要求不明的设备。

已经安装 esptool 4.x 时，等效命令为：

```sh
python -m esptool --chip esp32c3 --port PORT --baud 460800 write_flash --flash_mode dio --flash_freq 80m --flash_size 8MB 0x0 nabo-kitchen-v0.1.0-full.bin
```

将 `PORT` 换成刚核实的端口。执行此命令会写入固件，并非仅查询设备。

## 已有兼容厨房的保留数据升级

先确认现有分区表与清单完全一致：NVS `0x9000/0x6000`、PHY `0xF000/0x1000`、应用 `0x10000/0x1F0000`、厨房资源 `0x200000/0x500000`、files `0x700000/0x100000`。其他或未知布局不能套用此步骤。解压 ZIP 后，只写入：

```sh
python -m esptool --chip esp32c3 --port PORT --baud 460800 write_flash --flash_mode dio --flash_freq 80m --flash_size 8MB 0x10000 app.bin 0x200000 kitchen_bundle.bin
```

对上述兼容布局，该操作保留 bootloader、分区表、NVS/PHY 和 files。此升级路径不要添加 `erase_flash`、完整镜像或其他段。包内独立 bootloader 和分区表供核对或另行规划的首次安装使用，不属于日常升级写入项。

## 开始做饭

1. 等待开场动画，短按 OK 进入厨房；如有未确认的计时提示，先处理提示。
2. 上下键移动，OK 进入或切换；双击上键返回。长按上键回到开场区域，普通按键可再次进入厨房。
3. 选择人数、时间和菜品组合后颠勺。下键不限次换菜，OK 查看用料并进入步骤；结果页长按 OK 收藏或取消收藏。
4. 冰箱里先选分类，OK 勾选食材，长按 OK 查看推荐。用料页下键加入采购，长按下键翻动长清单。
5. 计时页上下键选择，长按 OK 暂停或继续，长按下键取消。断电时正在运行的计时器恢复为暂停，不会假装断电期间仍准确计时。
6. 每次重启和跨天后确认本地日期；日期由手动维护。近期做过的菜只降低推荐权重，七天内仍可能再次抽到。

份量、时间和食材排除仅供家庭烹饪参考，不构成过敏原或交叉接触筛查，请自行核对菜谱及原料。144 道菜谱和插画草案均仍在审校中，火力、熟度及食品处理需结合实际食材和厨具调整。

## 已验证范围

交付应用沿用已接受的 514,704 字节原构建，SHA256 为 `a6f4a167bf1d79d11bd7dbbf2a46d56eef5362446bbcd41313ace56effcb1202`；资源沿用已上机的 3,756,062 字节镜像，SHA256 为 `f1c0f215a460b6c4e7c8805527a6077ff2b8473626eb55205a77f9bda26d2931`。资源读回、三次启动和六张完整设备画面通过，抽样菜图区 100,800 像素与生产代码一致。音频数据消费未报写入错误，但用户暂未观察到更新后的面板和声音，实际观感/听感仍未验证。

首次安装完整镜像由构建产物合成，其中 bootloader 与实机验证时保留的兼容旧 bootloader 不同；尚未单独观察此次打包 bootloader 的首次安装行为。结构及打包校验不等于完整镜像已经实机刷测。长期运行、全部菜谱实厨、实体断电/满存储情形及全套图的逐张审美验收仍待完成。详见 [发布溯源](release-validation.json) 和 [设备报告](../../development/offline-art-device-validation-20261029.json)。

源码沿用原有 MIT 许可。NABO 形象、美术、动画、语音及菜谱文案不在 MIT 授权内，随包字体另遵其许可。
