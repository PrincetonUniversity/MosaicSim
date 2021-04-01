#include "Bpred.h"

using namespace std;

Bpred::Bpred(TypeBpred type, int bht_size) : type(type), bht_size(bht_size) {

  // for testing purposes: models the behavior of a "probabilistic" branch predictor
  if(type==bp_probabilistic) {  
    srand(time(NULL)); // initialize a random seed
  }
  // 1-bit saturating counters BP
  else if (type==bp_onebit) {    
    bht = new int[bht_size];
    for(int i=0;i<bht_size;i++)
       bht[i]=1;  // init to taken    
  }
  // 2-bit saturating counters BP
  else if (type==bp_twobit) {
    bht = new int[bht_size];
    for(int i=0;i<bht_size;i++)
       bht[i]=2;  // init to weakly taken    
  }
  // gshare BP: made of a table of 2-bit saturating counters + a global history register  
  else if (type==bp_gshare) {
    gshare_global_hist = 0;
    bht = new int[bht_size];
    for(int i=0;i<bht_size;i++)
       bht[i]=1;  // init to weakly not-taken    
  }
  else if (type!=bp_none && type!=bp_always_NT && type!=bp_always_T && type!=bp_perfect) {
    cout << "Unknown branch predictor!!!\n";
    assert(false);
  }
}
  
// predict a branch and return if the prediction was correct
bool Bpred::predict_and_check(uint64_t pc, bool actual_taken) {
  if(type==bp_none) {
    return false;   
  } 
  else if(type==bp_perfect) {
    return true;
  }
  // models the behavior of an Always-NOT-Taken static branch predictor
  else if (type==bp_always_NT) {
    if(actual_taken)
      return false;   
    else 
      return true;
  }
  // models the behavior of an Always-Taken static branch predictor
  else if (type==bp_always_T) {
    if(actual_taken)
      return true;   
    else 
      return false;
  }
  // dynamic branch predictor with 1-bit saturating counters
  else if (type==bp_onebit) {
    // query the predictor
    int idx = pc & (bht_size-1);
    int pred_taken = (bht[idx]==1);

    // update the 1-bit counter
    if (actual_taken) {
      bht[idx]++;                  // increase counter
      bht[idx] = min(bht[idx],1);  // force the counter to be in the range [0..1]
    }
    else {
      bht[idx]--;    // decrease counter
      bht[idx] = max(bht[idx],0);  // force the counter to be in the range [0..1]
    }
    assert(bht[idx]>=0 && bht[idx]<=1);
    return (pred_taken==actual_taken);   
  }
  // dynamic branch predictor with 2-bit saturating counters
  else if (type==bp_twobit) {
    // query the predictor
    int idx = pc & (bht_size-1);
    int pred_taken = (bht[idx]>=2);  // 0,1 not taken  |  2,3 taken
 
    // update the 2-bit counter
    if (actual_taken) {
      bht[idx]++;                  // increase counter
      bht[idx] = min(bht[idx],3);  // force the counter to be in the range [0..3]
    }
    else {
      bht[idx]--;    // decrease counter
      bht[idx] = max(bht[idx],0);  // force the counter to be in the range [0..3]
    }
    assert(bht[idx]>=0 && bht[idx]<=3);
    return (pred_taken==actual_taken);   
  }
  // GSHARE branch predictor with 2-bit saturating counters
  else if (type==bp_gshare) {
    // query the predictor
    int idx = (pc ^ gshare_global_hist) & (bht_size-1);
    int pred_taken = (bht[idx]>=2);  // 0,1 not taken  |  2,3 taken
 
    // update the 2-bit counter
    if (actual_taken) {
      bht[idx]++;                  // increase counter
      bht[idx] = min(bht[idx],3);  // force the counter to be in the range [0..3]
    }
    else {
      bht[idx]--;    // decrease counter
      bht[idx] = max(bht[idx],0);  // force the counter to be in the range [0..3]
    }
    assert(bht[idx]>=0 && bht[idx]<=3);
    
    // update the global history register
    int mask = (1<<gshare_global_hist_bits)-1;
    gshare_global_hist = ( (gshare_global_hist << 1) | actual_taken ) & mask;
//cout << gshare_global_hist << " pc:" << pc << " i:" << idx << " c:" << bht[idx] << " p:" << pred_taken << " a:" << actual_taken << endl;
    return (pred_taken==actual_taken);   
  }
  // for testing purposes: models the behavior of a "probabilistic" branch predictor
  else if(type==bp_probabilistic) {  
    if (rand()%100 < prob_mispredict)
      return false;   
    else 
      return true; 
  }
  else
    return true; //default to perfect branch predictor
}