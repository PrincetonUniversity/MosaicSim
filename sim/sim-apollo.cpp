//=======================================================================
// Copyright 2018 Princeton University.
//=======================================================================

#include "sim-apollo.hpp"
#include "DRAMSim.h"
#include <iostream>
#include <fstream>
#include <sstream> 
#include <cstdio> 
#include <string> 

using namespace apollo;
using namespace std;

vector<string> split(const string &s, char delim) {
    stringstream ss(s);
    string item;
    vector<string> tokens;
    while (getline(ss, item, delim)) {
        tokens.push_back(item);
    }
    return tokens;
}


class Context {
   public:
      bool live;
      int id;
      int bbid;
      int processed;
      std::vector<Node*> active_list;
      std::set<Node*> start_set;
      std::set<Node*> next_start_set;
      std::vector<Node *> next_active_list;
      std::map<Node*, int> remaining_cycles_map;  // tracks remaining cycles for each node
      std::map<Node*, int> pending_parents_map; // tracks the # of pending parents (intra BB)
      std::map<Node*, int> pending_external_parents_map; // tracks the # of pending parents (across BB)
      Context(int id) : live(true), id(id), bbid(-1), processed(0) {}

      void initialize(BasicBlock *bb) {
         if (bbid != -1)
            assert(false);
         bbid = bb->id;
         active_list.push_back(bb->entry);
         remaining_cycles_map.insert( std::make_pair(bb->entry, 0) );

         // for each node in the BB initialize the 
         for ( int i=0; i<bb->inst.size(); i++ ) {
            Node *n = bb->inst.at(i);
            pending_parents_map.insert( std::make_pair(n, n->n_parents) );
            pending_external_parents_map.insert( std::make_pair(n, n->n_external_parents));
         }
      }
      bool launchNode(Node *n) {
         if(pending_parents_map.at(n) > 0 || pending_external_parents_map.at(n) > 0) {
            return false;
         }
         next_active_list.push_back(n);
         next_start_set.insert(n);
         std::cout << "Node [" << n->name << " @ context " << id << "]: Ready to Execute\n";
         remaining_cycles_map.insert( std::make_pair(n, n->lat) );
         return true;
      }
      void updatePhiDependency(std::map<Node*, int> m) {
         std::map<Node*, int>::iterator it;
         for(it = m.begin(); it != m.end(); ++it) {
            int c = pending_parents_map.at(it->first);
            pending_parents_map.at(it->first) = c - it->second;
            if(pending_parents_map.at(it->first) == 0)
               assert(false); // Should never be the case since there should always be a dependency from artificial "entry" node
         }
      }
};


class Simulator
{ 
public:
   // TODO: Memory address overlap across contexts
   // TODO: Handle 0-latency instructions correctly
   class DRAMSimCallBack {
      public: 
         std::map<uint64_t, std::pair<Node*,Context*> > outstanding_access_map;
         void read_complete(unsigned id, uint64_t address, uint64_t mem_clock_cycle) {
            printf("[DRAM] Memory Read Complete. id:%d addr:%lu mem_cycle:%lu\n", id, address, mem_clock_cycle);
            if ( outstanding_access_map.find(address) == outstanding_access_map.end() )
               assert(false);
            Node *n = outstanding_access_map.at(address).first;
            Context *c = outstanding_access_map.at(address).second;
            c->remaining_cycles_map.at(n) = 0; // load marked as DONE !
            outstanding_access_map.erase(address);
         }
         void write_complete(unsigned id, uint64_t address, uint64_t clock_cycle) {
            printf("[DRAM] Memory Write Complete: %d %lu cycle=%lu\n", id, address, clock_cycle);
            if( outstanding_access_map.find(address) == outstanding_access_map.end() )
               assert(false);
            outstanding_access_map.erase(address);
         }
   };

   Graph g;
   DRAMSimCallBack cb;
   int cycle_count = 0;
   int curr_context_id = 0;
   std::vector<Context*> context_list;
   std::vector<int> cf; // List of basic blocks in a program order 
   std::map<int, std::vector<uint64_t> > memory;
   DRAMSim::MultiChannelMemorySystem *mem;
   
   std::map<Node*, std::pair<int, bool> > curr_owner; // source, owner (most recent context id)
   std::map<DNode, std::vector<DNode> > deps; // source, destinations (source hasn't finished and destinations are waiting)
   // remove deps when finished & new owner appears
   std::map<int, std::map<Node *, int> > future_contexts; // context id, list of nodes, processed phi dependency // FIX   
   /* Tentative */
   void registerDep(Node *s, int cid) { // Context (SRC) Created
      if(curr_owner.find(s) == curr_owner.end())
         curr_owner.insert(make_pair(s, make_pair(cid,false)));
      else {
         curr_owner.at(s).first = cid;
         curr_owner.at(s).second = false;
      }
   }
   void finishDep(Node *s, int cid) { // (SRC) Node Finishes
      DNode src = make_pair(s, cid);
      if(deps.find(src) != deps.end()) {
         std::vector<DNode> users = deps.at(src);
         for(int i=0; i<users.size(); i++) {
            Context *c = context_list.at(users.at(i).second);
            c->pending_external_parents_map.at(s)--;
         }   
      }
      curr_owner.at(s).second = true;
   }
   void updateDep(Node *s, DNode dst) { // Context (DST) Created
      int cid = curr_owner.at(s).first;
      bool done = curr_owner.at(s).second;
      DNode src = make_pair(s, cid);
      if(done)
         context_list.at(dst.second)->pending_external_parents_map.at(dst.first)--;
      else {
         if(deps.find(src) == deps.end()) {
            deps.insert(make_pair(src, vector<DNode>()));
            deps.at(src).push_back(dst);   
         }
      }
   }
   /* */
   void initialize() {
      DRAMSim::TransactionCompleteCB *read_cb = new DRAMSim::Callback<DRAMSimCallBack, void, unsigned, uint64_t, uint64_t>(&cb, &DRAMSimCallBack::read_complete);
      DRAMSim::TransactionCompleteCB *write_cb = new DRAMSim::Callback<DRAMSimCallBack, void, unsigned, uint64_t, uint64_t>(&cb, &DRAMSimCallBack::write_complete);
      mem = DRAMSim::getMemorySystemInstance("sim/config/DDR3_micron_16M_8B_x8_sg15.ini", "sim/config/dramsys.ini", "..", "Apollo", 16384); 
      mem->RegisterCallbacks(read_cb, write_cb, NULL);
      mem->setCPUClockSpeed(2000000000);  
   }
  
   void readCF() 
   {
      string line;
      string last_line;
      ifstream cfile ("input/ctrl.txt");
      int last_bbid = -1;
      if (cfile.is_open()) {
       while (getline (cfile,line)) {
         vector<string> s = split(line, ',');
         if (s.size() != 3) {
            assert(false);
         }
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

   void readMemory() 
   {
      std::map<int, std::vector<uint64_t> > memory;
      string line;
      string last_line;
      ifstream cfile ("input/memory.txt");
      if (cfile.is_open()) {
       while (getline (cfile,line)) {
         vector<string> s = split(line, ',');
         if (s.size() != 4) {
            assert(false);
         }
         int id = stoi(s.at(1));
         if(memory.find(id) == memory.end())
            memory.insert(make_pair(id, vector<uint64_t>()));
         memory.at(id).push_back(stoull(s.at(2)));
       }
      }
      cfile.close();
   }
   void readToyGraph()
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

      cout << g;
      createContext(0); // starting basic block id
   }
   void readGraph() 
   {
      ifstream cfile ("input/graph.txt");
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
      
      cout << g;
      createContext(0); // starting basic block id
   }

   Context* createContext(int bid)
   {
      if (g.bbs.size() <= bid)
         assert(false);
      BasicBlock *bb = g.bbs.at(bid);
      int cid = curr_context_id;
      context_list.push_back(new Context(cid));
      curr_context_id++;
      context_list.at(cid)->initialize(bb);
      if (future_contexts.find(cid) != future_contexts.end()) {
         context_list.at(cid)->updatePhiDependency(future_contexts.at(cid));
         future_contexts.erase(cid);
      }
      for (int i=0; i<bb->inst.size(); i++) {
         Node *n = bb->inst.at(i);
         if(n->n_external_edges > 0)
            registerDep(n, cid);
         if(n->n_external_parents > 0) {
            std::set< std::pair<Node*, TEdge> >::iterator it;
            for (it = n->external_parents.begin(); it != n->external_parents.end(); ++it) {
               Node *s = it->first;
               TEdge t = it->second;
               if(t == data_dep)
                  updateDep(s, make_pair(n, cid));
            }
         }
      }
      std::cout << "Context [" << cid << "] created (with bbid =" << bid << ")\n";     
      return context_list.at(cid);
   }

   void updateFutureDependent(int cid, Node *d)
   {
      if (future_contexts.find(cid) != future_contexts.end() && future_contexts.at(cid).find(d) != future_contexts.at(cid).end())
         future_contexts.at(cid).at(d)++;
      else {
         future_contexts[cid][d]=1;
      }
   }

   void process_context(Context *c)
   {
      // process ALL nodes in <active_list>, ie, those being executed in this cycle
      for ( int i=0; i < c->active_list.size(); i++ ) {

         Node *n = c->active_list.at(i);
         if ( c->start_set.find(n) != c->start_set.end() ) {
            
            std::cout << "Node [" << n->name << "]: Starts Execution \n";
            // Memory instructions treatment
            if (n->type == LD || n->type == ST) {
               uint64_t addr = n->id * 64; // TODO: Use Real Address
               std::cout << "Node [" << n->name << "]: Inserts Memory Transaction for Address "<< addr << "\n";
               if ( !mem->willAcceptTransaction(addr) )
                  assert(false);
               if ( n->type == LD ) {
                  mem->addTransaction(false, addr);
                  cb.outstanding_access_map.insert( std::make_pair(addr, std::make_pair(n,c)) );
               }
               else if ( n->type == ST ) {
                  mem->addTransaction(true, addr);
                  cb.outstanding_access_map.insert( std::make_pair(addr, std::make_pair(n,c)) );
               }
            }
         }
         else 
            std::cout << "Node [" << n->name << " @ context " << c->id << "]: Processing \n";

         // decrease the remaining # of cycles for current node <n>
         int remaining_cycles = c->remaining_cycles_map.at(n);
         if (remaining_cycles > 0)
           remaining_cycles--;
         if ( remaining_cycles > 0 || remaining_cycles == -1 ) {  // Node <n> will continue execution in next cycle 
            // A remaining_cycles == -1 represents a outstanding memory request being processed currently by DRAMsim
            c->remaining_cycles_map.at(n) = remaining_cycles;
            c->next_active_list.push_back(n);  
         }
         else if (remaining_cycles == 0) { // Execution Finished for node <n>
   	      std::cout << "Node [" << n->name << " @ context " << c->id << "]: Finished Execution \n";
      	   if ( n->type == TERMINATOR ) {
      	      // Create new context with next bbid in cf (bbid list)
      		   // Note that context ids are incrementing starting from 0, so they correspond to cf indices
               if(cf.size() > c->id+1)
      		     createContext(cf.at(c->id+1));   
      	   }
            c->remaining_cycles_map.erase(n);
            if ( n->type != NAI )
               c->processed++;
            // Update Dependents
            if(n->n_external_edges > 0)
               finishDep(n, c->id);
            std::set< std::pair<Node*, TEdge> >::iterator it;
            for ( it = n->dependents.begin(); it != n->dependents.end(); ++it ) {
               Node *d = it->first;
               TEdge t = it->second;
               if (t == data_dep || t == bb_dep) { // only update intra
                  if(d->bbid == c->bbid) {
                     c->pending_parents_map.at(d)--;
                     c->launchNode(d);
                  }
               }
               else if (t == phi_dep  && cf.size() > c->id+1 && (cf.at(c->id+1) == d->bbid)) {
                  if(context_list.size() > c->id+1) { // context_list[c->id+1] exists {
                     Context *cc = context_list.at(c->id+1);
                     cc->pending_parents_map.at(d)--;
                     cc->launchNode(d);
                  }
                  else
                     updateFutureDependent(c->id+1, d);
               }
            }
         }
         else
            assert(false);
      }
      // Continue with following active instructions
      c->start_set = c->next_start_set;
      c->active_list = c->next_active_list;
      c->next_start_set.clear();
      c->next_active_list.clear();
      // Check if current context is done
      if ( c->processed == g.bbs.at(c->bbid)->inst_count ) {
         std::cout << "Context [" << c->id << "]: Finished Execution (Executed " << c->processed << " instructions) \n";
         c->live = false;
      }
   }

   void process_memory()
   {
      mem->update();
   }

   bool process_cycle()
   {
      std::cout << "Cycle: " << cycle_count << "\n";
      cycle_count++;
      bool simulate = false;

      if (cycle_count > 2000)
         assert(false);
      for (int i=0; i<context_list.size(); i++) {
         if(context_list.at(i)->live)
            process_context( context_list.at(i) );
         if (context_list.at(i)->live)
            simulate = true;
      }
      process_memory();
      return simulate;
   }

   void run()
   {
      bool simulate = true;
      while (simulate) {
         simulate = process_cycle();
      }
      mem->printStats(false);
   }
};

int main(int argc, char const *argv[])
{
   Simulator sim;
   sim.readGraph();
   sim.readMemory();
   sim.readCF();
   sim.initialize();
   sim.run();
   return 0;
} 
