#ifndef GRAPH_H
#define GRAPH_H
#include "../common.hpp"


using namespace std;

/** \brief  Type if edges (dependencies). 

    It can be either a data dependecy or a value (Phi) dependecy.
 */
typedef enum {DATA_DEP, PHI_DEP} TEdge;

/** \brief Node represents one instruction, together with its
    dependecies.

    \todo nodes should be only "containers" carring some basic stuff. 
    All the dependecies should be taken care by BB and Graph class.
 */
class Node {
public:
  /** \brief Node's ID  */
  int id;
  /** \brief Type of instruction  */
  TInstr typeInstr;
  /** \brief BasicBlock ID   */
  int bbid;
  /** \brief String that contains the name of the instruction.  */
  string name;
  /** \brief   */
  int lat;
  /** \brief   */
  int width;
  /** \brief Nodes that depend on this instruction  */
  set<Node*> dependents;
  /** \brief  Node's parents */
  set<Node*> parents;
  /** \brief   */
  set<Node*> external_dependents;
  /** \brief   */
  set<Node*> external_parents;
  /** \brief   */
  set<Node*> phi_dependents;
  /** \brief   */
  set<Node*> phi_parents;
  /** \brief   */
  set<Node*> store_addr_dependents;

  /** \brief Default constructor  */
 Node(int id, TInstr typeInstr, int bbid, string node_name, int lat): 
  id(id), typeInstr(typeInstr), bbid(bbid), name(node_name), lat (lat) {}
  /** \brief Insert the #dest node to the correct set of dependecy and
      also inserts itself to do destination node as parent. */
  void addDependent(Node *dest, TEdge type);
  /** \brief Erase the #dest from  the correct set of dependecy and
      also erases itself as a parent in the #dest node  */
  void eraseDependent(Node *dest, TEdge type);
  /** \brief  Addapted output for Node */
  friend ostream &operator<<(ostream &os, Node &n);
};

/** \brief A Basic block  of isnructions
    
    \todo deal with nodes dependecies here
 */
class BasicBlock {
public:
  /** \brief   */
  vector<Node*> inst;
  /** \brief   */
  int id;
  /** \brief   */
  unsigned int inst_count;
  /** \brief   */
  unsigned int ld_count;
  /** \brief   */
  unsigned int st_count;

  /** \brief   */
 BasicBlock(int id): id(id), inst_count(0), ld_count(0), st_count(0) {}
  /** \brief Adds a new instruction. Counts the number stores and
      loads. */
  void addInst(Node* n);
};

/** \brie Contains a map of nodes and BBs 

    \todo For now, this class looks like an interface for the 
    Node class. Many attributes are duplicated. 
 */
class Graph {
public:
  /** \brief   */
  map<int, Node *> nodes;
  /** \brief   */
  map<int, BasicBlock*> bbs;

  /** \brief   */
  ~Graph() { eraseAllNodes(); } 
  /** \brief   */
  void addBasicBlock(int id);
  /** \brief   */
  void addNode(int id, TInstr type, int bbid, string name, int lat, int vecWidth=1);
  /** \brief   */
  Node *getNode(int id);
  /** \brief   */
  void eraseNode(Node *n);
  /** \brief   */
  void eraseAllNodes();
  /** \brief   */
  void addDependent(Node *src, Node *dest, TEdge type) ;
  /** \brief   */
  void eraseDependent(Node *src, Node *dest, TEdge type);
  /** \brief   */
  friend ostream &operator<<(ostream &os, Graph &g);
};
#endif
