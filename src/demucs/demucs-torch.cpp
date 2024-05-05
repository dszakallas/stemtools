#include <iostream>
#include <string>
#include <system_error>
#include <torch/script.h>

#include "../audio/audio.hpp"
#include "demucs.hpp"

int main(int argc, char **argv) {
  if (argc < 4) {
    std::cerr << "Usage: " << argv[0] << " <model> <input> <output>"
              << std::endl;
    return 1;
  }

  std::string model_file = argv[1];
  std::string ifile = argv[2];
  std::string odir = argv[3];

  std::error_code err;

  audio::init();

  auto source = audio::openSource(ifile, err);
  if (err) {
    std::cerr << "Error opening audio file: " << err.message() << std::endl;
    return -1;
  }

  auto demucs = demucs::Demucs(model_file, err);

  audio::SinkOpts sinkOpts{
      .sampleRate = 44100,
      .sampleFormat = AV_SAMPLE_FMT_S16,
      .bitRate = 16,
  };
  auto sink = audio::openSink(odir + "/out.wav", sinkOpts, err);
  if (err) {
    std::cerr << "Error opening audio file: " << err.message() << std::endl;
    return -1;
  }

  audio::Resampler resamplerIn(source->adecContext, demucs.codecParams, err);
  if (err) {
    std::cerr << "Error creating resampler: " << err.message() << std::endl;
    return -1;
  }

  audio::Resampler resamplerOut(demucs.codecParams, sink->aencContext, err);
  if (err) {
    std::cerr << "Error creating resampler: " << err.message() << std::endl;
    return -1;
  }

  auto chain = *source >> resamplerIn >> demucs >> resamplerOut;

  audio::run(chain, *sink, err);
  if (err) {
    std::cerr << err.category().name() << ": " << err.message() << std::endl;
    std::cerr << "Error running graph: " << err.message() << std::endl;
    return -1;
  }
}
