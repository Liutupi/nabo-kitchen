# NABO 深夜厨房 · LATE KITCHEN

**v0.2.0 体验固件已发布：**[下载固件和分段包](https://github.com/Liutupi/nabo-kitchen/releases/tag/v0.2.0) · [安装与按键说明](https://github.com/Liutupi/nabo-kitchen/blob/v0.2.0/docs/community/v0.2.0/installation.zh_CN.md) · [发布验证记录](https://github.com/Liutupi/nabo-kitchen/blob/firmware/docs/community/v0.2.0/publication-verification.json)。

[社区项目 1755](https://ai-passport.folotoy.cn/plays/1755/) 已更新为 v0.2.0 / 4104 并公开，固件匿名回下载与 GitHub 逐字节一致，中英说明、封面和四张介绍图已同步。[当前状态与公开核验](https://github.com/Liutupi/nabo-kitchen/blob/firmware/docs/community/v0.2.0/community-publication-verification.json)。

<img src="https://raw.githubusercontent.com/Liutupi/nabo-kitchen/v0.2.0/assets/community/v0.2.0/01-cover.webp" width="450" alt="NABO 深夜厨房宣传插画">

当前 v0.2.0 为独立离线体验版，包含 145 道 D2 菜谱与完整菜图、58 种食材、抽菜、冰箱推荐、采购、步骤跟做、收藏和三个可暂停计时器。选中项整块变色，首页常驻计时器入口；开机不问日期，近期降权按完成做饭次数。用户确认尚未三道实际下厨验收，按体验版发布；菜谱、美术与完整硬件验收仍在继续，详细范围以发布说明为准。下文设计方向不等于已实现功能。

FoloToy AI Passport 工牌（ESP32-C3，240×320 竖屏）上的家常菜助手。NABO 下了夜班开了一间小厨房，帮你决定**今天吃什么**，再一步一步带你**把菜做出来**。

> 产品名为暂定。“深夜食堂”只作为画风和氛围的参考，产品名称、画面和文字里都不使用这几个字。

## 它解决什么

| 痛点 | 功能 |
|---|---|
| 不知道吃什么 | **颠勺抽菜**：抽一道菜或荤素组合；近期做过的菜按完成做饭次数降低权重，仍可再次抽到 |
| 不知道怎么做 | **跟做模式**：一屏一步，有火候和做法图标；可以同时开 3 个计时器 |
| 家里有什么做什么 | **冰箱模式**：勾选手头的食材，推荐能做的菜，并标出还缺几样 |
| 买菜 | **买菜单**：把选好的菜合并成一张食材清单 |
| 坚持做饭 | **图鉴和等级**：做完一道菜就点亮一格，从“夜宵学徒”升到“深夜厨神” |

v0.2 是**纯离线、只用三个按键**的版本，欢迎语和提示音本地播放，不含语音识别或联网功能。语音“下一步”、在线菜谱等仍留待以后评估。

## 仓库是什么

这个仓库只有两个分支：

- **`main`**：规格、菜谱（145 道）、美术、浏览器原型、打包工具、交接。
- **[`firmware`](https://github.com/Liutupi/nabo-kitchen/tree/firmware)**：工牌固件源码和发布材料。它基于 FoloToy 官方模板，和 main 是两套独立的目录，不要混在一起。

## 从哪里读起

当前已交付可运行的[浏览器原型](prototype/README.md)、145 道菜谱（D2 质量版）、完整菜图、静态美术、四段原视频、宿主机工具和独立离线固件。原型可用 `node prototype/server.mjs` 打开 `http://127.0.0.1:4173/`。首版应用与新资源已完成有限实机检查，详情见发布记录；不等于全部实厨或实体观察验收。

2026-10-28 的源码与候选资源包固定在 [开发检查点](checkpoints/2026-10-28/README.md)，含二进制、许可、验证报告、预览图和 SHA-256 清单。继续开发先读 HANDOFF 顶部；不要把宿主测试通过视为菜谱、手机实做或真机验收完成。

| 你是 | 先读 |
|---|---|
| 任何人，每次开工前 | [HANDOFF.md](HANDOFF.md)：当前状态和下一步 |
| 第一次来 | [AGENTS.md](AGENTS.md)：分工、目录归属和规矩 |
| 做架构或固件 | [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) |
| 做玩法 | [game-art/kitchen/DESIGN.md](game-art/kitchen/DESIGN.md) |
| 做界面 | [game-art/kitchen/UI_DESIGN.md](game-art/kitchen/UI_DESIGN.md)，样板图 `game-art/kitchen/reference/ui-mockup.png` |
| 做美术 | [game-art/kitchen/ART_BRIEF.md](game-art/kitchen/ART_BRIEF.md)，再看 [CODEX_PROMPTS.md](game-art/kitchen/CODEX_PROMPTS.md) 或 [GROK_PROMPTS.md](game-art/kitchen/GROK_PROMPTS.md) |
| 写或审菜谱 | [data/RECIPE_FORMAT.md](data/RECIPE_FORMAT.md) |
| 想看进度 | [docs/ROADMAP.md](docs/ROADMAP.md)，最新评估见 [docs/REVIEW-2026-10-29.md](docs/REVIEW-2026-10-29.md) |
| 在多台电脑上协作 | [docs/WORKFLOW.md](docs/WORKFLOW.md) |

## 许可

本项目与 FoloToy 官方无关联。源码采用 MIT 许可，NABO 形象和美术素材不在 MIT 授权范围内，详见 [NOTICE.md](NOTICE.md)。
