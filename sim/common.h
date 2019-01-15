#ifndef COMMON_H
#define COMMON_H

#include <vector>
#include <map>
#include <unordered_map>
#include <queue>
#include <set>
#include <cstdint>
#include "assert.h"
#include "misc/Config.h"
#include "misc/Statistics.h"

class Config;
class Statistics;

extern Config cfg;
extern Statistics stat;


struct Transaction {
	Transaction(int id, int coreId, uint64_t addr, bool isLoad) : id(id), coreId(coreId), addr(addr), isLoad(isLoad) {};
	int id;
	int coreId;
	uint64_t addr;
	bool isLoad;
};
typedef pair<Transaction*, uint64_t> TransactionOp;
class TransactionOpCompare {
public:
  bool operator() (const TransactionOp &l, const TransactionOp &r) const {
    if(r.second < l.second) 
      return true;
  	else
      return false;
  }
};

#endif
