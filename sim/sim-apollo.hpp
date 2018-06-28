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

#define NUM_INST_TYPES 14
class Node;
typedef std::pair<Node*,int> DNode;  // Dynamic Node: a pair of <node,context>
typedef enum {I_ADD, FP_ADD, I_SUB, FP_SUB, LOGICAL, I_MULT, FP_MULT, I_DIV, FP_DIV, LD, ST, ENTRY, TERMINATOR, PHI} TInstr;
typedef enum {DATA_DEP, BB_DEP, PHI_DEP} TEdge;

// TJH: For now, we can assume that one instruction type corresponds to one functional unit 
// since accelerators do not really use one functional unit to do multiple types of operations
// Also, let's assume only mult and div units are shared (others are replicated and thus there's no limit)

class Config {
public:
   // Config
  bool CF_one_context_at_once;
  bool CF_all_contexts_concurrently;
  // Resource
  int lsq_size;
  // Latency
  int instr_latency[NUM_INST_TYPES];
  int num_units[NUM_INST_TYPES];
  int load_ports;
  int store_ports;
  int outstanding_load_requests;
  int outstanding_store_requests;
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
  int bbid;
  std::string name;

  Node(int id, TInstr typeInstr, int bbid, std::string name, int lat): 
            id(id), typeInstr(typeInstr), bbid(bbid), name(name), lat (lat) {
  } 
  
  // Constructor for the BB's entry point
  Node(int bbid) : id(-1), lat(0), typeInstr(ENTRY), bbid(bbid), name("BB-Entry") {}

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
    os << "I[" << n.name << "], lat=" << n.lat << "\n";
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
    entry->addDependent(n, BB_DEP);
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
      std::cout << it->first << ":" << *it->second;
    std::cout << "";
    return os;
  }

};

class Reader {
public:
  /*void read(Simulator &sim) {
    readCfg("input/config.txt", sim.cfg);
    readGraph("input/graph2.txt", sim.g, sim.cfg);
    readProfMemory("input/memory.txt", sim.memory);
    readProfCF("input/ctrl.txt", sim.cf);
  }*/
  // helper function: split a string [with delimiter] into a vector
  vector<string> split(const string &s, char delim) {
     stringstream ss(s);
     string item;
     vector<string> tokens;
     while (getline(ss, item, delim)) {
        tokens.push_back(item);
     }
     return tokens;
  }
  void readCfg(std::string name, Config &cfg) {
    // TODO: Read Config From the File
    cfg.lsq_size = 512;
    cfg.CF_one_context_at_once = true;
    cfg.CF_all_contexts_concurrently = false;
    cfg.instr_latency[I_ADD] = 5;
    cfg.instr_latency[FP_ADD] = 1;
    cfg.instr_latency[I_SUB] = 1;
    cfg.instr_latency[FP_SUB] = 1;
    cfg.instr_latency[LOGICAL] = 1;
    cfg.instr_latency[I_MULT] = 3;
    cfg.instr_latency[FP_MULT] = 3;
    cfg.instr_latency[I_DIV] = 9;
    cfg.instr_latency[FP_DIV] = 9;
    cfg.instr_latency[LD] = -1;
    cfg.instr_latency[ST] = 1;
    cfg.instr_latency[ENTRY] = 1;
    cfg.instr_latency[TERMINATOR] = 1;
    cfg.instr_latency[PHI] = 1;
    cfg.num_units[I_ADD] = 1;
    cfg.num_units[FP_ADD] = -1;
    cfg.num_units[I_SUB] = -1;
    cfg.num_units[FP_SUB] = -1;
    cfg.num_units[LOGICAL] = -1;
    cfg.num_units[I_MULT] = 1;
    cfg.num_units[FP_MULT] = 1;
    cfg.num_units[I_DIV] = 1;
    cfg.num_units[FP_DIV] = 1;
    cfg.num_units[LD] = -1;
    cfg.num_units[ST] = -1;
    cfg.num_units[ENTRY] = -1;
    cfg.num_units[TERMINATOR] = -1;
    cfg.num_units[PHI] = -1;
    cfg.load_ports = 4;
    cfg.store_ports = 4;
    cfg.outstanding_load_requests = 128;
    cfg.outstanding_store_requests = 128;
  }
  // Read Dynamic Control Flow data from profiling file. 
  // Format:   <string_bb_name>,<current_bb_id>,<next_bb_id>
  // vector <cf> will be the sequential list of executed BBs
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
  // Read Dynamic Memory accesses from profiling file.
  // <memory> will be a map of { <instr_id>, <queue of addresses> }
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
  void readGraph(std::string name, Graph &g, Config &cfg) {
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
        g.addNode( id, type, bbid, name, cfg.instr_latency[type]);
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
};

}
