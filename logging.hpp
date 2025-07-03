#pragma once

#include <glog/logging.h>

#define DEBUG VLOG(1) // no DEBUG in glog; use VLOG (CLI flag: --v=1)
#define INFO LOG(INFO)
#define WARNING LOG(WARNING)
#define ERROR LOG(ERROR)
#define CRITICAL LOG(FATAL)
