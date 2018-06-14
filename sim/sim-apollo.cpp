//=======================================================================
// Copyright 2018 Princeton University.
//=======================================================================

#include "sim-apollo.hpp"
using namespace apollo;
using namespace std;

#include "dramsim2/DRAMSim.h"
#include <iostream>
#include <fstream>
#include <sstream> 
#include <cstdio> 

Graph g;
int cycle_count = 0;
int curr_context_id = 0;
std::vector<Context*> context_list;
std::map< uint64_t, std::pair<Node*,Context*> > outstanding_access_map; 
std::vector<int> cf; // List of basic blocks in a program order
DRAMSim::MultiChannelMemorySystem *mem;
// TODO: Memory address overlap across contexts
// TODO: Handle 0-latency instructions correctly
vector<string> split(const string &s, char delim) {
    stringstream ss(s);
    string item;
    vector<string> tokens;
    while (getline(ss, item, delim)) {
        tokens.push_back(item);
    }
    return tokens;
}
void readCF() {
   string line;
   ifstream cfile ("ctrl.txt");
   int last_bbid = -1;
   if (cfile.is_open()) {
    while (getline (cfile,line)) {
      vector<string> s = split(line, ',');
      if(s.size() != 3)
         assert(false);
      if(stoi(s.at(1)) != last_bbid && last_bbid != -1)
         assert(false);
      last_bbid = stoi(s.at(1));
      cf.push_back(stoi(s.at(2)));
    }
  }
  cfile.close();
}
class DRAMSimCallBack {
   public: 
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

void process_context(Context *c)
{
   std::set<Node*> next_start_set;
   std::vector<Node *> next_active_list;

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
               outstanding_access_map.insert( std::make_pair(addr, std::make_pair(n,c)) );
            }
            else if ( n->type == ST ) {
               mem->addTransaction(true, addr);
               outstanding_access_map.insert( std::make_pair(addr, std::make_pair(n,c)) );
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
         next_active_list.push_back(n);  
      }
      else if (remainig_cycles == 0) { // Execution Finished for node <n>
         std::cout << "Node [" << n->name << "]: Finished Execution \n";

         c->remaining_cycles_map.erase(n);
         if ( n->type != NAI )
            c->processed++;

         // Update Dependents
         std::set< std::pair<Node*, TEdge> >::iterator it;
         for ( it = n->dependents.begin(); it != n->dependents.end(); ++it ) {
            Node *d = it->first;
            TEdge t = it->second;
            
            if ( t == data_dep || t == bb_dep ) {
               if ( c->pending_parents_map.find(d) == c->pending_parents_map.end() )  
                  assert(false);
               
               // decrease # of pending ancestors of dependent <d>
               c->pending_parents_map.at(d)--;

               // if <d> has no more pending ancestors -> push it for "execution" in next cycle
               if ( c->pending_parents_map.at(d) == 0 ) {
                  next_active_list.push_back(d);
                  next_start_set.insert(d);
                  std::cout << "Node [" << d->name << "]: Ready to Execute\n";
                  c->remaining_cycles_map.insert( std::make_pair(d, d->lat) );
               }
            }
            else if ( t == phi_dep ) {// && getNextBB() == d->bid) {
               // TODO: Update different context's node
            }
         }
      }
      else
         assert(false);
   }

   // Continue with following active instructions
   c->start_set = next_start_set;
   c->active_list = next_active_list;
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

void readGraph()
{
   Node *nodes[10];  
   int id = 1;

   g.addBasicBlock(0);
   nodes[1] = g.addNode(id++, ADD, 0, "1-add $1,$3,$4");
   nodes[2] = g.addNode(id++, LD, 0,"2-LD $1,$3,$4");
   nodes[3] = g.addNode(id++, LOGICAL, 0,"3-xor $1,$3,$4");
   nodes[4] = g.addNode(id++, DIV, 0,"4-mult $1,$3,$4");
   nodes[5] = g.addNode(id++, SUB, 0,"5-sub $1,$3,$4");
   nodes[6] = g.addNode(id++, LOGICAL, 0,"6-xor $1,$3,$4");

   // add some dependents
   nodes[1]->addDependent(nodes[2], /*type*/ data_dep);
   nodes[1]->addDependent(nodes[3], /*type*/ data_dep);
   nodes[1]->addDependent(nodes[4], /*type*/ data_dep);
   nodes[2]->addDependent(nodes[5], /*type*/ data_dep);
   nodes[2]->addDependent(nodes[6], /*type*/ data_dep);
   nodes[6]->addDependent(nodes[4], /*type*/ data_dep);
   cout << g;
}
int main(int argc, char const *argv[])
{
   DRAMSimCallBack cb;
   DRAMSim::TransactionCompleteCB *read_cb = new DRAMSim::Callback<DRAMSimCallBack, void, unsigned, uint64_t, uint64_t>(&cb, &DRAMSimCallBack::read_complete);
   DRAMSim::TransactionCompleteCB *write_cb = new DRAMSim::Callback<DRAMSimCallBack, void, unsigned, uint64_t, uint64_t>(&cb, &DRAMSimCallBack::write_complete);
   mem = DRAMSim::getMemorySystemInstance("sim/dramsim2/ini/DDR3_micron_16M_8B_x8_sg15.ini", "sim/dramsys.ini", "..", "Apollo", 16384); 
   mem->RegisterCallbacks(read_cb, write_cb, NULL);
   mem->setCPUClockSpeed(2000000000);
   readGraph();
   cycle_count = 0;
   context_list.push_back( new Context(curr_context_id++) );
   context_list.at(0)->initialize( g.bbs.at(0) );
   bool simulate = true;
   while (simulate) {
      simulate = process_cycle();
   }
   mem->printStats(false);  
   return 0;
} 