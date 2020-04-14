#ifndef BPRED_H
#define BPRED_H

#include "../common.h"
using namespace std;

typedef enum {bp_perfect, bp_always_NT, bp_always_T, bp_onebit, bp_twobit, bp_gshare, bp_probabilistic} TypeBpred;

class Bpred {
public:
 
  TypeBpred type = bp_perfect;
  int misprediction_penalty = 0;

  // for bp_onebit, bp_twobit, bp_gshare
  int bht_size=1;
  int *bht;

  // for bp_gshare
  int gshare_global_hist_bits;
  int gshare_global_hist;

  // for bp_probabilistic (this BP is just for testing purposes)
  int prob_mispredict = 17;  // we can set here the prob we want the BP to achieve and check that we get it

  Bpred(TypeBpred, int);
  bool predict(bool taken, uint64_t pc);
};

#endif