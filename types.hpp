#pragma once

#include <array>
#include <cstddef>
#include <future>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;
using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using str = std::string;

template <typename T, std::size_t N> using arr = std::array<T, N>;
template <typename T> using vec = std::vector<T>;
template <typename T> using fut = std::future<T>;
template <typename T> using up = std::unique_ptr<T>;
template <typename T> using sp = std::shared_ptr<T>;
template <typename T> using opt = std::optional<T>;
template <typename... Ts> using tup = std::tuple<Ts...>;
