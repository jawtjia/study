#include "dsp_core/noise_suppressor.h"
#include <algorithm>
#include <cmath>

namespace dsp {

static inline float dbToLin(float db) { return std::pow(10.0f, db / 20.0f); }

void SpectralSubtractNS::initialize(const NsConfig& cfg) {
  cfg_ = cfg;
  if (cfg_.hopSize <= 0 || cfg_.hopSize > cfg_.windowSize) cfg_.hopSize = cfg_.windowSize / 2; // 50% overlap
  stft_.initialize(cfg_.sampleRateHz, cfg_.windowSize, cfg_.hopSize);
  gainSpectral_.assign(cfg_.windowSize, 1.0f);
  noiseMag_.assign(cfg_.windowSize, 0.0f);
  // use sqrt-Hann for PR with 50% overlap
  hann_.assign(cfg_.windowSize, 0.0f);
  for (int n = 0; n < cfg_.windowSize; ++n) {
    float h = 0.5f * (1.0f - std::cos(2.0f * 3.14159265358979323846f * n / (cfg_.windowSize - 1)));
    hann_[n] = std::sqrt(h);
  }
  ola_.assign(cfg_.windowSize, 0.0f);
  inFifo_.clear();
}

bool SpectralSubtractNS::isSpeechFrame(const std::vector<std::complex<float>>& X,
                                       const std::vector<float>& N) const {
  float num = 0.0f, den = 1e-6f;
  const int K = static_cast<int>(X.size());
  for (int k = 0; k < K / 2; ++k) {
    float mag = std::abs(X[k]);
    num += mag * mag;
    den += N[k] * N[k];
  }
  float snrLin = num / den;
  float snrDb = 10.0f * std::log10(std::max(snrLin, 1e-6f));
  return snrDb > cfg_.snrVadThresholdDb;
}

void SpectralSubtractNS::process(const float* in, int numSamples, std::vector<float>& out) {
  out.clear();
  int idx = 0;
  std::vector<std::complex<float>> X;
  while (idx < numSamples) {
    int toProc = std::min(cfg_.hopSize, numSamples - idx);
    inFifo_.insert(inFifo_.end(), in + idx, in + idx + toProc);
    idx += toProc;

    while (static_cast<int>(inFifo_.size()) >= cfg_.windowSize) {
      std::vector<float> frame(inFifo_.begin(), inFifo_.begin() + cfg_.windowSize);
      // analysis window
      for (int n = 0; n < cfg_.windowSize; ++n) frame[n] *= hann_[n];

      X.resize(cfg_.windowSize);
      std::vector<std::complex<float>> xComplex(cfg_.windowSize);
      for (int n = 0; n < cfg_.windowSize; ++n) xComplex[n] = {frame[n], 0.0f};
      X = xComplex;
      fft(X, false);

      bool speech = isSpeechFrame(X, noiseMag_);
      for (int k = 0; k < cfg_.windowSize; ++k) {
        float mag = std::abs(X[k]);
        if (!speech) noiseMag_[k] = cfg_.noiseUpdateAlpha * noiseMag_[k] + (1.0f - cfg_.noiseUpdateAlpha) * mag;
        else noiseMag_[k] = 0.995f * noiseMag_[k] + 0.005f * mag;
      }

      for (int k = 0; k < cfg_.windowSize; ++k) {
        float mag = std::abs(X[k]);
        float est = std::max(mag - cfg_.overSubtraction * noiseMag_[k], 0.0f);
        float g = (mag > 1e-6f) ? (est / mag) : 1.0f;
        gainSpectral_[k] = std::max(g, cfg_.gainFloor);
      }

      for (int k = 0; k < cfg_.windowSize; ++k) X[k] *= gainSpectral_[k];
      std::vector<std::complex<float>> xTime = X;
      fft(xTime, true);
      // synthesis window + OLA
      for (int n = 0; n < cfg_.windowSize; ++n) {
        float y = xTime[n].real() * hann_[n];
        ola_[n] += y;
      }
      // emit hop
      for (int n = 0; n < cfg_.hopSize; ++n) out.push_back(ola_[n]);
      // shift OLA left by hop
      for (int n = 0; n < cfg_.windowSize - cfg_.hopSize; ++n) ola_[n] = ola_[n + cfg_.hopSize];
      for (int n = cfg_.windowSize - cfg_.hopSize; n < cfg_.windowSize; ++n) ola_[n] = 0.0f;
      // consume input hop
      inFifo_.erase(inFifo_.begin(), inFifo_.begin() + cfg_.hopSize);
    }
  }
}

void SpectralSubtractNS::flush(std::vector<float>& tail) {
  tail.clear();
  // flush remaining OLA head up to hopSize
  int remain = std::min<int>(cfg_.hopSize, cfg_.windowSize);
  for (int n = 0; n < remain; ++n) tail.push_back(ola_[n]);
  std::fill(ola_.begin(), ola_.end(), 0.0f);
  inFifo_.clear();
}

}


