"""菜谱内容质量检查（只读，不改任何文件）。

check_recipes.py 管“格式对不对”，这个脚本管“像不像一份好菜谱”。它找的是
人工审稿时最容易放过、但在厨房里最影响体验的几类问题：

  W1 要用温度计       家里一般没有探针温度计，应改成看颜色、手感等判断方法
  W2 单独的卫生步骤   “洗手洗刀板”占掉一整步，用户要多按一次 OK
  W3 泛称食材         步骤里写“海鲜”“配菜”，没有写具体的食材名
  W4 模板重复         同一句步骤在很多道菜里一字不差地出现
  W5 难度和标签不符   难度 1 或“适合新手”，却用了鲍鱼、整鸡这类难处理的食材
  W6 组合式命名       同一主料用“X炒Y / X焖Y”批量组合出的菜太多，菜单会显得单调

用法：
  python tools/lint_recipes.py                 # 打印摘要
  python tools/lint_recipes.py --json out.json # 另外输出逐道明细
"""
import argparse
import collections
import glob
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
THERMO = re.compile(r"\d+\s*℃")
HYGIENE = re.compile(r"(洗手|洗净刀板|清洗刀板|清洗.*台面|生熟.*分开|独立.*刀板)")
GENERIC = ("海鲜", "配菜", "主料")
HARD = {"abalone", "chicken_whole", "crab", "octopus", "fish_whole"}
COMBO_PREFIX_LIMIT = 8  # 同一前缀超过这个数，就提示组合式命名


def load():
    out = []
    for f in sorted(glob.glob(str(ROOT / "data/recipes/curated/*.json"))):
        out.append(json.loads(Path(f).read_text(encoding="utf-8")))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--json")
    ap.add_argument("--repeat", type=int, default=4, help="同一句步骤出现多少次算模板重复")
    a = ap.parse_args()
    recipes = load()
    step_count = collections.Counter(s["text"] for r in recipes for s in r["steps"])
    detail = {}
    for r in recipes:
        w = []
        for i, s in enumerate(r["steps"], 1):
            t = s["text"]
            if THERMO.search(t):
                w.append(("W1", i, t))
            if HYGIENE.search(t) and not s.get("timer") and len(re.sub(HYGIENE, "", t)) < 14:
                w.append(("W2", i, t))
            if any(g in t for g in GENERIC):
                w.append(("W3", i, t))
            if step_count[t] >= a.repeat:
                w.append(("W4", i, t))
        ids = {x["id"] for x in r["ingredients"]}
        if (r["difficulty"] == 1 or "适合新手" in r.get("tags", [])) and ids & HARD:
            w.append(("W5", 0, f"难度 {r['difficulty']} / 标签 {r.get('tags')}，含 {sorted(ids & HARD)}"))
        if w:
            detail[r["id"]] = {"name": r["name"], "warnings": w}

    prefix = collections.Counter(r["name"][:2] for r in recipes)
    combos = {p: n for p, n in prefix.items() if n > COMBO_PREFIX_LIMIT}

    codes = collections.Counter(c for d in detail.values() for c, *_ in d["warnings"])
    affected = {c: sum(1 for d in detail.values() if any(x[0] == c for x in d["warnings"])) for c in codes}
    print(f"共 {len(recipes)} 道菜，{len(detail)} 道有提示")
    names = {"W1": "要用温度计", "W2": "单独的卫生步骤", "W3": "泛称食材", "W4": "模板重复", "W5": "难度/标签不符"}
    for c in sorted(codes):
        print(f"  {c} {names[c]}：{affected[c]} 道菜，{codes[c]} 处")
    print(f"  W6 组合式命名（同一前缀超过 {COMBO_PREFIX_LIMIT} 道）：" + "、".join(f"{p}×{n}" for p, n in sorted(combos.items(), key=lambda x: -x[1])))
    uniq = len(step_count)
    total = sum(step_count.values())
    print(f"步骤总数 {total}，不重复的 {uniq}（{uniq * 100 // total}%）")
    if a.json:
        Path(a.json).write_text(json.dumps({"summary": {"recipes": len(recipes), "flagged": len(detail), "by_code": affected, "combo_prefixes": combos},
                                            "detail": detail}, ensure_ascii=False, indent=1), encoding="utf-8")
    return 0


if __name__ == "__main__":
    sys.exit(main())
