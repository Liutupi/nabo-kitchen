# 交接

**规则**：新状态插在“当前状态”下面，旧状态往下推；每块不超过 15 行，只写做了什么、下一步、阻塞。哈希、日志、验证明细写进 JSON 或专门的文档，在这里只放链接。完整规矩见 [docs/WORKFLOW.md](docs/WORKFLOW.md)。

2026-10-29 之前的长篇交接已原样归档：[main 版](docs/archive/HANDOFF-main-2026-10-29.md)、[kitchen-model 版](docs/archive/HANDOFF-kitchen-model-2026-10-29.md)。固件的详细交接仍在 `firmware` 分支的 `docs/development/offline-kitchen-handoff.zh_CN.md`。

---

## 当前状态（2026-10-29 晚，Claude 总架构师）

### 2026-10-29 · Codex（Windows）· 任务 6 开发候选完成，待实机

- 原型和实际固件已去日期依赖：按完成次数降权，菜单跨重启保留，计数器加设备种子抽签，等级只看不同菜数/累计次数。
- 选中项整块暖橙、深色文字，覆盖主页、条件、列表和设置；[生产 C 主机绘制预览](https://github.com/Liutupi/nabo-kitchen/blob/firmware/assets/kitchen/validation/v0.2/focus-preview.png)，不是设备实拍。
- 145 道算法对拍：160 组权重、40 组抽签/重抽通过；浏览器 31 项通过，旧 64 道设计模型明确冻结为 v0.1 参考。
- KST2 增加抽签计数，旧 KST1 日期历史只在首次迁移时排序；新版本不再以日期参与业务，断电计时仍恢复暂停。
- [候选与验证](https://github.com/Liutupi/nabo-kitchen/blob/firmware/docs/development/offline-v02-validation-20261029.json)；任务 6 实机重启/按键验收尚未完成，未刷机、未发布。
- 下一步仅待新候选应用+资源的具体刷写同意和实机验收；任务 7 发布按用户要求暂缓，仍需 v0.2 实厨至少 3 道。

### 2026-10-29 · Codex（Windows）· 任务 5 完成

- 重建 145 道 D2 KRC、字库、145 完整菜图及资源包；“腩、煸、蘸、黏”覆盖，动画与身份未改。
- 资源包 **3,768,770 B**，5 MiB 分区剩 **1,474,110 B**；旧子预算超出如实保留，[精确资源与验证](game-art/kitchen/dish-hero/batches/2026-10-29-d2/resource-candidate/validation.json)。
- 默认元数据切到 D2；仅过滤旧存档中 35 个退役菜 ID，保留其他记录/设置，未知或损坏 ID 仍原子拒绝。
- 内容工具 42 项通过，固件完整门禁/严格归档通过（21 项能力相关跳过）；实际 C 已遍历 145 道，未上机、未发布。
- 下一步任务 6：先原型再 C，去日期依赖、按做饭次数降权、选中项整块变色；完成后再请求新候选实机验证。

### 2026-10-29 · Codex（Windows）· 任务 4 完成

- 36 张 D2 新菜图全部交付，145/145 道有完整透明菜图，缺图 0；逐张登记 `review`，保留原图、提示词和哈希。
- 对齐 87 道原本已有图的可用标志；独立比对确认做法内容完全未改。菠菜和咸蛋肉饼另留修图前原图及修订记录。
- 菜图 5 项测试和 145 道 schema/引用检查通过；[验证清单](game-art/kitchen/dish-hero/batches/2026-10-29-d2/validation.json)、[双底色小图](game-art/kitchen/dish-hero/batches/2026-10-29-d2/contact_160px.png)。
- 下一步任务 5：重建 D2 KRC、字体、资源包，并处理退役菜谱存档；之后 v0.2 和选中项整块变色，本轮不发布。

### 2026-10-29 · Codex（Windows）· 任务 4 第二批 24/36

- 累计交付 24 张新菜图，145 道中现有 133 张；剩余 12 张继续，原图/提示词/哈希和双底色小图已更新。
- [批次验证](game-art/kitchen/dish-hero/batches/2026-10-29-d2/validation.json)，全部保持 `review`；已有图不重复生成。
- 用户新增界面要求：选中项整块变色，文字/图标有明确对比，红框仅作辅助；排入任务 6，同步原型和固件，按键规则不变。
- 下一步仍按顺序补图、重建资源、v0.2；本轮不发布。

### 2026-10-29 · Codex（Windows）· 任务 4 第一批 12/36

- 新交付 12 张完整菜图，透明原图、提示词、SHA256 与菜谱引用已登记，全部保持 `review`。
- 145 道现有 121 张，剩余 24 张继续制作；[批次与逐项检查](game-art/kitchen/dish-hero/batches/2026-10-29-d2/validation.json)、[160 像素双底色预览](game-art/kitchen/dish-hero/batches/2026-10-29-d2/contact_160px.png)。
- 145 道菜谱 schema/引用检查通过；未改做法，未动设备或发布。下一步补完本批，再做 D2 资源和 v0.2。

### 2026-10-29 · Codex（Windows）· 任务 4 进行中

- 按实际菜图文件和登记清单确认：145 道中已有 109 张，仅补 36 张；[批次计划与提示词](game-art/kitchen/dish-hero/batches/2026-10-29-d2/manifest.json)。
- 另有 87 道已有图但 `dish.hero=false`，不重复生成；新图交付后一起对齐可用标志。
- 本批沿用已通过的菜品风格，逐道保留透明原图、提示词和哈希，完成后登记 `review`。
- 其他机器请勿重复制作这个批次；下一步仍是资源重建和 v0.2 固件，发布最后处理。

### 2026-10-29 · Codex（Windows）· 任务 3 素材完成，先完善再发布

- 用户纠正优先级：先完成菜图、D2 资源和 v0.2 固件，社区更新与授权留到最后；本次未发布新 Release 或社区版本。
- 干净截图模式、五张成图及计时操作说明已完成；第三、第五张使用完整设备画面，没有 TEST 或无效时钟。
- 新应用经同意仅写应用并独立读回，其他保留区不变；用户确认新应用画面和进入厨房正常，测试已退出、串口关闭。
- [固件验证与素材检查点](https://github.com/Liutupi/nabo-kitchen/blob/firmware/docs/community/v0.1.1/release-validation.json)；v0.1.1 名称暂为未发布准备，当前设备仍是旧 144 道资源。
- 下一步：任务 4，补齐 D2 的 36 张菜图，再按 5、6、7 推进；第 7 项仍需用户用 v0.2 实际做过至少 3 道菜。

### 2026-10-29 · Codex（Windows）· 任务 2 完成

- 完整 8 MiB 备份已校验并只留本机；通过 Edge 社区“安装最新版”完成 v0.1.0 首次完整安装。
- bootloader、分区表、应用、资源独立读回均与发布产物逐字节一致；受控重启无看门狗、panic 或存储故障。
- 用户确认进入厨房、走完一道菜到完成页，实体暂停/取消计时正常；这是流程验证，不计实厨或单独听感验收。
- 本次 bootloader 首装通过，任务 2 无需重打 v0.1.1；[固件脱敏验证报告](https://github.com/Liutupi/nabo-kitchen/blob/firmware/docs/development/offline-first-install-validation-20261029.json)。
- 下一步：任务 3；另记“暂停入口不易发现”的提示优化建议，既定按键规则不变。

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
| 美术 | **145/145 道完整菜图，缺图 0**；36 张 D2 新图已交付，全部保持 `review`，退役图保留 |
| 设备 | 测试工牌仍是此前验证的应用与 144 道旧资源；v0.2 候选待单独同意上机 |
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
| 2026-10-29 | 先完善固件与图片，再统一发布；任务 3 的社区更新和授权延后处理 | 刘东坡 |

| 2026-10-29 | 选中项整块变色并调整文字/图标对比，红框作辅助；任务 6 同步原型和固件 | 刘东坡 |
