# 交接

新状态插在最上面。规则见 [docs/WORKFLOW.md](docs/WORKFLOW.md)。

---

## 最新状态 · 2026-10-28 · Codex（Windows）· 自主推进静态美术

**用户已明确授权后续直接推进，无需逐轮等待新指令。当前素材表 166 张静态图片全部齐全，本次新增 147 项均为 review。**

- 第 3 轮：新增 44 张食材，合计 50/50（v0.1 为 30 个、v0.2 为 20 个），全部 160×160 透明 PNG。ingredients/_contact_40px.png 将全部 50 个按实际 40 px 放在两种规定底色上，原先通过的 6 张逐字节保持不变。
- 第 4 轮：新增 69 张分层，合计 76/76（器皿及投影 12、汤汁 17、熟料 40、点缀 7），全部 640×480 独立透明画布。dish-layers/_combos.png 按现有 3 道 curated JSON 的实际 dish 字段合成番茄炒蛋、蒜蓉菜心、清蒸鲈鱼，按 160×120 在两种底色检查。修正了新稿中土豆丝过粗及白灼虾缺头的辨识问题。
- 第 5/6 轮已无依赖的静态部分：新增 34 项，含 14 个火候/做法图标、4 个徽章、5 个界面小图、开机字标、4 件独立道具和 6 个 C 造型表情。24 个图标/字标沿用样板的原生矢量体系，SVG 原稿与 PNG 同名保存；字标 NABO/深夜厨房 已转为矢量路径。其余使用内置 image_gen。修正了两个小图标锅柄圆端的画布裁切。
- 新图均按 CSV ID 命名，交付尺寸、透明通道、内容位置与 SHA-256 已逐项核验；147 张新 PNG 都有同名 JSON，记录提示词或原生源稿、原图位置、变换和校验。原生生成大图、废稿和检查图在本机 private/，不进入 Git。
- 已检查食材 40 px、表情 40 px、做法/火候 24 px、三道菜 160×120、深浅两种底色和 RGB565 模拟。全部 17 项 A1 原图及 K1/K2 校验不变；定稿规格、reference、data、prototype 和 tools 未修改。
- ASSET_LIST.csv：15 项已通过状态保持 done；既有 A/B 备选、K1/K2 和本轮新图保持 review（本轮授权继续不等于代替用户验收）。不是动画或真机验收完成，也没有把模型原图当成 64 色固件资源。

### 后续与交接

- **Grok 视频**：12 个视频路径仍为空，4 个核心动作是 idle、toss、serve、tend。按原分工由 Grok 制作；本次未访问或提交视频生成服务。已准备本机 private/grok-handoff/nabo-kitchen-C-grok-start-20261028.zip，含 K1/K2、C 定稿、身份参考、原有 GROK_PROMPTS 和校验清单；大小 1,934,699 字节，SHA-256 168765aeea843b8955efef5cf7b0c2cef1d043a8de1ef6c4515db5dd48c9d368。压缩包内容/CRC/源文件一致性通过，仍明确标注 K1/K2 为 review。
- **招牌菜精绘**：候选菜单尚未标出首批 30 道，不能把未选择的候选菜当成已选菜单。现有 3 道样例的分层合成已可用；精绘留待菜单明确，不改菜谱和规格来绕过该输入。
- **后续工程**：M1 原型、T1 抽帧/限色/打包仍按既定负责人推进；M2/M3/M4 依赖这些输入。本轮没有创建固件或 Release，也未刷机。精确 #3B2730 映射与每组 64 色仍需 T1 验证。
- 本机可交互检查页在 private/delivery/review.html，素材/校验统计在 private/delivery/audit.json。无进行中的出图任务；当前静态清单已补齐，后续依赖视频、菜单及打包/原型输入。
- 本次按食材、分层、界面三个范围小步提交，最后 pull --rebase 后 push。

---

## 最新状态 · 2026-10-28 下午 · Codex（Windows）· A2 第 2 轮

**刘东坡已选定 C，菜品和食材风格通过；K1、K2 起始图已交付 review。**

- 本轮用户决定：厨房造型采用 C（藏青盘扣短褂、深棕围裙、右肩白毛巾）。已将 costume/costume_C.png 原样复制为 costume/costume_final.png，未重绘或替换已认可造型。A/B 留档，不再等待选择。
- A1 的 C 及菜品/食材风格样板对应 15 行改为 done；此前精确描边色与每组 64 色的处理仍属于后续打包验证，不改既定规格。
- grok-start/K1.png：1024×1024、纯 #00FF00 绿幕；双手空手轻轻交握、微笑看观众，左臂白抹布；C 造型的右肩毛巾保留。
- grok-start/K2.png：同尺寸纯绿幕；右手（画面左）握木柄空炒锅，锅位于胸前偏自身右侧，左手（画面右）持锅铲；表情专注、略带笑意，锅内没有食物、火苗或蒸汽。
- 已检查帽标 NABO 与心电线正向、身份/服装连续、150 px 识别、头部叠图、底边腰部锚点和道具留白。两图帽顶约 y60（主体 alpha128 测得 K1 y62、K2 y58），人物和道具无绿色像素；K2 全图做统一比例/平移对位，未镜像或局部重绘。生成表情带来的细微轮廓变化仍待审图。
- 本轮使用内置 image_gen。完整提示词、SHA-256、原图位置及检查记录见 grok-start/K1.json、K2.json；原生生成图和大/小尺寸检查图在本机 private/a2/，不进入 Git。交付图按规定尺寸提交。
- K1/K2 在 ASSET_LIST.csv 中标为 review。当前等待刘东坡审核这两张起始图；确认后可按 GROK_PROMPTS.md 进入图生视频。尚未生成视频、动画帧或下一轮 24 个食材，也未做真机验证。
- 收工按 WORKFLOW 在 main 小步提交后 pull --rebase、push；没有操作阻塞。

---

## 最新状态 · 2026-10-28 下午 · Codex（Windows）

**阶段：A1 第 1 轮已交付，17 项标为 review，等待刘东坡选造型和审图。**

- 已创建私有仓库 [Liutupi/nabo-kitchen](https://github.com/Liutupi/nabo-kitchen)，本地 main 已通过 git push -u origin main 建立上游。收工按流程 rebase 后同步本轮交付。
- 造型：costume/costume_A.png、costume_B.png、costume_C.png，均为 1024×1024、正面交握姿势、纯 #00FF00 绿幕；帽标 NABO 方向正确。costume/compare_small.png 是实际人物 150 px 高的并排检查图。
- 菜品：dish-layers/ 内的 vessel_plate、vessel_plate_shadow、sauce_tomato__plate、egg__scrambled、tomato__wedge_cooked、garnish_scallion、garnish_shine，均为 640×480 独立透明 PNG，盘子中心固定 (320,270)，已按规定顺序实际合成。
- 食材：ingredients/ 内 egg、tomato、pork_rib、choy_sum、fish_whole、tofu，均为 160×160 透明 PNG；小脸只在食材上，熟料和菜品没有脸。
- 样板：style/style_board.png 为 960×1280，将 4 个 240×320 页面按 2 倍呈现：结果页深/浅底、冰箱页浅/深底。全部使用本轮实际图层和食材；B 造型只用于预览，**未定稿**。
- 已检查 150 px 人物、160×120 合成菜、40×40 食材、两种实际底色和 RGB565 模拟；源图尺寸、透明通道和文件 ID 检查通过。本轮没有真机验证。
- 使用内置 image_gen。完整提示词、输入/交付记录、SHA-256、坐标及待审事项见 style/style_board.json；额外原始生成图和检查图保存在本机 private/a1/，不进入 Git。规格尺寸的交付原图已进入版本库。

### 审图注意与下一步

1. 描边视觉上接近目标 #3B2730，但模型原图的实际色值并非精确一致，局部仍有轻微色彩变化。保留未限色原图供审图；建议在既定打包流程里统一映射描边色并检查每组 64 色，不能把本轮视为最终限色验收通过。**未修改任何定稿规格。**
2. 刘东坡从 A/B/C 选择厨房造型，并审核菜品和食材风格。确认本轮之后才开始 A2 的 K1/K2 及后续批量素材。
3. 本轮没有进行中的出图任务或操作阻塞；当前停在 A1 review，未制作下一轮素材、原型或固件。

---

## 最新状态 · 2026-10-28 下午 · Claude（总架构师）

**界面风格改版。**刘东坡看了第一版线框图，觉得太土、不够精致可爱，希望参照 NABO 工具箱的界面。已经改为和工具箱同一家族的视觉语言：

- 新增 `game-art/kitchen/UI_DESIGN.md`：风格、配色、字体、公共部件、各页版式。
- 新增高保真样板 `game-art/kitchen/reference/ui-mockup.png`（10 个页面），源文件在 `reference/ui-mockup/`。菜品和食材是 SVG 占位图，NABO 暂时借用工具箱的造型。
- `ART_BRIEF.md` 同步更新：取消木吧台和整屏背景；NABO 改成和工具箱一样的右下角半身；菜品、食材、图标统一使用 `#3B2730` 粗描边；食材图标加小脸。
- `CODEX_PROMPTS.md` 第 1 轮改为“造型三选一，加上菜品和食材的风格样板”。
- `palette/kitchen.json` 升到 0.2；`ASSET_LIST.csv` 删掉 5 项背景和木牌，新增开机字标和灯笼。
- 删除旧的 `reference/wireframes.*`。

下一步不变：Codex 做第 1 轮；刘东坡挑菜谱。

---

## 2026-10-28 · Claude（总架构师）

**阶段：M0 设计已完成，等你做决定。**

已完成：

- 架构：`docs/ARCHITECTURE.md`，包括分区预算、模块划分、两种运行模式、菜谱库格式、推荐算法、MCP 工具、按键约定、测试。
- 路线图和分工：`docs/ROADMAP.md`、`AGENTS.md`、`docs/WORKFLOW.md`。
- 玩法和界面：`game-art/kitchen/DESIGN.md`（当时的线框图已被下午的样板取代）。
- 美术规格：`game-art/kitchen/ART_BRIEF.md`，素材总表 `ASSET_LIST.csv`，色表 `palette/kitchen.json`，第一轮提示词 `CODEX_PROMPTS.md` 和 `GROK_PROMPTS.md`。
- 菜谱格式：`data/RECIPE_FORMAT.md`、`data/schema/recipe.schema.json`、`data/ingredients.json`（50 个食材的 ID）；3 道样例菜谱；候选菜单 `data/recipes/CANDIDATES.md`。

还没做：原型、工具、固件。所有尺寸和预算都是估算，还没有实测。

### 待你决定

1. **NABO 的厨房造型**：方案 A、B、C 在 ART_BRIEF 第 3 节，我推荐 B。等 Codex 出了图再选也可以。
2. **候选菜单**：在 `data/recipes/CANDIDATES.md` 里删掉你家不做的菜，补上你常做的。
3. **产品名**：暂定“NABO 深夜厨房 · LATE KITCHEN”。
4. **仓库公开还是私有**：建议开发阶段先私有，发布时再公开。

### 下一步

| 谁 | 做什么 |
|---|---|
| Codex | 按 `CODEX_PROMPTS.md` 第 1 轮出造型 A/B/C 和样板屏 |
| 刘东坡 | 做上面的 4 个决定；挑出第一批 30 道菜 |
| Claude | 菜谱定下后整理 30 道 JSON；开始做 M1 浏览器原型（先用占位图） |

### 进行中

（无）

---

## 决定记录

| 日期 | 决定 | 谁定的 |
|---|---|---|
| 2026-10-28 | 单独分出“NABO 厨房版”小智固件，只留小智和厨房 | 刘东坡 |
| 2026-10-28 | 画风参照“深夜食堂”的氛围：深夜小店、暖光、木吧台。产品名和画面里不用“深夜食堂”几个字 | 刘东坡 |
| 2026-10-28 | 菜品图采用“分层合成为主，招牌菜精绘为辅” | 刘东坡同意 |
| 2026-10-28 | 界面放弃木吧台写实风，改为沿用 NABO 工具箱的视觉语言，加上食物点缀色、格子桌布条和深夜元素；要精致、可爱 | 刘东坡 |
| 2026-10-28 | 本地精选库加上小智扩展库；用户菜谱存在 files 分区 | 架构（Claude） |
| 2026-10-28 | 分工：Claude 负责架构、规格、原型和工具；Codex 负责美术和固件；Grok 负责视频 | 架构（Claude），等刘东坡确认 |
| 2026-10-28 | 厨房造型选 C；A1 菜品和食材风格通过，继续第 2 轮 K1/K2 | 刘东坡（本次聊天确认） |
| 2026-10-28 | 后续工作由 Codex 自主推进，无需逐轮等待新指令；既定规则不改，交付保持 review 供审图 | 刘东坡（本次聊天授权） |
