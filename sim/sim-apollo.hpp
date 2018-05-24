//=======================================================================
// Copyright 2018 University of Princeton.
//
// Project Apollo - simulator
// Authors: 
//=======================================================================

// Header file

#include <iostream>                         
#include <set>
#include <stack>
#include <algorithm>
#include <iterator> 

namespace apollo {

typedef enum {add, sub, logical, mult, div, ld, st, branch_cond, branch_uncond} TInstr;
typedef enum {data_dep, always_mem_dep, maybe_mem_dep, cf_dep} TEdge;

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
   public:
      std::set<Edge> dependents;    // set of dependent nodes (defined as Edges) 
      enum {instr, special} type;   // indicates the "type" of node
      int instr_id;
      int instr_lat;
      TInstr instr_type;
      std::string instr_name;
      bool visited;

      // constructor for an "instruction"-type Node
      Node(int id, int lat, TInstr type, std::string name) : 
               instr_id(id), instr_lat(lat), instr_type(type), instr_name(name), 
               type(instr), visited(false)     {}

      // constructor for a "special" Node (ie, a BB's entry/exit point)
      Node() : instr_name("special"), type(special), visited(false) {}

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
         os << "I[" << n.instr_name << "], lat=" << n.instr_lat << ", Deps = {";
         for ( std::set<Edge>::iterator it = n.dependents.begin(); it != n.dependents.end(); ++it )
            std::cout << "[" << it->dst->instr_name << "], ";
         std::cout << "}";
      }

      // Note this is Depth First Search -> it calculates the accumulated latency of ALL nodes
      int calculate_accum_latency_from_me() {
         int lat = instr_lat;
         if (visited) {
            std::cout << "error! cycle detected\n";
            return 0;
         }
         else visited = true;

         for ( std::set<Edge>::iterator it = dependents.begin(); it != dependents.end(); ++it )
            lat += it->dst->calculate_accum_latency_from_me();
         return lat;
      }

      // A recursive function used by topologicalSort
      void topologicalSort(std::stack<int> &Stack) {
         // Mark the current node as visited.
         if (visited) {
            std::cout << "error! cycle detected\n";
            return;
         }
         else visited = true;

         // Recur for all the nodes depending on this node
         std::set<Edge>::iterator it;
         for (  it = dependents.begin(); it != dependents.end(); ++it )
            if ( !visited )
               it->dst->topologicalSort(Stack);
 
         // Push the laency of current node into the stack
         Stack.push(instr_lat);        
      }
};

class Graph {
   public:
      std::set<Node *> nodes;

      int get_num_nodes() { return nodes.size(); }
   
      // adding an "instruction"-type Node
      Node *addNode(int id, int lat, TInstr type, std::string name) {
         Node *n = new Node(id, lat, type, name);
         nodes.insert(n);
         return n;
      }

      // adding a "special"-type Node
      Node *addNode() {
         Node *n = new Node;
         nodes.insert(n);
         return n;
      }

      // return an exsisting node given an instr_id
      Node *getNode(int id) {
         // search the Node with <instr_id> == <id>
         for ( std::set<Node *>::iterator it = nodes.begin(); it != nodes.end(); ++it )
            if ( (*it)->instr_id == id )
               return *it;
         return NULL;  // not found -> but this should not happen !!!
      }
   
      void eraseNode(Node *n) { 
         if (n) {
            nodes.erase( n ); 
            delete n;
         }
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
            std::cout << "node_" << i++ << ": " << **it << std::endl;
         std::cout << "";
      }

      void clear_all_visits() {
         for ( std::set<Node *>::iterator it = nodes.begin(); it != nodes.end(); ++it )
            (*it)->visited = false;
      }

      // This is a DFS search (eg., if you simply want to make sure you visit ALL the nodes but 
      //    do not need a special visiting order)
      int calculate_accum_latency() {
         clear_all_visits();
         Node *n = (*nodes.begin()); // FIXME: maybe we need to locate "a correct entry point"
         return n->calculate_accum_latency_from_me();
      }

      // Calculating the "Critical Path" -> for that we use a Topological sorting
      int calculate_critical_path() {
         std::stack<int> Stack;
         int cp_length = 0;

         clear_all_visits();

         // Call the recursive helper function to store Topological
         // traverse ALL nodes one by one and call the recursive helper function
         for ( std::set<Node *>::iterator it = nodes.begin(); it != nodes.end(); ++it )
            if ( ! (*it)->visited )
               (*it)->topologicalSort(Stack);

         // traverse the STACK and accumulate latencies
         while (Stack.empty() == false) {
            std::cout << Stack.top() << " ";
            Stack.pop();
         }
         return cp_length;
      }
};

}