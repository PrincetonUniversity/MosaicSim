//=======================================================================
// Copyright 2018 Princeton University.
//=======================================================================

// Header file

#include <iostream>                         
#include <set>
#include <map>
#include <vector>
#include <queue>
#include <iterator>
#include <iostream>
#include <fstream>
#include <sstream> 
#include <cstdio> 
#include <string>  
#include "assert.h"

using namespace std;
namespace apollo {
class Node;
typedef std::pair<Node*,int> DNode;
typedef enum {NAI, ADD, SUB, LOGICAL, MULT, DIV, LD, ST, TERMINATOR, PHI} TInstr;
typedef enum {data_dep, bb_dep, phi_dep} TEdge;


class Node {
public:
   std::set<Node*> dependents;
   std::set<Node*> parents;
   std::set<Node*> external_dependents;
   std::set<Node*> external_parents;
   std::set<Node*> phi_dependents;

   int id;
   int lat;
   TInstr type;
   int bbid;
   std::string name;

   Node(int id, TInstr type, int bbid, std::string name): 
                  id(id), type(type), bbid(bbid), name(name) {
      lat = getLatency(type);
   }
   
   // Constructor for the BB's entry point
   Node(int bbid) : id(-1), lat(0), type(NAI), bbid(bbid), name("BB-Entry") {}

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
      if(type == data_dep || type == bb_dep) {
         if(dest->bbid == this->bbid) {
            dependents.insert(dest);
            dest->parents.insert(this);
         }
         else {
            external_dependents.insert(dest);
            dest->external_parents.insert(this);
         }
      }
      else if(type == phi_dep) {
         phi_dependents.insert(dest);
      }
   }

   void eraseDependent(Node *dest, TEdge type) {
      int count = 0;
      if(type == data_dep || type == bb_dep) {
         if(dest->bbid == this->bbid) {
            count += dependents.erase(dest);
            dest->parents.erase(dest);
         }
         else {
            count += external_dependents.erase(dest);
            dest->external_parents.erase(dest);
         }
      }
      else if(type == phi_dep) {
         count += phi_dependents.erase(dest);
      }
      assert(count == 1);
   }

   // Print Node
   friend std::ostream& operator<<(std::ostream &os, Node &n) {
      os << n.name;
      //os << "I[" << n.name << "], lat=" << n.lat << ", Deps = {";
      //std::set< std::pair<Node*,TEdge> >::iterator it;
      //for (  it = n.dependents.begin(); it != n.dependents.end(); ++it )
      //   std::cout << "[" << it->first->name << "], ";
      //std::cout << "}";
      return os;
   }

};

class BasicBlock {
   public:
      int id;
      int inst_count;
      std::vector<Node*> inst;
      Node *entry;

      BasicBlock(int id): id(id), inst_count(0) {
         entry = new Node(id);
      }

      void addInst(Node* n) {
         assert(entry != NULL);
         entry->addDependent(n, bb_dep);
         inst.push_back(n);
         inst_count++;
      }
};
class Graph {
   public:
      std::map<int, Node *> nodes;
      std::map<int, BasicBlock*> bbs;
      int initialBlock;
      Graph() : initialBlock(0) { }
      ~Graph() { eraseAllNodes(); } 

      void addBasicBlock(int id) {
         bbs.insert( std::make_pair(id, new BasicBlock(id)) );
      }

      Node *addNode(int id, TInstr type, int bbid, std::string name) {
         Node *n = new Node(id, type, bbid, name);
         nodes.insert(std::make_pair(n->id, n));
         assert(bbs.find(bbid)!= bbs.end());
         bbs.at(bbid)->addInst(n);
         return n;
      }

      // Return an exsisting node given an instruction <id> 
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

      // Print Graph
      friend std::ostream &operator<<(std::ostream &os, Graph &g) {
         os << "Graph: Total_nodes=" << g.nodes.size() << std::endl;
         for (std::map<int, Node *>::iterator it = g.nodes.begin(); it != g.nodes.end(); ++it)
            std::cout << it->first << ":" << *it->second << std::endl;
         std::cout << "";
         return os;
      }

   };

vector<string> split(const string &s, char delim) {
    stringstream ss(s);
    string item;
    vector<string> tokens;
    while (getline(ss, item, delim)) {
        tokens.push_back(item);
    }
    return tokens;
}

void readCF(std::vector<int> &cf)
{
   string line;
   string last_line;
   ifstream cfile ("input/ctrl.txt");
   int last_bbid = -1;
   if (cfile.is_open()) {
    while (getline (cfile,line)) {
      vector<string> s = split(line, ',');
      assert(s.size() == 3);
      if (stoi(s.at(1)) != last_bbid && last_bbid != -1) {
         cout << last_bbid << " / " << s.at(1) << "\n";
         cout << last_line << " / " << line << "\n";
         assert(false);
      }
      last_bbid = stoi(s.at(2));
      cf.push_back(stoi(s.at(2)));
      last_line = line;
    }
   }
   cfile.close();
}

void readMemory(std::map<int, std::queue<uint64_t> > &memory)
{
   string line;
   string last_line;
   ifstream cfile ("input/memory.txt");
   if (cfile.is_open()) {
    while (getline (cfile,line)) {
      vector<string> s = split(line, ',');
      assert(s.size() == 4);
      int id = stoi(s.at(1));
      if(memory.find(id) == memory.end())
         memory.insert(make_pair(id, queue<uint64_t>()));
      memory.at(id).push(stoull(s.at(2)));
    }
   }
   cfile.close();
}
void readToyGraph(Graph &g)
{
   Node *nodes[10];  
   int id = 1;
   nodes[1] = g.addNode(id++, ADD, 0, "1-add $1,$3,$4");
   nodes[2] = g.addNode(id++, LD, 0,"2-LD $1,$3,$4");
   nodes[3] = g.addNode(id++, LOGICAL, 0,"3-xor $1,$3,$4");
   nodes[4] = g.addNode(id++, DIV, 0,"4-mult $1,$3,$4");
   nodes[5] = g.addNode(id++, SUB, 0,"5-sub $1,$3,$4");
   nodes[6] = g.addNode(id++, LOGICAL, 0,"6-xor $1,$3,$4");

   // add some dependents
   nodes[1]->addDependent(nodes[2], data_dep);
   nodes[1]->addDependent(nodes[3], data_dep);
   nodes[1]->addDependent(nodes[4], data_dep);
   nodes[2]->addDependent(nodes[5], data_dep);
   nodes[2]->addDependent(nodes[6], data_dep);
   nodes[6]->addDependent(nodes[4], data_dep);
   g.initialBlock = 0;
   cout << g;
}
void readGraph(Graph &g)
{
   ifstream cfile ("input/graph.txt");
   if (cfile.is_open()) {
      string temp;
      getline(cfile,temp);
      g.initialBlock = stoi(temp);
      getline(cfile,temp);
      int numBB = stoi(temp);
      getline(cfile,temp);
      int numNode = stoi(temp);
      getline(cfile,temp);
      int numEdge = stoi(temp);
      string line;
      for (int i=0; i<numBB; i++)
         g.addBasicBlock(i);
      for(int i=0; i<numNode; i++) {
         getline(cfile,line);
         vector<string> s = split(line, ',');
         int id = stoi(s.at(0));
         TInstr type = static_cast<TInstr>(stoi(s.at(1)));
         int bbid = stoi(s.at(2));
         string name = s.at(3);
         name = s.at(3).substr(0, s.at(3).size()-1);
         g.addNode(id, type, bbid, name);
      }
      for(int i=0; i<numEdge; i++) {
         getline(cfile,line);
         vector<string> s = split(line, ',');
         TEdge type = static_cast<TEdge>(stoi(s.at(2)));
         g.addDependent(g.getNode(stoi(s.at(0))), g.getNode(stoi(s.at(1))), type);
      }
      if(getline(cfile,line))
         assert(false);
   }
   else
      assert(false);
   cfile.close();
   g.initialBlock = 0;
   cout << g;
}
}


