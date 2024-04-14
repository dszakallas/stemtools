#pragma once

namespace util {

// Defer a function call until the end of the scope.
template <class F>
struct defer {
  defer(F f) : f(f) {}
  ~defer() { f(); }
  defer(const defer&) = delete;
  defer(defer&&) = delete;
  defer& operator=(const defer&) = delete;
  defer& operator=(defer&&) = delete;
  F f;
};

} // namespace util

