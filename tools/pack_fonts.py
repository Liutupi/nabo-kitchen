"""Build KFN1 and a licensed offline UI font from pinned source fonts."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import string
import _bootstrap
from PIL import Image, ImageDraw, ImageFont, features
from fontTools import subset
from fontTools.ttLib import TTFont
from font_codec import Glyph, coverage_bytes, pack_fonts, unpack_fonts
from check_recipes import ROOT, load_recipes
from recipe_codec import canonical

FONT_BUDGET = 380000
ASCII = set(chr(n) for n in range(32, 127))


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def strings(value):
    if isinstance(value, str):
        yield value
    elif isinstance(value, list):
        for item in value:
            yield from strings(item)
    elif isinstance(value, dict):
        for key, item in value.items():
            if not key.startswith("_"):
                yield from strings(item)


def repertoires(root, recipes):
    chars_file = root / "tools/fonts/level1_3500.txt"
    level1 = set(chars_file.read_text(encoding="utf-8")) - set("\r\n")
    provenance = json.loads(chars_file.with_suffix(".source.json").read_text(encoding="utf-8"))
    if len(level1) != 3500 or sha(chars_file) != provenance["output_sha256"]:
        raise ValueError("3500-character source provenance mismatch")
    recipe_chars = set("".join(canonical(r).decode("utf-8") for r in recipes))
    labels = [r["name"] for r in recipes]
    for path in ["data/ingredients.json", "data/seasonings.json", "data/lines.json"]:
        labels += list(strings(json.loads((root / path).read_text(encoding="utf-8"))))
    ui_source = "".join(p.read_text(encoding="utf-8") for pattern in ["*.mjs", "*.html"]
                        for p in (root / "prototype").glob(pattern))
    ui_chars = ASCII | set("".join(labels)) | set("↑↓←→℃°·×—…≥≤剩余")
    ui_chars |= {ch for ch in ui_source if 0x3000 <= ord(ch) <= 0x9fff or ch in "↑↓←→℃°·×—…≥≤"}
    body = level1 | recipe_chars | ui_chars
    if any(ord(c) < 32 for c in body):
        raise ValueError("control character in font repertoire")
    return body, ui_chars, recipe_chars, provenance


def render_glyph(font, char, bpp):
    x0, y0, x1, y1 = font.getbbox(char, anchor="ls")
    w, h = x1 - x0, y1 - y0
    advance = round(font.getlength(char))
    if w <= 0 or h <= 0:
        return Glyph(ord(char), 0, 0, 0, 0, advance, b"")
    image = Image.new("L", (w, h))
    ImageDraw.Draw(image).text((-x0, -y0), char, font=font, fill=255, anchor="ls")
    maximum = (1 << bpp) - 1
    values = [(p * maximum + 127) // 255 for p in image.tobytes()]
    return Glyph(ord(char), w, h, x0, y0, advance, coverage_bytes(values, bpp))


def bitmap_font(path, ident, px, chars, bpp):
    font = ImageFont.truetype(str(path), px)
    ascent, descent = font.getmetrics()
    return dict(id=ident, px=px, bpp=bpp, ascent=ascent, descent=descent,
                line_height=ascent + descent,
                glyphs=[render_glyph(font, ch, bpp) for ch in sorted(chars)])


def source_check(path, chars):
    font = TTFont(str(path), recalcTimestamp=False)
    supported = font.getBestCmap()
    missing = sorted(ch for ch in chars if ord(ch) not in supported)
    font.close()
    if missing:
        raise ValueError("source font lacks: " + "".join(missing))
    return supported


def browser_subset(source, chars, target):
    font = TTFont(str(source), recalcTimestamp=False)
    options = subset.Options()
    options.name_IDs = [0, 1, 2, 3, 4, 5, 6, 13, 14, 16, 17]
    options.name_languages = ["*"]
    worker = subset.Subsetter(options=options)
    worker.populate(unicodes={ord(ch) for ch in chars})
    worker.subset(font)
    replacements = {1: "KitchenHan", 2: "Medium", 3: "KitchenHan Medium subset 20261028",
                    4: "KitchenHan Medium", 6: "KitchenHan-Medium", 16: "KitchenHan", 17: "Medium"}
    for record in list(font["name"].names):
        if record.nameID in replacements:
            font["name"].setName(replacements[record.nameID], record.nameID,
                                 record.platformID, record.platEncID, record.langID)
    font.flavor = "woff2"
    font.recalcTimestamp = False
    target.parent.mkdir(parents=True, exist_ok=True)
    font.save(str(target))
    font.close()


def proof(pack, target):
    result = Image.new("RGB", (800, 400))
    draw = ImageDraw.Draw(result)
    caption = ImageFont.truetype("C:/Windows/Fonts/arial.ttf", 12)
    sample = "番茄炒蛋 鸡肉74℃ 鱼肉63℃ 剩余1:30"
    for side, (bg, ink) in enumerate([("#2C2845", "#FBF1E3"), ("#FBF1E3", "#15122A")]):
        x0 = side * 400
        draw.rectangle((x0, 0, x0 + 399, 400), fill=bg)
        for row, name in enumerate(["han12", "han15", "han16", "han18", "han20"]):
            x, baseline = x0 + 12, 48 + row * 56
            for ch in sample:
                glyph = pack.glyph(name, ord(ch))
                if glyph.width and glyph.height:
                    result.paste(ink, (x + glyph.x, baseline + glyph.y,
                                      x + glyph.x + glyph.width, baseline + glyph.y + glyph.height),
                                 glyph.image(pack.fonts[name]["bpp"]))
                x += glyph.advance
            draw.text((x0 + 12, baseline + 8), name, fill=ink, font=caption)
        x, baseline = x0 + 16, 368
        for ch in "07:12":
            glyph = pack.glyph("latin40", ord(ch))
            if glyph.width and glyph.height:
                result.paste(ink, (x + glyph.x, baseline + glyph.y,
                                  x + glyph.x + glyph.width, baseline + glyph.y + glyph.height), glyph.image(2))
            x += glyph.advance
    result.save(target)


def build(root=ROOT, out=None, browser_out=None):
    root = Path(root)
    out = Path(out) if out else root / "build/kitchen"
    browser_out = Path(browser_out) if browser_out else root / "prototype/assets/kitchen-han.woff2"
    sources = json.loads((root / "tools/fonts/sources.json").read_text(encoding="utf-8"))
    cache = root / "private/font-source"
    for entry in sources["records"]:
        if entry["name"] == "standard-gf0023-2020.pdf":
            continue
        p = cache / entry["name"]
        if not p.is_file() or sha(p) != entry["sha256"]:
            raise ValueError("source missing/checksum mismatch; run python tools/fetch_fonts.py: " + entry["name"])
    han = cache / "SourceHanSansSC-Medium.otf"
    latin_woff = root / "game-art/kitchen/reference/ui-mockup/archivo-black-latin-400-normal.woff2"
    latin = cache / "ArchivoBlack-Regular.ttf"
    converted = TTFont(str(latin_woff), recalcTimestamp=False)
    converted.flavor = None
    converted.save(str(latin))
    converted.close()
    recipes = load_recipes(root)
    body_chars, title_chars, recipe_chars, provenance = repertoires(root, recipes)
    source_check(han, body_chars)
    source_check(latin, ASCII)
    fonts = []
    for size in [12, 15, 16, 18, 20]:
        chars = body_chars if size in (12, 16, 18) else title_chars
        fonts.append(bitmap_font(han, "han" + str(size), size, chars, 2))
    for size in [12, 16, 22, 34, 40]:
        fonts.append(bitmap_font(latin, "latin" + str(size), size, ASCII, 2))
    blob = pack_fonts(fonts)
    decoded = unpack_fonts(blob)
    for font in fonts:
        for glyph in font["glyphs"]:
            if decoded.glyph(font["id"], glyph.cp) != glyph:
                raise ValueError("font glyph/metrics roundtrip mismatch")
    out.mkdir(parents=True, exist_ok=True)
    target = out / "kitchen_fonts.bin"
    target.write_bytes(blob)
    chars_path = out / "kitchen_fonts.chars.txt"
    chars_path.write_text("".join(sorted(body_chars)), encoding="utf-8")
    if recipe_chars - body_chars:
        raise ValueError("missing current recipe glyph")
    browser_subset(han, body_chars, browser_out)
    for name in ["OFL.SourceHanSans.txt", "OFL.ArchivoBlack.txt"]:
        shutil.copy2(cache / name, out / name)
        license_target = root / "tools/fonts" / name
        if not license_target.exists():
            shutil.copy2(cache / name, license_target)
    shutil.copy2(cache / "OFL.SourceHanSans.txt", browser_out.parent / "OFL.txt")
    notice = ("KitchenHan is a renamed subset of Source Han Sans SC Medium 2.005, "
              "copyright 2014-2025 Adobe. SIL Open Font License 1.1, see OFL.txt. "
              "No original Reserved Font Name is used as the subset family name.\n")
    (browser_out.parent / "FONT-NOTICE.txt").write_text(notice, encoding="utf-8")
    browser_meta = browser_out.parent / "sources.json"
    metadata = json.loads(browser_meta.read_text(encoding="utf-8")) if browser_meta.exists() else {}
    metadata["font"] = {"source_name": "SourceHanSansSC-Medium.otf",
                        "source_sha256": sha(han), "source_commit": sources["source_han_commit"],
                        "output_sha256": sha(browser_out), "characters": len(body_chars),
                        "family": "KitchenHan", "processing": "Unicode subset, reserved family renamed, WOFF2; official pinned SC Medium source"}
    browser_meta.write_text(json.dumps(metadata, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    proof(decoded, out / "fonts-proof.png")
    report = {"format": "KFN1", "packages": {target.name: {"bytes": len(blob), "budget_bytes": FONT_BUDGET,
               "within_budget": len(blob) <= FONT_BUDGET, "sha256": sha(target)}},
              "roundtrip": True, "glyphs": sum(len(f["glyphs"]) for f in fonts),
              "font_profiles": [{"id": f["id"], "px": f["px"], "bpp": f["bpp"], "glyphs": len(f["glyphs"])} for f in fonts],
              "source_fonts": {"han_sha256": sha(han), "archivo_woff_sha256": sha(latin_woff)},
              "level1": provenance, "body_characters": len(body_chars), "ui_characters": len(title_chars),
              "recipe_glyphs_missing": [], "browser_subset_bytes": browser_out.stat().st_size,
              "pillow": Image.__version__, "freetype": features.version("freetype2"),
              "proof": "fonts-proof.png", "scope": "KFN glyph masks/metrics roundtrip; no C/device rendering or kitchen acceptance"}
    (out / "fonts-report.json").write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=ROOT)
    parser.add_argument("--out", type=Path)
    parser.add_argument("--browser-out", type=Path)
    args = parser.parse_args()
    try:
        report = build(args.root, args.out, args.browser_out)
        print(json.dumps({k: report[k] for k in ["packages", "glyphs", "body_characters", "browser_subset_bytes", "roundtrip"]}))
    except (ValueError, OSError) as error:
        parser.exit(1, str(error) + "\n")


if __name__ == "__main__":
    main()
