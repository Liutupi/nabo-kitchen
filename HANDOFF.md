# 交接

新状态插在最上面。规则见 [docs/WORKFLOW.md](docs/WORKFLOW.md)。

---

## 最新状态 · 2026-10-29 · Codex（Windows）· 全部缺失菜图已交付 GitHub

**127 张缺失完整菜图已补齐，当前 144/144 道有图，缺图 0；新增行全部为 `review`，等待逐图审美、实厨与实机验收。** 已推送并快进到 [codex/kitchen-model](https://github.com/Liutupi/nabo-kitchen/tree/codex/kitchen-model)，交付提交 `116ee8ea2ed72e5e5094805c397b8f1449d08b75`；同一成果保存在 `codex/kitchen-dish-art-20261029`。main 保留接手入口，勿混合内容/固件两个分支根目录。

- **接手美术**：本机 `D:\AI PASSPORT XIAOZHI\nabo-kitchen-art`。先看 [完整批次与复现命令](https://github.com/Liutupi/nabo-kitchen/blob/codex/kitchen-model/game-art/kitchen/dish-hero/batches/2026-10-29/README.md) 和该分支顶部 HANDOFF；原图、640×480 透明交付、提示词、原始/统一 LF 配方哈希均在 Git，manifest 为 `complete_pending_review`，不依赖本机缓存。原有 17 张、82 分层、菜谱、NABO C 造型和帽标、定稿规格未改。蛏子壳形已返修，清蒸鱼贴边首稿已拒收。
- **本机验证**：144 道 schema/引用及全部 PNG/原图哈希通过；菜图 5 项、原型 3 项、未改生产 C 代码真实资源绘制 11 项通过。328 张静态 PNG 限色解码往返通过，144 道均被 C 程序读取到完整图；已查看 11 页双底色解码预览及六页实际 C 页面。[报告与预览](https://github.com/Liutupi/nabo-kitchen/tree/codex/kitchen-model/game-art/kitchen/dish-hero/batches/2026-10-29)。GitHub 当前无开放 PR，该提交未返回 commit status，不能将本机 PASS 当作 GitHub CI 通过。
- **资源候选**：[候选包和许可](https://github.com/Liutupi/nabo-kitchen/tree/codex/kitchen-model/game-art/kitchen/dish-hero/batches/2026-10-29/resource-candidate)。整包 **3,756,062 B**，SHA256 `f1c0f215a460b6c4e7c8805527a6077ff2b8473626eb55205a77f9bda26d2931`，实际 5 MiB 区剩 **1,486,818 B**。仅菜图包变化，五个其他 payload 与现有固件一致。菜图 1,596,273 B，**超出旧 200 KB 子预算**，已在交接提出，未修改既定预算；实际分区容量检查通过。
- **下一步**：逐菜检查精确盘心、描边、全套风格与成菜准确度；通过后同步改 CSV/元数据为 `done`，返修保留版本与哈希。新资源在真实设备上的启动、内存、显示、按键流程和长期使用仍待验收。每 30 分钟 GitHub 跟进已开启，仅在实质变化、完成、失败/冲突或需要决定时通知。

本轮**没有刷机、替换应用或发布 Release**。设备仍用原 17 张菜图资源，固件入口仍为下方 `codex/runtime-fixes-20261029` 的 `c7cdc0b`，用户已确认欢迎语声音正常。

---

## 最新状态 · 2026-10-29 · Codex（Windows）· 菜图补全进行中

用户要求完成全部未完成菜图并持续关注 GitHub。**美术在 [codex/kitchen-dish-art-20261029](https://github.com/Liutupi/nabo-kitchen/tree/codex/kitchen-dish-art-20261029) 继续，从内容分支 `codex/kitchen-model` 的 `c31bae7` 创建。** 本机 `D:\AI PASSPORT XIAOZHI\nabo-kitchen-art`。已核对 144 道菜、17 张原有完整菜图、127 张待补；逐菜实际交付进度、生成提示词、配方哈希与双底色小图见 [可恢复批次清单](https://github.com/Liutupi/nabo-kitchen/tree/codex/kitchen-dish-art-20261029/game-art/kitchen/dish-hero/batches/2026-10-29)。请其他电脑先核对该清单，避免重复出图。

首批 4 张完整菜图、原图及元数据已推送，状态 `review`；当前继续生成剩余项。保持已通过的菜品风格、稳定 ID、640×480 透明画布与 160×120 显示，不修改菜谱/既定规则。已设置每 30 分钟跟进远端分支、PR/CI 和交接，仅在实质变化、冲突、批次完成或失败时通知；主任务制作时不重复生成。后续每批校验、小步提交并 rebase/push。当前无开放 PR，内容分支未返回 commit status；这不代表 CI 已通过。

固件最新状态仍以下方 Windows 运行时块为准，本批没有构建或刷写设备。新图需打包、实际资源容量和逐图视觉复核后才能成为设备候选，`review` 不是实厨或实机验收。

---

## 最新状态 · 2026-10-29 · Codex（Windows）· 运行时优化与实机复测

**已按最新交接继续独立离线厨房开发。本块覆盖下方旧候选的看门狗/无声/提示复发状态，但不代表全部菜谱或长期硬件认证完成。**

- **最新固件代码**在 [codex/runtime-fixes-20261029](https://github.com/Liutupi/nabo-kitchen/tree/codex/runtime-fixes-20261029)，提交 `c7cdc0b94a6009672333dc1736b4f1c33f0a0537`，从 `codex/offline-kitchen` 的 `f799a2f` 继续。先读该分支 [详细交接](https://github.com/Liutupi/nabo-kitchen/blob/codex/runtime-fixes-20261029/docs/development/offline-kitchen-handoff.zh_CN.md)、[脱敏验证记录](https://github.com/Liutupi/nabo-kitchen/blob/codex/runtime-fixes-20261029/docs/development/offline-runtime-validation-20261029.json) 和 `AGENTS.md`。本机源码目录 `D:\AI PASSPORT XIAOZHI\nabo-kitchen-runtime`。main 仍是入口，不能把固件分支根目录合进设计目录。
- 可编辑菜谱、美术、原型仍在 [codex/kitchen-model](https://github.com/Liutupi/nabo-kitchen/tree/codex/kitchen-model)。本轮没有修改 144 道草案、58 食材、13 种海鲜、17 张完整菜图、欢迎语 PCM 或资源包。
- **启动修正**：资源 CRC 改为等价查表；资源包每最多 4096 字节、菜库每一道主动让出 CPU，工作任务阻塞一个正数 tick，看门狗保持开启。实机 6 次受控启动均匹配新 ELF 前缀，未见看门狗/panic，资源校验约在开机后 1,655 ms 完成。
- **计时恢复**：确认时钟提示时立即保存暂停状态，避免旧的 1.5 秒延迟窗口。用户实际确认提示后，6 次复位没有再出现该提示，原暂停计时器保留；将来无可信时钟而中断运行计时，仍会按设计暂停并提示。
- **无声定位**：新增只读声音/音量、语音请求/消费、音频写入错误、计时状态诊断。实机原保存设置为声音关闭、音量 70%，所以欢迎语请求数为 0。用户在设置打开声音后已明确确认实际声音正常；每次启动只请求一遍、消费 31,200 个采样，写入错误为零。没有换驱动/PCM，也没有绕过静音设置。
- **实际刷入**：经具体候选同意并再次确认目标设备后，先校验分区表与现有资源匹配，只写 `0x10000` 应用并独立校验。应用 514,704 B，SHA256 `a6f4a167bf1d79d11bd7dbbf2a46d56eef5362446bbcd41313ace56effcb1202`；ELF `698af70572f6700387fa72f9af029fdede66b95dfcc2302f6ab01eeef1b9793f`。资源保持 `7556d051d6adb1868b452c5f341dd1592ff3415ac551b1ec6a077ca3cad339ec`。未刷 bootloader、分区表、NVS/PHY/files。
- 完整候选归档 `build/firmware/ea2bae8053a427827bc912aff570aa2fa9b84ce1da8e95ca2e19dd3d421c2f4b/`，分段包 `build/candidates/offline-runtime-ea2bae8053a4/`。生成文件与私密设备记录只在本机忽略目录。该完整镜像哈希不能代表保留旧 bootloader 的整机 Flash；不要重编译后冒充当前已测文件。
- **实机隔离流程**已走通四菜及额外 4 次确认完成的换组、第四张卡片聚焦、海鲜两页及勾选、用料加入采购、计时启动/暂停/继续/取消。仅在 TEST 模式自动注入按键，不等于实体计时长按手感通过。用户存档序号在隔离期间始终为 145；结束已恢复原暂停计时与用户设置（声音开、70%）、退出 TEST、关闭串口。最后空闲堆 198,336 B，最大块 114,688 B；UI 栈最低剩余 5,664 B。
- **门禁**：完整 ESP-IDF 5.5.3 构建、宿主测试、镜像/归档/分段校验通过；真实资源绘制 10 项，NVS 100 次极限保存及 484 个模拟断电切点通过。Windows 测试工具补齐原生 DLL/COFF/libc/路径兼容；21 项依赖目录符号链接权限的宿主检查明确跳过，未要求改变系统权限。

```text
Build: PASS
Host tests: PASS（21 项宿主符号链接能力检查跳过）
Device tests: PASS（本次有限启动、声音、恢复与隔离流程）
Unverified: 长期厨房使用、全部菜谱实做、真实断电/满载 NVS、实体计时长按手感和厨房环境计时铃听感
```

后续从 runtime-fixes 分支继续。USB 截图偶发不完整行，已拒收并重采完整 320 行画面；分帧/流控可作为后续诊断工具优化，不把每次采集都视为成功。本轮未创建 Release，也未把有限复测写成完整认证。

---

## 最新状态 · 2026-10-29 · Codex（macOS）· 跨电脑接手入口

**当前未通过实机认证。用户已要求把本轮源码与交接保存到 GitHub，然后换另一台电脑继续。本块覆盖下方 2026-10-28 冻结状态，main 仅增加入口，最新代码和内容分别在以下分支。**

- **固件**：[codex/offline-kitchen](https://github.com/Liutupi/nabo-kitchen/tree/codex/offline-kitchen)，已推送 `f799a2f3196e0d28a1777663a7cb65fcd8d84c4d`。先读 [离线固件详细交接](https://github.com/Liutupi/nabo-kitchen/blob/codex/offline-kitchen/docs/development/offline-kitchen-handoff.zh_CN.md) 和该分支的 `AGENTS.md`。
- **菜谱、美术、工具、原型**：[codex/kitchen-model](https://github.com/Liutupi/nabo-kitchen/tree/codex/kitchen-model)，先读 [最新交接](https://github.com/Liutupi/nabo-kitchen/blob/codex/kitchen-model/HANDOFF.md)。建议两个独立克隆；只拉旧 main 不会得到当前固件或新增素材。
- 当前是用户已同意的**独立离线厨房**，已移除小智网络/配网/云对话；旧介绍里的“小智加厨房”是历史基线。当前为 **144 道草案、58 食材、13 种海鲜、17 张完整菜图**。无限换菜、计时页长按下取消当前计时、开机等 OK 进入、`NABO 陪你做饭` 文案已实现；仍不是千道真实菜谱。
- 设备实际应用 SHA256 `5b77ba87157481f4be8e5e70540f0984d1f2131e8cfc3117a60503c47203ed15`，资源 `7556d051d6adb1868b452c5f341dd1592ff3415ac551b1ec6a077ca3cad339ec`，按同意仅写应用/资源，保留原兼容 bootloader、分区及用户数据。串口已关闭，未强行进入 TEST 或删除计时记录。
- **必须先修复**：重复启动又出现 IDLE 看门狗，采样定位到 `krc_read_body` 的 CRC / `load_item`；粗粒度阶段间延时 2 ms 仍不足。用户确认欢迎语依旧无声，不能将驱动无报错当成发声。用户可按 OK 消除时钟提示，但串口重启后提示又出现，需查计时状态持久化。详细地址、ELF 身份、复现与下一步均已写入固件交接。
- 完整 ESP-IDF 5.5.3 构建及宿主测试通过；**Device tests: FAIL**（看门狗复现、欢迎语无声）。NVS 100 次极限保存/484 个模拟断电切点通过属于宿主结果，真实满载/断电、新功能物理操作仍待验收。请勿发布为正式认证固件。
- 下一台电脑先检查工作树，再拉取上述分支。新的构建和刷机继续遵守固件 AGENTS 的 ESP-IDF、完整门禁、具体候选同意及分段保留数据要求。私密日志、NVS、设备身份和本机 ELF/固件产物未上传。

---

## 最新状态 · 2026-10-28 · Codex（Windows）· 成果冻结与 GitHub 交接

**按刘东坡本次指令收束并保存全部当前交付，仓库改为公开。以本块为当前状态，下面的“菜单未定 / 视频缺失 / 等 K1/K2”等段落为历史记录。**

- 仓库：[Liutupi/nabo-kitchen](https://github.com/Liutupi/nabo-kitchen)。本轮在 `codex/prototype-packing` 隔离实现，按协作流程合入 `main`，保留同名分支和固定标签 `checkpoint-2026-10-28`。接手以远端 main / 此标签为起点；不需要原聊天或本机 private 缓存。
- 冻结入口：[checkpoints/2026-10-28/README.md](checkpoints/2026-10-28/README.md)。保存六个候选运行时资源包、审图包、许可、源哈希、检查报告和预览图；清单由 `python tools/verify_checkpoint.py --sources` 核对。源码、规定尺寸原图、四条选中原 MP4、原型字体均在正常仓库目录。private/build/dist、依赖、大图草稿、设备备份和 NVS 不提交。
- **已确定的产品决定**：造型 C；菜品和食材风格通过；首批 30 道菜名通过（13 荤 / 12 素 / 5 汤，名单见 `data/recipes/REVIEW.md`）；近期历史只降权，7 天内仍可抽到，不做硬去重。原型已按此实现。旧 ROADMAP/DESIGN 的硬去重措辞与此决定冲突，未擅自改定稿规格，后续由 Claude 同步。
- **M1 实现**：`prototype/` 已走通 P0–P13，使用实际 30 道菜和美术，包含种子/整数权重、单菜/四菜、冰箱、买菜单、图鉴、三独立计时器、保存恢复、长按与原子双 UP。最终 20/20 自动化测试通过（10 纯模型 + 10 Edge UI，覆盖全部 148 步）。报告、真实测试输出及 18 张截图在 `prototype/validation/`。手机尺寸触摸为浏览器模拟；刘东坡手机完整做一道菜的 M1 验收仍待进行。
- **D1 内容**：curated 共 30 份 JSON，新增 27 份原创草案；Schema、全部外键/图片与字体覆盖通过，最长 KRC 正文 1176 B，小于 2048 B。菜单选择通过不等于做法内容验收，量、时长、熟度和口味仍需厨房逐道审核。原有菜心/鲈鱼补充明确用量与熟度并递增版本，调料表追加 `cola`，没有改已有 ID。
- **美术 / 视频**：静态清单 172 张齐全，含本次补的 6 张 640×480 透明熟料及同名 JSON；50 个食材、82 张菜品图层。四个 Grok 原视频 idle/toss/serve/tend 已收到，720×720 / 24 fps，源文件逐字节保留，选择记录在 `grok-clips/PICKS.md`。工程检查帽标正向、身份/服装、动作和画面边界；音轨未播放或打包。4 个动作和新增 6 张静态图均为 review。全表 done 15 / review 161 / todo 8，不把自动检查视为用户美术验收。
- **T1 候选实现**：`tools/` 已有 KSP1 限色/分层、KAN1 抽帧/抠绿/帧差、KRC1 真实 heatshrink 和 KFN1 点阵字库；共 30 项 Python 测试通过，另有完整源数据构建与字体覆盖检查。静态/菜谱/动画重复构建一致，最终字体 BIN 与 WOFF2 也独立重建逐字节一致。PNG/视频缩小与限色有损，逐像素往返一致指量化索引图；没有声称还原原始连续色彩。

### 实际候选包大小

| 文件 | 字节 | 备注 |
|---|---:|---|
| kitchen_dish.bin | 149,460 | 静态菜品层 |
| kitchen_icons.bin | 52,383 | 食材及图标 |
| kitchen_ui.bin | 3,779 | 字标及界面图 |
| kitchen_nabo.bin | 495,064 | 4 动作、42 帧、192×192、12 fps、共享 59 色 |
| kitchen_recipes.krc | 26,113 | 30 道，字体覆盖已检查 |
| kitchen_fonts.bin | 959,955 | **超过原 380,000 B 分项预算** |
| **运行时包小计** | **1,686,754** | 不含精绘、声音、小智基线资源、容器开销和浏览器字体 |

中文使用固定版本的 Source Han Sans SC Medium，12/16/18 px 覆盖一级字 3500 加当前菜谱/界面共 3628 字，15/20 px 为界面/菜名子集；数字 Archivo Black。浏览器改名子集 KitchenHan 为 1,243,708 B，保留 OFL。字体来源、字符表、下载 SHA 与重建入口在 `tools/fonts/`、`tools/FONT_FORMAT.md`。

### 接手后依次做什么

1. `git switch main` 后 `git pull --rebase`，读本块，再读 `AGENTS.md`、`docs/WORKFLOW.md` 及相关规格。用 `python tools/verify_checkpoint.py --sources` 核对冻结清单。若继续跨目录实现，另开分支并在交接记录范围。
2. `node prototype/server.mjs`，访问 `http://127.0.0.1:4173/`。重建工具及固定依赖见 `prototype/README.md`、`tools/FORMAT.md`、`tools/ANIMATION_FORMAT.md`、`tools/FONT_FORMAT.md`；原型无需 CDN/npm 运行依赖。Python 依赖可装在独立 venv 或忽略的 private/t1/vendor。
3. **先收敛 T1 两处问题**：字库 959,955 B 超分项预算，且六包小计不证明真实 assets 分区可装下；需要继续优化与实际基线预算核对。动画当前候选使用 192×192，源 CSV 写 256×256 frames，接入前必须由架构/美术复核或按定稿规格重建，不能当作规格已经通过。
4. 四动作目前只完成原视频工程检查与宿主打包，**浏览器仍显示静态 C/K2，动画播放、音效尚未接入**。确认候选格式后接播放并复核动作时长（源 4–5 秒重定时为规定游戏帧数）、循环、帽标和角色比例。新增美术保留 review；后续 8 个非核心视频与招牌菜精绘未做。
5. 继续 M2：将 `prototype/model.mjs` 纯逻辑和确定测试时钟接口作为标准，做 C 规则/计时/保存对拍，加入故意改坏权重/按键/计时的检出测试；随后 M3 C 渲染比对。当前没有 C 版本、编译或对拍结果。
6. D1 等刘东坡厨房逐道审做法，M1 等手机完整跟做。M4 才从指定 table v1.6.0 基线分出 `ai-passport-nabo-xiaozhi-kitchen`，核实 files 分区、真实 assets/App 预算和堆内存；再构建、按授权真机验收、家庭连续使用一周后发布。**本次没有创建固件仓库、刷机或 Release。**

当前没有进行中的出图任务。已完成的宿主结果可以直接继续使用；待验收与工程问题均已留在本块及检查点，未改既定规格绕过。收工按小范围提交、pull --rebase、push，并核对远端 main 与固定标签。

---

## 最新状态 · 2026-10-28 · Codex（Windows）· M1/T1 持续推进

**当前目标：按路线图继续推进，仅产品取舍和验收事项需要刘东坡决定。上一轮完成了静态图交付，本轮开始 M1 原型和 T1 管线实现。**

- 已同步远端；在 codex/prototype-packing 分支协作实现 prototype/ 和 tools/，按 AGENTS 的跨目录协作规则隔离改动，供架构复核。定稿规格保持不变。
- 并行工作：浏览器原型及纯逻辑、静态图片/菜谱打包与往返解码验证；先使用现有 3 道 curated 样例，不能把候选菜单视作已验收菜谱。
- 首批 30 道菜单仍需产品负责人选择；Codex 会提供具体方案供审定，并在等待期间继续无依赖的工程工作。
- Grok 视频仍缺原始输入，原型可显示静态姿势并明确此限制；不制作伪造动画来冒充 A2 完成。
- 当前在制：M1/T1。尚未做 M2 固件模型、M3 C 渲染、M4 基线分出或真机验证。

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
| 2026-10-28 | 批准首批 30 道菜名：13 荤、12 素、5 汤，具体名单见 data/recipes/REVIEW.md；做法内容仍待厨房验收 | 刘东坡（本次聊天确认） |
| 2026-10-28 | 最近做过的菜只降低权重，7 天内仍可再次抽到；覆盖旧文档硬去重措辞，规格同步交 Claude | 刘东坡（本次聊天确认） |
| 2026-10-28 | 固定全部当前成果并上传 GitHub，写好交接便于继续开发；仓库由私有改为公开，美术许可范围不变 | 刘东坡（本次聊天指令） |
