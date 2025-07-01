#pragma once

#include "cstdlib"
#include "sys/resource.h"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <iostream>
#include <type_traits>
#include <utility>

namespace utils
{

inline int Rand(int min, int max) { return min + (rand() % (max - min + 1)); }

template <typename F, typename... Args>
inline double Timed(F &&f, Args &&...args)
{
    auto start = std::chrono::steady_clock::now();
    f(std::forward<Args>(args)...);
    auto end = std::chrono::steady_clock::now();
    double elapsed = std::chrono::duration<double>(end - start).count();
    return elapsed;
}

template <typename F, typename... Args>
inline std::pair<double, std::invoke_result_t<F, Args...>>
TimedResult(F &&f, Args &&...args)
{
    using ReturnType = std::invoke_result_t<F, Args...>;

    auto start = std::chrono::steady_clock::now();
    ReturnType ret = f(std::forward<Args>(args)...);
    auto end = std::chrono::steady_clock::now();
    double elapsed = std::chrono::duration<double>(end - start).count();
    return {elapsed, ret};
}

/**
 * @brief Increase system's file descriptor limit for the current processes (and
 * it's children if any). This is required because the default limit of 1024 on
 * most systems is not enough for 24 streams since a single stream opens
 * multiple descriptors (the exact number will depend on the pipeline).
 */
static void IncreaseFileDescriptorLimit()
{
    rlimit rlim;
    if (getrlimit(RLIMIT_NOFILE, &rlim))
    {
        perror("getrlimit failed");
        return;
    }

    const rlim_t min_desired = 1440;
    if (rlim.rlim_cur < min_desired)
    {
        rlim.rlim_cur = std::min(min_desired, rlim.rlim_max);
        if (setrlimit(RLIMIT_NOFILE, &rlim))
        {
            perror("setrlimit failed");
        }
        else
        {
            std::cout << "Raised file descriptor limit to " << rlim.rlim_cur
                      << "\n";
        }
    }
}

} // namespace utils
