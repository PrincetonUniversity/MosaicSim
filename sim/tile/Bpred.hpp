#pragma once 
#include "../common.hpp"

using namespace std;

/** \bref   */
typedef enum {bp_none, bp_always_NT, bp_always_T, bp_onebit, bp_twobit, bp_gshare, bp_perfect, bp_probabilistic} TypeBpred;

class Bpred {
public:
 
  /** \brief   */
  TypeBpred type = bp_perfect;
  /** \brief   */
  int misprediction_penalty = 0;

  /** \brief for bp_onebit, bp_twobit, bp_gshare   */
  int bht_size=1;
  /** \brief   */
  int *bht;

  /** \brief for bp_gshare   */
  int gshare_global_hist_bits;
  /** \brief   */
  int gshare_global_hist;

  /** \brief for bp_probabilistic (this BP is just for testing
      purposes)

      We can set here the prob we want the BP to achieve and check
      that we get it*/
  int prob_mispredict = 17;

  /** \bref   */
  Bpred(TypeBpred, int);
  /** \bref   */
  bool predict_and_check(uint64_t pc, bool actual_taken);
};
