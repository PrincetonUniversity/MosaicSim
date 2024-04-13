#pragma once
#include <stdint.h>
#include <vector>
#include <string>
using namespace std;

/* \brief Taking care of the memory transactions deidcated to the
   accelerator.  */
class AccBlock {
public:
  uint64_t start;
  uint64_t end;
  int issued;
  int finished;
  int nb_elem;
  int nb_total_elem;
  int cacheline_size;
  uint64_t arried_at;
  AccBlock (uint64_t addr, int size, double factor, int cacheline_size, int cycle);
  /* \brief Checks if addr is in the block of memory */
  bool contains(uint64_t addr);
  /* \brief Checks if all the memory requests has been completed */
  bool completed();
  /* \brief Checks if all the memory requests has been sent to the DRAM */
  bool completed_requesting();
  /* \brief gets the next address to be sent */
  uint64_t next_to_send();
  /* \brief Notifies that a request has been sent to the DRAM  */
  void sent();
  /* \brief Notifies that a request has been completed  */
  void transaction_complete();
};

/** \bref   */
vector<string> split(const string &s, char delim);
