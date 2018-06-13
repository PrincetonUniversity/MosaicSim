#include <iostream>                         
#include <set>
#include <map>
#include <vector>
#include <stack>
#include <algorithm>
#include <iterator> 
#include "assert.h"

namespace apollo {
class Node;
typedef enum {NAI, ADD, SUB, LOGICAL, MULT, DIV, LD, ST, TERMINATOR, PHI} TInstr;
typedef enum {data_dep, always_mem_dep, maybe_mem_dep, cf_dep, phi_dep} TEdge;

class Context
{
public:
   bool live;
   int id;
   int bid;
   int count;
   Context(int id, int bid) : live(true), id(id), bid(bid), count(0) {}
   std::vector<Node*> active_list;
   std::set<Node*> newly_active_nodes;
   std::map<Node*, int> latency_map;
   std::map<Node*, int> dep_map;
};

class BasicBlock {
public:
   int id;
   int inst_count;
   std::vector<Node*> inst;   
   BasicBlock(int id) : id(id), inst_count(0) {}
   void addInst(Node* n) {
      inst.push_back(n);
      inst_count++;
   }
};

class Edge {
public:
   TEdge type;
   Node *src;   
   Node *dst;
   Edge(Node *s, Node *d, TEdge type) : src(s), dst(d), type(type) {}
   bool operator< (const Edge &e) const {         
      return (this < &e);
   }
   bool operator== (const Edge &e) const {
      return ( this->src == e.src && this->dst == e.dst );
   }
};

class Node {
public:
   std::set<Edge> dependents;    // set of dependent nodes (defined as Edges) 
   int id;
   int lat;
   TInstr type;
   int bid;
   std::string name;
   int parents;
   
   Node(int id, TInstr type, int bid, std::string name): id(id), type(type), bid(bid), name(name), parents(0)  {
      lat = getLatency(type);
   }            
   Node(int bid) : id(-1), lat(0), type(NAI), bid(bid), name("default"),  parents(0) {}

   int getLatency(TInstr type) {
      int lat = -1;
      switch(type) {
         case ADD:
         case SUB:
         case LOGICAL:
            lat = 1;
            break;
         case MULT:
            lat = 3;
            break;
         case DIV:
            lat = 9;
            break;
         case LD:
            lat = -1;
            break;
         case ST:
            lat = 3;
            break;
         case TERMINATOR:
            lat = 1;
            break;
         case PHI:
            lat = 0;
            break;
         case NAI:
            lat = 0;
            break;
         default:
            assert(false);
            break;
      }
      return lat;
   }
   void addDependent(Node *dest, TEdge type) {
      Edge e(this, dest, type);
      dependents.insert(e); 
      dest->parents++; 
   }

   void eraseDependent(Node *dest, TEdge type) {
      std::set<Edge>::iterator it;
      it = std::find( dependents.begin(), dependents.end(), Edge(this,dest,type) );
      if ( it != dependents.end() ) {  // found
         dependents.erase(*it);
         dest->parents--; 
      }
   }
   // FIX
   /*
   // Outputing a Node -----------------------------------------------------------
   friend std::ostream &operator<<(std::ostream &os, Node &n) {
      os << "I[" << n.name << "], lat=" << n.lat << ", Deps = {";
      std::set<Edge>::iterator it;
      for (  it = n.dependents.begin(); it != n.dependents.end(); ++it )
         std::cout << "[" << it->dst->name << "], ";
      std::cout << "}";
   }
   */
};

// ******************************************************************************
class Graph {
public:
   std::map<int, Node *> nodes;
   std::map<int, BasicBlock*> bbs;
   int get_num_nodes() { return nodes.size(); }
   int size() { return nodes.size(); }

   ~Graph() { eraseAllNodes(); } 

   void addBasicBlock(int id) {
       bbs.insert(std::make_pair(id, new BasicBlock(id)));
   }
   // Adding an "instruction"-type Node ------------------------------------------
   Node *addNode(int id, TInstr type, int bid, std::string name) {
      Node *n = new Node(id, type, bid, name);
      nodes.insert(std::make_pair(n->id, n));
      if(bbs.find(bid) == bbs.end())
         assert(false);
      bbs.at(bid)->addInst(n);
      return n;
   }
   // Adding a "special"-type Node -----------------------------------------------
   Node *addNode(int bid) {
      Node *n = new Node(bid);
      nodes.insert(std::make_pair(-1, n));
      return n;
   }
   Node *getNode(int id) {
      if(nodes.find(id) != nodes.end())
         return nodes.at(id);
      else
         return NULL;
   }
   void eraseNode(Node *n) { 
      if (n) {
         nodes.erase(n->id); 
         delete n;
      }
   }
   void eraseAllNodes() { 
      for ( std::map<int, Node *>::iterator it = nodes.begin(); it != nodes.end(); ++it )
         eraseNode(it->second);
   }

   void addDependent(Node *src, Node *dest, TEdge type) {
      src->addDependent(dest, type);
   }
   void eraseDependent(Node *src, Node *dest, TEdge type) {
      src->eraseDependent(dest, type);
   }
   // FIX
   /* 
   // Outputing a Graph --------------------------------------------------------------
   friend std::ostream &operator<<(std::ostream &os, Graph &g) {
      os << "Graph: total_nodes=" << g.get_num_nodes() << std::endl;
      int i=0;
      std::set<Node *>::iterator it;
      for ( it = g.nodes.begin(); it != g.nodes.end(); ++it )
         std::cout << "node_" << i++ << ": " << **it << std::endl;
      std::cout << "";
   } 
   */
};
}
