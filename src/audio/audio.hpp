#pragma once

#include <memory>
#include <string>

#include <avcpp/av.h>
#include <avcpp/avutils.h>
#include <avcpp/codec.h>
#include <avcpp/codeccontext.h>
#include <avcpp/format.h>
#include <avcpp/formatcontext.h>
#include <avcpp/audioresampler.h>
#include <system_error>

namespace audio {

struct FileSource {
  av::FormatContext formatContext;
  av::AudioDecoderContext adecContext;
  ssize_t streamIndex;
  av::Stream stream;
  bool read(av::AudioSamples &samples, std::error_code &ec) noexcept;
};

std::unique_ptr<FileSource> openSource(const std::string path,
                                       std::error_code &err) noexcept;

struct FileSink {
  av::FormatContext formatContext;
  av::OutputFormat outputFormat;
  av::AudioEncoderContext aencContext;
  ssize_t streamIndex;
  void write(const av::AudioSamples &samples, std::error_code &ec) noexcept;
  ~FileSink();
};

struct SinkOpts {
  int sampleRate;
  av::SampleFormat sampleFormat;
  int64_t bitRate;
};

std::unique_ptr<FileSink> openSink(const std::string path, SinkOpts opts,
                                   std::error_code &err) noexcept;

struct Resampler {
  Resampler(av::AudioDecoderContext &src, av::AudioEncoderContext &dst,
            std::error_code &err)
      : resampler(av::AudioResampler(
            dst.channelLayout(), dst.sampleRate(), dst.sampleFormat(),
            src.channelLayout(), src.sampleRate(), src.sampleFormat(), err)),
        frameSize(dst.frameSize()){};
  void write(const av::AudioSamples &samples, std::error_code &err) noexcept;
  bool read(av::AudioSamples &samples, std::error_code &err) noexcept;
  av::AudioResampler resampler;
  size_t frameSize;
};

template <class _Source, class _Trans>
struct SourceChain {
  _Source &source;
  _Trans &transformer;

  bool read(av::AudioSamples &samples, std::error_code &err) noexcept {
    bool hasFrames = source.read(samples, err);
    if (err)
      return false;

    if (hasFrames) {
      transformer.write(samples, err);
      if (err)
        return false;
    }

    hasFrames = transformer.read(samples, err);
    if (err)
      return false;
    return hasFrames;
  }
};

template <class _Source, class _Trans>
SourceChain<_Source, _Trans> operator>>(_Source &source, _Trans &transformer) noexcept {
  return {source, transformer};
}

template <class _Source, class _Sink>
void run(_Source &source, _Sink &sink, std::error_code &err) noexcept {
  bool hasFrames = true;
  av::AudioSamples samples(nullptr);
  while (hasFrames) {
    hasFrames = source.read(samples, err);
    if (err) break;

    if (hasFrames) {
      sink.write(samples, err);
      if (err) break;
    }
  }
}

void init();

} // namespace audio
