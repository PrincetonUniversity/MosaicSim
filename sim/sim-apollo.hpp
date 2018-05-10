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

typedef enum {add, sub, mult, div, ld, st, branch_cond, branch_uncond} TInstr;
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
      TInstr instr_type;
      int instr_id;
      std::string instr_name;
      std::set<Edge> dependents;
   
   public:
      Node(TInstr type, int id, std::string name) : 
               instr_type(type), instr_id(id), instr_name(name) {}
   
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

      friend std::ostream &operator<<(std::ostream &os, const Node &n) {
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
      Graph() {}
   
      int get_num_nodes() { return nodes.size(); }
   
      Node *addNode(TInstr instr_type, int instr_id, std::string instr_name = "") {
         Node *n = new Node(instr_type, instr_id, instr_name);
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