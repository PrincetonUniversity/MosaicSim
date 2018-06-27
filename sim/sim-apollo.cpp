//=======================================================================
// Copyright 2018 Princeton University.
//=======================================================================

#include "sim-apollo.hpp"
#include "DRAMSim.h"

using namespace apollo;
using namespace std;
/*

class Memop {
  public:
  uint64_t addr;
  Node* node;
  Context* context;
  bool started=false;
  bool completed=false;
 
  Memop(uint64_t addr,Node* node,Context* context) :
    addr(addr),node(node),context(context){}
 
  bool operator==(Memop other_memop) {
    return (addr==other_memop.addr) && (node->id==other_memop.node->id) && (context->id==other_memop.context->id);
  }
  bool operator!=(Memop other_memop) {
    return !(*this==other_memop);
  }
 
  bool operator<(Memop other_memop) {
    //  mem_1 is at an earlier context OR same context but mem1 node id is lower
    return (context->id < other_memop.context->id) || (((context->id == other_memop.context->id)) && (node->id < other_memop.node->id));
  }
 
  bool exists_unresolved_memop (deque<Memop>* memop_q, TInstr op_type) {    
    for (deque<Memop>::iterator it = memop_q->begin(); it!=memop_q->end() ; ++it) {
      if(*it==*this || *this<*it)
      return false;
      if(!(it->started) && it->node->typeInstr==op_type)
        return true;
    }
    return false;
  }

  bool exists_conflicting_alias (deque<Memop>* memop_q, TInstr op_type) {    
    for (deque<Memop>::iterator it = memop_q->begin(); it!=memop_q->end() ; ++it) {
      if(*it==*this || *this<*it)
        return false;
      if(!(it->completed) && it->node->typeInstr==op_type && addr==it->addr)
        return true;
    }
    return false;
  }

  void update_in(deque<Memop>* memop_q) {
    deque<Memop>::iterator it = memop_q->begin();
    while (it!=memop_q->end() && *it!=*this) {
      ++it;     
    }
    if (it==memop_q->end()) {
      memop_q->push_back(*this);
    }
    else {
      *it=*this;
    }      
  }
};
*/

class MemOp {
public:
  uint64_t addr;
  DNode d;
  bool started=false;
  bool completed=false;
  MemOp(uint64_t addr, DNode d) : addr(addr), d(d){}
};

class LoadStoreQ {
public:
  map<DNode, MemOp*> tracker;
  deque<MemOp> q;
  bool initialized = false;
  int size;
  void initialize(int s) { 
    size = s;
    initialized = true;
  }
  bool checkSize(int requested_space) {
    if(q.size() <= size - requested_space) {
      return true;
    }
    else {
      int ct = 0;
      for(deque<MemOp>::iterator it = q.begin(); it!= q.end(); ++it) {
        if(it->completed)
          ct++;
        else
          break;
      }
      if(ct > requested_space) {
        for(int i=0; i<requested_space; i++)
          q.pop_front();
        return true;
      }
      return false;
    }
  }
  void insert(uint64_t addr, DNode d) {
    assert(initialized);
    MemOp *temp = new MemOp(addr, d);
    q.push_back(*temp);
    tracker.insert(make_pair(d, temp));
  }
  bool exists_unresolved_memop (DNode in, TInstr op_type) {    
    for(deque<MemOp>::iterator it = q.begin(); it!= q.end(); ++it) {
      Node *n = it->d.first;
      int cid = it->d.second;
      if(it->d == in)
        return false;
      else if(cid > in.second || ((cid == in.second) && (n->id > in.first->id)))
        return false;
      if(!(it->started) && n->type == op_type)
        return true;
    }
    return false;
  }
  bool exists_conflicting_memop (DNode in, TInstr op_type) {
    uint64_t addr = tracker.at(in)->addr;
    for(deque<MemOp>::iterator it = q.begin(); it!= q.end(); ++it) {
      Node *n = it->d.first;
      int cid = it->d.second;
      if(it->d == in)
        return false;
      else if(cid > in.second || ((cid == in.second) && (n->id > in.first->id)))
        return false;
      if(!(it->completed) && n->type == op_type && it->addr == addr)
        return true;
    }
    return false;
  }
};
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
  
  void initialize(BasicBlock *bb, std::map<Node*, std::pair<int, bool> > &curr_owner, std::map<DNode, std::vector<DNode> > &deps, std::map<int, std::set<Node*> > &handled_phi_deps, LoadStoreQ &lsq, map<int, queue<uint64_t> >& memory) {
 
    assert(bbid == -1);
    bbid = bb->id;
    int mem_count = 0;
    for ( int i=0; i<bb->inst.size(); i++ ) {
      Node *n = bb->inst.at(i);
      if (n->type == ST || n->type == LD)
        mem_count++;
    }
    if(!lsq.checkSize(mem_count))
      assert(false); // Context creation failed due to limited LSQ size
    active_list.push_back(bb->entry);
    remaining_cycles_map.insert(std::make_pair(bb->entry, 0));
    
    // traverse all the BB's instructions and initialize the pending_parents map
    for ( int i=0; i<bb->inst.size(); i++ ) {
      Node *n = bb->inst.at(i);

      if (n->typeInstr == ST || n->typeInstr == LD) {
        //add to store queue as soon as context is created. this way, there's a serialized ordering of stores in lsq
        uint64_t addr = memory.at(n->id).front(); //in this context, this is always the only store to be done by node n
        lsq.insert(addr, make_pair(n, id));
        memory.at(n->id).pop();
        //Memop memop = *(new Memop(addr,n,this));
        //lsq->push_back(memop);
        //memop.update_in(lsq); //add new entry to lsq
      }
          
      if(n->typeInstr == PHI)
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
        sim->lsq.tracker.at(make_pair(n,c->id))->completed = true;
        //Context::Memop memop = *(new Context::Memop(addr,n,c));
        //memop.completed=true;
        //memop.update_in(sim->lsq);
        cout << "Node [" << n->name << " @ context " << c->id << "]: Read Transaction Returns "<< addr << "\n";
        if (c->remaining_cycles_map.find(n) == c->remaining_cycles_map.end())
          cout << *n << " / " << c->id << "\n";
        c->remaining_cycles_map.at(n) = 0; // mark "Load" as finished for sim-apollo

        q.pop();
        if (q.size() == 0)
          outstanding_accesses_map.erase(addr);
      }
      void write_complete(unsigned id, uint64_t addr, uint64_t clock_cycle) {
        assert(outstanding_accesses_map.find(addr) != outstanding_accesses_map.end());
        queue<std::pair<Node*, Context*>> &q = outstanding_accesses_map.at(addr);
        Node *n = q.front().first;
        Context *c = q.front().second;
        sim->lsq.tracker.at(make_pair(n,c->id))->completed=true;
        //Context::Memop memop = *(new Context::Memop(addr,n,c));
        //memop.completed=true;
        //memop.update_in(sim->lsq);
        //c->remaining_cycles_map.at(n) = 0; //TJH: Stores are finished after they write to store buffer so we don't update latency
         
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

  vector<int> cf; // List of basic blocks in "sequential" program order 
  int cf_iterator = 0;
  map<int, queue<uint64_t> > memory; // List of memory accesses per instruction in a program order
  //map<int, queue<uint64_t> >& memory_ref=memory;
  /* Handling External Dependencies */
  map<Node*, pair<int, bool> > curr_owner; // Source of external dependency (Node), Context ID for the node (cid), Finished
  map<DNode, vector<DNode> > deps; // Source of external dependency (Node,cid), Destinations for that source (Node, cid)
  /* Handling Phi Dependencies */
  map<int, set<Node*> > handled_phi_deps; // Context ID, Processed Phi node
  LoadStoreQ lsq;
   
  // **** simulator CONFIGURATION parameters
  struct {

    int lsq_size = 512;
    bool CF_one_context_at_once = true;
    bool CF_all_contexts_concurrently = false;

    // define an array of FUs
    TFU resource_array[MAX_FU_types];

  } cfg;

  map<TypeofFU, int> remaining_resources_map;  // tracks remaining resources for each type of FU

  void initFU(TypeofFU type, int lat, int max) {
    cfg.resource_array[type].lat = lat;
    cfg.resource_array[type].max = max;
    remaining_resources_map.insert( std::make_pair(type, max) );
  }

  void initResources() {
    // TODO: read FU parameters from a config file
    initFU(FU_I_ALU,    1, 10);  // format: FU_type, latency, max_units
    initFU(FU_FP_ALU,   1, 10);
    initFU(FU_I_MULT,   3, 10);
    initFU(FU_FP_MULT,  3, 10);
    initFU(FU_I_DIV,    9, 10);
    initFU(FU_FP_DIV,   9, 10);
    initFU(FU_BRU,      1, 10);    
    initFU(FU_IN_MEMPORT,  -1, 10);      // IN_MEMPORT (loads) does not have a pre-defined latency (the memory hierarchy gives) 
    initFU(FU_OUT_MEMPORT, 3, 10);       // OUT_MEMPORT (stores) 
    initFU(FU_OUTSTANDING_MEM, -1, 10);  // OUTSTANDING_MEM does not have a latency
  }
  // **** end of configuration parameters stuff **

  void initialize() {
    DRAMSim::TransactionCompleteCB *read_cb = new DRAMSim::Callback<DRAMSimCallBack, void, unsigned, uint64_t, uint64_t>(&cb, &DRAMSimCallBack::read_complete);
    DRAMSim::TransactionCompleteCB *write_cb = new DRAMSim::Callback<DRAMSimCallBack, void, unsigned, uint64_t, uint64_t>(&cb, &DRAMSimCallBack::write_complete);
    mem = DRAMSim::getMemorySystemInstance("sim/config/DDR3_micron_16M_8B_x8_sg15.ini", "sim/config/dramsys.ini", "..", "Apollo", 16384); 
    mem->RegisterCallbacks(read_cb, write_cb, NULL);
    mem->setCPUClockSpeed(2000000000);  
    lsq.initialize(cfg.lsq_size);
    if (cfg.CF_one_context_at_once) {
      cf_iterator = 0;
      createContext( cf.at(cf_iterator) );  // create just the very firt context from <cf>
    }
    else if (cfg.CF_all_contexts_concurrently)  // create ALL contexts to max parallelism
      for ( cf_iterator=0; cf_iterator < cf.size(); cf_iterator++ )
        createContext( cf.at(cf_iterator) );
  }

  void createContext(int bbid)
  {
    assert( bbid < g.bbs.size() );
    /* Create Context */
    int cid = context_list.size();
    Context *c = new Context(cid);
    context_list.push_back(c);
    BasicBlock *bb = g.bbs.at(bbid);
    c->initialize(bb, curr_owner, deps, handled_phi_deps, lsq, memory);
    cout << "Context [" << cid << "]: Created (BB=" << bbid << ")\n";     
  }

  void process_context(Context *c)
  {
    // traverse ALL the active instructions in the context
    for (int i=0; i < c->active_list.size(); i++) {
      Node *n = c->active_list.at(i);
      if (c->start_set.find(n) != c->start_set.end()) {
        
        // check resource availability before continuing
        // if ( remaining_resources_map


        cout << "Node [" << n->name << " @ context " << c->id << "]: Starts Execution \n";
        /* check if it's a Memory Request -> enqueue in DRAMsim */
        if (n->typeInstr == LD || n->typeInstr == ST) {
        
          //uint64_t addr = memory.at(n->id).front();  // get the 1st (oldest) memory access for this <id>

          bool must_stall=false;
          //Context::Memop memop=*(new Context::Memop(addr,n,c));
          //memop.started=true;
          //memop.update_in(lsq);
          DNode d = make_pair(n,c->id);
          lsq.tracker.at(d)->started = true;

          //you have to stall if any older loads or stores haven't started (i.e., don't know their address yet)
          //you also have to stall if all the older loads or stores know their address, but one hasn't completed 

          if (n->type == ST)
            //must_stall=memop.exists_unresolved_memop(lsq,ST) || memop.exists_unresolved_memop(lsq,LD) || memop.exists_conflicting_alias(lsq,ST) || memop.exists_conflicting_alias(lsq,LD);
            must_stall = lsq.exists_unresolved_memop(d, ST) || lsq.exists_unresolved_memop(d, LD) || lsq.exists_conflicting_memop(d, ST) || lsq.exists_conflicting_memop(d, LD);
          else if (n->type == LD)
            must_stall = lsq.exists_unresolved_memop(d, ST) || lsq.exists_conflicting_alias(d, ST);
            //must_stall=memop.exists_unresolved_memop(lsq,ST) ||  memop.exists_conflicting_alias(lsq,ST);         
      
          if(must_stall) {
            c->next_active_list.push_back(n);
            c->next_start_set.insert(n);
            continue;
          }
          uint64_t addr = lsq.tracker.at(d)->addr;
       
          //memory.at(n->id).pop(); // ...and take it out of the queue
          cout << "Node [" << n->name << " @ context " << c->id << "]: Inserts Memory Transaction for Address "<< addr << "\n";
          assert(mem->willAcceptTransaction(addr));
          if (n->typeInstr == LD) { 
            mem->addTransaction(false, addr);
          }
          else if (n->typeInstr == ST) {
            mem->addTransaction(true, addr);
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
        if (cfg.CF_one_context_at_once && n->typeInstr == TERMINATOR) {
          if ( cf_iterator < cf.size()-1 ) {  // if there are more pending contexts in the <cf> vector
            cf_iterator++;
            createContext( cf.at(cf_iterator) );
          }
        }
        c->remaining_cycles_map.erase(n);
        if (n->typeInstr != ENTRY)
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
      // TODO: call the Context "destructor" to free memory !!
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

    // process ALL LIVE contexts
    for (int i=0; i<context_list.size(); i++) {
      if (context_list.at(i)->live) {
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
  sim.initResources();
  readGraph(sim.g, sim.cfg.resource_array);
  readProfMemory(sim.memory);
  readProfCF(sim.cf);
  sim.initialize();
  sim.run();
  return 0;
} 
