# 多机位协作流程

同一个仓库会在多台电脑上，由多个助手（Claude、Codex）和你本人一起修改。下面这些规矩，是为了避免互相覆盖。

## 每次开工

```sh
git pull --rebase
```

然后读 `HANDOFF.md` 顶部的“最新状态”。如果你要做的事已经有人在做（看 HANDOFF 里的“进行中”），先不要动。

## 每次收工

1. 小步提交。提交说明的开头写清范围，例如 `art: 食材 egg/tomato 第一稿`、`docs: 推荐权重调整`、`data: 新增 12 道粤菜`。
2. 在 `HANDOFF.md` 的“当前状态”下面插一块新状态（**不超过 15 行**）：日期、哪台电脑或哪个助手、做了什么、下一步、有没有阻塞。哈希和验证明细放 JSON，HANDOFF 里只放链接。
3. 执行 `git pull --rebase && git push`。

## 分支

- 文档、菜谱、美术交付这类小改动，直接在 `main` 上 rebase 后推送。
- 长期分支只有 `main`（内容）和 `firmware`（固件）。
- 改动较大，或者会动到别人负责的目录时，开临时分支：`claude/<主题>`、`codex/<主题>`，完成后合并回 `main` 或 `firmware`，**合并后删除临时分支**。
- 发布用标签（`v0.1.0`、`v0.2.0`），不另开发布分支。
- 不要使用 force push。

## 冲突热点

| 文件 | 规矩 |
|---|---|
| `HANDOFF.md` | 只插入新的状态块，不改别人的旧记录；任务表的状态列可以更新 |
| `game-art/kitchen/ASSET_LIST.csv` | 每个素材一行；只改自己负责那几行的 `status` 和 `note` 两列；新增行加在文件末尾 |
| `data/ingredients.json` | 只在末尾追加食材，**已有的 ID 永远不改名**（美术文件和菜谱都依赖这些 ID） |

## 大文件

- Grok 原视频放进 `game-art/kitchen/grok-clips/`，**只提交选中的那一条**，选择记录写在 `grok-clips/PICKS.md`。没选中的草稿留在本机的 `private/` 里。
- 原图按规格尺寸提交（通常是目标尺寸的 4 倍），不要提交 8K 的大图。
- 单个文件超过 20 MB 之前，先在 HANDOFF 里提出来讨论，再决定是否改用 Git LFS。

## 不能提交的东西

`private/` 下的所有内容：设备整片 Flash 备份、NVS、配网信息、串口日志、Wi-Fi 密码。这个目录已经写进 `.gitignore`。

## 状态词

`ASSET_LIST.csv` 和 HANDOFF 统一使用这几个状态：`todo` → `wip` → `review`（交付了，等审核）→ `done`（已验收）；被退回的标为 `redo`，并在 note 里写明原因。
