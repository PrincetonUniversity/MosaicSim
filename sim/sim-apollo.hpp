 //=======================================================================
// Copyright 2018 University of Princeton.
//
// Project Apollo - simulator
// Authors: 
//=======================================================================

// Header file

#include <set>
#include <iostream>                         
#include <algorithm>
#include <iterator> 

namespace apollo {

typedef enum {add=1, sub, logical, mult, div, ld, st, branch_cond, branch_uncond} TInstr;
typedef enum {always_mem_dep, maybe_mem_dep, cf_dep} TEdge;

class Node;

class Edge {
   public:
      TEdge edge_type;
      Node *src;   
      Node *dst;

      Edge(Node *s, Node *d, TEdge type) : src(s), dst(d), edge_type(type) {}
      
      bool operator< (const Edge &e) const {         
         return (this < &e);
      }
      
      bool operator==(const Edge &e) const {
         return ( this->src == e.src && this->dst == e.dst );
      }
};

class Node {
   private:
      std::set<Edge> dependents;    // set of dependent nodes (defined as Edges) 
      enum {instr, special} type;   // indicates the "type" of node
      int instr_id;
      int instr_lat;
      TInstr instr_type;
      std::string instr_name;

   public:
      // constructor for an "instruction"-type Node
      Node(int id, int lat, TInstr type, std::string name) : 
               instr_id(id), instr_lat(lat), instr_type(type), instr_name(name), type(instr) {}

      // constructor for a "special" Node (ie, a BB's entry/exit point)
      Node() : instr_name("special"), type(special) {}

      void addDependent(Node *dest, TEdge type) {
         Edge e(this, dest, type);
         dependents.insert(e);  
      }
   
      void eraseDependent(Node *dest, TEdge type) {
         std::set<Edge>::iterator it;
         it = std::find( dependents.begin(), dependents.end(), Edge(this,dest,type) );
         if ( it != dependents.end() )  // found
            dependents.erase(*it);
      }

      friend std::ostream &operator<<(std::ostream &os, Node &n) {
         os << "instr[" << n.instr_name << "], Deps = {";
         for ( std::set<Edge>::iterator it = n.dependents.begin(); it != n.dependents.end(); ++it )
            std::cout << "[" << it->dst->instr_name << "], " ;
         std::cout << "}" << std::endl;
      }
};

class Graph {
   private:
      std::set<Node *> nodes;

   public:
      int get_num_nodes() { return nodes.size(); }
   
      // adding an "instruction"-type Node
      Node *addNode(int id, int lat, TInstr type, std::string name) {
         Node *n = new Node( id, lat, type, name);
         nodes.insert(n);
         return n;
      }

      // adding a "special"-type Node
      Node *addNode() {
         Node *n = new Node;
         nodes.insert(n);
         return n;
      }
   
      void eraseNode(Node *n) { 
         nodes.erase( n ); 
         delete n;
      }
   
      void addDependent(Node *src, Node *dest, TEdge type) {
         src->addDependent(dest, type);
      }

      void eraseDependent(Node *src, Node *dest, TEdge type) {
         src->eraseDependent(dest, type);
      }

      friend std::ostream &operator<<(std::ostream &os, Graph &g) {
         os << "Graph: total_nodes=" << g.get_num_nodes() << std::endl;
         int i=0;
         for ( std::set<Node *>::iterator it = g.nodes.begin(); it != g.nodes.end(); ++it )
            std::cout << "node_" << ++i << ": " << **it;
         std::cout << std::endl;
      }
};

}