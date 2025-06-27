#pragma once

#include "cstdlib"
#include <chrono>
#include <type_traits>
#include <utility>

namespace utils {

inline int Rand(int min, int max) { return min + (rand() % (max - min + 1)); }

template <typename F, typename... Args>
inline double Timed(F &&f, Args &&...args) {
  auto start = std::chrono::steady_clock::now();
  f(std::forward<Args>(args)...);
  auto end = std::chrono::steady_clock::now();
  double elapsed = std::chrono::duration<double>(end - start).count();
  return elapsed;
}

template <typename F, typename... Args>
inline std::pair<double, std::invoke_result_t<F, Args...>>
TimeResult(F &&f, Args &&...args) {
  using ReturnType = std::invoke_result_t<F, Args...>;

  auto start = std::chrono::steady_clock::now();
  ReturnType ret = f(std::forward<Args>(args)...);
  auto end = std::chrono::steady_clock::now();
  double elapsed = std::chrono::duration<double>(end - start).count();
  return {elapsed, ret};
}

} // namespace utils
