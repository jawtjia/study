#include "common/wav_io.h"
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <cmath>
#include <vector>
#include <cstdint>

namespace wavio {
// helper functions must be declared before use
static uint32_t readLE32(FILE* f) { uint8_t b[4]; fread(b,1,4,f); return b[0]|(b[1]<<8)|(b[2]<<16)|(b[3]<<24); }
static uint16_t readLE16(FILE* f) { uint8_t b[2]; fread(b,1,2,f); return b[0]|(b[1]<<8); }
static void writeLE32(FILE* f, uint32_t v) { uint8_t b[4] = { (uint8_t)(v&255),(uint8_t)((v>>8)&255),(uint8_t)((v>>16)&255),(uint8_t)((v>>24)&255)}; fwrite(b,1,4,f); }
static void writeLE16(FILE* f, uint16_t v) { uint8_t b[2] = { (uint8_t)(v&255),(uint8_t)((v>>8)&255)}; fwrite(b,1,2,f); }
static inline float s16_to_f(int16_t v){ return std::max(-1.0f, std::min(1.0f, v/32768.0f)); }
static inline int16_t f_to_s16(float v){ float c=std::max(-1.0f,std::min(1.0f,v)); return (int16_t)std::round(c*32767.0f); }
static inline float clamp11(float v){ if (v>1.0f) return 1.0f; if (v<-1.0f) return -1.0f; return v; }

bool readWavFlex(const std::string& path, AudioBuffer& out) {
  FILE* f = std::fopen(path.c_str(), "rb");
  if (!f) return false;
  char riff[4]; if (fread(riff,1,4,f)!=4 || std::strncmp(riff,"RIFF",4)!=0) { fclose(f); return false; }
  (void)readLE32(f);
  char wave[4]; if (fread(wave,1,4,f)!=4 || std::strncmp(wave,"WAVE",4)!=0) { fclose(f); return false; }
  int channels=1, sampleRate=16000, bitsPerSample=16; uint32_t dataSize=0; long dataPos=0; uint16_t audioFormat=1; uint16_t effectiveFormat=1;
  while (!std::feof(f)) {
    char id[4]; if (fread(id,1,4,f)!=4) break; uint32_t sz = readLE32(f);
    if (std::strncmp(id,"fmt ",4)==0) {
      audioFormat = readLE16(f);
      channels = readLE16(f);
      sampleRate = readLE32(f);
      (void)readLE32(f); (void)readLE16(f);
      bitsPerSample = readLE16(f);
      effectiveFormat = audioFormat;
      if (sz > 16) {
        uint16_t cbSize = readLE16(f);
        if (audioFormat == 0xFFFE && cbSize >= 22) {
          // WAVE_FORMAT_EXTENSIBLE
          uint16_t validBits = readLE16(f); (void)validBits;
          uint32_t chanMask = readLE32(f); (void)chanMask;
          uint8_t subFmt[16]; fread(subFmt,1,16,f);
          // GUID first 4 bytes indicate the subtype: 1=PCM, 3=IEEE_FLOAT
          uint32_t subTag = (uint32_t)subFmt[0] | ((uint32_t)subFmt[1]<<8) | ((uint32_t)subFmt[2]<<16) | ((uint32_t)subFmt[3]<<24);
          if (subTag == 1) effectiveFormat = 1; else if (subTag == 3) effectiveFormat = 3;
          // skip remaining extension if any
          int toSkip = (int)cbSize - 22; if (toSkip > 0) std::fseek(f, toSkip, SEEK_CUR);
        } else {
          // skip remaining fmt bytes
          std::fseek(f, (long)sz - 16 - 2, SEEK_CUR);
        }
      }
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
  std::vector<uint8_t> buf(dataSize); fread(buf.data(),1,dataSize,f); fclose(f);
  out.sampleRateHz = sampleRate; out.numChannels = channels; out.samples.clear();
  if (effectiveFormat == 1) {
    if (bitsPerSample == 16) {
      size_t frames = dataSize / (channels * 2);
      out.samples.resize(frames * channels);
      const int16_t* p = reinterpret_cast<const int16_t*>(buf.data());
      for (size_t i=0;i<frames;++i){
        for (int ch=0; ch<channels; ++ch) out.samples[i*channels+ch] = s16_to_f(p[i*channels+ch]);
      }
      out.format = SampleFormat::S16;
      return true;
    } else if (bitsPerSample == 24) {
      size_t frames = dataSize / (channels * 3);
      out.samples.resize(frames * channels);
      for (size_t i=0;i<frames;++i){
        for (int ch=0; ch<channels; ++ch){
          const uint8_t* b = &buf[(i*channels+ch)*3];
          int32_t v = (int32_t)( (b[0]) | (b[1]<<8) | (b[2]<<16) );
          if (v & 0x800000) v |= ~0xFFFFFF;
          float f32 = (float)std::max(-1.0f, std::min(1.0f, (v / 8388608.0f)));
          out.samples[i*channels+ch] = f32;
        }
      }
      out.format = SampleFormat::S24;
      return true;
    } else if (bitsPerSample == 32) {
      size_t frames = dataSize / (channels * 4);
      out.samples.resize(frames * channels);
      const int32_t* p = reinterpret_cast<const int32_t*>(buf.data());
      for (size_t i=0;i<frames;++i){
        for (int ch=0; ch<channels; ++ch){
          float f32 = (float)std::max(-1.0f, std::min(1.0f, (p[i*channels+ch] / 2147483648.0f)));
          out.samples[i*channels+ch] = f32;
        }
      }
      out.format = SampleFormat::S32;
      return true;
    } else {
      return false;
    }
  } else if (effectiveFormat == 3 && bitsPerSample == 32) {
    size_t frames = dataSize / (channels * 4);
    out.samples.resize(frames * channels);
    const float* p = reinterpret_cast<const float*>(buf.data());
    for (size_t i=0;i<frames;++i){
      for (int ch=0; ch<channels; ++ch){
        float v = clamp11(p[i*channels+ch]);
        out.samples[i*channels+ch] = v;
      }
    }
    out.format = SampleFormat::F32;
    return true;
  }
  return false;
}

bool writeWavFlex(const std::string& path, const AudioBuffer& in, SampleFormat outFmt, int outChannels, int outSampleRate) {
  int channels = (outChannels>0?outChannels:in.numChannels);
  int sampleRate = (outSampleRate>0?outSampleRate:in.sampleRateHz);
  int bitsPerSample = (outFmt==SampleFormat::S16?16:(outFmt==SampleFormat::S24?24:(outFmt==SampleFormat::S32?32:32)));
  uint16_t audioFormat = (outFmt==SampleFormat::F32?3:1);
  size_t frames = in.samples.size() / std::max(1,in.numChannels);
  // 组装待写入的交织字节缓冲
  std::vector<uint8_t> payload;
  payload.reserve(frames * channels * (bitsPerSample/8));
  for (size_t i=0;i<frames;++i){
    for (int ch=0; ch<channels; ++ch){
      // 源通道选择：若输出通道多于输入，复制最后一个或第一个（这里取第一个）
      int srcCh = (ch < in.numChannels ? ch : 0);
      float v = clamp11(in.samples[i*in.numChannels + srcCh]);
      if (outFmt == SampleFormat::S16){
        int16_t s = f_to_s16(v);
        uint8_t b0 = (uint8_t)(s & 0xFF), b1=(uint8_t)((s>>8)&0xFF);
        payload.push_back(b0); payload.push_back(b1);
      } else if (outFmt == SampleFormat::S24){
        int32_t s24 = (int32_t)std::lround(v * 8388607.0f);
        if (s24 > 8388607) s24 = 8388607; if (s24 < -8388608) s24 = -8388608;
        uint8_t b0=(uint8_t)(s24 & 0xFF), b1=(uint8_t)((s24>>8)&0xFF), b2=(uint8_t)((s24>>16)&0xFF);
        payload.push_back(b0); payload.push_back(b1); payload.push_back(b2);
      } else if (outFmt == SampleFormat::S32) {
        int32_t s32 = (int32_t)std::lround(v * 2147483647.0f);
        if (s32 > 2147483647) s32 = 2147483647; if (s32 < -2147483648) s32 = -2147483648;
        uint8_t b0=(uint8_t)(s32 & 0xFF), b1=(uint8_t)((s32>>8)&0xFF), b2=(uint8_t)((s32>>16)&0xFF), b3=(uint8_t)((s32>>24)&0xFF);
        payload.push_back(b0); payload.push_back(b1); payload.push_back(b2); payload.push_back(b3);
      } else { // F32
        union { float f; uint8_t b[4]; } u; u.f = v;
        payload.push_back(u.b[0]); payload.push_back(u.b[1]); payload.push_back(u.b[2]); payload.push_back(u.b[3]);
      }
    }
  }
  // 写 WAV 头
  FILE* f = std::fopen(path.c_str(), "wb"); if (!f) return false;
  uint32_t byteRate = (uint32_t)(sampleRate * channels * (bitsPerSample/8));
  uint16_t blockAlign = (uint16_t)(channels * (bitsPerSample/8));
  uint32_t dataSize = (uint32_t)payload.size();
  std::fwrite("RIFF",1,4,f);
  writeLE32(f, 36 + dataSize);
  std::fwrite("WAVE",1,4,f);
  std::fwrite("fmt ",1,4,f);
  writeLE32(f, 16);
  writeLE16(f, audioFormat);
  writeLE16(f, (uint16_t)channels);
  writeLE32(f, (uint32_t)sampleRate);
  writeLE32(f, byteRate);
  writeLE16(f, blockAlign);
  writeLE16(f, (uint16_t)bitsPerSample);
  std::fwrite("data",1,4,f);
  writeLE32(f, dataSize);
  if (dataSize) std::fwrite(payload.data(),1,dataSize,f);
  std::fclose(f);
  return true;
}

bool readRawPcmFlex(const std::string& path, const char* format, int sampleRate, int channels, AudioBuffer& out){
  FILE* f = std::fopen(path.c_str(), "rb"); if (!f) return false;
  std::fseek(f,0,SEEK_END); long sz = std::ftell(f); std::fseek(f,0,SEEK_SET);
  std::vector<uint8_t> buf(sz); fread(buf.data(),1,sz,f); fclose(f);
  out.sampleRateHz = sampleRate; out.numChannels = channels; out.samples.clear();
  std::string fmt = format ? format : "";
  if (fmt == "s16le"){
    size_t frames = sz / (channels*2);
    out.samples.resize(frames * channels);
    const int16_t* p = reinterpret_cast<const int16_t*>(buf.data());
    for (size_t i=0;i<frames;++i){ for (int ch=0; ch<channels; ++ch) out.samples[i*channels+ch]=s16_to_f(p[i*channels+ch]); }
    out.format = SampleFormat::S16; return true;
  } else if (fmt == "s24le"){
    size_t frames = sz / (channels*3);
    out.samples.resize(frames * channels);
    for (size_t i=0;i<frames;++i){
      for (int ch=0; ch<channels; ++ch){
        const uint8_t* b = &buf[(i*channels+ch)*3];
        int32_t v = (int32_t)( (b[0]) | (b[1]<<8) | (b[2]<<16) );
        if (v & 0x800000) v |= ~0xFFFFFF;
        out.samples[i*channels+ch] = (float)std::max(-1.0f,std::min(1.0f, v/8388608.0f));
      }
    }
    out.format = SampleFormat::S24; return true;
  } else if (fmt == "f32le"){
    size_t frames = sz / (channels*4);
    out.samples.resize(frames * channels);
    const float* p = reinterpret_cast<const float*>(buf.data());
    for (size_t i=0;i<frames;++i){ for (int ch=0; ch<channels; ++ch){ float v = clamp11(p[i*channels+ch]); out.samples[i*channels+ch]=v; } }
    out.format = SampleFormat::F32; return true;
  }
  return false;
}

// moved to the top

bool readWav(const std::string& path, WavData& out) {
  FILE* f = std::fopen(path.c_str(), "rb");
  if (!f) return false;
  char riff[4]; fread(riff,1,4,f); if (std::strncmp(riff,"RIFF",4)!=0) { fclose(f); return false; }
  (void)readLE32(f); // file size
  char wave[4]; fread(wave,1,4,f); if (std::strncmp(wave,"WAVE",4)!=0) { fclose(f); return false; }

  int channels=1, sampleRate=16000, bitsPerSample=16; uint32_t dataSize=0; long dataPos=0; uint16_t audioFormat=1;
  while (!std::feof(f)) {
    char id[4]; if (fread(id,1,4,f)!=4) break; uint32_t sz = readLE32(f);
    if (std::strncmp(id,"fmt ",4)==0) {
      audioFormat = readLE16(f);
      channels = readLE16(f);
      sampleRate = readLE32(f);
      (void)readLE32(f); // byte rate
      (void)readLE16(f); // block align
      bitsPerSample = readLE16(f);
      if (sz > 16) std::fseek(f, sz - 16, SEEK_CUR);
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
  out.sampleRateHz = sampleRate; out.numChannels = 1; // 统一输出为 mono int16
  out.samples.clear();
  // 读取为内存缓冲
  std::vector<uint8_t> buf(dataSize);
  fread(buf.data(), 1, dataSize, f);
  fclose(f);
  // 解码不同位宽/格式，并 downmix 到 mono
  if (audioFormat == 1) { // PCM integer
    if (bitsPerSample == 16) {
      const int16_t* p = reinterpret_cast<const int16_t*>(buf.data());
      size_t frames = dataSize / (channels * 2);
      out.samples.resize(frames);
      for (size_t i = 0; i < frames; ++i) {
        int acc = 0;
        for (int ch = 0; ch < channels; ++ch) acc += p[i*channels + ch];
        out.samples[i] = static_cast<int16_t>(acc / channels);
      }
      return true;
    } else if (bitsPerSample == 24) {
      size_t frames = dataSize / (channels * 3);
      out.samples.resize(frames);
      for (size_t i = 0; i < frames; ++i) {
        int acc = 0;
        for (int ch = 0; ch < channels; ++ch) {
          const uint8_t* b = &buf[(i*channels + ch)*3];
          int32_t v = (int32_t)( (b[0]) | (b[1]<<8) | (b[2]<<16) );
          // sign extend 24-bit to 32-bit
          if (v & 0x800000) v |= ~0xFFFFFF;
          // scale to int16
          int32_t s16 = (v >> 8);
          acc += (int16_t)s16;
        }
        out.samples[i] = static_cast<int16_t>(acc / channels);
      }
      return true;
    } else {
      return false;
    }
  } else if (audioFormat == 3 && bitsPerSample == 32) { // IEEE float
    const float* p = reinterpret_cast<const float*>(buf.data());
    size_t frames = dataSize / (channels * 4);
    out.samples.resize(frames);
    for (size_t i = 0; i < frames; ++i) {
      float acc = 0.0f;
      for (int ch = 0; ch < channels; ++ch) acc += p[i*channels + ch];
      float m = acc / static_cast<float>(channels);
      if (m > 1.0f) m = 1.0f; if (m < -1.0f) m = -1.0f;
      out.samples[i] = static_cast<int16_t>(std::round(m * 32767.0f));
    }
    return true;
  }
  return false;
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

bool readRawPcm(const std::string& path, const char* format, int sampleRate, int channels, WavData& out) {
  FILE* f = std::fopen(path.c_str(), "rb");
  if (!f) return false;
  std::fseek(f, 0, SEEK_END);
  long sz = std::ftell(f);
  std::fseek(f, 0, SEEK_SET);
  std::vector<uint8_t> buf(sz);
  fread(buf.data(), 1, sz, f);
  fclose(f);
  out.sampleRateHz = sampleRate; out.numChannels = 1; out.samples.clear();
  std::string fmt = format ? format : "";
  if (fmt == "s16le") {
    const int16_t* p = reinterpret_cast<const int16_t*>(buf.data());
    size_t frames = sz / (channels * 2);
    out.samples.resize(frames);
    for (size_t i = 0; i < frames; ++i) {
      int acc = 0; for (int ch = 0; ch < channels; ++ch) acc += p[i*channels + ch];
      out.samples[i] = static_cast<int16_t>(acc / channels);
    }
    return true;
  } else if (fmt == "s24le") {
    size_t frames = sz / (channels * 3);
    out.samples.resize(frames);
    for (size_t i = 0; i < frames; ++i) {
      int acc = 0;
      for (int ch = 0; ch < channels; ++ch) {
        const uint8_t* b = &buf[(i*channels + ch)*3];
        int32_t v = (int32_t)( (b[0]) | (b[1]<<8) | (b[2]<<16) );
        if (v & 0x800000) v |= ~0xFFFFFF;
        int32_t s16 = (v >> 8);
        acc += (int16_t)s16;
      }
      out.samples[i] = static_cast<int16_t>(acc / channels);
    }
    return true;
  } else if (fmt == "f32le") {
    const float* p = reinterpret_cast<const float*>(buf.data());
    size_t frames = sz / (channels * 4);
    out.samples.resize(frames);
    for (size_t i = 0; i < frames; ++i) {
      float acc = 0.0f; for (int ch = 0; ch < channels; ++ch) acc += p[i*channels + ch];
      float m = acc / static_cast<float>(channels);
      if (m > 1.0f) m = 1.0f; if (m < -1.0f) m = -1.0f;
      out.samples[i] = static_cast<int16_t>(std::round(m * 32767.0f));
    }
    return true;
  }
  return false;
}

} // namespace wavio


