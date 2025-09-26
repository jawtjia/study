#pragma once

#include <cstdint>
#include <vector>
#include <memory>

namespace dsp {

struct PipelineConfig {
  int sampleRateHz = 16000;
  int numChannels = 1;
  int frameDurationMs = 10;  // processing hop size
  bool enableHighPass = true;
  bool enableNoiseSuppressor = true;
  int noiseSuppressStrength = 2; // 1=low,2=med,3=high
};

class HighPassFilter;
class SpectralSubtractNS;

// Simple pass-through placeholder for future AEC integration
class EchoCancellerStub {
public:
  void initialize(int sampleRateHz, int numChannels) {
    (void)sampleRateHz;
    (void)numChannels;
  }
  void process(const float* input, int numSamples, float* output) {
    if (output == nullptr || input == nullptr) return;
    for (int i = 0; i < numSamples; ++i) output[i] = input[i];
  }
};

// High-level pipeline that wires modules together and handles streaming buffers.
class DspPipeline {
public:
  DspPipeline();
  ~DspPipeline();

  void initialize(const PipelineConfig& config);

  // Process int16 mono buffer (interleaved if multi-channel not supported yet)
  // Returns number of samples written to output (same as input length for streaming processing)
  int processBuffer(const int16_t* input, int numSamples, std::vector<int16_t>& output);

  // Flush any internal overlap buffers (e.g., at end of stream)
  void flush(std::vector<int16_t>& output);

  // Float path: mono float [-1,1]
  int processBufferFloat(const float* input, int numSamples, std::vector<float>& output);
  void flushFloat(std::vector<float>& output);

private:
  PipelineConfig cfg_{};
  std::unique_ptr<HighPassFilter> hpf_;
  std::unique_ptr<SpectralSubtractNS> ns_;
  EchoCancellerStub aec_{};

  // streaming temp buffers
  std::vector<float> floatIn_;
  std::vector<float> floatProc_;
  std::vector<float> floatOut_;
};

} // namespace dsp


