#ifndef ACCELERATOR_H
#define ACCELERATOR_H

#include "../common.h"
#include "../sim.h"
#include "Tile.h"
#include <string>


using namespace std;

typedef enum{CONVOLUTION,MATRIX_MULT} OPCODE;

class LATransaction : public Transaction {
public:
  LATransaction(int id, int src_id, int dst_id) : Transaction(id,src_id,dst_id) {}
  OPCODE opcode;
  int numColumnsA=50;
  int numRowsA;

  int numColumnsB;
  int numRowsB;

  int cycle_count;
  int avg_power=30; //in milliWatts  
};

class Accelerator: public Tile {  
public:
  Accelerator(Simulator* sim, uint64_t clockspeed) : Tile(sim, clockspeed) {}
  Transaction* currentTransaction; 
  uint64_t final_cycle=0;
  bool tile_complete=false;
  
  bool transaction_pending=false;
  vector<string> split(const string &s, char delim);
  //int timing_model(GemmTransaction* t);  
  bool process();
  bool ReceiveTransaction(Transaction* t);
};
#endif
