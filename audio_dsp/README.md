# AudioDSPPractice

跨平台离线语音处理链（C++/CMake）：高通/DC 抑制 + 频域降噪（谱减）+ 预留 AEC/RNNoise 接口。支持用 Android 上 dump 的 PCM/WAV 在 PC 上快速验证处理效果并输出指标。

## 目录结构

```
audio_dsp/
  CMakeLists.txt
  src/
    dsp_core/          # 核心算法（HPF、NS、FFT/STFT）
    common/            # WAV 读写
    offline_processor/ # 离线命令行工具
```

## 构建（Windows / MinGW）

```
cd C:\work\Code\study\audio_dsp
mkdir build && cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++
mingw32-make -j
```

（VS 生成器或 Ninja 构建可参考对话记录，或按需补充。）

## 使用

- 不处理噪声（旁路，WAV 输入）：
```
.\offline_processor.exe -i input.wav -o out_no_ns.wav --no-ns
```
- 开启降噪（WAV 输入）：
```
.\offline_processor.exe -i input.wav -o out_ns.wav
```

- 原始 PCM 输入（支持 s16le/s24le/f32le，多通道自动 downmix 到 mono）：
```
.\offline_processor.exe --raw-in mic.pcm -f s24le -sr 48000 -ac 2 -o out_ns.wav
```

### 输出格式保持/可选覆盖
- 默认：保持与输入相同的采样率、通道数与位宽（位宽不再默认改为 16-bit）。
- 可选覆盖：
```
--out-sr <rate> --out-ac <channels> --out-bits {16|24|32|f32}
```
示例：保留 48k/2ch 并写 24-bit：
```
.\offline_processor.exe -i input.wav --out-bits 24 --out-ac 2 --out-sr 48000 -o out.wav
```
示例：输入为 32-bit PCM，默认跟随位宽写 32-bit；若需显式：
```
.\offline_processor.exe -i input_32bit.wav --out-bits 32 -o out.wav
```

### 诊断与长度对齐
- 详细信息：
```
-v  # 打印输入/输出帧数与data区字节估计
```
- 长度对齐（样本数与输入一致）：
```
--keep-length  # NS 尾部补零或裁剪，使输出帧数=输入帧数
```

## 实现细节与音质

- 全链路浮点：读入后统一转换为 float 交织缓冲（-1..1），处理链（HPF/NS/FFT）均在浮点域进行，避免中途量化到 int16 造成的精度损失。
- 多通道策略（当前）：为兼容单通道 NS，先 downmix 成单声道做降噪，再将结果复制回原通道数输出；采样率保持不变。若需“每通道独立 NS”，可后续开启对应开关。
- 位宽：输出位宽默认跟随输入，若需要可用 `--out-bits {16|24|32|f32}` 指定。

程序会显示处理进度，并在结束时打印输入/输出 RMS & 峰值（dBFS）。

## 将 Android dump 的 PCM 转为 WAV

若 dump 得到的是裸 `PCM`（int16 mono 16k/48k）：
```
ffmpeg -f s16le -ar 16000 -ac 1 -i mic.pcm mic.wav
ffmpeg -f s16le -ar 16000 -ac 1 -i spk_ref.pcm spk_ref.wav
```

## 参数与扩展

- 降噪参数位于 `dsp_core/noise_suppressor.h::NsConfig`：
  - `windowSize`: 建议 16k 取 512；自动 50% 重叠（hop=window/2）
  - `overSubtraction`: 0.6–1.0（先试 0.8）
  - `gainFloor`: 0.15–0.25（先试 0.2）
  - `snrVadThresholdDb`: 0–6（车内场景先试 2–3）
- 已从朴素 DFT 改为 Radix-2 FFT，使用 sqrt-Hann 窗 + 50% overlap + OLA 合成，避免重建失真。
- 预留 AEC 与 RNNoise 接口，后续可集成 WebRTC AEC3/RNNoise。

## 注意事项

- 输入 WAV 需为 16-bit PCM，当前实现仅支持单声道。
- 若处理结果听感异常，优先检查：
  - 是否禁用了 NS（`--no-ns`）用于对照
  - `windowSize/hop` 与窗函数是否默认（512/256 + sqrt-Hann）
  - 降噪参数是否过激（`overSubtraction`、`gainFloor`）
- 大文件会显示进度；性能瓶颈在 FFT 与频域处理，可进一步采用优化 FFT 库或 SIMD。
