#pragma once

#include <cstdint>
#include <vector>

namespace dsp {

// Simple DC blocker high-pass filter: y[n] = x[n] - x[n-1] + R * y[n-1]
class HighPassFilter {
public:
  void initialize(int sampleRateHz, float pole = 0.995f) {
    (void)sampleRateHz; // not used for this simple DC blocker
    r_ = pole;
    x1_ = 0.0f;
    y1_ = 0.0f;
  }

  inline float processSample(float x) {
    float y = x - x1_ + r_ * y1_;
    x1_ = x;
    y1_ = y;
    return y;
  }

  void processBuffer(const float* in, int n, float* out) {
    for (int i = 0; i < n; ++i) out[i] = processSample(in[i]);
  }

  void reset() {
    x1_ = 0.0f;
    y1_ = 0.0f;
  }

private:
  float r_ = 0.995f;
  float x1_ = 0.0f;
  float y1_ = 0.0f;
};

} // namespace dsp


