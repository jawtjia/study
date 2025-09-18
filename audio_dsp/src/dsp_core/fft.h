#pragma once

#include <vector>
#include <complex>
#include <cmath>

namespace dsp {

// Iterative radix-2 Cooley-Tukey FFT (in-place)
// a.size() must be power of two. inverse=false: forward FFT; inverse=true: IFFT (not normalized)
inline void fft(std::vector<std::complex<float>>& a, bool inverse) {
  const size_t n = a.size();
  if (n == 0) return;
  // bit reversal
  for (size_t i = 1, j = 0; i < n; ++i) {
    size_t bit = n >> 1;
    for (; j & bit; bit >>= 1) j ^= bit;
    j ^= bit;
    if (i < j) std::swap(a[i], a[j]);
  }
  for (size_t len = 2; len <= n; len <<= 1) {
    float ang = 2.0f * 3.14159265358979323846f / static_cast<float>(len) * (inverse ? 1.0f : -1.0f);
    std::complex<float> wlen(std::cos(ang), std::sin(ang));
    for (size_t i = 0; i < n; i += len) {
      std::complex<float> w(1.0f, 0.0f);
      for (size_t j = 0; j < len / 2; ++j) {
        std::complex<float> u = a[i + j];
        std::complex<float> v = a[i + j + len / 2] * w;
        a[i + j] = u + v;
        a[i + j + len / 2] = u - v;
        w *= wlen;
      }
    }
  }
  if (inverse) {
    float invN = 1.0f / static_cast<float>(n);
    for (size_t i = 0; i < n; ++i) a[i] *= invN;
  }
}

} // namespace dsp


