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
   std::map<int, std::map<Node *, int> > future_contexts; // context id, list of nodes, processed inter-context dependency
   std::map<int, std::vector<uint64_t> > memory;
   DRAMSim::MultiChannelMemorySystem *mem;

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
            for(int i=0; i<name.size(); i++)
               cout << i <<" : " << name.at(i) << "\n";
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

      /*Node *nodes[10];  
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
      nodes[6]->addDependent(nodes[4], data_dep);*/
      
      cout << g;
      createContext(0); // starting basic block id
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
         context_list.at(cid)->updateDependency(future_contexts.at(cid));
         future_contexts.erase(cid);
      }
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

   void updateDependent(Context *c, Node *d)
   {
      if (c->pending_parents_map.find(d) == c->pending_parents_map.end())  
         assert(false);
      
      // decrease # of pending ancestors of dependent <d>
      c->pending_parents_map.at(d)--;

      // if <d> has no more pending ancestors -> push it for "execution" in next cycle
      if ( c->pending_parents_map.at(d) == 0 ) {
         c->next_active_list.push_back(d);
         c->next_start_set.insert(d);
         std::cout << "Node [" << d->name << "]: Ready to Execute\n";
         c->remaining_cycles_map.insert( std::make_pair(d, d->lat) );
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
            std::cout << "Node [" << n->name << "]: Processing \n";

         // decrease the remaining # of cycles for current node <n>
         int remainig_cycles = c->remaining_cycles_map.at(n);
         if (remainig_cycles > 0)
           remainig_cycles--;
         if ( remainig_cycles > 0 || remainig_cycles == -1 ) {  // Node <n> will continue execution in next cycle 
            // A remaining_cycles == -1 represents a outstanding memory request being processed currently by DRAMsim
            c->remaining_cycles_map.at(n) = remainig_cycles;
            c->next_active_list.push_back(n);  
         }
         else if (remainig_cycles == 0) { // Execution Finished for node <n>

	     std::cout << "Node [" << n->name << "]: Finished Execution \n";

	    if ( n->type == TERMINATOR ) {
	      cout << "Hit Terminator \n";
	       int newcid = c->id+1;
	       if ( cf.size() > newcid ){
	          //create new context with next bbid in cf (bbid list)
		  //note that context ids are incrementing starting from 0, so they correspond to cf indices
		  createContext(cf.at(newcid));	     
		  std::cout << "Created new context with BID: " << cf.at(newcid) << "\n";}	    
	    }
	    
            std::cout << "Node [" << n->name << "]: Finished Execution \n";
	    

            c->remaining_cycles_map.erase(n);
            if ( n->type != NAI )
               c->processed++;

            // Update Dependents
            std::set< std::pair<Node*, TEdge> >::iterator it;
            for ( it = n->dependents.begin(); it != n->dependents.end(); ++it ) {
               Node *d = it->first;
               TEdge t = it->second;
               
               if (t == data_dep || t == bb_dep) {
                  updateDependent(c, d);
               }
               else if (t == phi_dep  && (cf.at(c->id+1) == d->bbid)) {
                  if(context_list.size() > c->id +1) // context_list[c->id+1] exists
                     updateDependent(context_list.at(c->id+1),d);
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

      if (cycle_count > 500)
         assert(false);
      for (int i=0; i<context_list.size(); i++) {
         process_context( context_list.at(i) );
         if ( context_list.at(i)->live )
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
