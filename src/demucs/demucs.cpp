#include "demucs.hpp"

#include <memory>
#ifdef DEMUCS_TORCH
#include "_torch/demucs.hpp"
#endif

#ifdef DEMUCS_ONNX
#include "_onnx/demucs.hpp"
#endif

namespace demucs {

std::map<std::string, Device> deviceMap = {
    {"cpu", Device::CPU},
    {"cuda", Device::CUDA},
    {"metal", Device::Metal},
};

std::unique_ptr<Demucs> openDemucs(const std::string &path,
                                   std::error_code &err, const Device device,
                                   const Opts &opts) {
  if (path.ends_with(".pt")) {
#ifdef DEMUCS_TORCH
    return std::make_unique<_torch::Demucs>(path, err, device, opts);
#else
    std::cerr << "Torch backend not available\n";
    err = std::make_error_code(std::errc::not_supported);
    return {};
#endif
  }

  if (path.ends_with(".onnx")) {
#ifdef DEMUCS_ONNX
    return std::make_unique<_onnx::Demucs>(path, err, device, opts);
#else
    std::cerr << "ONNX backend not available\n";
    err = std::make_error_code(std::errc::not_supported);
    return {};
#endif
  }

  std::cerr << "Unknown model format\n";
  err = std::make_error_code(std::errc::not_supported);
  return {};
}

} // namespace demucs
