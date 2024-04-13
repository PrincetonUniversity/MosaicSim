#pragma once

#include "../common.hpp"

using namespace std;

/** \brief   */
class Statistics {
public:
  /** \brief   */
  map<string, pair<uint64_t, int>> stats;
  /** \brief   */
  map<string, pair<uint64_t, int>> old_stats;
  /** \brief   */
  int num_types = 4;
  /** \brief   */
  int printInterval = 500000; 
  /** \brief   */
  double global_energy = 0.0;
  /** \brief   */
  double global_avg_power = 0.0;
  /** \brief   */
  double acc_energy = 0.0;

  double DRAM_energy = 0.0;
  double LLC_energy = 0.0;
  double cores_energy = 0.0;
  
  /** \brief   */
  Statistics(); 
  /** \brief   */
  void registerStat(string str, int type);

  /** \brief   */
  void checkpoint();

  /** \brief   */
  uint64_t get(string str);

  /** \brief   */
  uint64_t get_epoch(string str);

  /** \brief   */
  void set(string str, uint64_t val);
  
  /** \brief   */
  void update(string str, uint64_t inc=1);
  
  /** \brief   */
  void reset();

  /** \brief   */
  void print(ostream& ofile);
  
  /** \brief   */
  void print_raw(ostream& ofile);

  /** \brief   */
  void print_epoch(ostream& ofile);
  
  Statistics& operator+=(const Statistics& rhs);
  
  
};
