//=======================================================================
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
class Node;
typedef enum {NAI, ADD, SUB, LOGICAL, MULT, DIV, LD, ST, BR_COND, BR_UNCOND, PHI} TInstr;
typedef enum {data_dep, always_mem_dep, maybe_mem_dep, cf_dep, phi_dep} TEdge;

class BasicBlock {
public:
   int id;
   std::vector<Node*> inst;   
   BasicBlock(int id) : id(id) {}
   void addInst(Node* n) {
      inst.push_back(n);
   }
};

// TJH: Probably won't really need edge class 
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
   int parent_count;
   enum {instr, special} type;   // indicates the "type" of node
   int instr_id;
   int instr_lat;
   TInstr instr_type;
   std::string instr_name;
   bool visited;
   int bb_id;

   // Constructor for an "instruction"-type Node -----------------------------
   Node(int id, int lat, TInstr type, std::string name, int bb_id):
            parent_count(0), type(instr), instr_id(id), instr_lat(lat), instr_type(type), 
            instr_name(name), visited(false), bb_id(bb_id) {}
            
   // Constructor for a "special" Node (ie, a BB's entry/exit point)----------
   Node() : parent_count(0), type(special), instr_id(0), instr_lat(0), instr_type(NAI), 
            instr_name("special"), visited(false), bb_id(-1) {}

   // -----------------------------------------------------------------------
   void addDependent(Node *dest, TEdge type) {
      Edge e(this, dest, type);
      dependents.insert(e); 
      dest->parent_count++; 
   }

   // -----------------------------------------------------------------------
   void eraseDependent(Node *dest, TEdge type) {
      std::set<Edge>::iterator it;
      it = std::find( dependents.begin(), dependents.end(), Edge(this,dest,type) );
      if ( it != dependents.end() ) {  // found
         dependents.erase(*it);
         dest->parent_count--; 
      }
   }
   /*
   // Outputing a Node -----------------------------------------------------------
   friend std::ostream &operator<<(std::ostream &os, Node &n) {
      os << "I[" << n.instr_name << "], lat=" << n.instr_lat << ", Deps = {";
      std::set<Edge>::iterator it;
      for (  it = n.dependents.begin(); it != n.dependents.end(); ++it )
         std::cout << "[" << it->dst->instr_name << "], ";
      std::cout << "}";
   }

   // Recursive function for Depth First Search ------------------------------------
   int calculate_accum_latency_from_me() {
      int lat = 0;
      
      if ( !visited ) {
         visited = true;
         lat = instr_lat;
         
         // Recursively traverse ALL dependents
         std::set<Edge>::iterator it;
         for ( it = dependents.begin(); it != dependents.end(); ++it )
            lat += it->dst->calculate_accum_latency_from_me();
      }
      return lat;
   }

   // Recursive function for doing a Topological Sort traversal -----------------------
   void topologicalSort(std::stack<Node *> &Stack) {
      
      if ( !visited ) {
         visited = true;

         // Recursively traverse ALL dependents
         std::set<Edge>::iterator it;
         for (  it = dependents.begin(); it != dependents.end(); ++it )
            if ( !visited ) {
               it->dst->topologicalSort(Stack); std::cout << " a ";
            }

         // Push current Node into the stack
         Stack.push(this);        
      }
   }*/
};

// ******************************************************************************
class Graph {
public:
   std::set<Node *> nodes;

   int get_num_nodes() { return nodes.size(); }
   int size() { return nodes.size(); }

   // Destructor -----------------------------------------------------------------
   ~Graph() { eraseAllNodes(); } 

   // Adding an "instruction"-type Node ------------------------------------------
   Node *addNode(int id, int lat, TInstr type, std::string name, int bid) {
      Node *n = new Node(id, lat, type, name, bid);
      nodes.insert(n);
      return n;
   }

   // Adding a "special"-type Node -----------------------------------------------
   Node *addNode() {
      Node *n = new Node;
      nodes.insert(n);
      return n;
   }

   // Return an exsisting NODE given an <instr_id> ---------------------------------
   Node *getNode(int id) {
      // search the Node with <instr_id> == <id>
      for ( std::set<Node *>::iterator it = nodes.begin(); it != nodes.end(); ++it )
         if ( (*it)->instr_id == id )
            return *it;
      return NULL;  // not found -> but this should not happen !!!
   }

   // --------------------------------------------------------------------------------
   void eraseNode(Node *n) { 
      if (n) {
         nodes.erase( n ); 
         delete n;
      }
   }

   // --------------------------------------------------------------------------------
   void eraseAllNodes() { 
      for ( std::set<Node *>::iterator it = nodes.begin(); it != nodes.end(); ++it )
         eraseNode(*it);
   }

   // --------------------------------------------------------------------------------
   void addDependent(Node *src, Node *dest, TEdge type) {
      src->addDependent(dest, type);
   }

   // --------------------------------------------------------------------------------
   void eraseDependent(Node *src, Node *dest, TEdge type) {
      src->eraseDependent(dest, type);
   }

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

   // --------------------------------------------------------------------------------
   void clear_all_visits() {
      for ( std::set<Node *>::iterator it = nodes.begin(); it != nodes.end(); ++it )
         (*it)->visited = false;
   }

   // Graph's accumulated latency by doing a DFS search -----------------------------------
   int calculate_accum_latency() {
      clear_all_visits();
      Node *n = (*nodes.begin()); // FIXME: maybe we need to locate "a correct entry point"
      return n->calculate_accum_latency_from_me();
   }

   // Make a Topological Sorting ----------------------------------------------------------
   void make_topological_sort( std::stack<Node *> &Stack ) {
      clear_all_visits();
      
      // traverse ALL nodes one by one AND call the recursive helper function
      std::set<Node *>::iterator it;
      for (  it = nodes.begin(); it != nodes.end(); ++it )
         if ( ! (*it)->visited )
            (*it)->topologicalSort(Stack);
   }*/
};

}
