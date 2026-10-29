# 2026-10-29 工牌画面采集

以下是新资源刷入后，在 TEST 隔离模式取得的完整 240×320、320 行 USB 画面。仅保留完整且页面正确的采集；不是模拟器截图，也没有补画缺行。菜图区与未改生产 C 绘制共比对 100,800 个 RGB565 像素，差异 0，详见 [像素报告](capture-validation.json) 和上级 [设备报告](../device-validation.json)。

| 画面 | 菜谱 | 菜库索引 |
|---|---|---:|
| [capture-01.png](capture-01.png) | 蒸水蛋 | 143 |
| [capture-02.png](capture-02.png) | 玉米排骨汤 | 142 |
| [capture-03.png](capture-03.png) | 白菜炖豆腐 | 1 |
| [capture-04.png](capture-04.png) | 番茄炒蛋 | 27 |
| [capture-05.png](capture-05.png) | 青椒蛏子汤 | 81 |
| [capture-06.png](capture-06.png) | 清蒸鲈鱼 | 86 |

已退出 TEST、恢复原状态并关闭串口。截图证明设备绘制缓冲区内容；实际屏幕观感和扬声器听感仍待用户确认。鸡蛋炒饭的完整 USB 截图未取得，不能把它或其余全部 144 道写成逐图实机验收通过。

在内容仓库根目录，按上级 README 设置本机 `CC`、`CJSON_DIR` 和固件目录后，可离线复核所有已保留 PNG 的哈希及菜图区像素，不会访问设备：

```sh
python game-art/kitchen/dish-hero/batches/2026-10-29/verify_device_captures.py --firmware-root ../nabo-kitchen-runtime
```
