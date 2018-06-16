//=======================================================================
// Copyright 2018 Princeton University.
//=======================================================================

// Header file

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
typedef std::pair<Node*,int> DNode;
typedef enum {NAI, ADD, SUB, LOGICAL, MULT, DIV, LD, ST, TERMINATOR, PHI} TInstr;
typedef enum {data_dep, bb_dep, cf_dep, phi_dep} TEdge;

class Node {
public:
   std::set< std::pair<Node*,TEdge> > dependents; // TODO: Make it map of sets (type, dst)
   std::set< std::pair<Node*,TEdge> > external_parents;
   int id;
   int lat;
   TInstr type;
   int bbid;
   std::string name;
   int n_parents;
   int n_external_parents;
   int n_external_edges;

   Node(int id, TInstr type, int bbid, std::string name): 
                  id(id), type(type), bbid(bbid), name(name), n_parents(0), n_external_parents(0), n_external_edges(0)  {
      lat = getLatency(type);
      if(type == PHI)
         n_parents = 1;
   }
   
   // Constructor for the BB's entry point
   Node(int bbid) : id(-1), lat(0), type(NAI), bbid(bbid), name("BB-Entry"),  n_parents(0), n_external_parents(0), n_external_edges(0) {}

   // TODO: get latencies from a CONFIG file instead
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
      dependents.insert(std::make_pair(dest,type));
      if(type == data_dep || type == bb_dep) {
         if(dest->bbid == this->bbid)
            dest->n_parents++;
         else {
            n_external_edges++;
            dest->n_external_parents++;
            dest->external_parents.insert(std::make_pair(this,type));
         }
      }
   }

   void eraseDependent(Node *dest, TEdge type) {
      int count = 0;
      std::set< std::pair<Node*, TEdge> >::iterator it;
      for (it = dependents.begin(); it != dependents.end(); ++it) {
         Node *d = it->first;
         TEdge t = it->second;
         if (dest == d && type == t) {
            dependents.erase(*it);
            if(t == data_dep || t == bb_dep) {
               if(dest->bbid == this->bbid)
                  dest->n_parents--;
               else {
                  dest->external_parents.erase(std::make_pair(dest,type));
                  dest->n_external_parents--;
                  n_external_edges--;
               }
            }
            count++;
         }
      }
      if (count != 1)
         assert(false);
   }

   // Print
   friend std::ostream& operator<<(std::ostream &os, Node &n) {
      os << "I[" << n.name << "], lat=" << n.lat << ", Deps = {";
      std::set< std::pair<Node*,TEdge> >::iterator it;
      for (  it = n.dependents.begin(); it != n.dependents.end(); ++it )
         std::cout << "[" << it->first->name << "], ";
      std::cout << "}";
      return os;
   }

};

class BasicBlock {
   public:
      int id;
      int inst_count;
      std::vector<Node*> inst;
      Node *entry;
      Node *exit; // TJH: May or may not use it

      BasicBlock(int id): id(id), inst_count(0) {
         entry = new Node(id);
      }

      void addInst(Node* n) {
         if (entry == NULL)
            assert(false);
         // JLA: I do not see the need to mark ALL instructions within the BB as a dependant of the entry node
         entry->addDependent(/*node*/ n, /*edge type*/ bb_dep);
         inst.push_back(n);
         inst_count++;
      }
};
class Graph {
   public:
      std::map<int, Node *> nodes;
      std::map<int, BasicBlock*> bbs;
      int get_num_nodes() { return nodes.size(); }
      int size() { return nodes.size(); }

      ~Graph() { eraseAllNodes(); } 

      void addBasicBlock(int id) {
         bbs.insert( std::make_pair(id, new BasicBlock(id)) );
      }

      Node *addNode(int id, TInstr type, int bbid, std::string name) {
         Node *n = new Node(id, type, bbid, name);
         nodes.insert( std::make_pair(n->id, n) );
         
         // find the <bbid> BB and add this node as an "instruction" belonging to that BB
         // JLA: seems kind of reduntant, maybe the BasicBlock class can be defined simply as a 
         //        collection of "entry point" nodes; no need to add every single instruction 
         //        to the BB structure...
         if ( bbs.find(bbid) == bbs.end() )
            assert(false);  // if <bbid> not found it's an error
         bbs.at(bbid)->addInst(n);
         return n;
      }

      // Return an exsisting NODE given an instruction <id> 
      Node *getNode(int id) {
         if ( nodes.find(id) != nodes.end() )
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

      // Print
      friend std::ostream &operator<<(std::ostream &os, Graph &g) {
         os << "Graph: total_nodes=" << g.get_num_nodes() << std::endl;
         int i=0;
         for ( std::map<int, Node *>::iterator it = g.nodes.begin(); it != g.nodes.end(); ++it )
            std::cout << "node_" << i++ << ": " << *it->second << std::endl;
         std::cout << "";
         return os;
      }

   };

}
