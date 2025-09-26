#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace wavio {

enum class SampleFormat { S16, S24, S32, F32 };

struct AudioBuffer {
  int sampleRateHz = 16000;
  int numChannels = 1;                 // 原始通道数
  SampleFormat format = SampleFormat::S16; // 输入/目标位宽（写出时可覆盖）
  std::vector<float> samples;          // 交织 float [-1,1]
};

struct WavData {
  int sampleRateHz = 16000;
  int numChannels = 1; // decoded为mono后，此处返回1
  std::vector<int16_t> samples; // 始终返回mono int16 序列
};

bool readWav(const std::string& path, WavData& out);
bool writeWav(const std::string& path, const WavData& in);

// 读取原始PCM并转换为 mono int16
// format: "s16le" | "s24le" | "f32le"
bool readRawPcm(const std::string& path, const char* format, int sampleRate, int channels, WavData& out);

// 新：保持通道并转为 float 交织
bool readWavFlex(const std::string& path, AudioBuffer& out);
bool writeWavFlex(const std::string& path, const AudioBuffer& in, SampleFormat outFmt, int outChannels, int outSampleRate);
bool readRawPcmFlex(const std::string& path, const char* format, int sampleRate, int channels, AudioBuffer& out);

} // namespace wavio


