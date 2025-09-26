#include "dsp_core/dsp_core.h"
#include "dsp_core/hpf.h"
#include "dsp_core/noise_suppressor.h"
#include <algorithm>

namespace dsp {

DspPipeline::DspPipeline() = default;
DspPipeline::~DspPipeline() = default;

void DspPipeline::initialize(const PipelineConfig& config) {
  cfg_ = config;
  if (!hpf_) hpf_ = std::make_unique<HighPassFilter>();
  if (!ns_) ns_ = std::make_unique<SpectralSubtractNS>();

  hpf_->initialize(cfg_.sampleRateHz);

  NsConfig ncfg;
  ncfg.sampleRateHz = cfg_.sampleRateHz;
  int win = static_cast<int>(0.025f * cfg_.sampleRateHz);
  int pow2 = 1; while (pow2 < win) pow2 <<= 1; ncfg.windowSize = pow2;
  ncfg.hopSize = cfg_.sampleRateHz / 100; // 10 ms hop
  ns_->initialize(ncfg);

  aec_.initialize(cfg_.sampleRateHz, cfg_.numChannels);
}

int DspPipeline::processBuffer(const int16_t* input, int numSamples, std::vector<int16_t>& output) {
  floatIn_.resize(numSamples);
  for (int i = 0; i < numSamples; ++i) floatIn_[i] = static_cast<float>(input[i]) / 32768.0f;

  floatProc_ = floatIn_;

  if (cfg_.enableHighPass) {
    floatOut_.resize(numSamples);
    hpf_->processBuffer(floatProc_.data(), numSamples, floatOut_.data());
    floatProc_.swap(floatOut_);
  }

  if (cfg_.enableNoiseSuppressor) {
    std::vector<float> nsOut;
    ns_->process(floatProc_.data(), numSamples, nsOut);
    floatProc_.swap(nsOut);
  }

  output.resize(static_cast<size_t>(floatProc_.size()));
  for (size_t i = 0; i < floatProc_.size(); ++i) {
    float v = std::max(-1.0f, std::min(1.0f, floatProc_[i]));
    output[i] = static_cast<int16_t>(std::round(v * 32767.0f));
  }
  return static_cast<int>(output.size());
}

void DspPipeline::flush(std::vector<int16_t>& output) {
  std::vector<float> tail;
  if (ns_) ns_->flush(tail);
  output.resize(tail.size());
  for (size_t i = 0; i < tail.size(); ++i) {
    float v = std::max(-1.0f, std::min(1.0f, tail[i]));
    output[i] = static_cast<int16_t>(std::round(v * 32767.0f));
  }
}

int DspPipeline::processBufferFloat(const float* input, int numSamples, std::vector<float>& output) {
  floatProc_.assign(input, input + numSamples);

  if (cfg_.enableHighPass) {
    floatOut_.resize(numSamples);
    hpf_->processBuffer(floatProc_.data(), numSamples, floatOut_.data());
    floatProc_.swap(floatOut_);
  }
  if (cfg_.enableNoiseSuppressor) {
    std::vector<float> nsOut;
    ns_->process(floatProc_.data(), numSamples, nsOut);
    floatProc_.swap(nsOut);
  }
  output = floatProc_;
  return static_cast<int>(output.size());
}

void DspPipeline::flushFloat(std::vector<float>& output) {
  std::vector<float> tail;
  if (ns_) ns_->flush(tail);
  output = tail;
}

} // namespace dsp


