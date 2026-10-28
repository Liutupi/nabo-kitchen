# 菜谱格式

一道菜一个 JSON 文件，放在 `data/recipes/curated/<id>.json`。格式由 `schema/recipe.schema.json` 定义，检查命令：

```sh
python tools/check_recipes.py
```

完整例子见 `recipes/curated/fanqie-chaodan.json`。

## 字段

| 字段 | 必填 | 说明 |
|---|---|---|
| `id` | ✅ | 永久 ID，用拼音，用短横线连接，例如 `fanqie-chaodan`。**发布后不再修改**，因为做菜记录、图鉴和招牌菜图片都靠它对应 |
| `name` | ✅ | 菜名，最多 10 个字 |
| `aliases` | | 别名，给语音和搜索用，例如“西红柿炒鸡蛋” |
| `course` | ✅ | 用于搭配：`meat` 荤、`veg` 素、`soup` 汤、`staple` 主食。蛋和豆腐为主的菜可以同时写 `meat` 和 `veg` |
| `cuisine` | | 家常、粤、川、湘、鲁、江浙、东北、西北、其他 |
| `tags` | | 最多 4 个，从固定列表中选（见 schema） |
| `minutes` | ✅ | 从备菜到出锅的总时间，单位是分钟（不算腌制过夜这类时间） |
| `difficulty` | ✅ | 1 新手、2 普通、3 要点功夫 |
| `servings` | ✅ | 这份用量是几人份。界面会按人数换算 |
| `season` | | 当季月份，抽菜时当季的权重高一些 |
| `ingredients` | ✅ | 食材：`id` 必须在 `data/ingredients.json` 中；`main: true` 表示主料，冰箱匹配只看主料 |
| `seasonings` | ✅ | 调料：`id` 必须在 `data/seasonings.json` 中，用量写成文字，例如“1 小勺”“适量” |
| `steps` | ✅ | 步骤，见下文 |
| `dish` | ✅ | 菜品图怎么合成，见下文 |
| `source` | ✅ | `curated` 精选、`user` 用户手写、`ai` 小智保存 |
| `version` | ✅ | 内容改动后加 1 |

## 步骤怎么写

- 每一步**最多 48 个字**（屏幕上是 12 字一行、4 行）。写不下就拆成两步。
- 一步只做一件事，以动作开头：“热锅放油”“下排骨”“盖上锅盖焖”。
- 用量写具体数字（“2 大勺”“30 秒”），不写“少许”“片刻”。只有盐可以写“适量”。
- `method` 是做法图标，`heat` 是火候图标（`off` 表示不开火或者关火）。
- 需要等待的步骤加上 `timer`（秒），跟做时会自动出现计时胶囊。
- `tip` 是小贴士，最多 16 个字，只写真正有用的诀窍。
- 不写药膳功效、减肥、治病这类说法。

## 菜品图

```json
"dish": {"vessel": "plate", "sauce": "tomato", "layers": ["egg__scrambled", "tomato__wedge_cooked"], "garnish": ["scallion", "shine"], "hero": true}
```

- `vessel` 是器皿；`sauce` 是汤汁（没有就写 `null`）；`layers` 是熟料图层，最多 4 层，从下往上排；`garnish` 是点缀，最多 3 个。
- 所有图层 ID 都必须已经在 `game-art/kitchen/ASSET_LIST.csv` 里。需要的熟料还没有，就在 CSV 末尾加一行，状态写 `todo`。
- 有招牌菜精绘图的写 `hero: true`。没有精绘图之前，会先用合成图代替。

## 小智保存的菜谱

格式相同，`source` 写 `ai`。写入工牌之前，会检查 schema 和字体缺字，不通过就让大模型改写。用不到已有图层的菜，用 `vessel` 加 `sauce` 加最接近的熟料图层代替。
