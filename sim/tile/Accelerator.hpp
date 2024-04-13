#ifndef ACCELERATOR_H
#define ACCELERATOR_H

#include "../common.hpp"
#include "../sim.hpp"
#include "../memsys/CommMec.hpp" 
#include "Tile.hpp"
#include <string>
#include "accelerator_models/c-model/accelerators.hpp"

using namespace std;

/** \bref   */
class Accelerator: public Tile {  
public:
  /** \bref   */
  Transaction* currentTransaction; 
  /** \brief number of cores */
  int nb_cores;
  /** \bref   */
  uint64_t final_cycle=0;
  /** \bref   */
  config_sys_t sys_config;
  /** \bref  Flag that indicates that we are busy */
  bool processing=false;
  /** \bref  Flag that indicates that we are waiting for the DRAM */
  bool wait_on_DRAM=false;
  /** \brief Needed for the update of the stats.  */
  int chip_freq;
  /** \brief Needed for the update of the stats. Each DRAM access is a
      cachile long. */
  int size_of_cacheline = 0;
  double energy = 0.0;
  int dram_accesses = 0;
  /** \brief incoming invocation from the cores */
  PP_static_Buff<pair<int, string>> *incoming_trans;
  /** \brief request to DRAM to perform the memory transactions  */
  PP_static_Buff<tuple<uint64_t, uint64_t, double, uint64_t>> *DRAM_requests;
  /** \brief response from DRAM that the memory transactions finished  */
  PP_static_Buff<uint64_t> *DRAM_responses;

  /** \brief Transactions to complete */
  map<int, pair<string, int>> to_complete;

  /** \brief Accumulative bytes transfered for each accelerator for a given data size */
  map<pair<string, uint64_t>, uint64_t> acc_bytes;
  /** \brief Accumulative number of cycles each accelerator for a given data size */
  map<pair<string, uint64_t>, uint64_t> acc_cycles;
  /** \brief Accumulative energy for each accelerator for a given data size */
  map<pair<string, uint64_t>, double> acc_energy;
  
  /** \bref   */
  Accelerator(Simulator* sim, uint64_t clockspeed, 
	      PP_static_Buff<pair<int, string>> *incoming_trans,
	      PP_static_Buff<tuple<uint64_t, uint64_t, double, uint64_t>> *DRAM_requests,
	      PP_static_Buff<uint64_t> *DRAM_responses) :
    Tile(sim, clockspeed), nb_cores(sim->nb_cores),
    chip_freq(sim->cfg->chip_freq),
    size_of_cacheline(sim->cache->size_of_cacheline),
    incoming_trans(incoming_trans),
    DRAM_requests(DRAM_requests),
    DRAM_responses(DRAM_responses)
  {
    sys_config.tech = sim->cfg->technology_node;
    // mem BW in bytes per cycle
    sys_config.mem_bandwidth=(1e3 * sim->cfg->dram_bw) / sim->cfg->chip_freq;
    sys_config.dram_latency= sim->cfg->dram_latency;
    sys_config.n_IS_tiles= sim->cfg->num_IS;
    sys_config.n_acc_tiles= sim->cfg->num_accels;
    chip_freq = sim->cfg->chip_freq;
    size_of_cacheline = sim->cache->size_of_cacheline;
    name = "accelerator";
}
  
  /** \bref   */
  bool process();
  /** \bref   */
  bool execute();
  /** \bref   */
  bool invoke(string s);
  /** \brief Recieves the transaction from the cores   */
  void RecieveTransactions();
  /** \brief Gets the next ready transaction to be executed. If thre
      isn't any, returns nullptr */
  string getNextTransaction();
  /** \brief Updates the stats about the invocation */ 
  void update_stats(acc_perf_t &perf, string name, uint64_t total_size);
  void fastForward(uint64_t inc);
};

#endif
