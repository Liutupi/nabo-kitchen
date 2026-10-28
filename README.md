# NABO 深夜厨房 · LATE KITCHEN

FoloToy AI Passport 工牌（ESP32-C3，240×320 竖屏）上的家常菜助手。NABO 在一间深夜小店的吧台后面，帮你决定**今天吃什么**，再一步一步带你**把菜做出来**。

> 产品名为暂定。“深夜食堂”只作为画风和氛围的参考，产品名称、画面和文字里都不使用这几个字。

## 它解决什么

| 痛点 | 功能 |
|---|---|
| 不知道吃什么 | **颠勺抽菜**：NABO 颠一下锅，抽出今天的菜或者一整套搭配（一荤一素一汤），会避开最近吃过的菜 |
| 不知道怎么做 | **跟做模式**：一屏一步，有火候和做法图标；可以同时开 3 个计时器；按键或语音“下一步” |
| 家里有什么做什么 | **冰箱模式**：勾选手头的食材，推荐能做的菜，并标出还缺几样 |
| 买菜 | **买菜单**：把选好的菜合并成一张食材清单 |
| 坚持做饭 | **图鉴和等级**：做完一道菜就点亮一格，从“夜宵学徒”升到“深夜厨神” |

在线时，小智就是没有上限的扩展菜谱库：可以问调料怎么替代，也可以说“把这道菜存进菜谱”。

## 仓库是什么

这是**设计仓库**，作用和 `nabo-radio`、`nabo-fortune` 相同，用来放架构、规格、美术、菜谱数据、原型和打包工具。固件到 M4 阶段再从小智版分出一个独立仓库 `ai-passport-nabo-xiaozhi-kitchen`，见 [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)。

## 从哪里读起

| 你是 | 先读 |
|---|---|
| 任何人，每次开工前 | [HANDOFF.md](HANDOFF.md)：当前状态和下一步 |
| 第一次来 | [AGENTS.md](AGENTS.md)：分工、目录归属和规矩 |
| 做架构或固件 | [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) |
| 做玩法或界面 | [game-art/kitchen/DESIGN.md](game-art/kitchen/DESIGN.md) |
| 做美术 | [game-art/kitchen/ART_BRIEF.md](game-art/kitchen/ART_BRIEF.md)，再看 [CODEX_PROMPTS.md](game-art/kitchen/CODEX_PROMPTS.md) 或 [GROK_PROMPTS.md](game-art/kitchen/GROK_PROMPTS.md) |
| 写或审菜谱 | [data/RECIPE_FORMAT.md](data/RECIPE_FORMAT.md) |
| 想看进度 | [docs/ROADMAP.md](docs/ROADMAP.md) |
| 在多台电脑上协作 | [docs/WORKFLOW.md](docs/WORKFLOW.md) |

## 许可

本项目与 FoloToy 官方无关联。源码采用 MIT 许可，NABO 形象和美术素材不在 MIT 授权范围内，详见 [NOTICE.md](NOTICE.md)。
