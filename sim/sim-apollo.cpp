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

class DRAMSimCallBack
{
public: 
   void read_complete(unsigned id, uint64_t address, uint64_t clock_cycle) {
      printf("[Callback] read complete: %d 0x%lx cycle=%lu\n", id, address, clock_cycle);
      if(outstanding_access_map.find(address) == outstanding_access_map.end())
         assert(false);
      Node *n = outstanding_access_map.at(address).first;
      Context *c = outstanding_access_map.at(address).second;
      c->latency_map.at(n) = 0;
      outstanding_access_map.erase(address);
   }
   void write_complete(unsigned id, uint64_t address, uint64_t clock_cycle) {
      printf("[Callback] write complete: %d 0x%lx cycle=%lu\n", id, address, clock_cycle);
      if(outstanding_access_map.find(address) == outstanding_access_map.end())
         assert(false);
      outstanding_access_map.erase(address);
   }
};

void process_context(Context *c)
{
   std::set<Node*> next_newly_active_nodes;
   std::vector<Node *> next_active_list;
   for ( int i=0; i < c->active_list.size(); i++ ) {
      Node *n = c->active_list.at(i);
      if(c->newly_active_nodes.find(n) != c->newly_active_nodes.end()) {
         std::cout << "Node [" << n->name << "] starts execution\n";
         if(n->type == LD) {
            // TODO: WillAcceptTransaction
            uint64_t addr = n->id * 64; // TODO: Use Real Address
            mem->addTransaction(false, addr);
            std::cout << "Inserted a Read Transaction from Node " << n->name << "\n";
            outstanding_access_map.insert(std::make_pair(addr, std::make_pair(n,c)));
         }
         else if(n->type == ST) {
            uint64_t addr = n->id * 64; // TODO: Use Real Address
            mem->addTransaction(true, addr);
            outstanding_access_map.insert(std::make_pair(addr, std::make_pair(n,c)));
         }
      }
      else {
         std::cout << "Node [" << n->name << "] is processing now\n";
      }
      int lat = c->latency_map.at(n);
      if(lat > 0)
         lat--;
      if ( lat > 0  || lat == -1) { // node NOT done yet -> in progress 

         c->latency_map.at(n) = lat;
         next_active_list.push_back(n);  // Node <n> will continue execution in next cycle
      }
      else if (lat == 0) { // node done!!
         std::cout << "Node [" << n->name << "] finished execution \n";
         c->latency_map.erase(n);
         c->dep_map.erase(n);
         c->count++;
         // traverse ALL dependents
         std::set<Edge>::iterator it;
         for ( it = n->dependents.begin(); it != n->dependents.end(); ++it ) {
            Node *d = it->dst;
            if(it->type == data_dep) {
               // insert dependent d in a map (if not present yet)
               if (c->dep_map.find(d) == c->dep_map.end())  
                   c->dep_map.insert( std::make_pair(d, d->parents) );
               
               // decrease # of parents of dependent d
               c->dep_map.at(d)--;

               // if d has no more parents -> push it for "execution" in next cycle
               if (c->dep_map.at(d) == 0 ) {
                  next_active_list.push_back(d);
                  next_newly_active_nodes.insert(d);
                  std::cout << "Node [" << d->name << "] will start Execution \n";
                  c->latency_map.insert( std::make_pair(d, d->lat) );
                  c->dep_map.erase(d);
               }
            }
            else if(it->type == phi_dep) {// && getNextBB() == d->bid) {
               // do the same for the right context
            }
         }
      }
      else
         assert(false);
   }
   c->newly_active_nodes = next_newly_active_nodes;
   c->active_list = next_active_list;
   if(c->count == (g.bbs.at(c->bid)->inst_count+1)) {
      std::cout << "Finished after executing " << c->count << " instructions \n";
      c->live = false;
   }
}

void process_memory()
{
   mem->update();
}
bool process_cycle()
{
   std::cout << "Cycle : " << cycle_count << "\n";
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

   // initilize the latencies array (move to hpp)

   Node *nodes[10];
   int id = 1;

   // first, create ALL the nodes
   g.addBasicBlock(0);
   nodes[0] = g.addNode(0);  // this is the entry point
   nodes[1] = g.addNode(id++, ADD,0, "1-add $1,$3,$4");
   nodes[2] = g.addNode(id++, LD, 0,"2-LD $1,$3,$4");
   nodes[3] = g.addNode(id++, LOGICAL, 0,"3-xor $1,$3,$4");
   nodes[4] = g.addNode(id++, DIV, 0,"4-mult $1,$3,$4");
   nodes[5] = g.addNode(id++, SUB, 0,"5-sub $1,$3,$4");
   nodes[6] = g.addNode(id++, LOGICAL, 0,"6-xor $1,$3,$4");

   // add some dependents
   nodes[0]->addDependent(nodes[1], /*type*/ data_dep);
   nodes[0]->addDependent(nodes[6], /*type*/ data_dep);
   nodes[0]->addDependent(nodes[6], /*type*/ data_dep);
   nodes[1]->addDependent(nodes[2], /*type*/ data_dep);
   nodes[1]->addDependent(nodes[3], /*type*/ data_dep);
   nodes[1]->addDependent(nodes[4], /*type*/ data_dep);
   nodes[2]->addDependent(nodes[5], /*type*/ data_dep);
   nodes[2]->addDependent(nodes[6], /*type*/ data_dep);
   nodes[6]->addDependent(nodes[4], /*type*/ data_dep);
  
   cycle_count = 0;
   context_list.push_back(new Context(curr_context_id++, 0));
   context_list.at(0)->active_list.push_back(nodes[0]);
   context_list.at(0)->latency_map.insert(std::make_pair(nodes[0], nodes[0]->lat));
   bool simulate = true;
   while (simulate) {
      simulate = process_cycle();
   }
   mem->printStats(true);  
   return 0;
} 

// some sanity checks
//nodes[2]->eraseDependent(nodes[5], data_dep);
//nodes[2]->eraseDependent(nodes[6], data_dep);
//cout << g << endl << g.calculate_accum_latency() ;
//g.eraseNode(nodes[0]);
//g.eraseNode(nodes[1]);
//g.eraseNode(nodes[2]);
//cout << g.size() << endl;

//cout << g;
//cout << "Accum.latency=" << g.calculate_accum_latency() << endl << endl;

// Let's make a Topological Sort
/*std::stack<Node *> Stack;
g.make_topological_sort( Stack );
cout << "Topological Sort=";
while ( ! Stack.empty() ) {
  cout << Stack.top()->name << " ";
  Stack.pop();
}
cout << endl << endl;*/

// Let's EXECUTE the Graph