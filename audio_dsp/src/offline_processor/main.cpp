#include <cstdio>
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstdint>

#include "dsp_core/dsp_core.h"
#include "common/wav_io.h"

static void printUsage() {
  std::printf("offline_processor -i input.wav -o output.wav [-sr 16000] [--no-ns]\n");
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
  bool noNs = false;
  for (int i = 1; i < argc; ++i) {
    std::string a = argv[i];
    if (a == "-i" && i + 1 < argc) { inPath = argv[++i]; }
    else if (a == "-o" && i + 1 < argc) { outPath = argv[++i]; }
    else if (a == "-sr" && i + 1 < argc) { sampleRate = std::atoi(argv[++i]); }
    else if (a == "--no-ns") { noNs = true; }
    else { printUsage(); return 1; }
  }
  if (inPath.empty() || outPath.empty()) { printUsage(); return 1; }

  wavio::WavData wavIn;
  if (!wavio::readWav(inPath, wavIn)) {
    std::printf("Failed to read %s\n", inPath.c_str());
    return 2;
  }
  if (wavIn.numChannels != 1) {
    std::printf("Only mono WAV supported for now.\n");
    return 3;
  }
  sampleRate = wavIn.sampleRateHz;

  dsp::PipelineConfig cfg;
  cfg.sampleRateHz = sampleRate;
  cfg.numChannels = 1;
  cfg.frameDurationMs = 10;
  cfg.enableHighPass = true;
  cfg.enableNoiseSuppressor = !noNs;

  dsp::DspPipeline pipe;
  pipe.initialize(cfg);

  std::vector<int16_t> processed;
  processed.reserve(wavIn.samples.size());

  int hop = sampleRate / 100; // 10ms
  for (size_t i = 0; i < wavIn.samples.size(); i += hop) {
    if ((i % (hop * 100)) == 0) { // 每秒进度
      double progress = (100.0 * i) / std::max<size_t>(1, wavIn.samples.size());
      std::printf("Processing... %.1f%%\r", progress);
      std::fflush(stdout);
    }
    int n = static_cast<int>(std::min<size_t>(hop, wavIn.samples.size() - i));
    std::vector<int16_t> outFrame;
    pipe.processBuffer(&wavIn.samples[i], n, outFrame);
    processed.insert(processed.end(), outFrame.begin(), outFrame.end());
  }
  std::printf("Processing... 100.0%%\n");
  std::vector<int16_t> tail;
  pipe.flush(tail);
  processed.insert(processed.end(), tail.begin(), tail.end());

  wavio::WavData wavOut;
  wavOut.sampleRateHz = sampleRate;
  wavOut.numChannels = 1;
  wavOut.samples = processed;
  if (!wavio::writeWav(outPath, wavOut)) {
    std::printf("Failed to write %s\n", outPath.c_str());
    return 4;
  }

  double inRms, inPeak, outRms, outPeak;
  computeRms(wavIn.samples, inRms, inPeak);
  computeRms(processed, outRms, outPeak);
  std::printf("Input:  RMS %.1f dBFS, Peak %.1f dBFS\n", inRms, inPeak);
  std::printf("Output: RMS %.1f dBFS, Peak %.1f dBFS\n", outRms, outPeak);
  std::printf("Saved to %s\n", outPath.c_str());
  return 0;
}


