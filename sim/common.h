#ifndef COMMON_H
#define COMMON_H

#include <vector>
#include <map>
#include <unordered_map>
#include <queue>
#include <set>
#include <cstdint>

//#define NDEBUG
#include "assert.h"

#include "misc/Config.h"
#include "misc/Statistics.h"
//#include "memsys/Cache.h"

class Config;
class Statistics;
class Cache;

extern Config cfg;
extern Statistics stat;

#endif
