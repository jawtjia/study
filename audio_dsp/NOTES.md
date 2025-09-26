# Audio DSP 学习与实践笔记

本笔记汇总实现过程中的关键知识、路线与调参经验，持续更新。

## 学习/实践路线（确保每步有正反馈）
- 桌面快速打样 + Android 早期联调（Oboe/AAudio）混合路线。
- 主要步骤：I/O 骨架 → HPF/DC → 简单谱减 NS（离线）→ 稳定 NS（WebRTC/RNNoise）→ AEC3 → 延迟/漂移 → AGC/限幅 → 去混响（WPE，可选）→ 设备适配参数表。

## 核心算法与实现要点
- STFT/ISTFT：采用 Radix-2 FFT，sqrt-Hann 窗 + 50% overlap + OLA，避免重建失真。
- 简单谱减：静音段噪声谱跟踪 + 过减因子 overSubtraction + 增益地板 gainFloor。
- 参数建议（16 kHz）：windowSize=512、hop=256、overSubtraction=0.8、gainFloor=0.2、snrVadThresholdDb=2~3。
- 性能：FFT 替代朴素 DFT；预计算窗；大文件显示进度；必要时用优化 FFT 库或 SIMD。

## AEC/AGC/去混响（规划）
- AEC：优先 WebRTC AEC3；关键为远端参考、延迟与时钟漂移。指标：ERLE、双讲 DTD。
- AGC：WebRTC AGC2；目标电平 -18 dBFS，尾端硬限幅 -1 dBFS。
- 去混响：WPE（先离线再实时）。

## Android 适配要点
- 使用 Oboe/AAudio 低延迟模式；禁用系统内建 AEC/NS/AGC 防叠处理。
- AEC 参考从渲染路径获取；记录 ERLE、延迟估计、增益轨迹；A/B 切换与 WAV dump。

## 常见问题与排查
- 输出“嘟嘟/刺耳/语音弱”：多因窗/重叠/OLA 不匹配或过激谱减；改用 sqrt-Hann + 50% overlap，并温和参数。
- 处理卡顿：算法使用朴素 DFT；改为 FFT 后显著提速。
- AEC 无效：渲染/采集喂反或延迟未校准；检查参考通路与尾长。

## 后续计划
- 增加 ERLE/延迟粗估与 CSV 输出（离线评测）。
- 文档化 WebRTC AEC3 与 RNNoise 集成方法。
- 增加 CMakePresets 与跨平台构建指引。

---
更新记录：
- 2025-09-22 初始化笔记，整理学习路径、实现要点、排查建议。
