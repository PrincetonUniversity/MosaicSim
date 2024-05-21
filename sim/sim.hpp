#pragma once

#include <chrono>
#include <numeric>
#include "common.hpp"
#include "tile/DynamicNode.hpp"
#include "tile/Tile.hpp"
#include "tile/Core.hpp"
#include "tile/LoadStoreQ.hpp"
#include "memsys/SimpleDRAM.hpp"
#include "memsys/Cache.hpp"
#include "misc/Reader.hpp"
#include "misc/Statistics.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <omp.h>

using namespace std;

class DRAMSimInterface;
class Core;
class L2Cache;
class LLCache;
class Transaction;
class DESCQ;
class TransQueue;
class SimpleDRAM;

/** \brief Handler for the barrier instruction.  */
class Barrier {
public:
  /** \brief Number of cores  */
  int num_cores=0;
  /** \brief map from tile_id to DynamicNode    */
  map<int, DynamicNode*> barrier_map; //
  /** \brief  return true if barrier successfully registered. 
      Frees all barriers once #num_cores count is reached.
   */
  bool register_barrier(DynamicNode* d); //
};

/** \brief 

    TODO make this part of Statistics   */
struct loadStat {
  /** \brief   */
  uint64_t addr;
  /** \brief   */
  TInstr type;
  /** \brief   */
  long long issueCycle;
  /** \brief   */
  long long completeCycle;
  /** \brief   */
  int hit;
  /** \brief   */
  int nodeId;
};

/** \brief 

    TODO make this part of Statistics   */
struct runaheadStat {
  /** \brief   */
  int runahead; 
  /** \brief   */
  int coreId;
  /** \brief   */
  int nodeId;
};

/** \brief Orcestrates all the components of the simulator
    and creates the symphony. 
*/
class Simulator {
public:
  /** \brief Initial time that the simulation started  */
  chrono::high_resolution_clock::time_point init_time;
  /** \brief Temporary variable containing the current time */
  chrono::high_resolution_clock::time_point curr_time;
  /** \brief Temporary variable containing the current time */
  chrono::high_resolution_clock::time_point last_time = chrono::high_resolution_clock::now();
  /** \brief System file name for the DRAMSim3 

      TODO: make it part of the Config class. 
  */
  string DRAM_system;
  /** \brief Device file name for the DRAMSim3 

      TODO: make it part of the Config class. 
  */
  string DRAM_device;
  
  /** \brief Statistics for the Simulator class */
  Statistics *global_stat;
  /** \brief Configuration for the Simulator class */
  Config *cfg;

  //MLP stats
  /** \brief  cycles in which to collect mlp stats  */
  int mlp_epoch=1024; 
  /** \brief    */
  int curr_epoch_accesses=0; 
  /** \brief  outgoing dram accesses per epoch  */
  vector<int> accesses_per_epoch; 
  /** \brief  granularity of reading from trace file   */
  long long mem_chunk_size=1024; 
  /** \brief    */
  uint64_t last_instr_count = 0;
  /** \brief toto */
  uint64_t cycles=0;
  /** \brief    */
  map<int,Tile*> tiles;
  /** \brief    */
  int tileCount=0;
  /** \brief    */
  vector<uint64_t> clockspeedVec;
  /** \brief UNUSED!  */
  bool decoupling=false;
  /** \brief    */
  bool debug_mode=false;
  /** \brief    */
  bool mem_stats_mode=true;
  /** \brief    */
  bool seq_exe=false;
  /** \brief   */
  string outputDir="";
  /** \brief   */
  ofstream epoch_stats_out;
  /** \brief   */
  // Statistics epoch_stats;
  /** \brief   */
  vector<DESCQ*> descq_vec;
  /** \brief   */
  Barrier* barrier = new Barrier();
  /** \brief   */
  LLCache* cache;
  /** \brief every tile has a transaction priority queue   */
  unordered_map<int,priority_queue<TransactionOp, vector<TransactionOp>, TransactionOpCompare>> transq_map;
  /** \brief   */
  array< map<int, DynamicNode*>, 10000> partial_barrier_maps;
  /** \brief   */
  int transq_latency=3;
  /** \brief   */
  int clockspeed=2000; //default clockspeed in MHz
  /** \brief Number of cores  used.

      Can be different from number of tiles, since a tile can be an
      accelerator. */
  int nb_cores;
  /** \brief Number of files/pipes used for the dynamic files. */
  int nb_files = 1;
  /** \brief */
  DRAMSimInterface* memInterface;

  /** \brief   */
  //TODO: This should be part of the stats, but ti is used in sim::run as a condition on should the sim continue simulating. Find a more elegant way to do this (before parallelization */
  /* unordered_map<DynamicNode*, tuple<long long, long long, bool>> load_stats_map; */
  //TODO: REINTRODUCE_STATS
  /* /\** \brief   *\/ */
  /* uint64_t load_count=0; */
  /* /\** \brief   *\/ */
  /* vector<runaheadStat> runaheadVec; */
  /* /\** \brief   *\/ */
  /* vector<loadStat> load_stats_vector; */
  /* /\** \brief   *\/ */
  /* unordered_map<DynamicNode*, uint64_t> recvLatencyMap; */
  /* /\** \brief   *\/ */
  /* uint64_t total_recv_latency=0; */

  //TODO: REINTRODUCE_STATS
  /* /\** \brief   *\/ */
  /* vector<int> commQSizes; */
  /* /\** \brief   *\/ */
  /* vector<int> SABSizes;   */
  /* /\** \brief   *\/ */
  /* vector<int> SVBSizes; */
  /* /\** \brief   *\/ */
  /* vector<int> termBuffSizes; */
  /* /\** \brief   *\/ */
  /* int commQMax = 0; */
  /* /\** \brief   *\/ */
  /* int SABMax = 0; */
  /* /\** \brief   *\/ */
  /* int SVBMax = 0;  */
  /* /\** \brief   *\/ */
  /* int termBuffMax = 0; */

  /** \brief Control  flag for reading  input files. 
      
      0 - dynamic data pipes
      1 - dynamic data from binray files
      2 - dynamic data from ASCI files
  */
  int input_files_type = 0;
  /** \brief Dynamic data input pipe */
  vector<int> dyn_files;
  /** \brief Dynamic data input pipe */
  vector<int> acc_files;

  /** 
      \brief Forwards everything for #inc normalized cycles with respect
      to the speed of tile #src_tid.
      
      Forwards \link #cycles the internal counter \endlink, the memory and the core by inc
      cycles normalized with respect to the speed of tile src_tid.
      Possible problems: the cache is incremented here and in the
      Core::fastForward method as well.
  */
  void fastForward(int tid, uint64_t inc);
  /** \brief The main method that will execute all the instructions. 
      
      The main method that will executes all the available
      instruction by calling the Tile::process() method on each
      tile repeatedly.

      Once all the execution is done, this method takes care of
      printing all the global statistics. It also prints out global
      statistics every Statistics::printInterval cycles.  

      \todo Move all the printing in separate methods and just call
      them here.
   */
  void run();
  /** \brief Adds a new Core to the Simulator 
      
      This is the method that will read the instruction Graph, memory
      accesses etc. It also creates the DESCQ object. It calls internally 
      Simulator::regiserTile and Core::initialize(). 

      \todo cfgpath and cfgname are used only once. pass them as one
      parametar 
      
      \todo isntead of calling this fucntion, create a new
      object Core, and initialize everything that is needed. then pass
      the object to registerTile() method.
   */
  void registerCore(string wlpath, string cfgfile, PP_static_Buff<string> *acc_comm);
  /** \brief Resgister a new Tile as number tid

      Same as regiserCore(), BUT this method does not increase teh
      tileCount

      \warning tileCount not increased! Use this function only to swap
      Tile and not add new one.
   */
  void registerTile(Tile* tile);
  /** \brief 
      
   */
  bool InsertTransaction(Transaction* t, uint64_t cycle);
  /** \brief Register a partial barrier with size num_cores
      
   */
  bool register_partial_barrier(DynamicNode* d, int id, int num_cores);
  /** \brief Gets the correct DESCQ using the tile that executes the
      DynamicNode

      @parm d The DynamicNode 
   */
  int getAccelerator();
  /**\brief Writes all global stats and the stats for each tile on the
     standard output and in separate files
     
     TODO: need to add some control over this
  */
  void printStats();

  /**\brief Calculates and prints the energy stats  */
  void calculateGlobalEnergyPower();
  
  void releaseLock(DynamicNode* d);
  void read_dyn(int dyn_file);
  /** \brief The constructor of the Simulator class
      
   */
  Simulator(string config,
	    string DRAM_system = "DDR3_micron_16M_8B_x8_sg3",
	    string DRAM_device = "DDRsys" );
  
};


