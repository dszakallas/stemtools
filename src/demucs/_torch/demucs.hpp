#pragma once

#include <avcpp/codec.h>
#include <avcpp/codeccontext.h>
#include <string>
#include <torch/script.h>

#include "../../audio/audio.hpp"
#include "../demucs.hpp"

namespace demucs {
namespace _torch {

struct Demucs : public demucs::Demucs {
  torch::Device device;
  torch::jit::script::Module module;
  torch::Tensor envelope;
  Demucs(const std::string &path, std::error_code &err,
         const demucs::Device device = demucs::Device::CPU,
         const Opts &opts = demucs::defaultDemucsOpts);
  virtual void write(av::AudioSamples &samples, audio::PipeState state,
                     std::error_code &err) override;
  virtual audio::PipeState read(av::AudioSamples &samples,
                                std::error_code &err) override;
  virtual ~Demucs() = default;

private:
  uint64_t _outSampleSize;
  bool _closed;
  torch::Tensor _inBuffer, _outBuffer, _sumWeights;
};

} // namespace _torch
} // namespace demucs
