<p align="right"><strong>简体中文</strong> · <a href="installation.md">English</a></p>

# 安装 NABO 深夜厨房 v0.2.0

这是独立离线厨房体验版：145 道 D2 菜谱及完整菜图，不含云对话或配网。用户确认尚未完成三道实际下厨验收，本次按体验版发布。

从 [v0.2.0 发布页](https://github.com/Liutupi/nabo-kitchen/releases/tag/v0.2.0) 下载，并用 SHA256SUMS.txt 核对。ZIP 含分段镜像、中英说明、清单和许可，不含设备备份、凭据、日志或调试二进制。

## 首次安装或完整替换

社区“安装最新版”使用 nabo-kitchen-v0.2.0-full.bin，从 0x0 写入。完整镜像会替换当前固件，并覆盖 NVS/PHY 空隙，可能清空设置和厨房记录。先确认数据可重置；不要先做整片擦除。不要把 app.bin 写到 0x0。

```sh
python -m esptool --chip esp32c3 --port PORT --baud 460800 write_flash --flash_mode dio --flash_freq 80m --flash_size 8MB 0x0 nabo-kitchen-v0.2.0-full.bin
```

命令适用于 esptool 4.x，PORT 必须替换为刚确认的目标工牌端口；执行会实际写入。其他硬件、安全启动或加密要求不明的设备不适用。

## 已有兼容厨房的保留数据升级

先核对布局完全相同：NVS 0x9000/0x6000、PHY 0xF000/0x1000、应用 0x10000/0x1F0000、厨房资源 0x200000/0x500000、files 0x700000/0x100000。未知或不同布局不能照用。解压 ZIP 后，仅写：

```sh
python -m esptool --chip esp32c3 --port PORT --baud 460800 write_flash --flash_mode dio --flash_freq 80m --flash_size 8MB 0x10000 app.bin 0x200000 kitchen_bundle.bin
```

此路径保留 bootloader、分区表、NVS/PHY 和 files，不添加 erase_flash 或完整镜像。来自 v0.1 的兼容记录会迁移为 KST2，明确退役菜的引用会过滤；旧 v0.1 不能直接读取 KST2，不保证直接降级兼容。运行计时器在重启后恢复为暂停。

## 使用

开场短 OK 进入，不问日期。上下选择，短 OK 进入，双上键返回，长上键直接回首页。主页长 OK 按记住的条件抽菜，结果页下键不限次换菜。跟做时计时步骤第一次 OK 启动，下一次 OK 前进；短下键先滚长文，到底再按打开计时。

首页底部常驻“计时器”；选中短 OK 进入，长 OK 暂停/继续、长下键取消选中项、双上键返回。做完全部步骤才计完成。更多操作见 [使用说明](instructions.zh_CN.txt)。

## 验证和许可

应用 513,456 B，资源 3,768,770 B。保留已上机验证应用、匹配 ELF 与 D2 资源的精确字节，配用已通过 v0.1.0 首次安装的构建产物 bootloader，未使用设备读回内容打包。原型 33 项、实际 C 绘制 13 项及完整门禁通过；有限设备读回、启动、导航和用户实体入口确认通过。新的完整合并镜像没有再次进行首次安装；三道实厨、全量菜谱/美术和完整按键/听感验收未完成，见[验证明细](release-validation.json)。

菜谱、时间与份量需结合食材和厨具调整；不要把食材排除当成过敏原或交叉接触筛查。源码依随包 MIT；NABO 形象、美术、动画、语音和菜谱文字不随 MIT 授权，字体依各自许可。
