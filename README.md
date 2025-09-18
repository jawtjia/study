# AudioDSPPractice

最小可用的离线语音处理链（C++/CMake）：高通/DC 抑制 + 简单谱减降噪 + 预留 AEC/RNNoise 接口。可用你在 Android 上 dump 的 PCM/WAV 在 PC 上快速验证处理效果并输出 RMS 指标。

## 目录结构

```
src/
  dsp_core/        # 核心算法
  common/          # WAV 读写
  offline_processor/ # 离线命令行
```

## 构建（Windows / Linux / macOS）

```
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

生成可执行程序：`offline_processor`

## 使用

```
offline_processor -i input.wav -o output.wav
```

要求 `input.wav` 为 16-bit PCM mono（采样率自动读取）。程序会：
- 以 10 ms 帧处理，高通 + 谱减降噪
- 输出 `output.wav`
- 控制台打印输入/输出 RMS & 峰值（dBFS）

## 将 Android dump 的 PCM 转为 WAV

若你 dump 得到的是裸 `PCM`（int16 mono 16k 或 48k），可用 `ffmpeg`：

```
ffmpeg -f s16le -ar 16000 -ac 1 -i mic.pcm mic.wav
ffmpeg -f s16le -ar 16000 -ac 1 -i spk_ref.pcm spk_ref.wav
```

然后先对 `mic.wav` 跑降噪：

```
offline_processor -i mic.wav -o mic_ns.wav
```

后续会提供双通道离线 AEC 工具：输入 `mic.wav` + `spk_ref.wav`，计算 ERLE 与残余回声。

## 参数与扩展

- `dsp_core::PipelineConfig` 可控制是否启用高通/降噪、帧长等。
- `dsp_core` 已预留 AEC/RNNoise 接口，后续将加入 WebRTC AEC3/RNNoise 可选集成。

## 已知限制

- 当前降噪为简单谱减，存在“音乐伪影”；作为入门与可视化基线。建议后续替换为 WebRTC NS 或 RNNoise。
- 仅支持单声道处理；后续扩展多声道与 AEC 参考口。

# study
Directory "TestApp"
learn C++, Java, Kotlin, Python in Android platform.

Directory "StudyTest"
learn C++, Python by VSCode
