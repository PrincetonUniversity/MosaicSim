//=======================================================================
// Copyright 2018 Princeton University.
//=======================================================================

#include <iostream>                         
#include <set>
#include <map>
#include <unordered_map>
#include <vector>
#include <queue>
#include <iterator>
#include <iostream>
#include <fstream>
#include <sstream> 
#include <cstdio> 
#include <string>  
#include <algorithm>
#include <iterator>
#include <chrono>
#include "assert.h"
#define UNUSED 0
#define NUM_INST_TYPES 15
using namespace std;

typedef chrono::high_resolution_clock Clock;
typedef enum {I_ADDSUB, I_MULT, I_DIV, I_REM, FP_ADDSUB, FP_MULT, FP_DIV, FP_REM, LOGICAL, CAST, GEP, LD, ST, TERMINATOR, PHI} TInstr;
typedef enum {DATA_DEP, PHI_DEP} TEdge;

class Node;
class Config {
public:
  // Config parameters
  int  vInputLevel; // verbosity level
  bool cf_mode; // 0: one at a time / 1: all together
  bool mem_speculate;
  bool mem_forward;
  // Resources
  int lsq_size;
  int load_ports;
  int store_ports;
  int outstanding_load_requests;
  int outstanding_store_requests;
  int max_active_contexts_BB;
  // FUs
  int instr_latency[NUM_INST_TYPES];
  int num_units[NUM_INST_TYPES];
  // L1 cache
  bool ideal_cache;
  int L1_latency;
  int L1_size;     // KB
  int L1_assoc; 
};

class Node {
public:
  std::set<Node*> dependents;
  std::set<Node*> parents;
  std::set<Node*> external_dependents;
  std::set<Node*> external_parents;
  std::set<Node*> phi_dependents;
  // For PHI Nodes
  std::set<Node*> phi_parents;

  // For Store Nodes
  std::set<Node*> store_addr_dependents; // store_address_dependents
  int id;
  TInstr typeInstr;
  int bbid;
  std::string name;
  int lat;
  
  Node(int id, TInstr typeInstr, int bbid, std::string name, int lat): 
            id(id), typeInstr(typeInstr), bbid(bbid), name(name), lat (lat) {} 

  void addDependent(Node *dest, TEdge type) {
    if(type == DATA_DEP) {
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
      dest->phi_parents.insert(this);
    }
  }

  void eraseDependent(Node *dest, TEdge type) {
    int count = 0;
    if(type == DATA_DEP) {
      if(dest->bbid == this->bbid) {
        count += dependents.erase(dest);
        dest->parents.erase(this);
      }
      else {
        count += external_dependents.erase(dest);
        dest->external_parents.erase(this);
      }
    }
    else if(type == PHI_DEP) {
      count += phi_dependents.erase(dest);
      dest->phi_parents.erase(this);
    }
    assert(count == 1);
  }

  // Print Node
  friend std::ostream& operator<<(std::ostream &os, Node &n) {
    os << "[" << n.name << "]";
    return os;
  }
};

class BasicBlock {
public:
  std::vector<Node*> inst;
  int id;
  unsigned int inst_count;
  unsigned int ld_count;
  unsigned int st_count;

  BasicBlock(int id): id(id), inst_count(0), ld_count(0), st_count(0) {}
  
  void addInst(Node* n) {
    inst.push_back(n);
    inst_count++;
    if(n->typeInstr == LD)
      ld_count++;
    else if(n->typeInstr == ST)
      st_count++;
  }
};

class Graph {
public:
  std::map<int, Node *> nodes;
  std::map<int, BasicBlock*> bbs;
  ~Graph() { eraseAllNodes(); } 
  /* Temp */

  bool checkType(Node *n) {
    if(n->typeInstr == I_ADDSUB || n->typeInstr == FP_ADDSUB || n->typeInstr == LOGICAL || n->typeInstr == CAST)
      return true;
    else
      return false;
  }

  bool findOptimizableTerminator(Node *init, set<Node*> phis) {
    std::set<Node*> frontier;
    std::set<Node*> next_frontier;
    std::set<Node*> visited;
    frontier.insert(init);
    visited.insert(init);
    while(frontier.size() > 0) {
      for(auto fit = frontier.begin(); fit != frontier.end(); ++fit) {
        Node *n = *fit;
        for(auto it = n->parents.begin(); it!=n->parents.end(); ++it) {
          Node *sn = *it;
          if(sn->typeInstr == PHI && (phis.find(sn) != phis.end()))
            continue;
          if(!checkType(sn))
            return false;
          else if(visited.find(sn) == visited.end()) {
            visited.insert(sn);
            next_frontier.insert(sn);
          }
        }
      }
      frontier.clear();
      frontier = next_frontier;
      next_frontier.clear();
    }
    return true;
  }
  bool findOptimizablePhi(Node *init) {
    assert(init->typeInstr == PHI);
    std::set<Node*> frontier;
    std::set<Node*> next_frontier;
    std::set<Node*> visited;
    for(auto pit = init->phi_parents.begin(); pit!= init->phi_parents.end(); ++pit) {
      Node *sn = *pit;
      if(sn->bbid == init->bbid) {
        if(sn->typeInstr == PHI)
          assert(false); 
        if(!checkType(sn)) {
          return false;
        }
        visited.insert(sn);
        frontier.insert(sn);
      }
    }
    while(frontier.size() > 0) {
      for(auto fit = frontier.begin(); fit != frontier.end(); ++fit) {
        Node *n = *fit;
        for(auto it = n->parents.begin(); it!=n->parents.end(); ++it) {
          Node *sn = *it;
          if(sn->typeInstr == PHI && sn!= init)
            assert(false);
          if(sn->typeInstr == PHI && sn == init)
            continue;
          if(!checkType(sn)) {
            return false;
          }
          else if(visited.find(sn) == visited.end()) {
            visited.insert(sn);
            next_frontier.insert(sn);
          }
        }
      }
      frontier.clear();
      frontier = next_frontier;
      next_frontier.clear();
    }
    return true;
  }
  void inductionOptimization() {
    std::set<Node*> phis;
    std::vector<Node*> terms;
    for(auto it = nodes.begin(); it!= nodes.end(); ++it) {
      Node *n = it->second;
      if(n->typeInstr == PHI) {
        if(findOptimizablePhi(n)) {
          cout << "Optimized Phi : " << *n << "\n";
          phis.insert(n);
        }
      }
    }
    for(auto it = phis.begin(); it!= phis.end(); ++it) {
      Node *n = *it;
      for(auto pit = n->phi_parents.begin(); pit!= n->phi_parents.end(); ++pit) {
        Node *sn = *pit;
        if(sn->bbid == n->bbid)
          sn->eraseDependent(n, PHI_DEP);
      }
    }
    for(auto it = nodes.begin(); it!= nodes.end(); ++it) {
      Node *n = it->second;
      if(n->typeInstr == TERMINATOR) {
        if(findOptimizableTerminator(n, phis)) {
          for(auto iit = n->parents.begin(); iit != n->parents.end(); ++iit) {
            Node *sn = *iit;
            sn->eraseDependent(n, DATA_DEP);
          }
          cout << "Optimized Terminator : " << *n << "\n";
        }
      }
    }
  }

  void addBasicBlock(int id) {
    bbs.insert( std::make_pair(id, new BasicBlock(id)) );
  }

  void addNode(int id, TInstr type, int bbid, std::string name, int lat) {
    Node *n = new Node(id, type, bbid, name, lat);
    nodes.insert(std::make_pair(n->id, n));
    assert( bbs.find(bbid) != bbs.end() );   // make sure the BB exists
    bbs.at(bbid)->addInst(n);
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
      std::cout << it->first << ":" << *it->second <<"\n";
    std::cout << "";
    return os;
  }

};

class Reader {
public:
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
  void getCfg(int id, int val, Config &cfg) {
    switch (id) { 
      case 0:
        cfg.lsq_size = val; 
        break;
      case 1:
        cfg.cf_mode = val;
        break;
      case 2:
        cfg.mem_speculate = val;
        break;
      case 3:
        cfg.mem_forward = val;
        break;
      case 4:
        cfg.max_active_contexts_BB = val;
        break;
      case 5:
        cfg.ideal_cache = val;
        break;
      case 6:
        cfg.L1_size = val;
        break;
      case 7:
        cfg.load_ports = val;
        break;
      case 8:
        cfg.store_ports = val;
        break;
    }
  }
  void readCfg(std::string name, Config &cfg) { // TODO: Read config from <filename>
    string line;
    string last_line;
    cout << "CFG: " << name << "\n";
    ifstream cfile(name);
    int id = 0;
    if (cfile.is_open()) {
      while (getline (cfile,line)) {
        vector<string> s = split(line, ',');
        getCfg(id, stoi(s.at(0)), cfg);
        id++;
      }
    }
    else {
      cout << "Error opening Config\n";
      assert(false);
    }
    cfile.close();
    cfg.L1_latency = 1;
    cfg.L1_assoc = 8;
    cfg.instr_latency[I_ADDSUB] = 1;
    cfg.instr_latency[I_MULT] = 3;
    cfg.instr_latency[I_DIV] = 26;
    cfg.instr_latency[I_REM] = 1;
    cfg.instr_latency[FP_ADDSUB] = 1;
    cfg.instr_latency[FP_MULT] = 3;
    cfg.instr_latency[FP_DIV] = 26;
    cfg.instr_latency[FP_REM] = 1;
    cfg.instr_latency[LOGICAL] = 1;
    cfg.instr_latency[CAST] = 1;
    cfg.instr_latency[GEP] = 1;
    cfg.instr_latency[LD] = -1;
    cfg.instr_latency[ST] = 1;
    cfg.instr_latency[TERMINATOR] = 1;
    cfg.instr_latency[PHI] = 1;     // JLA: should it be 0 ?
    cfg.num_units[I_ADDSUB] = -1;
    cfg.num_units[I_MULT] =  -1;
    cfg.num_units[I_DIV] = -1;
    cfg.num_units[I_REM] = -1;
    cfg.num_units[FP_ADDSUB] = -1;
    cfg.num_units[FP_MULT] = -1;
    cfg.num_units[FP_DIV] = -1;
    cfg.num_units[FP_REM] = -1;
    cfg.num_units[LOGICAL] = -1;
    cfg.num_units[CAST] = -1;
    cfg.num_units[GEP] = -1;
    cfg.num_units[LD] = -1;
    cfg.num_units[ST] = -1;
    cfg.num_units[TERMINATOR] = -1;
    cfg.num_units[PHI] = -1;
    cfg.outstanding_load_requests = 9999;
    cfg.outstanding_store_requests = 9999;
  }
  // Read Dynamic Control Flow data from profiling file. 
  // Format:   <string_bb_name>,<current_bb_id>,<next_bb_id>
  // vector <cf> will be the sequential list of executed BBs
  void readProfCF(std::string name, std::vector<int> &cf) {
    string line;
    string last_line;
    ifstream cfile(name);
    bool init = false;
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
        if (!init) {
          cf.push_back(stoi(s.at(1)));
          init = true;
        }
        last_bbid = stoi(s.at(2));
        cf.push_back(last_bbid);
        last_line = line;
      }
    }
    else {
      cout << "Error opening CF profiling file\n";
      assert(false);
    }
    cout << "Finished Reading CF -- Total number of contexts : " << cf.size() << "\n";
    cfile.close();
  }
  // Read Dynamic Memory accesses from profiling file.
  // <memory> will be a map of { <instr_id>, <queue of addresses> }
  void readProfMemory(std::string name, std::unordered_map<int, std::queue<uint64_t> > &memory) {
    string line;
    string last_line;
    ifstream cfile(name);
    if (cfile.is_open()) {
      while ( getline(cfile,line) ) {
        vector<string> s = split(line, ',');
        assert(s.size() == 4);
        int id = stoi(s.at(1));
        if (memory.find(id) == memory.end())  // if it's NEW, insert a new entry into the <map>
          memory.insert(make_pair(id, queue<uint64_t>()));
        memory.at(id).push(stoull(s.at(2)));  // insert the <address> into the memory instructions's <queue>
      }
    }
    else {
      cout << "Error opening Memory profiling file\n";
      assert(false);
    }
    cout << "Finished Reading Memory Profile "<< "\n";
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
        string name = "";
        for(unsigned int i=3; i<s.size(); i++)
          name += s.at(i);     
        name = name.substr(2, name.size()-2);
        g.addNode( id, type, bbid, name, cfg.instr_latency[type]);
      }
      for (int i=0; i<numEdge; i++) {
        getline(cfile,line);
        vector<string> s = split(line, ',');
        int edgeT = stoi(s.at(2));
        if(edgeT >= 0) {
          TEdge type = static_cast<TEdge>(stoi(s.at(2)));
          g.addDependent(g.getNode(stoi(s.at(0))), g.getNode(stoi(s.at(1))), type);
        }
        else if (edgeT == -1) {
          g.getNode(stoi(s.at(1)))->store_addr_dependents.insert(g.getNode(stoi(s.at(0))));
        }
      }
      if (getline(cfile,line))
        assert(false);
    }
    else {
      cout << "Error opening Graph file\n";
      assert(false);
    }
    cfile.close();
    cout << g << endl;
  }
};
