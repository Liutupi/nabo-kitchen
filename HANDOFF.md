# 交接

**规则**：新状态插在“当前状态”下面，旧状态往下推；每块不超过 15 行，只写做了什么、下一步、阻塞。哈希、日志、验证明细写进 JSON 或专门的文档，在这里只放链接。完整规矩见 [docs/WORKFLOW.md](docs/WORKFLOW.md)。

2026-10-29 之前的长篇交接已原样归档：[main 版](docs/archive/HANDOFF-main-2026-10-29.md)、[kitchen-model 版](docs/archive/HANDOFF-kitchen-model-2026-10-29.md)。固件的详细交接仍在 `firmware` 分支的 `docs/development/offline-kitchen-handoff.zh_CN.md`。

---

## 当前状态（2026-10-29 晚，Claude 总架构师）

### 2026-10-29 · Codex（Windows）· 任务 1 完成

- 内容目录已跟踪 `main`，固件目录已跟踪 `firmware`；旧美术/发布目录转为只读用途的 detached 快照。
- 六个旧分支均通过目标分支祖先检查后删除，远端和本机现在只保留 `main`、`firmware`，v0.1.0 tag 未改。
- 30 分钟检查已改看两个主线，并遵守任务顺序及每块 15 行限制；[核验记录](docs/validation/branch-consolidation-20261029.json)。
- 下一步：任务 2；用户已连接测试工牌，先核对设备并完整备份，再验证社区首次安装路径。

| 项目 | 状态 |
|---|---|
| 已发布 | v0.1.0：[GitHub prerelease](https://github.com/Liutupi/nabo-kitchen/releases/tag/v0.1.0)，[社区项目 1755](https://ai-passport.folotoy.cn/plays/1755/?v=4068-2)。应用 `a6f4a167…`，资源 `f1c0f215…` |
| 分支 | **仅 `main` 和 `firmware`**；任务 1 已完成六个旧分支的祖先核对及本机/远端删除，定时检查已切换。明细见 [分支核验](docs/validation/branch-consolidation-20261029.json) |
| 菜谱 | **D2 质量版已完成**：145 道。改动说明见 [data/recipes/D2_PROPOSAL.md](data/recipes/D2_PROPOSAL.md)。`python tools/check_recipes.py` 通过，`python tools/lint_recipes.py` 只剩 8 处合理的重复 |
| 美术 | 109 道菜有完整菜图；**36 道新菜没有菜图**，暂时用图层合成显示；退役的 35 张在 `game-art/kitchen/dish-hero/retired/` |
| 设备 | 用户工牌运行的还是 v0.1.0（144 道旧菜谱） |
| 规格变更 | v0.2 去掉日期依赖，见 [DESIGN.md 第 8 节](game-art/kitchen/DESIGN.md) |
| 评估 | [docs/REVIEW-2026-10-29.md](docs/REVIEW-2026-10-29.md) |

### 给 Codex 的任务（按顺序）

| # | 任务 | 完成标准 |
|---|---|---|
| 1 | **切换本机仓库并删除旧分支**：内容目录改跟 `main`，固件目录改跟 `firmware`；30 分钟定时检查也改看这两个分支。确认 `git merge-base --is-ancestor` 都通过后，删除远端的 6 个 `codex/*` 分支：`kitchen-model`、`kitchen-dish-art-20261029`、`prototype-packing`（已在 main 里），`release-v0.1.0`、`runtime-fixes-20261029`、`offline-kitchen`（已在 firmware 里） | `git ls-remote --heads origin` 只剩 `main` 和 `firmware` |
| 2 | **v0.1.1：首次安装实机验证**。先完整备份一块工牌，从社区页面“安装最新版”刷入 v0.1.0 的完整镜像，确认能开机、进厨房、走完一道菜。不通过就改用实机验证过的 bootloader `c20c3a4b…` 重新打包 | 在 `firmware` 分支写验证 JSON；需要重打包时发布 v0.1.1 |
| 3 | **干净的宣传截图**：截图模式隐藏 TEST 角标，顶栏不显示 `-:--`。替换社区第 5 张图，和 v0.1.1 一起更新 | 五张图里都没有 TEST 和 `-:--` |
| 4 | **补 36 张菜图**：新菜谱 ID 见 `data/recipes/D2_PROPOSAL.md` 的 A、C 两节（也可以用 `dish.hero` 为 `false` 筛出来），风格和现有菜图一致；登记进 `ASSET_LIST.csv`，菜谱的 `dish.hero` 改成 `true` | `tools/tests/test_kitchen_heroes.py` 通过，145 道都有菜图 |
| 5 | **D2 资源重建**：重打包 KRC、字体（新增“腩、煸、蘸、黏”等字）和资源包；`build_firmware_metadata.py` 的默认输入从 10-28 检查点改成新的 KRC（现在 `test_fridge_metadata` 因为调料表新增了 3 项而失败，这是预期的）；固件读档时丢弃已删除的菜谱 ID（名单见 `data/D2_REMOVED.json`） | 全部宿主测试通过；资源包大小写进 HANDOFF |
| 6 | **v0.2 固件：去掉日期依赖**（DESIGN 第 8 节）：开机不再确认日期；“最近吃过”按做饭次数降权；顶栏只显示电量；等级不再看连续天数。原型同步修改 | 原型和 C 对拍通过；真机重启不再询问日期 |
| 7 | **v0.2 发布**：真机验收后发布 v0.2.0，更新社区 | 用户在厨房用 v0.2 做过至少 3 道菜 |

### 待刘东坡做

1. 开机看一眼、听一下现在的 v0.1.0（5 分钟），然后在 HANDOFF 写一句结果。
2. 拿到 v0.2 后，每天用它做一道菜，把问题记下来。

---

## 决定记录

| 日期 | 决定 | 谁定的 |
|---|---|---|
| 2026-10-28 | 单独做“NABO 厨房版”，后来改为**独立离线固件**（没有小智、语音、联网） | 刘东坡 |
| 2026-10-28 | 画风参照“深夜小店”的氛围，产品名和画面里不出现“深夜食堂” | 刘东坡 |
| 2026-10-28 | 界面沿用 NABO 工具箱的视觉语言；造型选 C | 刘东坡 |
| 2026-10-28 | 最近做过的菜只降权，不排除；重抽不限次 | 刘东坡 |
| 2026-10-28 | Codex 可以自主推进，规格不擅自改，交付保持 review | 刘东坡 |
| 2026-10-29 | 仓库公开；v0.1.0 发布到 GitHub 和社区 | 刘东坡 |
| 2026-10-29 | **D2 菜谱质量版**：按 D2_PROPOSAL 全部执行（瘦肉粥代替皮蛋瘦肉粥） | 刘东坡 |
| 2026-10-29 | **仓库整理**：kitchen-model 合并进 main，固件固定在 `firmware` 分支，删除其余 6 个 `codex/*` 分支（内容都已包含在这两个分支里） | 刘东坡 |
| 2026-10-29 | **去掉日期依赖**：按做饭次数降权，开机不再确认日期 | 刘东坡 |
| 2026-10-29 | 交接改成短格式，长篇记录归档 | 刘东坡 |
