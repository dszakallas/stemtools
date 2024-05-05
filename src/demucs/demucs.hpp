#pragma once

#include "../audio/audio.hpp"
#include <avcpp/codec.h>
#include <avcpp/codeccontext.h>
#include <map>
#include <memory>

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

enum class Device {
  CPU,
  CUDA,
  Metal,
};

extern std::map<std::string, Device> deviceMap;

struct Demucs {
  demucs::CodecParams codecParams;
  demucs::Opts opts;
  virtual void write(av::AudioSamples &samples, audio::PipeState state,
                     std::error_code &err) = 0;
  virtual audio::PipeState read(av::AudioSamples &samples,
                                std::error_code &err) = 0;
  virtual ~Demucs() = default;
};

std::unique_ptr<Demucs>
openDemucs(const std::string &path, std::error_code &err,
           const demucs::Device device = demucs::Device::CPU,
           const Opts &opts = demucs::defaultDemucsOpts);
} // namespace demucs
