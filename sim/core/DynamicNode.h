#ifndef DYNAMICNODE_H
#define DYNAMICNODE_H
#include <map>
#include <set>
#include <queue>
#include "../graph/Graph.h"

using namespace std;

class Graph;
class BasicBlock;
class Node;
class Core;
class DynamicNode;
class Context;
typedef pair<DynamicNode*, uint64_t> Operator;

class DynamicNode {
public:
  Node *n;
  Context *c;
  Core *core;
  TInstr type;
  bool issued = false;
  bool completed = false;
  bool isMem;
  bool isDESC;
  /* Memory */
  uint64_t addr;
  bool addr_resolved = false;
  bool speculated = false;
  int outstanding_accesses = 0;
  /* Depedency */
  int pending_parents;
  int pending_external_parents;
  vector<DynamicNode*> external_dependents;

  DynamicNode(Node *n, Context *c, Core *core, uint64_t addr = 0);
  bool operator== (const DynamicNode &in);
  bool operator< (const DynamicNode &in) const;
  void print(string str, int level = 0);
  void handleMemoryReturn();
  void tryActivate(); 
  bool issueMemNode();
  bool issueCompNode();
  bool issueDESCNode();
  void finishNode();
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
  bool live;
  unsigned int id;
  
  Core* core;
  BasicBlock *bb;

  int next_bbid;
  int prev_bbid;

  map<Node*, DynamicNode*> nodes;
  set<DynamicNode*, DynamicNodePointerCompare> issue_set;
  set<DynamicNode*, DynamicNodePointerCompare> speculated_set;
  set<DynamicNode*, DynamicNodePointerCompare> next_issue_set;
  set<DynamicNode*, DynamicNodePointerCompare> completed_nodes;
  set<DynamicNode*, DynamicNodePointerCompare> nodes_to_complete;

  priority_queue<Operator, vector<Operator>, OpCompare> pq;

  Context(int id, Core* core) : live(false), id(id), core(core) {}
  Context* getNextContext();
  Context* getPrevContext();
  void insertQ(DynamicNode *d);
  void print(string str, int level = 0);
  void initialize(BasicBlock *bb, int next_bbid, int prev_bbid);
  void process();
  void complete();
};



#endif
