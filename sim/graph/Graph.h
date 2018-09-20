#ifndef GRAPH_H
#define GRAPH_H
#include "../common.h"

using namespace std;

typedef enum {DATA_DEP, PHI_DEP} TEdge;
typedef enum {I_ADDSUB, I_MULT, I_DIV, I_REM, FP_ADDSUB, FP_MULT, FP_DIV, FP_REM, LOGICAL, 
              CAST, GEP, LD, ST, TERMINATOR, PHI, SEND, RECV, STADDR, STVAL, INVALID} TInstr;


class Node {
public:
  int id;
  TInstr typeInstr;
  int bbid;
  string name;
  int lat;
  set<Node*> dependents;
  set<Node*> parents;
  set<Node*> external_dependents;
  set<Node*> external_parents;
  set<Node*> phi_dependents;
  set<Node*> phi_parents;
  set<Node*> store_addr_dependents;

  Node(int id, TInstr typeInstr, int bbid, string node_name, int lat): 
    id(id), typeInstr(typeInstr), bbid(bbid), name(node_name), lat (lat) {}
  
  void addDependent(Node *dest, TEdge type);
  void eraseDependent(Node *dest, TEdge type);
  friend ostream &operator<<(ostream &os, Node &n);
};
class BasicBlock {
public:
  vector<Node*> inst;
  int id;
  unsigned int inst_count;
  unsigned int ld_count;
  unsigned int st_count;
  BasicBlock(int id): id(id), inst_count(0), ld_count(0), st_count(0) {}
  void addInst(Node* n);

};

class Graph {
public:
  map<int, Node *> nodes;
  map<int, BasicBlock*> bbs;
  ~Graph() { eraseAllNodes(); } 
  void addBasicBlock(int id);
  void addNode(int id, TInstr type, int bbid, string name, int lat);
  Node *getNode(int id);
  void eraseNode(Node *n);
  void eraseAllNodes();
  void addDependent(Node *src, Node *dest, TEdge type) ;
  void eraseDependent(Node *src, Node *dest, TEdge type);
  friend ostream &operator<<(ostream &os, Graph &g);
};


#endif
