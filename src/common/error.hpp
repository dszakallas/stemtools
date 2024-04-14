#pragma once

#include <system_error>
#include <string>

namespace error {
enum struct Code {
  Success = 0,
  Undefined = 1,
};

struct Category : public std::error_category {
  const char *name() const noexcept override;
  std::string message(int ev) const override;
};
} // namespace error

namespace std {
template <> struct is_error_code_enum<error::Code> : true_type {};
std::error_code make_error_code(error::Code e);
} // namespace std
