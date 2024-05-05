#include <iostream>
#include <string>
#include <system_error>

#include <argparse/argparse.hpp>

#include "../audio/audio.hpp"
#include "demucs.hpp"

int main(int argc, char **argv) {
  argparse::ArgumentParser program("demucs-test");

  program.add_argument("model").help(
      "Path to the model file. Engine can be either Torch or ONNX, determined "
      "by the file extension");
  program.add_argument("input").help("Path to the input audio file");
  program.add_argument("output").help("Path to the output directory");

  program.add_argument("-d", "--device")
      .default_value("cpu")
      .choices("cpu", "cuda", "metal")
      .help("Device to run the model on. Defaults to cpu");

  try {
    program.parse_args(argc, argv);
  } catch (const std::exception &err) {
    std::cerr << err.what() << std::endl;
    std::cerr << program;
    std::exit(1);
  }

  std::string model_file = program.get<std::string>("model");
  std::string ifile = program.get<std::string>("input");
  std::string odir = program.get<std::string>("output");

  auto device = demucs::deviceMap[program.get<std::string>("--device")];

  std::error_code err;

  audio::init();

  auto source = audio::openSource(ifile, err);
  if (err) {
    std::cerr << "Error opening audio file: " << err.message() << std::endl;
    return -1;
  }

  auto demucs = demucs::openDemucs(model_file, err, device);
  if (err) {
    std::cerr << "Error opening model: " << err.message() << std::endl;
    return -1;
  }

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

  audio::Resampler resamplerIn(source->adecContext, demucs->codecParams, err);
  if (err) {
    std::cerr << "Error creating resampler: " << err.message() << std::endl;
    return -1;
  }

  audio::Resampler resamplerOut(demucs->codecParams, sink->aencContext, err);
  if (err) {
    std::cerr << "Error creating resampler: " << err.message() << std::endl;
    return -1;
  }

  auto chain = *source >> resamplerIn >> *demucs >> resamplerOut;

  audio::run(chain, *sink, err);
  if (err) {
    std::cerr << err.category().name() << ": " << err.message() << std::endl;
    std::cerr << "Error running graph: " << err.message() << std::endl;
    return -1;
  }
}
