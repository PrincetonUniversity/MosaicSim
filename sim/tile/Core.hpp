#pragma once

#include "../sim.hpp"
#include "../memsys/CommMec.hpp"
#include "Bpred.hpp"
#include "LoadStoreQ.hpp"
#include "Tile.hpp"
#include "../misc/Memory_helpers.hpp"
#include "DESCQ.hpp"

using namespace std;

class DyanmicNode;
class Graph;
class Simulator;
class Context;
class DRAMSimInterface;
class Cache;
class L1Cache;
class L2Cache;

class DESCQ;

/** \brief   */
typedef chrono::high_resolution_clock Clock;

struct DNPointerLT {
  /** \brief   */
  bool operator()(const DynamicNode* a, const DynamicNode* b) const {
        return *a < *b;
    }
};

class IssueWindow {
public:
  /** \brief   */
  map<DynamicNode*, uint64_t, DNPointerLT> issueMap;
  /** \brief   */
  vector<DynamicNode*> issueQ;
  /** \brief   */
  vector<DynamicNode*> barrierVec;
 
  /** \brief instrn window size   */
  int window_size=1;
  /** \brief   */
  uint64_t window_start=0;
  /** \brief   */
  uint64_t curr_index=0;
  /** \brief   */
  uint64_t window_end=window_size-1;
  /** \brief total # issues per cycle */
  int issueWidth=1;
  /** \brief   */
  int issueCount=0;
  /** \brief   */
  uint64_t cycles=0;

  /** \brief Inserts a DyncamicNode into the issueMap and increases
      the curr_index.

      It also records a barrier instruction into the barrierVec
   */
  void insertDN(DynamicNode* d);
  /** \brief Check the window's size if there is space for an other
   instruction.

   It also makes sure that there is not an active barrier.
   */
  bool canIssue(DynamicNode* d);
  /** \brief   

   */
  void issue(DynamicNode* d);
  /** \brief Process the completed DynamicNode s in issue Map.

      Nodes that are marked as can_exit_rob are also treated here. 
   */
  void process();
};


class Core: public Tile {
public:
  /** \brief   */
  Graph g;
  /** \brief   */
  IssueWindow window;
  /** \brief   */
  bool windowFull=false;
  /** \brief   */
  Config local_cfg; 
  /** \brief   */
  L1Cache* cache;
  /** \brief   */
  L2Cache* l2_cache;
  /** \brief   */
  Bpred *bpred;
  /** \brief   */
  Statistics *stats;
  
  /** \brief   */
  chrono::high_resolution_clock::time_point curr;
  /** \brief   */
  chrono::high_resolution_clock::time_point last;
  /** \brief   */
  uint64_t last_processed_contexts;
  
  /** \brief   */
  vector<Context*> context_list;
  /** \brief   */
  vector<Context*> live_context;
  /** \brief   */
  int context_to_create = 0;
  /** \brief Number of context that has been created up to date. */
  uint64_t total_created_contexts = 0;
  /** \brief Number of context that has been destroied up to date.  */
  uint64_t total_destroied_contexts = 0;

  /* Resources */
  /** \brief   */
  unordered_map<int, int> available_FUs;
  /** \brief   */
  unordered_map<BasicBlock*, int> outstanding_contexts;
  /** \brief   */
  double total_energy = 0.0;
  /** \brief   */
  double avg_power = 0.0;

  /* Dynamic Traces */
  /** \brief List of basic blocks in "sequential" program order   */
  vector<int> cf;
  /** \brief how many entries we have deleted from cf */
  uint64_t deleted_cf = 0;
  /** \brief  */
  uint64_t max_cf_size = 150000;
  /** \brief  map of basic blocks indicating if "conditional" and "destinations" */
  map<int, pair<bool,set<int>>> bb_cond_destinations; 
  /** \brief   */
  bool finished_dyn_data = false;
  int last_bbid = -1;
  bool pilot_descq = false;
  /** \brief List of memory accesses per instruction in a program order */
  unordered_map<int, queue<uint64_t> > memory; 
  /** \brief List of memory accesses per instruction in a program order for the accelerator */
  queue<string> acc_args;  
  deque<pair<int, int>> partial_barrier_sizes;  

  /** \brief   */
  unordered_map<uint64_t, DynamicNode*> access_tracker;
  /** \brief   * Handling External/Phi Dependencies */
  unordered_map<int, Context*> curr_owner;
  
  /** \brief LSQ    */
  LoadStoreQ lsq=LoadStoreQ(false);
  /** \brief DESCQ, the inter-core communication bufffer */
  DESCQ *descq;
  int verbLevel;
  bool debug_mode;
  bool mem_stats_mode;
  int technology_node;
  /** \brief The communiciation buffer that is used to communicate
      with the accelerator tile */ 
  PP_static_Buff<string> *acc_comm;  
  int acc_transaction_id = 0;
  
  /** \brief   */
  Core(Simulator* sim, int clockspeed, bool pilot_descq);
  
  /** \brief Initializes internal structure (L1 and L2 cache, window, bpred etc.) */
  void initialize(int id, Simulator *sim, PP_static_Buff<string> *acc_comm);
  /** \brief Creates the upcoming Context and initializes some
      internal structers.  */
  bool createContext();
  /** \brief Process all the different comoponents of the core.

      It calls the process method for its wincod, L1 and L2 caches.
      Additionally it processthe live_context and trys to complete
      each one of them. Also, new context are created if there is
      enough resources.
  */
  bool process();
  /** \brief Returns true if the core can access the address
      addr. False if not.

      Other then checking for free ports, this method checks if that
      memory region is locked.
  */
  bool canAccess(bool &isLoad, uint64_t addr);
  /** \brief Creates a new tranasaction to access the memory adress
      tied to the Dynamic node d.  */
  void access(DynamicNode *d);
  /** \brief Handles completed memory transactions   */
  void accessComplete(MemTransaction *t);
  /** \brief Deletes finished contexts.
      
      It deletes contexts that are one milion cycles old. 
   */
  void deleteErasableContexts();  
  /** \brief Calculates the energy used by the core. 

      For inorder cores, we have the consumption per each instruction,
      so we calculate the sum of all instruction executed. For out of
      order cores, we just multiply the number of cycles with the
      constant of 6.875 (based on Xeon E7-8894V4 from McPAT on 22nm).
   */
  void calculateEnergyPower();
  /** \brief Return whether the branch has been correctly predicted or not  */
  bool predict_branch_and_check(DynamicNode* d);
  /** \brief   */
  string getInstrName(TInstr instr);
  /**  \brief Forwards the cycle counter.
       
       Forwards the cycle counter for the core, l1 and l2 caches, window
       of () and the DESCQ.
       Note, unused. 
  */
  void fastForward(uint64_t inc);
  /** \brief Reads for the input file data. 

      Reads the control flow and memory accesses.
   */ 
  void read_dyn_data(int *data);
  /** \brief Reads for the input for accelerators and partial barrier.

      Reads the accelerators and partial barrier dynamic info.
   */ 
  void read_acc_data();
};
