#ifndef DYNAMICNODE_H
#define DYNAMICNODE_H
#include <map>
#include <set>
#include <queue>
#include "../graph/Graph.h"
#include <unordered_set>

using namespace std;

//class DESCQ;
class Graph;
class BasicBlock;
class Node;
class Core;
class DynamicNode;
class Context;
class ExampleTransaction;
typedef pair<DynamicNode*, uint64_t> Operator;
typedef enum {NONE, DESC_FWD, LSQ_FWD, PENDING, RETURNED, FWD_COMPLETE} MemStatus; //pending: sent to mem system, returned: was sent and has returned back, none: no mem access required, desc_fwd:can be handled by decoupled stl fwding, lsq_fwd: can be handled by lsq forwarding
typedef enum {DISPATCH, LEFT_ROB} Stage; //stage of "pipeline" of dynamic node 

class DynamicNode {
public:
  Node *n;
  uint64_t windowNumber;
  Context *c;
  Core *core;
  TInstr type;
  bool issued = false;
  bool completed = false;
  bool can_exit_rob = false;
  Stage stage=DISPATCH; 
  MemStatus mem_status = NONE;
  string acc_args;
  bool isMem;
  bool isDESC;
  uint64_t desc_id;
  bool acc_initiated=false;
  ExampleTransaction* t;
  /* Memory */
  uint64_t addr;
  int width;
  bool addr_resolved = false;
  bool speculated = false;
  int outstanding_accesses = 0;
  /* Depedency */
  int pending_parents;
  int pending_external_parents;
  vector<DynamicNode*> external_dependents;
  
  DynamicNode(Node *n, Context *c, Core *core, uint64_t addr = 0);
  /*~DynamicNode() {
    //delete from lsq
    core->lsq.remove(d);
  }*/
  bool operator== (const DynamicNode &in);
  bool operator< (const DynamicNode &in) const;
  void print(string str, int level = 0);
  void handleMemoryReturn();
  void tryActivate(); 
  bool issueMemNode();
  bool issueCompNode();
  bool issueDESCNode();
  bool issueAccNode();
  void finishNode();
  void register_issue_try();
  void register_issue_success(); 
};

class OpCompare {
public:
  bool operator() (const Operator &l, const Operator &r) const {
    if(r.second < l.second) 
      return true;
    else if (l.second == r.second)
      return (*(r.first) < *(l.first));
    else
      return false;
  }
};

class DynamicNodePointerCompare {
public:
  bool operator() (const DynamicNode* l, const DynamicNode* r) const {
     return *l < *r;
  }
};

class Context {
public:
  unsigned int id;
  bool live;
  bool isErasable=false;
  uint64_t cycleMarkedAsErasable=0;

  Core* core;
  BasicBlock *bb;

  int next_bbid;
  int prev_bbid;

  map<Node*, DynamicNode*> nodes;
  set<DynamicNode*, DynamicNodePointerCompare> issue_set;
  set<DynamicNode*, DynamicNodePointerCompare> speculated_set;
  set<DynamicNode*, DynamicNodePointerCompare> next_issue_set;
  unordered_set<DynamicNode*/*, DynamicNodePointerCompare*/> completed_nodes;
  unordered_set<DynamicNode*/*, DynamicNodePointerCompare*/> nodes_to_complete;

  priority_queue<Operator, vector<Operator>, OpCompare> pq;

  Context(int id, Core* core) : id(id), live(false), core(core) {}
  Context* getNextContext();
  Context* getPrevContext();
  void insertQ(DynamicNode *d);
  void print(string str, int level = 0);
  void initialize(BasicBlock *bb, int next_bbid, int prev_bbid);
  void process();
  void complete();
};

#endif
