#pragma once

#include <vector>
#include <complex>
#include <cmath>
#include "dsp_core/fft.h"

namespace dsp {

// Use FFT from fft.h

class STFT {
public:
  void initialize(int sampleRateHz, int windowSize, int hopSize) {
    (void)sampleRateHz;
    winSize_ = windowSize;
    hopSize_ = hopSize;
    window_.assign(winSize_, 0.0f);
    for (int n = 0; n < winSize_; ++n) {
      window_[n] = 0.5f * (1.0f - std::cos(2.0f * 3.14159265358979323846f * n / (winSize_ - 1)));
    }
    inputBuffer_.assign(winSize_, 0.0f);
    olaBuffer_.assign(winSize_, 0.0f);
    inBuffered_ = 0;
  }

  // Feed samples; when enough for a frame, output time-domain samples via OLA
  void process(const float* in, int numSamples, std::vector<float>& out,
               const std::vector<float>& gainSpectral /*length winSize_*/,
               bool applyGain) {
    out.clear();
    for (int i = 0; i < numSamples; ++i) {
      // shift left when buffer is full for next hop
      if (inBuffered_ < winSize_) {
        inputBuffer_[inBuffered_++] = in[i];
      }
      if (inBuffered_ == winSize_) {
        // analysis
        frameTime_.resize(winSize_);
        for (int n = 0; n < winSize_; ++n) frameTime_[n] = inputBuffer_[n] * window_[n];
        frameFreq_.resize(winSize_);
        // pack to complex
        tempComplex_.resize(winSize_);
        for (int n = 0; n < winSize_; ++n) tempComplex_[n] = {frameTime_[n], 0.0f};
        // in-place FFT
        frameFreq_ = tempComplex_;
        fft(frameFreq_, false);

        if (applyGain && static_cast<int>(gainSpectral.size()) == winSize_) {
          for (int k = 0; k < winSize_; ++k) frameFreq_[k] *= gainSpectral[k];
        }

        // synthesis
        tempComplex_ = frameFreq_;
        fft(tempComplex_, true);
        for (int n = 0; n < winSize_; ++n) frameTime_[n] = tempComplex_[n].real() * window_[n];

        // OLA write hop segment
        if (static_cast<int>(out.size()) < hopSize_) out.resize(hopSize_, 0.0f);
        for (int n = 0; n < hopSize_; ++n) {
          float sample = olaBuffer_[n] + frameTime_[n];
          out[n] = sample;
        }

        for (int n = 0; n < winSize_ - hopSize_; ++n) {
          olaBuffer_[n] = frameTime_[n + hopSize_];
        }
        for (int n = winSize_ - hopSize_; n < winSize_; ++n) {
          olaBuffer_[n] = 0.0f;
        }

        for (int n = 0; n < winSize_ - hopSize_; ++n) {
          inputBuffer_[n] = inputBuffer_[n + hopSize_];
        }
        inBuffered_ = winSize_ - hopSize_;
      }
    }
  }

  void flush(std::vector<float>& tail) {
    tail.clear();
    if (inBuffered_ == 0) return;
    for (int n = inBuffered_; n < winSize_; ++n) inputBuffer_[n] = 0.0f;
    frameTime_.resize(winSize_);
    for (int n = 0; n < winSize_; ++n) frameTime_[n] = inputBuffer_[n] * window_[n];
    tempComplex_.resize(winSize_);
    for (int n = 0; n < winSize_; ++n) tempComplex_[n] = {frameTime_[n], 0.0f};
    // FFT/iFFT for the final partial frame
    frameFreq_ = tempComplex_;
    fft(frameFreq_, false);
    tempComplex_ = frameFreq_;
    fft(tempComplex_, true);
    for (int n = 0; n < winSize_; ++n) frameTime_[n] = tempComplex_[n].real() * window_[n];
    tail.resize(inBuffered_);
    for (int n = 0; n < static_cast<int>(tail.size()); ++n) {
      float sample = (n < static_cast<int>(olaBuffer_.size()) ? olaBuffer_[n] : 0.0f) + frameTime_[n];
      tail[n] = sample;
    }
    inBuffered_ = 0;
  }

  int windowSize() const { return winSize_; }
  int hopSize() const { return hopSize_; }

private:
  int winSize_ = 0;
  int hopSize_ = 0;
  int inBuffered_ = 0;
  std::vector<float> window_;
  std::vector<float> inputBuffer_;
  std::vector<float> olaBuffer_;
  std::vector<float> frameTime_;
  std::vector<std::complex<float>> frameFreq_;
  std::vector<std::complex<float>> tempComplex_;
  std::vector<float> gainSpectral_;
};

} // namespace dsp


