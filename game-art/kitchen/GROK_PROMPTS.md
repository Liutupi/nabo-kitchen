# 给 Grok 的视频任务（图生视频）

- 起始图：`grok-start/K1.png` 和 `K2.png`（第 2 轮由 Codex 交付）。
- 每条视频**从起始姿势开始，并回到起始姿势结束**。
- **镜头完全固定**：不推拉，不摇移，不变焦，不切镜头。背景保持纯绿幕。人物下半身被画面底边切掉，这条底边保持不动。
- 时长 3–5 秒。不要加文字、字幕或特效。
- 交付到 `grok-clips/<ID>.mp4`；同一个动作生成多次，就加上 `_v2`、`_v3`。选中的那条写进 `grok-clips/PICKS.md`。

每条提示词后面都附上这句通用要求：

> Locked-off static camera, no zoom, no pan. Keep the solid green background; the character is cropped at the waist by the bottom frame edge, keep that framing fixed. Keep the character's face, glasses, white cap and cap logo exactly as in the image; never mirror the cap logo. Smooth cartoon animation with anticipation and follow-through. End in the same pose as the first frame.

## v0.1

| ID | 起始图 | 提示词 |
|---|---|---|
| `idle` | K1 | The chef breathes gently and sways slightly, glances up at the viewer with a gentle smile, blinks once, and returns to the starting pose. Calm, cozy mood. Loopable. |
| `toss` | K2 | The chef dips the wok slightly down as anticipation, then flips it upward energetically with his right hand, leaning back a little, catches the motion, and nods with a satisfied grin. The wok stays empty (no food, no fire). Returns to the starting pose. |
| `serve` | K1 | The chef opens his right palm toward the left side of the frame in a welcoming "please enjoy" gesture, bows slightly and winks, then returns to the starting pose. |
| `tend` | K2 | The chef looks down into the wok with focus, gently stirs once with the spatula, then picks up a small round hand fan and fans the wok twice, and returns to the starting pose. Loopable. |

## v0.2

| ID | 起始图 | 提示词 |
|---|---|---|
| `think` | K1 | The chef looks down at a small paper note in his left hand, ticks something on it with a pencil, looks up thinking for a moment, then looks back down. Loopable. |

## v0.3

| ID | 起始图 | 提示词 |
|---|---|---|
| `ding` | K2 | The chef taps the rim of the wok twice with the spatula and calls out with an open mouth, cheerful, then returns to the starting pose. |
| `cheer` | K1 | The chef gives a thumbs up with his right hand and winks with one eye, then returns to the starting pose. |
| `shake` | K1 | The chef tilts his head, shrugs with open palms, shakes his head with a playful smile, and returns to the starting pose. |
| `taste` | K2 | The chef scoops a tiny bit from the wok with the spatula, tastes it, reacts to the heat by blowing air, then nods happily and returns to the starting pose. |
| `doze` | K1 | The chef's eyelids slowly droop, his head nods down and jerks back up, sleepy. Loopable. |
| `levelup` | K1 | The chef raises both fists above his head with joy, does a small happy hop, lands and returns to the starting pose. |
| `wave` | K1 | The chef waves his right hand at the viewer in a friendly welcome, then returns to the starting pose. |

`toss`、`levelup` 里锅和手举得比较高，要注意不能出画。
