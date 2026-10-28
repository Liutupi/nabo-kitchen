# AGENTS.md · nabo-kitchen（NABO 深夜厨房）

FoloToy AI Passport 工牌（ESP32-C3，无 PSRAM，240×320 竖屏，三个按键）上的家常菜助手。整机只装**小智加上厨房**，不装其他游戏。

**开工前必读**：`HANDOFF.md`（当前状态）→ `docs/WORKFLOW.md`（同步规矩）→ 和你任务相关的规格文档。

## 分工

| 谁 | 负责 | 不负责 | 入口文档 |
|---|---|---|---|
| **刘东坡（产品负责人）** | 拍板（造型、配色、名称、菜谱取舍）；提供和审核家常菜谱；在厨房和真机上验收 | — | `HANDOFF.md` 里的“待你决定” |
| **Claude（总架构师）** | 架构和接口；玩法和界面规格；美术规格和验收；菜谱格式和内容整理；浏览器原型；素材处理和打包工具（抽帧、限色、菜品分层合成）；审图 | 直接出美术图 | `docs/ARCHITECTURE.md`、`game-art/kitchen/DESIGN.md` |
| **Codex** | 所有静态美术（起始图、造型、食材、菜品分层、招牌菜、图标、界面套件、背景）；M4 起分出固件仓库，完成 C 实现、测试和 GitHub 发布 | 改规格文档里已定稿的规则（需要改就在 HANDOFF 里提出） | `game-art/kitchen/ART_BRIEF.md`、`CODEX_PROMPTS.md` |
| **Grok** | NABO 半身表演视频（图生视频） | 静态图 | `game-art/kitchen/GROK_PROMPTS.md` |

分工可以调整，但调整要写进 HANDOFF 的决定记录。

## 目录归属

“写”表示主要由谁改。别人要改，先在 HANDOFF 里留言，或者开一个分支。

| 目录 | 写 | 说明 |
|---|---|---|
| `docs/`、`game-art/kitchen/*.md`、`data/schema/` | Claude | 规格。美术交付时可以在 `ASSET_LIST.csv` 里更新状态列 |
| `game-art/kitchen/{costume,style,ingredients,dish-layers,dish-hero,icons,ui,props}/` | Codex | 原图按 ART_BRIEF 的 ID 命名 |
| `game-art/kitchen/grok-start/` | Codex | Grok 用的起始图 |
| `game-art/kitchen/grok-clips/` | Grok 交付，由人放进来 | 原视频，不转码 |
| `game-art/kitchen/reference/` | 只读 | `nabo-idle.png` 是 NABO 身份的基准 |
| `data/recipes/` | Claude 整理，刘东坡审核 | 菜谱 JSON |
| `prototype/`、`tools/` | Claude | 原型和打包工具 |
| `HANDOFF.md` | 所有人 | 只在顶部“最新状态”段落和决定记录里追加 |
| `private/` | 本机 | 被 Git 忽略。放设备备份、串口日志、草稿 |

## 规则

- **NABO 身份不能改**：以 `game-art/kitchen/reference/nabo-idle.png` 为准，脸、眼睛、黑色圆框眼镜、棕色头发、白色棒球帽、帽标（NABO 字样加心电线）、比例都保持不变。帽标**不能左右镜像**。服装按 ART_BRIEF 的定稿方案。
- **文件名严格使用 ID**：打包脚本靠文件名对号入座。ID 来自 `ASSET_LIST.csv` 和 `data/ingredients.json`。
- **原图和原视频都要留**，不要只交缩小版。
- **所有素材都是原创**：不参照、不临摹《深夜食堂》或其他作品的画面、标志、文字，也不使用真实品牌的包装。
- **原型是规则的标准答案**：改规则先改原型和规格，再改固件。
- 菜谱内容保持家常、准确，不写药膳疗效、减肥或治病的说法。
- 本项目与 FoloToy 官方无关联。NABO 形象和美术素材不随源码的 MIT 许可授权。
