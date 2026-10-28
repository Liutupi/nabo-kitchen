"""检查 data/recipes 下所有菜谱：schema、食材和调料 ID、图层 ID、步骤字数。

用法：python tools/check_recipes.py
依赖：pip install jsonschema
"""
import csv, json, sys
from pathlib import Path

import jsonschema

ROOT = Path(__file__).resolve().parent.parent
schema = json.loads((ROOT / "data/schema/recipe.schema.json").read_text(encoding="utf-8"))
ing = {i["id"] for i in json.loads((ROOT / "data/ingredients.json").read_text(encoding="utf-8"))["ingredients"]}
sea = {i["id"] for i in json.loads((ROOT / "data/seasonings.json").read_text(encoding="utf-8"))["seasonings"]}
with open(ROOT / "game-art/kitchen/ASSET_LIST.csv", encoding="utf-8") as f:
    assets = {row["id"] for row in csv.DictReader(f)}

errors, ids = [], set()
files = sorted((ROOT / "data/recipes").rglob("*.json"))
for p in files:
    r = json.loads(p.read_text(encoding="utf-8"))
    tag = p.relative_to(ROOT)
    try:
        jsonschema.validate(r, schema)
    except jsonschema.ValidationError as e:
        errors.append(f"{tag}: {e.message}")
        continue
    if p.stem != r["id"]:
        errors.append(f"{tag}: 文件名应为 {r['id']}.json")
    if r["id"] in ids:
        errors.append(f"{tag}: id 重复")
    ids.add(r["id"])
    errors += [f"{tag}: 未知食材 {i['id']}" for i in r["ingredients"] if i["id"] not in ing]
    errors += [f"{tag}: 未知调料 {s['id']}" for s in r["seasonings"] if s["id"] not in sea]
    errors += [f"{tag}: 未登记图层 {l}" for l in r["dish"]["layers"] if l not in assets]
    if not any(i.get("main") for i in r["ingredients"]):
        errors.append(f"{tag}: 至少要有一个主料（main: true）")

print(f"检查了 {len(files)} 道菜")
if errors:
    print("\n".join(errors))
    sys.exit(1)
print("全部通过")
