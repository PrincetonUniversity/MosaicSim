#include <iostream>                         
#include <set>
#include <map>
#include <vector>
#include <stack>
#include <algorithm>
#include <iterator> 
#include "assert.h"

namespace apollo {
typedef enum {NAI, ADD, SUB, LOGICAL, MULT, DIV, LD, ST, TERMINATOR, PHI} TInstr;
typedef enum {data_dep, bb_dep, always_mem_dep, maybe_mem_dep, cf_dep, phi_dep} TEdge;

class Node {
public:
   std::set<std::pair<Node*,TEdge> > dependents; // TODO: Make it map of sets (type, dst)
   int id;
   int lat;
   TInstr type;
   int bid;
   std::string name;
   int parents;
   
   Node(int id, TInstr type, int bid, std::string name): id(id), type(type), bid(bid), name(name), parents(0)  {
      lat = getLatency(type);
   }            
   Node(int bid) : id(-1), lat(0), type(NAI), bid(bid), name("BB-Entry"),  parents(0) {}

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
      dest->parents++; 
   }

   void eraseDependent(Node *dest, TEdge type) {
      int count = 0;
      std::set<std::pair<Node*, TEdge> >::iterator it;
      for ( it = dependents.begin(); it != dependents.end(); ++it ) {
         Node *d = it->first;
         TEdge t = it->second;
         if(dest == d && type == t) {
            dependents.erase(*it);
            dest->parents--;
            count++;
         }
      }
      if(count != 0)
         assert(false);
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
      if(entry == NULL)
         assert(false);
      entry->addDependent(n, bb_dep);
      inst.push_back(n);
      inst_count++;
   }
};

class Context
{
public:
   bool live;
   int id;
   int bid;
   int processed;
   Context(int id) : live(true), id(id), bid(-1), processed(0) {}
   void initialize(BasicBlock *bb) {
      if(bid != -1)
         assert(false);
      bid = bb->id;
      active_list.push_back(bb->entry);
      process_map.insert(std::make_pair(bb->entry, 0));
      for(int i=0; i<bb->inst.size(); i++) {
         Node *n = bb->inst.at(i);
         ready_map.insert(std::make_pair(n, n->parents));
      }
   }
   std::vector<Node*> active_list;
   std::set<Node*> start_set;
   std::map<Node*, int> process_map;
   std::map<Node*, int> ready_map;
};

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
   Node *addNode(int id, TInstr type, int bid, std::string name) {
      Node *n = new Node(id, type, bid, name);
      nodes.insert(std::make_pair(n->id, n));
      if(bbs.find(bid) == bbs.end())
         assert(false);
      bbs.at(bid)->addInst(n);
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
};
}
