#pragma once

#include <avcpp/codec.h>
#include <avcpp/codeccontext.h>
#include <string>
#include <torch/script.h>

#include "../audio/audio.hpp"

namespace demucs {

struct CodecParams {
  int _sampleRate;
  av::SampleFormat _sampleFormat;
  uint64_t _channelLayout;
  size_t _frameSize;

  constexpr int sampleRate() const { return _sampleRate; }
  constexpr av::SampleFormat sampleFormat() const { return _sampleFormat; }
  constexpr uint64_t channelLayout() const { return _channelLayout; }
  constexpr size_t frameSize() const { return _frameSize; }

};

struct Opts {
  float_t transitionPower;
  float_t overlap;
};

constexpr Opts defaultDemucsOpts = {.transitionPower = 1., .overlap = .25};

struct Demucs {
  torch::Device device;
  torch::jit::script::Module module;
  torch::Tensor envelope;
  CodecParams codecParams;
  Opts opts;
  Demucs(const std::string &path, std::error_code &err,
         torch::Device device = torch::kCPU,
         const Opts &opts = defaultDemucsOpts);
  void write(av::AudioSamples &samples, audio::PipeState state,
             std::error_code &err);
  audio::PipeState read(av::AudioSamples &samples, std::error_code &err);

private:
  uint64_t _outSampleSize;
  bool _closed;
  torch::Tensor _inBuffer, _outBuffer, _sumWeights;
};

} // namespace demucs
