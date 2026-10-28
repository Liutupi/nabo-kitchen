# KFN1 字库候选实现与重建

这是供后续 C 解码器复核的宿主机格式，尚未做 C3 渲染、内存和速度验证。它不修改定稿架构。当前字体包超出 380,000 字节分项预算；保留实际产物供继续优化，不视为预算验收通过。

## 重建

```powershell
python -m pip install --target private/t1/vendor -r tools/requirements-fonts.txt
python tools/fetch_fonts.py
python tools/pack_fonts.py
python -m unittest discover -s tools/tests -p test_kitchen_fonts.py -v
python tools/pack_recipes.py pack --font-chars build/kitchen/kitchen_fonts.chars.txt
```

`sources.json` 固定下载 URL、大小和 SHA-256。构建使用官方 SC Medium 源字体及仓库已有 Archivo Black；不接受校验不符的缓存。51 MB 官方字表 PDF 仅供追溯，可加 `--include-standard` 下载，普通构建使用已核验的 `tools/fonts/level1_3500.txt`。字体许可在同目录；修改后的浏览器子集族名为 KitchenHan，保留 OFL 声明。完整源字体、缓存和生成目录留在忽略目录。

汉字 12/16/18 px 覆盖一级字 3500 加当前菜谱、界面和 ASCII；15/20 px 覆盖界面与菜名。Archivo Black 为 12/16/22/34/40 px ASCII。全部保存 2bpp 覆盖率、整数字形度量；体积含重复字符在各字号的独立索引。浏览器 WOFF2 单独交付，不计入固件包。

## 布局

小端。头部 `<4sHH7I>` 共 36 字节：magic KFN1、版本 1、字形条目大小 16、字体数、字形总数、字体目录偏移、字形目录偏移、数据偏移、文件总长度、头部之后的 CRC32。

字体目录 `<16sHHIIhhHH>` 共 36 字节：ASCII ID、字号、bpp、字形起始索引、数量、ascent、descent、行高、reserved=0。字形目录 `<IBBbbBBIH>` 共 16 字节：Unicode scalar、宽、高、基线 x/y 有符号偏移、advance、codec、绝对数据偏移、压缩长度。每个字体内码点严格递增。

覆盖率按 MSB 顺序连续存 1/2bpp，末尾填充位必须为零。codec=0 原始，codec=1 使用与 KSP1 相同的有界 PackBits；编码器仅在更短时使用 RLE，相同载荷复用偏移。空白字可无位图但保留 advance。参考解码器检查版本、长度、CRC、目录边界、码点、尺寸、bpp、填充位和解压长度。解码后字形覆盖率与度量逐项等于限灰度后的输入；不声称还原源矢量轮廓。

输出为 `build/kitchen/kitchen_fonts.bin`、字符清单、`fonts-report.json` 与双底色 `fonts-proof.png`。冻结快照另见根目录 `checkpoints/2026-10-28/`。
