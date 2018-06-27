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
typedef std::pair<Node*,int> DNode;  // Dynamic Node: a pair of <node,context>
typedef enum {I_ADD, FP_ADD, I_SUB, FP_SUB, LOGICAL, I_MULT, FP_MULT, I_DIV, FP_DIV, LD, ST, ENTRY, TERMINATOR, PHI} TInstr;
typedef enum {DATA_DEP, BB_DEP, PHI_DEP} TEdge;

// these are for configuration of FUs
typedef enum { FU_I_ALU, FU_FP_ALU, FU_I_MULT, FU_FP_MULT, FU_I_DIV, FU_FP_DIV, FU_BRU,
               FU_IN_MEMPORT, FU_OUT_MEMPORT, FU_OUTSTANDING_MEM, FU_NULL } TypeofFU;

// helper function: given an "instruction" type it returns the type of FU used
TypeofFU getFUtype(TInstr typeInstr) {
  switch(typeInstr) {
    case I_ADD:
    case I_SUB:
    case LOGICAL: return FU_I_ALU;
    case I_MULT:  return FU_I_MULT;
    case FP_MULT: return FU_FP_MULT; 
    case I_DIV:   return FU_I_DIV; 
    case FP_DIV:  return FU_FP_DIV; 
    case TERMINATOR: return FU_BRU;  // Branch Unit
    case LD: return FU_IN_MEMPORT;
    case ST: return FU_OUT_MEMPORT;
    default:
      return FU_NULL;
  }
}

class Resources {
public:

  struct TFU {
    int lat, max, free;
  };
  std::map<TypeofFU, TFU> FUs;
  
  void addFUtype(TypeofFU type, int lat, int max) {
    TFU fu;
    fu.lat  = lat;
    fu.max  = max;
    fu.free = max;
    FUs.insert( std::make_pair(type, fu) );
  }

  void initialize(std::string file) {
    ifstream cfile(file); // TODO: read FU parameters from <file>
    addFUtype(FU_I_ALU,    1, 1);  // format: FU_type, latency, max_units
    addFUtype(FU_FP_ALU,   1, 1);
    addFUtype(FU_I_MULT,   3, 2);
    addFUtype(FU_FP_MULT,  3, 2);
    addFUtype(FU_I_DIV,    9, 2);
    addFUtype(FU_FP_DIV,   9, 2);
    addFUtype(FU_BRU,      1, 2);    
    addFUtype(FU_IN_MEMPORT,  -1, 10);     // IN_MEMPORT (loads) does not have a pre-defined latency (the memory hierarchy gives) 
    addFUtype(FU_OUT_MEMPORT, 3, 10);      // OUT_MEMPORT (stores) 
    addFUtype(FU_OUTSTANDING_MEM, -1, 10); // OUTSTANDING_MEM does not have a latency
  }

  int getInstrLatency(TInstr typeInstr) {
    TypeofFU  t = getFUtype(typeInstr);
    if (t == FU_NULL)
      return 0;
    else
      return FUs.at(t).lat;
  }
};

class Node {
public:
  std::set<Node*> dependents;
  std::set<Node*> parents;
  std::set<Node*> external_dependents;
  std::set<Node*> external_parents;
  std::set<Node*> phi_dependents;

  int id;
  int lat;
  TInstr typeInstr;
  TypeofFU typeFU;
  int bbid;
  std::string name;

  Node(int id, TInstr typeInstr, int bbid, std::string name, int lat): 
            id(id), typeInstr(typeInstr), bbid(bbid), name(name), lat (lat) {
    typeFU = getFUtype(typeInstr);
  } 
  
  // Constructor for the BB's entry point
  Node(int bbid) : id(-1), lat(0), typeInstr(ENTRY), bbid(bbid), name("BB-Entry"), typeFU(FU_NULL) {}

  void addDependent(Node *dest, TEdge type) {
    if(type == DATA_DEP || type == BB_DEP) {
      if(dest->bbid == this->bbid) {
        dependents.insert(dest);
        dest->parents.insert(this);
      }
      else {
        external_dependents.insert(dest);
        dest->external_parents.insert(this);
      }
    }
    else if(type == PHI_DEP) {
      phi_dependents.insert(dest);
    }
  }

  void eraseDependent(Node *dest, TEdge type) {
    int count = 0;
    if(type == DATA_DEP || type == BB_DEP) {
      if(dest->bbid == this->bbid) {
        count += dependents.erase(dest);
        dest->parents.erase(dest);
      }
      else {
        count += external_dependents.erase(dest);
        dest->external_parents.erase(dest);
      }
    }
    else if(type == PHI_DEP) {
      count += phi_dependents.erase(dest);
    }
    assert(count == 1);
  }

  // Print Node
  friend std::ostream& operator<<(std::ostream &os, Node &n) {
    os << "I[" << n.name << "], lat=" << n.lat << ", Deps = {";
    std::set< std::pair<Node*,TEdge> >::iterator it;
//    for (  it = n.dependents.begin(); it != n.dependents.end(); ++it )
//       std::cout << "[" << it->first->name << "], ";
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

    BasicBlock(int id): id(id), inst_count(0) { entry = new Node(id); }
    ~BasicBlock() { delete entry; }
    
    void addInst(Node* n) {
      assert(entry != NULL);
      entry->addDependent(n, BB_DEP);  // all instructions are made dependant of the "entry node"
      inst.push_back(n);
      inst_count++;
    }
};

class Graph {
  public:
    std::map<int, Node *> nodes;
    std::map<int, BasicBlock*> bbs;
    ~Graph() { eraseAllNodes(); } 

    void addBasicBlock(int id) {
      bbs.insert( std::make_pair(id, new BasicBlock(id)) );
    }

    Node *addNode(int id, TInstr type, int bbid, std::string name, int lat) {
      Node *n = new Node(id, type, bbid, name, lat);
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

  // helper function for reading text files
  vector<string> split(const string &s, char delim) {
     stringstream ss(s);
     string item;
     vector<string> tokens;
     while (getline(ss, item, delim)) {
        tokens.push_back(item);
     }
     return tokens;
  }

  // Read Dynamic Control Flow data from profiling file (ctrl.txt)
  // format of ctrl.txt:  
  //      <string_bb_name>,<current_bb_id>,<next_bb_id>
  // argument <cf> will contain the sequential list of executed BBs
  void readProfCF(std::string name, std::vector<int> &cf) {
    string line;
    string last_line;
    ifstream cfile(name);
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
        cf.push_back(last_bbid);
        last_line = line;
      }
    }
    cfile.close();
  }

  // Read Dynamic Memory accesses from profiling file (memory.txt)
  // argument <memory> will contain a map of { <instr_id>, <queue of addresses> }
  void readProfMemory(std::string name, std::map<int, std::queue<uint64_t> > &memory) {
    string line;
    string last_line;
    ifstream cfile(name);
    if (cfile.is_open()) {
      while ( getline(cfile,line) ) {
        vector<string> s = split(line, ',');
        assert(s.size() == 4);
        int id = stoi(s.at(1));
        // if it's a NEW memory instruction, insert it into the <map>
        if (memory.find(id) == memory.end()) 
          memory.insert(make_pair(id, queue<uint64_t>()));
        // insert the <address> into the memory instructions's <queue>
        memory.at(id).push(stoull(s.at(2)));
      }
    }
    cfile.close();
  }

  void readGraph(std::string name, Graph &g, Resources &res) {
    ifstream cfile(name);
    if (cfile.is_open()) {
      string temp;
      getline(cfile,temp);
      int numBB = stoi(temp);
      getline(cfile,temp);
      int numNode = stoi(temp);
      getline(cfile,temp);
      int numEdge = stoi(temp);
      string line;
      for (int i=0; i<numBB; i++)
        g.addBasicBlock(i);
      for (int i=0; i<numNode; i++) {
        getline(cfile,line);
        vector<string> s = split(line, ',');
        int id = stoi(s.at(0));
        TInstr type = static_cast<TInstr>(stoi(s.at(1)));
        int bbid = stoi(s.at(2));
        string name = s.at(3);
        name = s.at(3).substr(0, s.at(3).size()-1);
        g.addNode( id, type, bbid, name, res.getInstrLatency(type) );
      }
      for (int i=0; i<numEdge; i++) {
        getline(cfile,line);
        vector<string> s = split(line, ',');
        TEdge type = static_cast<TEdge>(stoi(s.at(2)));
        g.addDependent(g.getNode(stoi(s.at(0))), g.getNode(stoi(s.at(1))), type);
      }
      if (getline(cfile,line))
        assert(false);
    }
    else
      assert(false);
    cfile.close();
    cout << g;
  }
}
