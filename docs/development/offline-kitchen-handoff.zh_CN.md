<p align="right"><strong>简体中文</strong> · <a href="offline-kitchen-handoff.md">English</a></p>

# 离线厨房交接 — 2026-10-29

用户要求今天停止开发并更新 GitHub。本次保存源码检查点，不是发布版。不能因为分支存在就自动刷机或擦除设备。

## 工作位置

- 固件：`Liutupi/nabo-kitchen` 的 `codex/offline-kitchen` 分支；本机 `/Volumes/NABO-Build-20261004/ai-passport-nabo-kitchen-offline`。
- 可编辑菜谱、美术、打包工具和浏览器原型：同仓库 `codex/kitchen-model` 分支；本机 `/Volumes/NABO-Build-20261004/nabo-kitchen`。
- 固件基线为 FoloToy AI Passport 模板 `a06235e`；ESP32-C3、8 MiB Flash、无 PSRAM、ESP-IDF 5.5.3。原有工作区未覆盖。

## 工牌上实际运行的版本

已刷入并核验的是 **128 道菜、50 种食材、9 张完整菜图** 的版本。应用 SHA256：`8ccfcbdfa7f8ef0ee3e9987a0a9da2167eb91974414ac74ed4d09e15e4569562`；已装资源 SHA256：`21ee0798fbcc8abf69cbf243517d26e50a87f0c1fee20b5d7a27d9897ded51e0`。

bootloader 保留此前兼容离线候选中的版本（SHA256 `c20c3a4bd3d38e6670bda9ed1a67d6883b7290122e40a3a31371e2a84aa95bd7`）；分区表 SHA256 为 `23e0ea7332832e5612b11dd15cfd0b1682a05e1369a86308f89cbedadb11e499`。之后只按授权写应用和资源，因此不能用更新的完整镜像哈希代表这组实际分段。

用户已确认开机动画、欢迎语、开机停留等待 OK、实体冰箱分类与食材勾选正常。128 版隔离测试进入海鲜页、选择食材、抓取全部 320 行画面并退出测试。更早的 104 版隔离流程走通首页 → 条件 → 抽菜 → 用料 → 跟做。这些结果不代表全部计时、断电恢复、动画帧率或实际菜谱都已验收。

最后一次四菜设备测试被 `pending-save` 拒绝，未为绕过保护清除任何记录或计时；串口已关闭。原始设备日志保留在 GitHub 之外。下次继续时先确认用户希望测试的范围，不自动打开或复位串口。

## 最新源码检查点：尚未刷入

- 144 道菜谱草案、58 种食材、13 种海鲜、17 张完整菜图。其余使用既有图层或明确待配图占位；新增菜谱与美术均待家庭实做和视觉验收。
- 模型容量扩为 192 道；KST1 按稳定 ID 迁移旧菜品及食材选择。超过 64 道时采用有界候选池，多菜加权与全库穷举不等价。
- 无限重抽、删除无效的每日重抽设置、重抽计数边界保护；计时页长按下键可取消选中的计时器；开机和首页文字精确为 `NABO 陪你做饭`，欢迎语音文件未改。
- UI 任务栈改为 12 KiB，只读状态新增 `stack_free_min`，供下次实机测量。
- 资源包 2,480,194 B，SHA256 `7556d051d6adb1868b452c5f341dd1592ff3415ac551b1ec6a077ca3cad339ec`，5 MiB 资源区余 2,762,686 B。字库和菜图有意超过原设计单项预算，不代表旧预算已达标。

## 检查点验证

```text
Build: NOT RUN（最新组合源码）
Host tests: PASS（静态检查、144 道真实资源绘制和迁移）
Device tests: NOT RUN（最新组合源码）
Unverified: 完整固件门禁、新海鲜/图标观感、无限重抽、计时取消、
任务栈、NVS 容量和断电行为
```

静态检查通过。真实资源绘制 9 项通过；其中把旧测试写死的 50 食材期望改为读取 KRC 字典后复验。真实旧 128 道/50 食材 KST1 样本在实际 144 道/58 食材 metadata 下成功迁移，并检查缺少稳定 ID 时拒绝且不部分修改状态。组件测试覆盖 300 次重抽及 255 边界、取消计时和保存恢复。用户要求停止后，未再运行最新组合的完整固件构建。

192 道极限满载宿主存档为 8,231 B，超过原 8,192 B，当前缓冲为 9,216 B。两份满载存档加 NVS 开销和已有设置，可能装不进 24 KiB NVS。编码成功不等于实机满载保存可靠；不能擅自擦除设置或修改分区来修复。

## 下次继续

1. 拉取两个分支、检查工作树，读本文件和设计分支的 `HANDOFF.md`；核对 `assets/kitchen/checkpoint.json` 与资源源文件。
2. 激活 ESP-IDF 5.5.3 后运行 `./tools/validate.sh`；另运行 `CJSON_DIR="$IDF_PATH/components/json/cJSON" python3 tests/test_real_catalog_views.py` 和 `python3 tests/test_kitchen_ingredient_migration.py`。后两项不是主门禁默认包含的检查。
3. 校验按内容哈希归档的调试包，使用 `tools/pack_offline_release.py` 生成分段包；刷前确认新固件、设备身份和写入范围及授权。
4. 保留 NVS `0x9000..0xefff`、PHY `0xf000..0xffff`、files `0x700000..0x7fffff`。兼容布局为应用 `0x10000..0x1fffff`、厨房资源 `0x200000..0x6fffff`。合并镜像会填充空隙，保留数据时不能盲目整包刷入。
5. 用限时串口测试确认开机和新功能，记录连续四菜后的堆/栈，检查保存且不删除已有记录；结束后恢复正常模式并关闭串口。

`experiments/krc_stream/` 的 1000/1500 条实验只是合成索引概念验证，没有制作千道真实菜谱，也没有接入生产界面、推荐或存档。扩展前先读该目录双语 README。
