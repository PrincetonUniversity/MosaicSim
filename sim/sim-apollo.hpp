//=======================================================================
// Copyright 2018 University of Princeton.
//
// Project Apollo - simulator
// Authors: 
//=======================================================================

// Header file

using namespace std;

#include <set>
#include <vector>
#include <iostream>                         
#include <algorithm>
#include <iterator> 

namespace apollo {

typedef enum {add, sub, mult, div, ld, st, branch_cond, branch_uncond} TInstr;
typedef enum {true_mem_dep, maybe_mem_dep, CF_dep} TEdge;

class Node;

class Edge {
   private:
      TEdge edge_type;
      Node *src;
      Node *dst;

   public:
      Edge(Node *s, Node *d, TEdge type) : src(s), dst(d), edge_type(type) {}
};


class Node {
   private:
      int instr_id;
      std::string instr_name;
      TInstr instr_type;
      std::set<Edge *> adjs;
   
   public:
      Node(TInstr type, int id) : instr_type(type), instr_id(id) {}
   
      void insertEdge(Edge *e) {
         adjs.insert(e);  
      }
   
      void eraseEdge(Edge *e) {
         adjs.erase( e );
      }
   
      void insertEdge(Node *dest, TEdge type) {
         adjs.insert( new Edge(this, dest, type) );  
      }
   
      void eraseEdge(Node *dest) {
         Edge *e; // = adjs.find();
         eraseEdge(e);
         delete e;
      }
   
      friend std::ostream &operator<<(std::ostream &os, const Node &n) {
         os << n.instr_name << ": ";
         for ( std::set<Edge *>::iterator it = n.adjs.begin(); it != n.adjs.end(); ++it )
            std::cout << ", " << it->dst.instr_name;

         cout << endl;
      }
};


class Graph {
   private:
      std::set<Node *> nodes;

   public:
      Graph() {}
   
      int get_num_nodes() { return nodes.size(); }
   
      void insertNode(TInstr instr_type, int instr_id, std::string instr_name = "") {
         nodes.insert( new Node(instr_type, instr_id) );
      }
   
      void eraseNode(Node *n) { 
         nodes.erase( n ); 
         delete n;
      }
   
      void insertEdge(Node *s, Node *d, TEdge type) {
         s->insertEdge(d, type);
      }

      void eraseEdge(Node *s, Node *d) {
         s->eraseEdge(d);
      }
};

}