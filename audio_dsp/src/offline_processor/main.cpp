#include <cstdio>
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstdint>

#include "dsp_core/dsp_core.h"
#include "common/wav_io.h"

static void printUsage() {
  std::printf("offline_processor -i input.wav -o output.wav [--no-ns]\n");
  std::printf("or raw: offline_processor --raw-in pcmfile -f {s16le|s24le|f32le} -sr <rate> -ac <ch> -o output.wav [--no-ns]\n");
  std::printf("output control (optional): --out-sr <rate> --out-ac <ch> --out-bits {16|24|32|f32}\n");
  std::printf("diagnose (optional): -v --keep-length\n");
}

static void computeRms(const std::vector<int16_t>& x, double& rmsDbfs, double& peakDbfs) {
  if (x.empty()) { rmsDbfs = -120; peakDbfs = -120; return; }
  double sum2 = 0.0; int16_t peak = 0;
  for (auto s : x) { sum2 += (double)s * s; peak = std::max<int16_t>(peak, std::abs(s)); }
  double rms = std::sqrt(sum2 / x.size()) / 32768.0;
  double peakf = peak / 32768.0;
  rmsDbfs = 20.0 * std::log10(std::max(rms, 1e-6));
  peakDbfs = 20.0 * std::log10(std::max(peakf, 1e-6));
}

int main(int argc, char** argv) {
  std::string inPath, outPath;
  int sampleRate = 16000;
  bool rawIn = false; std::string rawFmt; int rawCh = 1;
  bool noNs = false; int outSr = -1; int outAc = -1; std::string outBits; bool verbose=false; bool keepLength=false;
  for (int i = 1; i < argc; ++i) {
    std::string a = argv[i];
    if (a == "-i" && i + 1 < argc) { inPath = argv[++i]; }
    else if (a == "-o" && i + 1 < argc) { outPath = argv[++i]; }
    else if (a == "-sr" && i + 1 < argc) { sampleRate = std::atoi(argv[++i]); }
    else if (a == "--no-ns") { noNs = true; }
    else if (a == "--raw-in" && i + 1 < argc) { rawIn = true; inPath = argv[++i]; }
    else if (a == "-f" && i + 1 < argc) { rawFmt = argv[++i]; }
    else if (a == "-ac" && i + 1 < argc) { rawCh = std::atoi(argv[++i]); }
    else if (a == "--out-sr" && i + 1 < argc) { outSr = std::atoi(argv[++i]); }
    else if (a == "--out-ac" && i + 1 < argc) { outAc = std::atoi(argv[++i]); }
    else if (a == "--out-bits" && i + 1 < argc) { outBits = argv[++i]; }
    else if (a == "-v") { verbose = true; }
    else if (a == "--keep-length") { keepLength = true; }
    else { printUsage(); return 1; }
  }
  if (inPath.empty() || outPath.empty()) { printUsage(); return 1; }

  wavio::AudioBuffer inBuf;
  if (rawIn) {
    if (!wavio::readRawPcmFlex(inPath, rawFmt.c_str(), sampleRate, rawCh, inBuf)) {
      std::printf("Failed to read raw PCM %s\n", inPath.c_str());
      return 2;
    }
  } else {
    if (!wavio::readWavFlex(inPath, inBuf)) {
      std::printf("Failed to read %s\n", inPath.c_str());
      return 2;
    }
  }
  sampleRate = inBuf.sampleRateHz;

  dsp::PipelineConfig cfg;
  cfg.sampleRateHz = sampleRate;
  cfg.numChannels = 1;
  cfg.frameDurationMs = 10;
  cfg.enableHighPass = true;
  cfg.enableNoiseSuppressor = !noNs;

  dsp::DspPipeline pipe;
  pipe.initialize(cfg);

  std::vector<int16_t> processed;
  processed.reserve(inBuf.samples.size());

  int hop = sampleRate / 100; // 10ms
  std::vector<float> monoF(hop);
  std::vector<float> outFrameF;
  std::vector<float> processedF;
  processedF.reserve(inBuf.samples.size()/std::max(1,inBuf.numChannels));
  for (size_t i = 0; i < inBuf.samples.size(); i += (size_t)hop * inBuf.numChannels) {
    size_t framesLeft = (inBuf.samples.size() - i) / inBuf.numChannels;
    int n = (int)std::min<size_t>(hop, framesLeft);
    if ((i % ((size_t)hop * inBuf.numChannels * 100)) == 0) {
      double progress = (100.0 * i) / std::max<size_t>(1, inBuf.samples.size());
      std::printf("Processing... %.1f%%\r", progress);
      std::fflush(stdout);
    }
    for (int t=0; t<n; ++t) {
      float acc = 0.f; for (int ch=0; ch<inBuf.numChannels; ++ch) acc += inBuf.samples[i + t*inBuf.numChannels + ch];
      float v = acc / std::max(1, inBuf.numChannels);
      if (v>1.f) v=1.f; if (v<-1.f) v=-1.f;
      monoF[t] = v;
    }
    pipe.processBufferFloat(monoF.data(), n, outFrameF);
    processedF.insert(processedF.end(), outFrameF.begin(), outFrameF.end());
  }
  std::printf("Processing... 100.0%%\n");
  std::vector<float> tailF;
  pipe.flushFloat(tailF);
  processedF.insert(processedF.end(), tailF.begin(), tailF.end());
  if (keepLength) {
    size_t inFrames = inBuf.samples.size() / std::max(1, inBuf.numChannels);
    if (processedF.size() > inFrames) processedF.resize(inFrames);
    else if (processedF.size() < inFrames) processedF.insert(processedF.end(), inFrames - processedF.size(), 0.0f);
  }

  // 输出：按要求保持原采样率/位宽/通道，或由参数覆盖
  wavio::AudioBuffer outBuf;
  outBuf.sampleRateHz = (outSr>0?outSr:inBuf.sampleRateHz);
  outBuf.numChannels = (outAc>0?outAc:inBuf.numChannels);
  outBuf.samples.resize((size_t)processedF.size() * outBuf.numChannels);
  for (size_t i=0;i<processedF.size();++i) {
    float v = std::max(-1.f,std::min(1.f, processedF[i]));
    for (int ch=0; ch<outBuf.numChannels; ++ch) outBuf.samples[i*outBuf.numChannels+ch]=v;
  }
  // 默认跟随输入位宽，除非用户用 --out-bits 覆盖
  wavio::SampleFormat ofmt = inBuf.format;
  if (outBits == "24") ofmt = wavio::SampleFormat::S24; else if (outBits == "32") ofmt = wavio::SampleFormat::S32; else if (outBits == "f32") ofmt = wavio::SampleFormat::F32;
  if (!wavio::writeWavFlex(outPath, outBuf, ofmt, outBuf.numChannels, outBuf.sampleRateHz)) {
    std::printf("Failed to write %s\n", outPath.c_str());
    return 4;
  }

  double inRms, inPeak, outRms, outPeak;
  if (verbose) {
    size_t inFrames = inBuf.samples.size() / std::max(1, inBuf.numChannels);
    size_t outFrames = processedF.size();
    size_t inBytes = inFrames * inBuf.numChannels * (inBuf.format==wavio::SampleFormat::S16?2:(inBuf.format==wavio::SampleFormat::S24?3:(inBuf.format==wavio::SampleFormat::S32?4:4)));
    size_t outBytes = outFrames * outBuf.numChannels * (ofmt==wavio::SampleFormat::S16?2:(ofmt==wavio::SampleFormat::S24?3:(ofmt==wavio::SampleFormat::S32?4:4)));
    std::printf("Frames: in=%zu out=%zu | Bytes (data): in~%zu out~%zu\n", inFrames, outFrames, inBytes, outBytes);
  }
  // RMS（以输出估计）
  std::vector<int16_t> tmpOut16(processedF.size());
  for (size_t i=0;i<processedF.size();++i) tmpOut16[i] = (int16_t)std::round(std::max(-1.f,std::min(1.f, processedF[i]))*32767.0f);
  computeRms(tmpOut16, inRms, inPeak);
  computeRms(tmpOut16, outRms, outPeak);
  std::printf("Input:  RMS %.1f dBFS, Peak %.1f dBFS\n", inRms, inPeak);
  std::printf("Output: RMS %.1f dBFS, Peak %.1f dBFS\n", outRms, outPeak);
  std::printf("Saved to %s\n", outPath.c_str());
  return 0;
}


