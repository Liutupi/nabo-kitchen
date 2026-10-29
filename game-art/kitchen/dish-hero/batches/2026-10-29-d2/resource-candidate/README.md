# D2 资源重建（未上机、未发布）

包含 145 道 D2 菜谱、145 张完整菜图和 82 个分层图；沿用已接受的 NABO 动画，不改变身份或帽标。菜图均为 `review`，工程检查不等于逐菜验收。

整包 **3,768,770 字节**，5 MiB 资源区还剩 **1,474,110 字节**。精确哈希、每个 payload、字形和验证范围见 [validation.json](validation.json)。旧菜图和字体子预算仍超出，未修改规格或预算掩盖问题。此包不含应用、设备备份或 NVS。

在 `main` 根目录，安装 `tools/requirements.txt` 及字体打包依赖后执行（`firmware` 是旁边独立检出的固件分支）：

```sh
python -m pip install -r tools/requirements.txt -r tools/requirements-fonts.txt
python tools/fetch_fonts.py
python tools/rebuild_d2_resources.py --firmware-root ../nabo-kitchen-runtime
python -m unittest discover -s tools/tests -v
python -m unittest discover -s tools -p 'test_kitchen_*.py' -v
```

重建脚本会重建字体、图片、KRC、显示元数据及退役 ID 白名单。需要同步到固件时加 `--sync-firmware`；脚本只允许复制到干净的 `firmware` 工作树，否则拒绝，保护其他电脑或未提交成果。**不刷机、不发布。** 动画输入固定为此前 `2026-10-29/resource-candidate/kitchen_nabo.bin`。

`build_firmware_metadata.py` 默认改为这里的 D2 KRC/字形清单；旧 10-28 fixture 仍可通过显式参数使用。35 个退役 ID 来源为 `data/D2_REMOVED.json`，仅白名单 ID 可以从旧存档中丢弃。包含退役菜的缓存菜单整体失效，历史、收藏、进度和计时分别过滤，其他设置和记录保留；未知或损坏 ID 仍拒绝整份输入，避免部分覆盖。

源字体的原许可保留在两个 `OFL.*.txt` 文件。图像、NABO 及菜谱文案的许可边界沿用项目声明，不随源码 MIT 授权。
