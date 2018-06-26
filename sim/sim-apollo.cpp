//=======================================================================
// Copyright 2018 Princeton University.
//=======================================================================

#include "sim-apollo.hpp"
#include "DRAMSim.h"

using namespace apollo;
using namespace std;

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
   std::map<Node*, int> pending_parents_map;   // tracks the # of pending parents (intra BB)
   std::map<Node*, int> pending_external_parents_map; // tracks the # of pending parents (across BB)

   Context(int id) : live(true), id(id), bbid(-1), processed(0) {}

   void initialize(BasicBlock *bb, std::map<Node*, std::pair<int, bool> > &curr_owner, std::map<DNode, std::vector<DNode> > &deps, std::map<int, std::set<Node*> > &handled_phi_deps, map<uint64_t,deque<tuple<int,int,bool,bool>>>& load_queue_map, map<uint64_t,deque<tuple<int,int,bool,bool>>>& store_queue_map, map<int, queue<uint64_t> >& memory) {
      assert(bbid == -1);
      bbid = bb->id;
      active_list.push_back(bb->entry);
      remaining_cycles_map.insert(std::make_pair(bb->entry, 0));

      // traverse all the BB's instructions and initialize the pending_parents map
      for ( int i=0; i<bb->inst.size(); i++ ) {
         Node *n = bb->inst.at(i);

      if (n->type == ST){
           //Luwa: add to store queue as soon as context is created. this way, there's a serialized ordering of stores in store queue
           
         deque<tuple<int, int, bool, bool>> addr_queue;
         uint64_t addr = memory.at(n->id).front(); //in this context, this is always the only store to be done by node n 
         if (store_queue_map.find(addr)!=store_queue_map.end())
           {addr_queue=store_queue_map.at(addr);}
         //luwa working here
         addr_queue.push_back(make_tuple(id, n->id, false, false));
         store_queue_map[addr]=addr_queue;
      }
       
      if (n->type == LD){
         //Luwa: add to load queue as soon as context is created. this way, there's a serialized ordering of loads in load queue
           
         deque<tuple<int, int, bool, bool>> addr_queue;
         uint64_t addr = memory.at(n->id).front(); //in this context, this is always the only load to be done by node n 
         if (load_queue_map.find(addr)!=load_queue_map.end())
           {addr_queue=load_queue_map.at(addr);}
         //luwa working here
         addr_queue.push_back(make_tuple(id, n->id, false, false));
         load_queue_map[addr]=addr_queue;
      }
    
      if(n->type == PHI)
            pending_parents_map.insert(std::make_pair(n, n->parents.size()+1));
         
         else
            pending_parents_map.insert(std::make_pair(n, n->parents.size()));
         pending_external_parents_map.insert(std::make_pair(n, n->external_parents.size()));
      }

      /* Handle Phi */
      if (handled_phi_deps.find(id) != handled_phi_deps.end()) {
         std::set<Node*>::iterator it;
         for(it = handled_phi_deps.at(id).begin(); it != handled_phi_deps.at(id).end(); ++it) {
            pending_parents_map.at(*it)--;
         }
         handled_phi_deps.erase(id);
      }

      /* Handle External Edges */
      // traverse ALL the BB's instructions
      for (int i=0; i<bb->inst.size(); i++) {
         Node *n = bb->inst.at(i);
         if (n->external_dependents.size() > 0) {
            if(curr_owner.find(n) == curr_owner.end())
               curr_owner.insert(make_pair(n, make_pair(id, false)));
            else {
               curr_owner.at(n).first = id;
               curr_owner.at(n).second = false;
            }
         }
         if (n->external_parents.size() > 0) {
            std::set<Node*>::iterator it;
            for (it = n->external_parents.begin(); it != n->external_parents.end(); ++it) {
               Node *s = *it;
               int src_cid = curr_owner.at(s).first;
               bool done = curr_owner.at(s).second;
               DNode src = make_pair(s, src_cid);
               if(done)
                  pending_external_parents_map.at(n)--;
               else {
                  if(deps.find(src) == deps.end()) {
                     deps.insert(make_pair(src, vector<DNode>()));
                  }
                  deps.at(src).push_back(make_pair(n, id));
               }
            }
         }
      }
      // No need to call launchNode; entryBB will trigger them instead. 
   }

   bool launchNode(Node *n) {
      if(pending_parents_map.at(n) > 0 || pending_external_parents_map.at(n) > 0) {
         //std::cout << "Node [" << n->name << " @ context " << id << "]: Failed to Execute - " << pending_parents_map.at(n) << " / " << pending_external_parents_map.at(n) << "\n";
         return false;
      }
      next_active_list.push_back(n);
      next_start_set.insert(n);
      std::cout << "Node [" << n->name << " @ context " << id << "]: Ready to Execute\n";
      remaining_cycles_map.insert(std::make_pair(n, n->lat));
      return true;
   }
};


class Simulator
{ 
public:
   // TODO: Memory address overlap across contexts
   // TODO: Handle 0-latency instructions correctly
  
  
   class DRAMSimCallBack {
      public:

         map< uint64_t, queue< pair<Node*, Context*> > > outstanding_accesses_map;
         Simulator* sim;
         DRAMSimCallBack(Simulator* sim):sim(sim){}
     
         void read_complete(unsigned id, uint64_t addr, uint64_t mem_clock_cycle) {
            assert(outstanding_accesses_map.find(addr) != outstanding_accesses_map.end());
            queue<std::pair<Node*, Context*>> &q = outstanding_accesses_map.at(addr);
            Node *n = q.front().first;
            Context *c = q.front().second;
            cout << "Node [" << n->name << " @ context " << c->id << "]: Read Transaction Returns "<< addr << "\n";
            if (c->remaining_cycles_map.find(n) == c->remaining_cycles_map.end())
               cout << *n << " / " << c->id << "\n";
       
            c->remaining_cycles_map.at(n) = 0; // mark "Load" as finished for sim-apollo
            sim->update_memop_queue(sim->load_queue_map,addr,make_tuple(c->id,n->id,true,true)); //set entry in load queue to finished
            q.pop();
            if (q.size() == 0)
               outstanding_accesses_map.erase(addr);
         }
         void write_complete(unsigned id, uint64_t addr, uint64_t clock_cycle) {
            assert(outstanding_accesses_map.find(addr) != outstanding_accesses_map.end());
            queue<std::pair<Node*, Context*>> &q = outstanding_accesses_map.at(addr);
            Node *n = q.front().first;
            Context *c = q.front().second;
                    
            c->remaining_cycles_map.at(n) = 0; //Luwa: just added this. don't know why we weren't marking stores as finished
            sim->update_memop_queue(sim->store_queue_map,addr,make_tuple(c->id,n->id,true,true)); //set entry in store queue to finished
            cout << "Node [" << n->name << " @ context " << c->id << "]: Write Transaction Returns "<< addr << "\n";
            q.pop();
       
            if(q.size() == 0)
               outstanding_accesses_map.erase(addr);
         }
   };
   
   Graph g;  
   DRAMSimCallBack cb=DRAMSimCallBack(this); 
   DRAMSim::MultiChannelMemorySystem *mem;

   int cycle_count = 0;
   vector<Context*> context_list;
   
   vector<int> cf; // List of basic blocks in a program order 
   int cf_iterator = 0;
   map<int, queue<uint64_t> > memory; // List of memory accesses per instruction in a program order
   map<int, queue<uint64_t> >& memory_ref=memory;
   /* Handling External Dependencies */
   map<Node*, pair<int, bool> > curr_owner; // Source of external dependency (Node), Context ID for the node (cid), Finished
   map<DNode, vector<DNode> > deps; // Source of external depndency (Node,cid), Destinations for that source (Node, cid)
   /* Handling Phi Dependencies */
   map<int, set<Node*> > handled_phi_deps; // Context ID, Processed Phi node
   
   map<uint64_t, deque<tuple<int,int,bool,bool>>>store_queue_obj;
   map<uint64_t, deque<tuple<int,int,bool,bool>>>load_queue_obj;
   map<uint64_t, deque<tuple<int,int,bool,bool>>>& store_queue_map = store_queue_obj; 
   map<uint64_t, deque<tuple<int,int,bool,bool>>>& load_queue_map = load_queue_obj; 
    
   // **** simulator CONFIGURATION PARAMETERS
   // TODO: read this from a file
   // TODO: merge resourcewith latencies -> now there is a getLatency() in .hpp
   struct {

      // simulator flags
      bool CF_one_context_at_once = false;
      bool CF_all_contexts_concurrently = true;

      // resource limits
      int num_iadders  = 10;
      int num_fpadders = 10;
      int num_imults   = 10;
      int num_fpmults  = 10;
      int num_idivs    = 10;
      int num_fpdivs   = 10;
      int num_in_memports  = 10;
      int num_out_memports = 10;
      int num_outstanding_mem = 10;
   } cfg;

   void initialize() {
      DRAMSim::TransactionCompleteCB *read_cb = new DRAMSim::Callback<DRAMSimCallBack, void, unsigned, uint64_t, uint64_t>(&cb, &DRAMSimCallBack::read_complete);
      DRAMSim::TransactionCompleteCB *write_cb = new DRAMSim::Callback<DRAMSimCallBack, void, unsigned, uint64_t, uint64_t>(&cb, &DRAMSimCallBack::write_complete);
      mem = DRAMSim::getMemorySystemInstance("sim/config/DDR3_micron_16M_8B_x8_sg15.ini", "sim/config/dramsys.ini", "..", "Apollo", 16384); 
      mem->RegisterCallbacks(read_cb, write_cb, NULL);
      mem->setCPUClockSpeed(2000000000);  

      // CHECK SIMULATOR CONFIG flags
      if (cfg.CF_one_context_at_once) {
         cf_iterator = 0;
         createContext( cf.at(cf_iterator) );  // create just the very firt context from <cf>
      }
      else if (cfg.CF_all_contexts_concurrently)  // create ALL contexts to max parallelism
         for ( cf_iterator=0; cf_iterator < cf.size(); cf_iterator++ )
            createContext( cf.at(cf_iterator) );
   }

   void update_memop_queue(map<uint64_t,deque<tuple<int,int,bool,bool>>>& memop_queue_map, uint64_t addr, tuple<int,int,bool,bool> memop_queue_entry) {   
      deque<tuple<int, int, bool, bool>>* addr_queue;
      if(memop_queue_map.find(addr)==memop_queue_map.end()) { //there's no memop_queue for addr
         addr_queue->push_back(memop_queue_entry);
         memop_queue_map[addr]=*addr_queue;
      }
      else{   
         deque<tuple<int,int,bool,bool>>* addr_queue = &memop_queue_map.at(addr);
         deque<tuple<int,int,bool,bool>>::iterator it = addr_queue->begin();   
         while (it!=addr_queue->end() && !((get<0>(*it)==get<0>(memop_queue_entry)) && (get<1>(*it)==get<1>(memop_queue_entry))) ) {
            ++it;
         }
         assert( it!=addr_queue->end() ); //there should have been an entry
         *it=memop_queue_entry;
      }
   }

   void createContext(int bbid)
   {
      assert( bbid < g.bbs.size() );
      /* Create Context */
      int cid = context_list.size();
      Context *c = new Context(cid);
      context_list.push_back(c);
      BasicBlock *bb = g.bbs.at(bbid);
      c->initialize(bb, curr_owner, deps, handled_phi_deps, load_queue_map, store_queue_map, memory_ref);
      cout << "Context [" << cid << "]: Created (BB=" << bbid << ")\n";     
   }

   bool memop_older(tuple<int,int,bool, bool> memop_1,tuple<int,int,bool, bool> memop_2) {
      //  mem_1 is at an earlier context OR same context but mem1 node id is lower
      return (get<0>(memop_1) < get<0>(memop_2)) || ((get<0>(memop_1) == get<0>(memop_2)) && (get<1>(memop_1) < get<1>(memop_2)));
   }

   bool memop_completed(tuple<int,int,bool, bool> memop) {
      return get<3>(memop);
   } 

   bool memop_started(tuple<int,int,bool, bool> memop) {
      return get<2>(memop);
   }

   //return pointer to preceeding store if curr_memop is load and vice versa  
   deque<tuple<int,int,bool,bool>>::iterator most_recent_memop (deque<tuple<int,int,bool,bool>> memop_queue, tuple<int,int,bool,bool> curr_memop, uint64_t addr) {
      bool older_memop_exists=false;
      std::deque<tuple<int,int,bool,bool>>::iterator it=memop_queue.begin();
      while (it!=memop_queue.end() && memop_older(*it,curr_memop)) { //continue looping until you reach 1st instruction younger than curr_memop
         ++it;
         older_memop_exists=true;
      }

      if (it==memop_queue.end() && !older_memop_exists)
         return it;
      return --it; //return most recent preceeding memop
   }

   void process_context(Context *c)
   {
      // traverse ALL the active instructions in the context
      for (int i=0; i < c->active_list.size(); i++) {
         Node *n = c->active_list.at(i);
         if (c->start_set.find(n) != c->start_set.end()) {
            cout << "Node [" << n->name << " @ context " << c->id << "]: Starts Execution \n";
            /* check if it's a Memory Request -> enqueue in DRAMsim */
            if (n->type == LD || n->type == ST) {
            
               uint64_t addr = memory.at(n->id).front();  // get the 1st (oldest) memory access for this <id>
      
               //Luwa: we just move to the next active node and check again in next cycle if mem dependency is resolved
               bool can_receive_forward=false;
               bool must_stall=false;
               tuple<int,int,bool,bool> curr_memop=make_tuple(c->id,n->id,false,false);
             
               if (store_queue_map.find(addr)!=store_queue_map.end()) { //store queue for addr exists, otherwise no forwarding or stalling necessary, just access memory as usual 
                  deque<tuple<int,int,bool,bool>>::iterator recent_store_ptr = most_recent_memop(store_queue_map.at(addr),curr_memop, addr);       
                  if (n->type==LD && most_recent_memop(store_queue_map.at(addr),curr_memop, addr)!=store_queue_map.at(addr).end()) { //meaning we found an older store        
                     if (!memop_started(*recent_store_ptr)) { 
                        c->next_active_list.push_back(n);
                        c->next_start_set.insert(n);
                        continue;
                     }         
                     else { //here, no need to stall AND we can receive load from prev store
                        memory.at(n->id).pop();
                        c->next_active_list.push_back(n);    
                        cout << "Node [" << n->name << " @ context " << c->id << "]: Receiving Store-Load Forwarding for Address "<< addr << "\n";
                        update_memop_queue(load_queue_map,addr,make_tuple(c->id,n->id,true,false)); //set entry in load queue to started
                        c->remaining_cycles_map.at(n)=0; //single cycle access to store queue       
                        continue;
                     }
                  }
                   
                  if (n->type==ST && recent_store_ptr!=store_queue_map.at(addr).end()) { //there's actually an older stor
                     update_memop_queue(store_queue_map,addr,make_tuple(c->id,n->id,true,false)); 
                     if (!memop_completed(*recent_store_ptr)) {
                        //add to store queue, mark as started (so we can forward to loads), but can't push to DRAM yet
                        c->next_active_list.push_back(n);
                        c->next_start_set.insert(n);
                        continue;
                     } //stores must commit (into DRAMSim) in order      
                  }
               }
               else {
                  cout << "Node [" << n->type << " @ context " << c->id << "]: NOT Receiving Store-Load Forwarding for Address "<< addr << "\n";
               }
          
               memory.at(n->id).pop(); // ...and take it out of the queue
               cout << "Node [" << n->name << " @ context " << c->id << "]: Inserts Memory Transaction for Address "<< addr << "\n";
               assert(mem->willAcceptTransaction(addr));
               if (n->type == LD){ 
                  mem->addTransaction(false, addr);        
                  update_memop_queue(load_queue_map,addr,make_tuple(c->id,n->id,true,false)); //set entry in load queue to started
               }
               else if (n->type == ST) {
                  mem->addTransaction(true, addr);
                  update_memop_queue(store_queue_map,addr,make_tuple(c->id,n->id,true,false)); //set entry in store queue to started
               }
               if (cb.outstanding_accesses_map.find(addr) == cb.outstanding_accesses_map.end())
                  cb.outstanding_accesses_map.insert(make_pair(addr, queue<std::pair<Node*, Context*>>()));
               cb.outstanding_accesses_map.at(addr).push(make_pair(n, c));
            }
         }
         //else 
         //   cout << "Node [" << n->name << " @ context " << c->id << "]: In Processing \n";

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
            cout << "Node [" << n->name << " @ context " << c->id << "]: Finished Execution \n";
      
            // If node <n> is a TERMINATOR create new context with next bbid in <cf> (a bbid list)
            //     iff <simulation mode> is "CF_one_context_at_once"
            if (cfg.CF_one_context_at_once && n->type == TERMINATOR) {
               if ( cf_iterator < cf.size()-1 ) {  // if there are more pending contexts in the Graph
                  cf_iterator++;
                  createContext( cf.at(cf_iterator) );
               }
            }
            c->remaining_cycles_map.erase(n);
            if (n->type != NAI)
               c->processed++;
            
            // Since node <n> ended, update dependents: decrease each dependent's parent count & try to launch each dependent
            set<Node*>::iterator it;
            for (it = n->dependents.begin(); it != n->dependents.end(); ++it) {
               Node *d = *it;
               c->pending_parents_map.at(d)--;
               c->launchNode(d);
            }

            // The same for external dependents: decrease parent's count & try to launch them
            if (n->external_dependents.size() > 0) {
               DNode src = make_pair(n, c->id);
               if (deps.find(src) != deps.end()) {
                  vector<DNode> users = deps.at(src);
                  for (int i=0; i<users.size(); i++) {
                     Node *d = users.at(i).first;
                     Context *cc = context_list.at(users.at(i).second);
                     cc->pending_external_parents_map.at(d)--;
                     cc->launchNode(d);
                  }
                  deps.erase(src);
               }
               curr_owner.at(n).second = true;
            }

            // Same for Phi dependents
            for (it = n->phi_dependents.begin(); it != n->phi_dependents.end(); ++it) {
               int next_cid = c->id+1;
               Node *d = *it;
               if ((cf.size() > next_cid) && (cf.at(next_cid) == d->bbid)) {
                  if (context_list.size() > next_cid) {
                     Context *cc = context_list.at(next_cid);
                     cc->pending_parents_map.at(d)--;
                     cc->launchNode(d);
                  }
                  else {
                     if (handled_phi_deps.find(next_cid) == handled_phi_deps.end()) {
                        handled_phi_deps.insert(make_pair(next_cid, set<Node*>()));
                     }
                     handled_phi_deps.at(next_cid).insert(d);
                  }
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
      // Check if the current context is done
      if ( c->processed == g.bbs.at(c->bbid)->inst_count ) {
         cout << "Context [" << c->id << "]: Finished Execution (Executed " << c->processed << " instructions) \n";
         c->live = false;
         // TODO: call the Context destructor to free memory ??
      }
   }

   void process_memory()
   {
      mem->update();
   }

   bool process_cycle()
   {
      cout << "[Cycle: " << cycle_count << "]\n";
      cycle_count++;
      bool simulate = false;
      assert(cycle_count < 2000);

      // process ALL the LIVE contexts
      for (int i=0; i<context_list.size(); i++) {
         if (context_list.at(i)->live){
            simulate = true;
            process_context( context_list.at(i) );
         }
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
   readGraph(sim.g);
   readProfMemory(sim.memory);
   readProfCF(sim.cf);
   sim.initialize();
   sim.run();
   return 0;
} 
