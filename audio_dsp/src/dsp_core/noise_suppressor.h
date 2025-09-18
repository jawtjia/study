#pragma once

#include <vector>
#include <complex>
#include <cstdint>

#include "dsp_core/stft.h"

namespace dsp {

struct NsConfig {
  int sampleRateHz = 16000;
  int windowSize = 512;     // next power of two >= 25 ms
  int hopSize = 256;        // default = windowSize/2; will be set in initialize
  float noiseUpdateAlpha = 0.95f; // noise spectrum smoothing
  float gainFloor = 0.10f;        // spectral floor
  float overSubtraction = 1.0f;   // subtraction factor
  float snrVadThresholdDb = 3.0f; // VAD threshold
};

class SpectralSubtractNS {
public:
  void initialize(const NsConfig& cfg);

  // Feed float mono samples; produce processed samples (same count)
  void process(const float* in, int numSamples, std::vector<float>& out);

  void flush(std::vector<float>& tail);

private:
  NsConfig cfg_{};
  STFT stft_{}; // reserved, not used directly now
  std::vector<float> gainSpectral_;
  std::vector<float> noiseMag_;
  std::vector<float> hann_;
  std::vector<float> ola_;
  std::vector<float> inFifo_;

  bool isSpeechFrame(const std::vector<std::complex<float>>& frameSpec,
                     const std::vector<float>& noiseMag) const;
};

} // namespace dsp


