# 2026-10-29 完整菜图补全

目标是补齐当前 144 道菜中缺失的 127 张 `hero_<菜谱ID>.png`。原有 17 张完整菜图、菜谱、食材 ID、分层图和定稿规格保持原样。

`manifest.json` 是可恢复的逐菜任务清单，记录稳定 ID、配方哈希、逐菜提示词和真实交付状态。只有 PNG、保留的原图、哈希及 JSON 元数据齐全后才注册进 `ASSET_LIST.csv` 并标 `review`。`review` 不代表用户审美、实厨或实机验收。

生成采用内置 OpenAI imagegen，每道单独生成，参考已交付的 `hero_xilanhua-chaoshanbei.png`，按真实配方主料和做法区分成品。输出必须是真透明 RGBA、4:3、器皿完整、不裁切；不画食物脸、文字、背景或程序特效。不以配菜、复制图或占位图冒充完成。

原始生成图保存在 `../../originals/`。交付仅把整张画布等比缩至 640×480，不重画、不裁切、不去除透明通道。每 12 道输出一页 `review-XX.png`：同一张图在实际 160×120 大小、深色卡和奶油卡上分别显示，并模拟 RGB565。

可用 Python 3 和 Pillow 重建登记/审图（在仓库根目录执行）：

```sh
python game-art/kitchen/dish-hero/batches/2026-10-29/delivery.py ingest hero_<菜谱ID> <生成原图路径>
python game-art/kitchen/dish-hero/batches/2026-10-29/delivery.py sheets
python -m unittest discover -s tools/tests -p test_kitchen_heroes.py
python tools/check_recipes.py
```

每批先检查远端内容分支有没有新增配方、同 ID 交付或规格变化，再继续未完成项；不覆盖其他机器的新图。GitHub 已设置每 30 分钟跟进，主任务活跃时避免重复出图，仅报告实质更新/冲突。

图片完成后仍需重新打包，核对真实资源区容量及 160×120 限色解码结果。当前工牌中的资源未随本批改动，不能将源码美术交付写成刷机完成。
