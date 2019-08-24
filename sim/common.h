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

//#define chunk_size 1024

class Config;
class Statistics;
class Cache;

/*typedef enum {I_ADDSUB, I_MULT, I_DIV, I_REM, FP_ADDSUB, FP_MULT, FP_DIV, FP_REM, LOGICAL, 
              CAST, GEP, LD, ST, TERMINATOR, PHI, SEND, RECV, STADDR, STVAL, LD_PROD, INVALID,  BS_DONE, CORE_INTERRUPT, CALL_BS, BS_WAKE, BS_VECTOR_INC, BARRIER, ACCELERATOR} TInstr;
*/

typedef enum {I_ADDSUB, I_MULT, I_DIV, I_REM, FP_ADDSUB, FP_MULT, FP_DIV, FP_REM, LOGICAL, CAST, GEP, LD, ST, TERMINATOR, PHI, SEND, RECV, STADDR, STVAL, LD_PROD, INVALID,  BS_DONE, CORE_INTERRUPT, CALL_BS, BS_WAKE, BS_VECTOR_INC, BARRIER, ACCELERATOR, ATOMIC_ADD, ATOMIC_FADD, ATOMIC_MIN, ATOMIC_CAS, TRM_ATOMIC_FADD, TRM_ATOMIC_MIN, TRM_ATOMIC_CAS} TInstr;

extern Config cfg;
extern Statistics stat;

#endif
