# 2026-10-29 完整菜图补全

**已补齐 127 张，当前 144 道菜均有完整菜图，缺图 0。交付状态全部保持 `review`。** 原有 17 张完整菜图、菜谱、食材 ID、分层图和定稿规格保持原样。内容入口为 `codex/kitchen-model`，本批工作同时保存在 `codex/kitchen-dish-art-20261029`。

`manifest.json` 是可恢复的逐菜任务清单，记录稳定 ID、配方哈希、逐菜提示词和真实交付状态。只有 PNG、保留的原图、哈希及 JSON 元数据齐全后才注册进 `ASSET_LIST.csv` 并标 `review`。`review` 不代表用户审美、实厨或实机验收。

生成采用内置 OpenAI imagegen，每道单独生成，参考已交付的 `hero_xilanhua-chaoshanbei.png`，按真实配方主料和做法区分成品。输出必须是真透明 RGBA、4:3、器皿完整、不裁切；不画食物脸、文字、背景或程序特效。不以配菜、复制图或占位图冒充完成。

原始生成图保存在 `../../originals/`。交付仅把整张画布等比缩至 640×480，不重画、不裁切、不去除透明通道。每 12 道输出一页 `review-XX.png`：同一张图在实际 160×120 大小、深色卡和奶油卡上分别显示，并模拟 RGB565。

每张新图的 `recipe_sha256` 保留生成时原始菜谱文件的哈希；`recipe_lf_sha256` 统一 CRLF 为 LF，便于 Windows/macOS/Linux 核对配方文本。图片、生成原图及二进制包始终按原始字节核对。既有 17 张图的元数据未改。

可用 Python 3 和 Pillow 核对交付/重建审图（在内容仓库根目录执行；完整打包依赖见 `tools/requirements.txt`）：

```sh
python game-art/kitchen/dish-hero/batches/2026-10-29/delivery.py verify
python game-art/kitchen/dish-hero/batches/2026-10-29/delivery.py sheets
python -m unittest discover -s tools/tests -p test_kitchen_heroes.py
python tools/check_recipes.py
```

每批先检查远端内容分支有没有新增配方、同 ID 交付或规格变化，再继续未完成项；不覆盖其他机器的新图。GitHub 已设置每 30 分钟跟进，主任务活跃时避免重复出图，仅报告实质更新/冲突。

## 验证与审图

- [覆盖与哈希报告](coverage-validation.json)：144/144，保留原有 17 张、菜谱及定稿规格。
- [静态打包报告](static_report.json)：328 张静态 PNG，226 个菜品 sprite（82 分层 + 144 完整图），每个调色组不超过 64 色，索引解码往返一致。
- `decoded-review-01.png` 至 `decoded-review-11.png`：127 张新增图的实际 160×120 限色解码、RGB565、深/浅底对照；已逐页查看。`review-XX.png` 为缩图对照。局部返修记录见 [VISUAL_REVIEW.md](VISUAL_REVIEW.md)。
- [本机 C 绘制报告](runtime-validation.json)：使用固件 `c7cdc0b94a6009672333dc1736b4f1c33f0a0537` 的原生产 C 代码，11 项测试通过，144 道均读取到完整菜图；包含整帧/分条绘制、全部菜谱步骤与滚动、采购/食材界面、计时/开机提醒。KAB CRC、损坏/截断拒绝和 1837 次协作让出回调通过。
- [实际 C 页面预览](host-rendered-candidate.png)；[检查汇总](validation-summary.json)。另有菜图 5 项、原型 3 项测试通过，144 道 schema/素材引用通过。

## 资源候选与复现

[resource-candidate/](resource-candidate/README.md) 包含可独立核对的六个 payload、KAB 包及许可。仅菜图包变化，菜谱、字体、食材/界面、NABO 动画五个 payload 与已接受固件资源逐字节一致。**出图阶段未刷机；随后用户明确要求刷机验证，2026-10-29 已仅更新工牌资源区到本包。已接受的应用没有构建、替换或重刷。**

最终 `kitchen_dish.bin` 为 **1,596,273 B**，超过旧工具的 200,000 B 菜图预算；没有修改预算/规格来使其显示通过。实际整包 **3,756,062 B**，SHA256 `f1c0f215a460b6c4e7c8805527a6077ff2b8473626eb55205a77f9bda26d2931`，在实际 5 MiB 分区内还剩 **1,486,818 B**。有限设备验证见下节；全部菜谱和长期使用仍待验收。

## 2026-10-29 后续资源刷写验证

[设备验证报告](device-validation.json) 和 [六张设备画面](device-captures/README.md)：仅从 `0x200000` 写本资源包，完整读回 SHA256 一致；应用、分区表、bootloader、NVS/PHY、files 均核对保留。3 次受控启动匹配原 ELF，资源校验均在约 2,278 ms 完成，未见看门狗或崩溃。最后空闲堆 198,336 B、最大块 114,688 B，UI 栈最低余 6,968 B。

蒸水蛋、玉米排骨汤、白菜炖豆腐、番茄炒蛋、青椒蛏子汤、清蒸鲈鱼取得完整 320 行画面；实际菜图区合计 100,800 个 RGB565 像素与生产 C 预期一致。11 次截图请求中，1 张跳错页画面和 4 次不完整/未完整结束的传输均排除；鸡蛋炒饭的完整 USB 截图没有取得，未计入通过项。原有 USB 截图分帧/流控仍需改进。

已经退出 TEST 并关闭串口，恢复原 1 个暂停计时器、声音开启/70% 音量；存档序号从 151 到 151。当前实际屏幕与扬声器表现仍待用户确认，逐菜审美/实厨保持 `review`。旧 `runtime-validation.json` 是出图结束时的本机阶段记录，新的设备结果以 `device-validation.json` 为准。

在旁边准备固件仓库 `codex/runtime-fixes-20261029`（上述提交）、本机 C 编译器以及 `CJSON_DIR` 指向 cJSON 源码后执行；不需要 ESP32 固件编译或连接设备：

```sh
python tools/pack_assets.py pack --out build/kitchen-art-review
python game-art/kitchen/dish-hero/batches/2026-10-29/delivery.py sheets --decoded build/kitchen-art-review/decoded/dish
python game-art/kitchen/dish-hero/batches/2026-10-29/build_candidate.py --firmware-root ../nabo-kitchen-runtime --packed build/kitchen-art-review
# 设置 CC 为本机 C 编译器，CJSON_DIR 为含 cJSON.c/cJSON.h 的目录
python game-art/kitchen/dish-hero/batches/2026-10-29/verify_runtime.py --firmware-root ../nabo-kitchen-runtime --assets game-art/kitchen/dish-hero/batches/2026-10-29/resource-candidate --out game-art/kitchen/dish-hero/batches/2026-10-29
```

下一步按 11 页预览逐菜审美和实厨审核，尤其检查精确盘心、描边、全套风格及配方形态；通过后再将对应 CSV 行和元数据一起改为 `done`。需要返修时先保留当前版本及哈希。已完成的有限设备检查不替代全部菜谱、实物屏幕/声音和长期使用验收。
