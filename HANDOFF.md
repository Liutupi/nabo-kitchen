# 交接

新状态插在最上面。规则见 [docs/WORKFLOW.md](docs/WORKFLOW.md)。

---

## 最新状态 · 2026-10-28 · Claude（总架构师）

**阶段：M0 设计已完成，等你做决定。**

已完成：

- 架构：`docs/ARCHITECTURE.md`，包括分区预算、模块划分、两种运行模式、菜谱库格式、推荐算法、MCP 工具、按键约定、测试。
- 路线图和分工：`docs/ROADMAP.md`、`AGENTS.md`、`docs/WORKFLOW.md`。
- 玩法和界面：`game-art/kitchen/DESIGN.md`，线框图在 `game-art/kitchen/reference/wireframes.png`。
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
| 2026-10-28 | 本地精选库加上小智扩展库；用户菜谱存在 files 分区 | 架构（Claude） |
| 2026-10-28 | 分工：Claude 负责架构、规格、原型和工具；Codex 负责美术和固件；Grok 负责视频 | 架构（Claude），等刘东坡确认 |
