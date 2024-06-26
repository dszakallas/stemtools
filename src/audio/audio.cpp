#include <cstdint>
#include <frame.h>
#include <memory>
#include <string>

#include <avcpp/av.h>
#include <avcpp/avutils.h>
#include <avcpp/codec.h>
#include <avcpp/codeccontext.h>
#include <avcpp/format.h>
#include <avcpp/formatcontext.h>

#include <system_error>

#include "../common/error.hpp"
#include "audio.hpp"

namespace audio {

std::unique_ptr<FileSink> openSink(const std::string path, SinkOpts opts,
                                   std::error_code &err) noexcept {
  auto sink = std::make_unique<FileSink>();
  auto &formatContext = sink->formatContext;
  auto &aencContext = sink->aencContext;
  auto &streamIndex = sink->streamIndex;
  auto &outputFormat = sink->outputFormat;
  outputFormat = av::guessOutputFormat(path, path);
  formatContext.setFormat(outputFormat);
  av::Codec codec = av::findEncodingCodec(outputFormat, false);
  aencContext = av::AudioEncoderContext(codec);

  aencContext.setSampleRate(opts.sampleRate);
  aencContext.setSampleFormat(opts.sampleFormat);
  aencContext.setBitRate(opts.bitRate);
  aencContext.setChannelLayout(AV_CH_LAYOUT_STEREO);
  aencContext.open(err);
  if (err) {
    std::cerr << "Failed to open encoder" << std::endl;
    return nullptr;
  }
  av::Stream ost = formatContext.addStream(aencContext);
  formatContext.openOutput(path, err);
  if (err) {
    std::cerr << "Failed to open file as sink" << std::endl;
    return nullptr;
  }
  formatContext.dump();
  formatContext.writeHeader();
  return sink;
}

void FileSink::write(const av::AudioSamples &samples, PipeState state,
                     std::error_code &err) noexcept {
  if (!state.hasFrames || state.isClosed) {
    return;
  }
  av::Packet pkt = aencContext.encode(samples, err);
  if (err || !pkt) {
    return;
  }

  std::cerr << "Writing packet: " << samples.samplesCount() << std::endl;

  pkt.setStreamIndex(streamIndex);
  formatContext.writePacket(pkt, err);
}

FileSink::~FileSink() noexcept {
  formatContext.flush();
  formatContext.writeTrailer();
}

std::unique_ptr<FileSource> openSource(const std::string path,
                                       std::error_code &err) noexcept {
  auto source = std::make_unique<FileSource>();
  auto &formatContext = source->formatContext;
  auto &adecContext = source->adecContext;
  auto &streamIndex = source->streamIndex;
  auto &stream = source->stream;

  formatContext.openInput(path, err);
  if (err) {
    std::cerr << "Failed to open file as source" << std::endl;
    return nullptr;
  }

  formatContext.findStreamInfo();

  for (size_t i = 0; i < formatContext.streamsCount(); ++i) {
    auto st = formatContext.stream(i);
    if (st.isAudio()) {
      streamIndex = i;
      stream = st;
      break;
    }
  }

  if (stream.isNull()) {
    std::cerr << "No audio stream found" << std::endl;
    err = std::make_error_code(error::Code::Undefined);
    return nullptr;
  }

  if (!stream.isValid()) {
    std::cerr << "Invalid audio stream" << std::endl;
    err = std::make_error_code(error::Code::Undefined);
    return nullptr;
  }

  adecContext = av::AudioDecoderContext(stream);
  auto codec = av::findDecodingCodec(adecContext.raw()->codec_id);
  adecContext.setCodec(codec);
  adecContext.setRefCountedFrames(true);
  adecContext.open(av::Codec(), err);
  if (err) {
    std::cerr << "Failed to open codec" << std::endl;
    return nullptr;
  }

  return source;
}

PipeState FileSource::read(av::AudioSamples &samples,
                           std::error_code &err) noexcept {
  while (true) {
    av::Packet pkt = formatContext.readPacket(err);
    if (err || !pkt)
      return {.isClosed = true};
    if (pkt.streamIndex() == streamIndex) {
      samples = adecContext.decode(pkt, err);
      return {.hasFrames = true};
    }
  }
}

void Resampler::write(const av::AudioSamples &samples, PipeState state,
                      std::error_code &err) noexcept {
  if (!state.hasFrames) {
    _inputClosed = state.isClosed;
    return;
  }
  resampler.push(samples, err);
}

PipeState Resampler::read(av::AudioSamples &samples,
                          std::error_code &err) noexcept {
  if (_outputClosed) {
    return {.isClosed = true};
  }
  samples =
      av::AudioSamples(resampler.dstSampleFormat(), frameSize,
                       resampler.dstChannelLayout(), resampler.dstSampleRate());
  auto hasFrames = resampler.pop(samples, false, err);
  if (err) {
    return {};
  }

  if (!hasFrames && _inputClosed) {
    hasFrames = resampler.pop(samples, true, err);
    if (err) {
      return {};
    }
    _outputClosed = true;
  }

  return {.hasFrames = hasFrames, .isClosed = !hasFrames && _outputClosed};;
}

void init() { av::init(); }

} // namespace audio
