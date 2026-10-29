# NABO 深夜厨房 · LATE KITCHEN

**v0.1.0 首版体验固件已发布：**[下载固件和分段包](https://github.com/Liutupi/nabo-kitchen/releases/tag/v0.1.0) · [安装与按键说明](https://github.com/Liutupi/nabo-kitchen/blob/codex/release-v0.1.0/docs/community/v0.1.0/installation.zh_CN.md) · [发布验证记录](https://github.com/Liutupi/nabo-kitchen/blob/codex/release-v0.1.0/docs/community/v0.1.0/publication-verification.json)。

[社区首版现已公开](https://ai-passport.folotoy.cn/plays/1755/?v=4068-2)，项目 **1755**、通过版本 **4068**；匿名固件下载与 GitHub 逐字节一致，五张公开图核验通过。[公开验证记录](https://github.com/Liutupi/nabo-kitchen/blob/codex/release-v0.1.0/docs/community/v0.1.0/community-publication-verification.json)。

<img src="https://raw.githubusercontent.com/Liutupi/nabo-kitchen/v0.1.0/assets/community/v0.1.0/01-cover.webp" width="450" alt="NABO 深夜厨房宣传插画">

首版是独立离线厨房，包含 144 道菜谱和完整菜图草案、58 种食材（含 13 种海鲜）、抽菜、冰箱推荐、采购、步骤跟做、收藏与三个可暂停计时器，不包含小智云对话或语音识别。菜谱和美术仍在审校，更新后实际面板/听感与长期使用仍待验收；详细边界以发布页为准。下文保留产品设计方向，未来功能不代表已在首版实现。

FoloToy AI Passport 工牌（ESP32-C3，240×320 竖屏）上的家常菜助手。NABO 下了夜班开了一间小厨房，帮你决定**今天吃什么**，再一步一步带你**把菜做出来**。

> 产品名为暂定。“深夜食堂”只作为画风和氛围的参考，产品名称、画面和文字里都不使用这几个字。

## 它解决什么

| 痛点 | 功能 |
|---|---|
| 不知道吃什么 | **颠勺抽菜**：NABO 颠一下锅，抽出今天的菜或者一整套搭配（一荤一素一汤），最近吃过的菜降低权重，7 天内仍可抽到 |
| 不知道怎么做 | **跟做模式**：一屏一步，有火候和做法图标；可以同时开 3 个计时器；按键或语音“下一步” |
| 家里有什么做什么 | **冰箱模式**：勾选手头的食材，推荐能做的菜，并标出还缺几样 |
| 买菜 | **买菜单**：把选好的菜合并成一张食材清单 |
| 坚持做饭 | **图鉴和等级**：做完一道菜就点亮一格，从“夜宵学徒”升到“深夜厨神” |

在线时，小智就是没有上限的扩展菜谱库：可以问调料怎么替代，也可以说“把这道菜存进菜谱”。

## 仓库是什么

这是**设计与发布入口仓库**，main 保存架构、规格、原型和交接入口。可编辑的最新菜谱、美术及打包工具在 [codex/kitchen-model](https://github.com/Liutupi/nabo-kitchen/tree/codex/kitchen-model)，首版固件及发布材料在 [codex/release-v0.1.0](https://github.com/Liutupi/nabo-kitchen/tree/codex/release-v0.1.0)，旧运行时修复基线保存在 [codex/runtime-fixes-20261029](https://github.com/Liutupi/nabo-kitchen/tree/codex/runtime-fixes-20261029)。设计和固件根目录不同，请按顶部交接继续，不要直接混合两个分支的根目录。

## 从哪里读起

当前已交付可运行的[浏览器原型](prototype/README.md)、144 道菜谱和全部完整菜图草案、静态美术、四段原视频、宿主机工具和独立离线固件。main 中部分设计资料和 30 菜阶段记录是历史快照，最新内容请进入上面的内容分支。原型可用 `node prototype/server.mjs` 打开 `http://127.0.0.1:4173/`。首版应用与新资源已完成有限实机检查，详情见发布记录；不等于全部实厨或实体观察验收。

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
| 想看进度 | [docs/ROADMAP.md](docs/ROADMAP.md) |
| 在多台电脑上协作 | [docs/WORKFLOW.md](docs/WORKFLOW.md) |

## 许可

本项目与 FoloToy 官方无关联。源码采用 MIT 许可，NABO 形象和美术素材不在 MIT 授权范围内，详见 [NOTICE.md](NOTICE.md)。
