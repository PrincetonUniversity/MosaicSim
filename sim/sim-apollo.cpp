//=======================================================================
// Copyright 2018 University of Princeton.
//
// Project Apollo - simulator
// Authors: 
//=======================================================================

#include "sim-apollo.hpp"
using namespace apollo;
using namespace std;

#include "dramsim2/DRAMSim.h"
#include <iostream> 
#include <cstdio> 

Graph g;
int cycle_count = 0;
int curr_context_id = 0;
std::vector<Context*> context_list;
std::map<uint64_t, std::pair<Node*,Context*> > outstanding_access_map; 
DRAMSim::MultiChannelMemorySystem *mem;
// TODO: Memory address overlap across contexts
// TODO: Handle 0-latency instructions correctly
class DRAMSimCallBack
{
public: 
   void read_complete(unsigned id, uint64_t address, uint64_t clock_cycle) {
      printf("[DRAM] Memory Read Complete: %d %lu cycle=%lu\n", id, address, clock_cycle);
      if(outstanding_access_map.find(address) == outstanding_access_map.end())
         assert(false);
      Node *n = outstanding_access_map.at(address).first;
      Context *c = outstanding_access_map.at(address).second;
      c->process_map.at(n) = 0;
      outstanding_access_map.erase(address);
   }
   void write_complete(unsigned id, uint64_t address, uint64_t clock_cycle) {
      printf("[DRAM] Memory Write Complete: %d %lu cycle=%lu\n", id, address, clock_cycle);
      if(outstanding_access_map.find(address) == outstanding_access_map.end())
         assert(false);
      outstanding_access_map.erase(address);
   }
};

void process_context(Context *c)
{
   std::set<Node*> next_start_set;
   std::vector<Node *> next_active_list;
   for (int i=0; i <c->active_list.size(); i++ ) {
      Node *n = c->active_list.at(i);
      if(c->start_set.find(n) != c->start_set.end()) {
         std::cout << "Node [" << n->name << "]: Starts Execution \n";
         if(n->type == LD || n->type == ST) {
            uint64_t addr = n->id * 64; // TODO: Use Real Address
            std::cout << "Node [" << n->name << "]: Inserts Memory Transaction for Address "<< addr << "\n";
            if(!mem->willAcceptTransaction(addr))
               assert(false);
            if(n->type == LD) {
               mem->addTransaction(false, addr);
               outstanding_access_map.insert(std::make_pair(addr, std::make_pair(n,c)));
            }
            else if(n->type == ST) {
               mem->addTransaction(true, addr);
               outstanding_access_map.insert(std::make_pair(addr, std::make_pair(n,c)));
            }
         }
      }
      else {
         std::cout << "Node [" << n->name << "]: Processing \n";
      }
      int lat = c->process_map.at(n);
      if(lat > 0)
         lat--;
      if (lat > 0 || lat == -1) { // Continue Execution 
         c->process_map.at(n) = lat;
         next_active_list.push_back(n);
      }
      else if (lat == 0) { // Execution Finished
         std::cout << "Node [" << n->name << "]: Finished Execution \n";
         c->process_map.erase(n);
         if(n->type != NAI)
            c->processed++;
         // Update Dependents
         std::set<std::pair<Node*, TEdge> >::iterator it;
         for ( it = n->dependents.begin(); it != n->dependents.end(); ++it ) {
            Node *d = it->first;
            TEdge t = it->second;
            if(t == data_dep || t == bb_dep) {
               if (c->ready_map.find(d) == c->ready_map.end())  
                  assert(false);
               c->ready_map.at(d)--;
               if (c->ready_map.at(d) == 0) {
                  next_active_list.push_back(d);
                  next_start_set.insert(d);
                  std::cout << "Node [" << d->name << "]: Ready to Execute\n";
                  c->process_map.insert(std::make_pair(d, d->lat));
               }
            }
            else if(t == phi_dep) {// && getNextBB() == d->bid) {
               // TODO: Update different context's node
            }
         }
      }
      else
         assert(false);
   }
   c->start_set = next_start_set;
   c->active_list = next_active_list;
   if (c->processed == g.bbs.at(c->bbid)->inst_count) {
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
   if(cycle_count > 500)
      assert(false);
   for(int i=0; i<context_list.size(); i++) {
      process_context(context_list.at(i));
      if(context_list.at(i)->live)
         simulate = true;
   }
   process_memory();
   return simulate;
}

int main(int argc, char const *argv[])
{
   DRAMSimCallBack cb;
   DRAMSim::TransactionCompleteCB *read_cb = new DRAMSim::Callback<DRAMSimCallBack, void, unsigned, uint64_t, uint64_t>(&cb, &DRAMSimCallBack::read_complete);
   DRAMSim::TransactionCompleteCB *write_cb = new DRAMSim::Callback<DRAMSimCallBack, void, unsigned, uint64_t, uint64_t>(&cb, &DRAMSimCallBack::write_complete);
   mem = DRAMSim::getMemorySystemInstance("sim/dramsim2/ini/DDR2_micron_16M_8b_x8_sg3E.ini", "sim/dramsys.ini", "..", "Apollo", 16384); 
   mem->RegisterCallbacks(read_cb, write_cb, NULL);
   mem->setCPUClockSpeed(2000000000);

   Node *nodes[10];
   int id = 1;

   g.addBasicBlock(0);
   nodes[1] = g.addNode(id++, ADD, 0, "1-add $1,$3,$4");
   nodes[2] = g.addNode(id++, LD, 0,"2-LD $1,$3,$4");
   nodes[3] = g.addNode(id++, LOGICAL, 0,"3-xor $1,$3,$4");
   nodes[4] = g.addNode(id++, DIV, 0,"4-mult $1,$3,$4");
   nodes[5] = g.addNode(id++, SUB, 0,"5-sub $1,$3,$4");
   nodes[6] = g.addNode(id++, LOGICAL, 0,"6-xor $1,$3,$4");

   cout << g;

   // add some dependents
   nodes[1]->addDependent(nodes[2], /*type*/ data_dep);
   nodes[1]->addDependent(nodes[3], /*type*/ data_dep);
   nodes[1]->addDependent(nodes[4], /*type*/ data_dep);
   nodes[2]->addDependent(nodes[5], /*type*/ data_dep);
   nodes[2]->addDependent(nodes[6], /*type*/ data_dep);
   nodes[6]->addDependent(nodes[4], /*type*/ data_dep);
   
   cycle_count = 0;
   context_list.push_back(new Context(curr_context_id++));
   context_list.at(0)->initialize(g.bbs.at(0));
   bool simulate = true;
   while (simulate) {
      simulate = process_cycle();
   }
   mem->printStats(false);  
   return 0;
} 