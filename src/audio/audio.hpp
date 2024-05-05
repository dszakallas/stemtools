#pragma once

#include <memory>
#include <string>

#include <avcpp/audioresampler.h>
#include <avcpp/av.h>
#include <avcpp/avutils.h>
#include <avcpp/codec.h>
#include <avcpp/codeccontext.h>
#include <avcpp/format.h>
#include <avcpp/formatcontext.h>
#include <system_error>

namespace audio {

struct PipeState {
  bool hasFrames;
  bool isClosed;
};

struct FileSource {
  av::FormatContext formatContext;
  av::AudioDecoderContext adecContext;
  ssize_t streamIndex;
  av::Stream stream;
  PipeState read(av::AudioSamples &samples, std::error_code &ec) noexcept;
};

std::unique_ptr<FileSource> openSource(const std::string path,
                                       std::error_code &err) noexcept;

struct FileSink {
  av::FormatContext formatContext;
  av::OutputFormat outputFormat;
  av::AudioEncoderContext aencContext;
  ssize_t streamIndex;
  void write(const av::AudioSamples &samples, PipeState state,
             std::error_code &ec) noexcept;
  ~FileSink() noexcept;
};

struct SinkOpts {
  int sampleRate;
  av::SampleFormat sampleFormat;
  int64_t bitRate;
};

std::unique_ptr<FileSink> openSink(const std::string path, SinkOpts opts,
                                   std::error_code &err) noexcept;

struct Resampler {
  template <class _C1, class _C2>
  Resampler(_C1 &&src, _C2 &&dst, std::error_code &err) noexcept
      : resampler(av::AudioResampler(
            dst.channelLayout(), dst.sampleRate(), dst.sampleFormat(),
            src.channelLayout(), src.sampleRate(), src.sampleFormat(), err)),
        frameSize(dst.frameSize()){};
  void write(const av::AudioSamples &samples, PipeState state,
             std::error_code &err) noexcept;
  PipeState read(av::AudioSamples &samples, std::error_code &err) noexcept;
  av::AudioResampler resampler;
  size_t frameSize;
  bool _inputClosed;
  bool _outputClosed;
};

template <class _Source, class _Trans> struct SourceChain {
  _Source &source;
  _Trans &transformer;

  PipeState read(av::AudioSamples &samples, std::error_code &err) {
    auto state = source.read(samples, err);
    if (err)
      return {};

    if (!state.hasFrames && !state.isClosed)
      return state;

    transformer.write(samples, state, err);
    if (err)
      return {};

    state = transformer.read(samples, err);
    if (err)
      return {};

    return state;
  }
};

template <class _Source, class _Trans>
SourceChain<_Source, _Trans> operator>>(_Source &&source,
                                        _Trans &&transformer) noexcept {
  return {source, transformer};
}

template <class _Source, class _Sink>
void run(_Source &source, _Sink &sink, std::error_code &err) noexcept {
  PipeState state;
  av::AudioSamples samples(nullptr);
  while (!state.isClosed) {
    state = source.read(samples, err);
    if (err)
      break;

    if (!state.hasFrames && !state.isClosed)
      continue;

    sink.write(samples, state, err);
    if (err)
      break;
  }
}

void init();

} // namespace audio
