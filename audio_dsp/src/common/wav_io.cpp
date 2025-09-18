#include "common/wav_io.h"
#include <cstdio>
#include <cstring>
#include <algorithm>

namespace wavio {

static uint32_t readLE32(FILE* f) { uint8_t b[4]; fread(b,1,4,f); return b[0]|(b[1]<<8)|(b[2]<<16)|(b[3]<<24); }
static uint16_t readLE16(FILE* f) { uint8_t b[2]; fread(b,1,2,f); return b[0]|(b[1]<<8); }
static void writeLE32(FILE* f, uint32_t v) { uint8_t b[4] = { (uint8_t)(v&255),(uint8_t)((v>>8)&255),(uint8_t)((v>>16)&255),(uint8_t)((v>>24)&255)}; fwrite(b,1,4,f); }
static void writeLE16(FILE* f, uint16_t v) { uint8_t b[2] = { (uint8_t)(v&255),(uint8_t)((v>>8)&255)}; fwrite(b,1,2,f); }

bool readWav(const std::string& path, WavData& out) {
  FILE* f = std::fopen(path.c_str(), "rb");
  if (!f) return false;
  char riff[4]; fread(riff,1,4,f); if (std::strncmp(riff,"RIFF",4)!=0) { fclose(f); return false; }
  (void)readLE32(f); // file size
  char wave[4]; fread(wave,1,4,f); if (std::strncmp(wave,"WAVE",4)!=0) { fclose(f); return false; }

  int channels=1, sampleRate=16000, bitsPerSample=16; uint32_t dataSize=0; long dataPos=0;
  while (!std::feof(f)) {
    char id[4]; if (fread(id,1,4,f)!=4) break; uint32_t sz = readLE32(f);
    if (std::strncmp(id,"fmt ",4)==0) {
      uint16_t audioFormat = readLE16(f);
      channels = readLE16(f);
      sampleRate = readLE32(f);
      (void)readLE32(f); // byte rate
      (void)readLE16(f); // block align
      bitsPerSample = readLE16(f);
      if (sz > 16) std::fseek(f, sz - 16, SEEK_CUR);
      if (audioFormat != 1 || bitsPerSample != 16) { fclose(f); return false; }
    } else if (std::strncmp(id,"data",4)==0) {
      dataPos = std::ftell(f);
      dataSize = sz;
      std::fseek(f, sz, SEEK_CUR);
    } else {
      std::fseek(f, sz, SEEK_CUR);
    }
  }
  if (dataPos == 0 || dataSize == 0) { fclose(f); return false; }
  std::fseek(f, dataPos, SEEK_SET);
  out.sampleRateHz = sampleRate; out.numChannels = channels;
  out.samples.resize(dataSize / 2);
  fread(out.samples.data(), 1, dataSize, f);
  fclose(f);
  return true;
}

bool writeWav(const std::string& path, const WavData& in) {
  FILE* f = std::fopen(path.c_str(), "wb");
  if (!f) return false;
  int channels = in.numChannels;
  int sampleRate = in.sampleRateHz;
  int bitsPerSample = 16;
  uint32_t dataSize = static_cast<uint32_t>(in.samples.size() * sizeof(int16_t));
  std::fwrite("RIFF",1,4,f);
  writeLE32(f, 36 + dataSize);
  std::fwrite("WAVE",1,4,f);
  std::fwrite("fmt ",1,4,f);
  writeLE32(f, 16);
  writeLE16(f, 1);
  writeLE16(f, static_cast<uint16_t>(channels));
  writeLE32(f, static_cast<uint32_t>(sampleRate));
  writeLE32(f, static_cast<uint32_t>(sampleRate * channels * bitsPerSample / 8));
  writeLE16(f, static_cast<uint16_t>(channels * bitsPerSample / 8));
  writeLE16(f, static_cast<uint16_t>(bitsPerSample));
  std::fwrite("data",1,4,f);
  writeLE32(f, dataSize);
  std::fwrite(in.samples.data(), 1, dataSize, f);
  std::fclose(f);
  return true;
}

} // namespace wavio


