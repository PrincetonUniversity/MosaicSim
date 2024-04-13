#pragma once

#include "../graph/Graph.hpp"
#include <unordered_set>

using namespace std;

class Graph;
class BasicBlock;
class Node;
class Core;
class DynamicNode;
class Context;
class ExampleTransaction;

/** \brief   */
typedef pair<DynamicNode*, uint64_t> Operator;
/** \brief Provides the memory status
    
    pending: sent to mem system
    returned: was sent and has returned back 
    none: no mem access required 
    desc_fwd:can be handled by decoupled stl fwding 
    lsq_fwd: can be handled by lsq forwarding
 */
typedef enum {NONE, DESC_FWD, LSQ_FWD, PENDING, RETURNED, FWD_COMPLETE} MemStatus;
/** \brief   Stage of "pipeline" of dynamic node */
typedef enum {DISPATCH, LEFT_ROB} Stage; 

/** \brief   */
class DynamicNode {
public:
  /** \brief   */
  Node *n;
  /** \brief   */
  uint64_t windowNumber;
  /** \brief   */
  Context *c;
  /** \brief   */
  Core *core;
  /** \brief   */
  TInstr type;
  /** \brief   */
  bool issued = false;
  /** \brief   */
  bool completed = false;
  /** \brief   */
  bool can_exit_rob = false;
  /** \brief   */
  Stage stage=DISPATCH; 
  /** \brief   */
  MemStatus mem_status = NONE;
  /** \brief   */
  string acc_args;
  /** \brief   */
  bool isMem;
  /** \brief   */
  bool atomic=false;
  /** \brief   */
  bool requestedLock=false;
  /** \brief   */
  bool isDESC;
  /** \brief   */
  uint64_t desc_id;
  /** \brief   */
  ExampleTransaction* t;
  /** \brief branches extra latency or caches extra latency */
  int extra_lat=0;
  /** \brief branches misspredicetd */
  bool mispredicted=true;
  /** \brief Partial Barrier's size */
  int partial_barrier_size;
  /** \brief Partial Barrier's id */
  int partial_barrier_id;
  
  /* Memory */
  /** \brief   */
  uint64_t addr;
  /** \brief   */
  int width;
 
  /** \brief   */
  bool addr_resolved = false;
  /** \brief   */
  bool speculated = false;
  /** \brief   */
  int outstanding_accesses = 0;
  /** \brief   */
  int pending_parents;
  /** \brief   */
  int pending_external_parents;
  /** \brief   */
  vector<DynamicNode*> external_dependents;
  /** \brief Verbosity level */ 
  int &verbosity;
  
  /** \brief   */
  DynamicNode(Node *n, Context *c, Core *core,  uint64_t addr = 0);
  /** \brief   */
  bool operator== (const DynamicNode &in);
  /** \brief   */
  bool operator< (const DynamicNode &in) const;
  /** \brief   */
  void print(string str, int level = 0);
  /** \brief   */
  void handleMemoryReturn();
  /** \brief   */
  void tryActivate(); 
  /** \brief   */
  bool issueMemNode();
  /** \brief   */
  bool issueCompNode();
  /** \brief   */
  bool issueDESCNode();
  /** \brief   */
  bool issueAccNode();
  /** \brief   */
  void finishNode();
  /** \brief   */
  void register_issue_try();
  /** \brief   */
  void register_issue_success();
};

class OpCompare {
public:
  /** \brief   */
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
  /** \brief   */
  bool operator() (const DynamicNode* l, const DynamicNode* r) const {
     return *l < *r;
  }
};

class Context {
public:
  /** \brief   */
  uint64_t id;
  /** \brief   */
  bool live;
  /** \brief   */
  bool isErasable=false;
  /** \brief   */
  uint64_t cycleMarkedAsErasable=0;
  /** \brief Verbosity  level */
  int verbosity;
  
  /** \brief   */
  Core* core;
  /** \brief   */
  BasicBlock *bb;

  /** \brief   */
  int next_bbid;
  /** \brief   */
  int prev_bbid;

  /** \brief   */
  map<Node*, DynamicNode*> nodes;
  /** \brief   */
  set<DynamicNode*, DynamicNodePointerCompare> issue_set;
  /** \brief   */
  set<DynamicNode*, DynamicNodePointerCompare> speculated_set;
  /** \brief   */
  set<DynamicNode*, DynamicNodePointerCompare> next_issue_set;
  /** \brief   */
  unordered_set<DynamicNode*/*, DynamicNodePointerCompare*/> completed_nodes;
  /** \brief   */
  unordered_set<DynamicNode*/*, DynamicNodePointerCompare*/> nodes_to_complete;
  /** \brief   */
  priority_queue<Operator, vector<Operator>, OpCompare> pq;
  /** \brief vector contating new DynamicNode to be insert.
      
      This was added to create a thread safe insertQ */
  DynamicNode *to_insert;

  /** \brief   */
  Context(uint64_t id, Core* core, int verbosity) :
    id(id), live(false), core(core), verbosity(verbosity), to_insert(nullptr) {}
  /** \brief   */
  Context* getNextContext();
  /** \brief   */
  Context* getPrevContext();
  /** \brief   */
  void insertQ(DynamicNode *d);
  /** \brief   */
  void print(string str, int level = 0);
  /** \brief   */
  void initialize(BasicBlock *bb, int next_bbid, int prev_bbid);
  /** \brief   */
  void process();
  /** \brief   */
  void complete();
};

