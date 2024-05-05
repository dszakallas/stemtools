#include <c10/core/TensorOptions.h>
#include <cstdint>
#include <format>

#include "demucs.hpp"

namespace demucs {

void print_model(const torch::jit::script::Module &model, size_t level = 0) {
  std::string indentation(level * 2, ' ');
  std::cout << std::format("{}attrs:", indentation) << std::endl;
  for (const auto &attr : model.named_attributes(false)) {
    std::cout << std::format("{}  {}:{}", indentation, attr.name,
                             attr.value.type()->str())
              << std::endl;
  }
  std::cout << indentation << "layers:" << std::endl;
  for (const auto &child : model.named_children()) {
    std::cout << std::format("{}  {}:", indentation, child.name) << std::endl;
    print_model(child.value, level + 2);
  }
}

auto dfloat(torch::Device device) {
  return torch::dtype(torch::kFloat32).device(device);
}

Demucs::Demucs(const std::string &path, std::error_code &err,
               torch::Device device, const Opts &opts)
    : device(device) {

  try {
    module = torch::jit::load(path, device);
  } catch (const c10::Error &e) {
    std::cerr << "Error loading the model\n";
    err = std::make_error_code(std::errc::io_error);
    return;
  }

  module.eval();

  print_model(module);

  uint32_t sampleRate = module.attr("samplerate").toInt();
  float_t segment = module.attr("segment").toDouble();
  uint32_t frameSize = std::floor((1. - opts.overlap) * segment * sampleRate);
  uint32_t bufferSize = std::floor(segment * sampleRate);
  auto sources = module.attr("sources").toListRef();

  for (const auto &source : sources) {
    std::cerr << "Source: " << source.toStringRef() << std::endl;
  }

  uint32_t sourceLength = module.attr("sources").toListRef().size();

  std::cerr << "Demucser with model:" << std::endl;

  std::cerr << "Class: " << module.type()->name()->name() << std::endl;
  std::cerr << "Sample rate: " << sampleRate << std::endl;
  std::cerr << "Segment length: " << segment << std::endl;

  envelope =
      torch::cat({torch::arange(1, std::floor(bufferSize / 2) + 1, 1, {device}),
                  torch::arange(bufferSize - std::floor(bufferSize / 2), 0, -1,
                                {device})});

  envelope /= (envelope.max());
  envelope = torch::pow(envelope, opts.transitionPower);

  codecParams = CodecParams{
      ._sampleRate = 44100,
      ._sampleFormat = AV_SAMPLE_FMT_FLT,
      ._channelLayout = AV_CH_LAYOUT_STEREO,
      ._frameSize = frameSize,
  };

  _inBuffer = torch::zeros({1, 2, bufferSize}, dfloat(device));
  _outBuffer = torch::zeros({1, sourceLength, 2, bufferSize}, dfloat(device));
  _sumWeights = torch::zeros(bufferSize, dfloat(device));
  _outSampleSize = 0;
}

void Demucs::write(av::AudioSamples &samples, audio::PipeState state,
                   std::error_code &err) {
  std::cerr << std::format("Demucs::write {{.isClosed={},.hasFrames={}}}",
                           state.isClosed, state.hasFrames)
            << std::endl;

  _closed = state.isClosed;

  if (!state.hasFrames || state.isClosed)
    return;

  // TODO: Either this is should be moved to specialized enveloping resampler, or the resampler
  // should be moved here. It's bad relying on frame sizes being right.

  // copy overlap to the beginning of the buffer
  auto size = _inBuffer.size(-1);
  auto overlap = _inBuffer.slice(-1, codecParams.frameSize(), size);
  auto overlapWeights = _sumWeights.slice(0, codecParams.frameSize(), size);
  _inBuffer.slice(-1, 0, overlap.size(-1)) = overlap;
  _sumWeights.slice(0, 0, overlap.size(-1)) = overlapWeights;

  // copy new samples to the end of the buffer
  auto samplesBegin = size - codecParams.frameSize();
  auto samplesEnd = size - codecParams.frameSize() + samples.samplesCount();
  _inBuffer.slice(-1, samplesBegin, samplesEnd) =
      torch::from_blob(samples.data(), {samples.samplesCount(), 2}, dfloat(device)).permute({1, 0});
  
  // end of stream; not sure if model is causal, so zero pad for safety
  _inBuffer.slice(-1, samplesEnd, size) = 0;

  _sumWeights.slice(0, samplesBegin, size) = 0;
  _outBuffer.slice(-1, samplesBegin, size) = 0;

  auto out = module.forward({_inBuffer}).toTensor();

  _outBuffer += envelope * out;
  _sumWeights += envelope;
  _outBuffer /= _sumWeights;
  // once again, end of stream
  _outSampleSize = samples.samplesCount();
}

audio::PipeState Demucs::read(av::AudioSamples &samples, std::error_code &err) {
  // TODO: no buffering, write - read should be interleaved
  audio::PipeState state = {.hasFrames = !_closed, .isClosed = _closed};

  if (state.isClosed) {
    std::cerr << std::format("Demucs::read {{.isClosed={},.hasFrames={}}}",
                             state.isClosed, state.hasFrames)
              << std::endl;
    return state;
  }

  samples = av::AudioSamples(codecParams.sampleFormat(), _outSampleSize,
                             codecParams.channelLayout(), codecParams.sampleRate());


  auto outData = _outBuffer[0][0] // extract the drums only for now
          .slice(-1, -codecParams.frameSize(),
                 _outBuffer.size(-1) - codecParams.frameSize() + _outSampleSize)
          .permute({1, 0})
          .contiguous();

  std::memcpy(
      samples.data(),
      outData.to(torch::kCPU).data_ptr<float>(),
      _outSampleSize * 2 * sizeof(float));

  return state;
}

} // namespace demucs
