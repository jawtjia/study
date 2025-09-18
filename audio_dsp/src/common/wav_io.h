#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace wavio {

struct WavData {
  int sampleRateHz = 16000;
  int numChannels = 1;
  std::vector<int16_t> samples; // interleaved if >1 channels
};

bool readWav(const std::string& path, WavData& out);
bool writeWav(const std::string& path, const WavData& in);

} // namespace wavio


