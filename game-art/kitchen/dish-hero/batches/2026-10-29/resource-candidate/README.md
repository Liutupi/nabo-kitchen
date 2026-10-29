# 144 道完整菜图资源候选

这是离线厨房的资源候选，未刷入工牌，不包含应用、bootloader、分区表、NVS 或设备备份。

- `kitchen_dish.bin`：82 分层 + 144 张完整菜图，1,596,273 B。
- 其余五个 payload 与固件分支 `codex/runtime-fixes-20261029` 的 `c7cdc0b94a6009672333dc1736b4f1c33f0a0537` 中 `assets/kitchen` 逐字节一致。
- `kitchen_bundle.bin`：3,756,062 B，SHA256 `f1c0f215a460b6c4e7c8805527a6077ff2b8473626eb55205a77f9bda26d2931`；实际 5 MiB 资源区剩余 1,486,818 B。
- 每个文件的字节数与 SHA256、确定性打包、生产 C 读取/绘制结果见上级 [runtime-validation.json](../runtime-validation.json)。复现步骤见上级 [README](../README.md)。

仅完成本机工程验证；逐菜视觉、实厨、真实设备显示/启动/内存和长期使用仍待验收。素材及派生字库声明和许可随包保留。

Archivo Black 的许可原文含一处行尾空格，保留原文；本批 `.gitattributes` 仅对该许可文件关闭空白检查。
