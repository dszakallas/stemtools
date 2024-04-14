#include <iostream>
#include <string>
#include <torch/script.h>
#include <system_error>

#include "../audio/audio.hpp"

int main(int argc, char** argv) {
  if (argc < 4) {
    std::cerr << "Usage: " << argv[0] << " <model> <input> <output>"
              << std::endl;
    return 1;
  }

  std::string model_file = argv[1];
  std::string wav_file = argv[2];
  std::string out_dir = argv[3];

  torch::jit::script::Module module;

  try {
    // Deserialize the ScriptModule from a file
    module = torch::jit::load(model_file);

    // Proceed with model inference or other operations
  } catch (const c10::Error &e) {
    std::cerr << "Error loading the model\n";
    return -1;
  }

  module.eval();
  std::cerr << "Model class: " << module.type()->name()->name() << std::endl;

  std::cerr << "Sample rate: " << module.attr("samplerate").toInt()
            << std::endl;


  std::error_code err;

  audio::init();

  auto source = audio::openSource(wav_file, err);
  if (err) {
    std::cerr << "Error opening audio file: " << err.message() << std::endl;
    return -1;
  }

  audio::SinkOpts sinkOpts{
      .sampleRate = 48000,
      .sampleFormat = AV_SAMPLE_FMT_S16,
      .bitRate = 16,
  };
  auto sink = audio::openSink(out_dir + "/out.wav", sinkOpts, err);
  if (err) {
    std::cerr << "Error opening audio file: " << err.message() << std::endl;
    return -1;
  }

  audio::Resampler resampler(source->adecContext, sink->aencContext, err);
  if (err) {
    std::cerr << "Error creating resampler: " << err.message() << std::endl;
    return -1;
  }
  auto chain = *source >> resampler;

  // TODO attach model to chain

  audio::run(chain, *sink, err);
  if (err) {
    std::cerr << err.category().name() << ": " << err.message() << std::endl;
    std::cerr << "Error running graph: " << err.message() << std::endl;
    return -1;
  }
}
