#include "error.hpp"

namespace error {

const char *Category::name() const noexcept { return "stemtools::error::Code"; }

std::string Category::message(int ev) const {
  switch (static_cast<Code>(ev)) {
  case Code::Success:
    return "Success";
  case Code::Undefined:
  default:
    return "Undefined error occurred";
  }
}

const std::error_category& category() {
    static Category instance;
    return instance;
}
} // namespace error

namespace std {
std::error_code make_error_code(error::Code e) {
  return {static_cast<int>(e), error::category()};
}
} // namespace std
