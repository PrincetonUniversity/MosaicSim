#ifndef LSQ_H
#define LSQ_H
#include "../common.h"
using namespace std;

class DynamicNode;
class DynamicNodePointerCompare;
class Core;

class LoadStoreQ {
public:
  Core* core;
  deque<DynamicNode*> lq;
  deque<DynamicNode*> sq;
  unordered_map<uint64_t, set<DynamicNode*, DynamicNodePointerCompare>> lm;
  //program order sorted set of dynamic nodes (loads) that access an address
  unordered_map<uint64_t, set<DynamicNode*, DynamicNodePointerCompare>> sm;
  //program order sorted set of dynamic nodes (stores) that access an address
  set<DynamicNode*, DynamicNodePointerCompare> us;
  set<DynamicNode*, DynamicNodePointerCompare> ul;
  set<DynamicNode*,DynamicNodePointerCompare> unresolved_st_set;
  //program order sorted set of dynamic nodes (stores) whose addresses are unresolved
  set<DynamicNode*,DynamicNodePointerCompare> unresolved_ld_set;

  int size;
  bool set_processed = false;

  LoadStoreQ(Core* thecore);
  void resolveAddress(DynamicNode *d);
  void insert(DynamicNode *d);
  bool checkSize(int num_ld, int num_st);
  bool check_unresolved_load(DynamicNode *in);
  bool check_unresolved_store(DynamicNode *in);
  int check_load_issue(DynamicNode *in, bool speculation_enabled);
  bool check_store_issue(DynamicNode *in);
  int check_forwarding (DynamicNode* in);

  std::vector<DynamicNode*> check_speculation (DynamicNode* in);
};
#endif
