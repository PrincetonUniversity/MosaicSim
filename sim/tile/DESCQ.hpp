#pragma once
#include "DynamicNode.hpp"
#include "../common.hpp"
#include "Core.hpp"
using namespace  std;


/** \bref   */
class DESCCompare {
public:
  bool operator() (const DynamicNode* l, const DynamicNode* r) const {
    return l->desc_id < r->desc_id || (l->type==SEND || l->type==LD_PROD || l->type==STVAL) || l<r;
  }
};

/** \bref Managing inter-core communication:

 */
class DESCQ {
public:
  /** \bref   */
  priority_queue<Operator, vector<Operator>, OpCompare> pq;
  /** \bref   */
  unordered_map<uint64_t,DynamicNode*> stval_map;
  /** \bref sorted by desc_id   */
  set<DynamicNode*> execution_set;

  /** \bref  for stores (stval) */
  map<uint64_t, DynamicNode*> SAB; 
  /** \bref  for holding SEND and LD_PROD instructions  */
  unordered_map<uint64_t, DynamicNode*>commQ; 

  /** \bref map from recv desc_id to desc_id of send doing stl fwd */
  unordered_map<uint64_t, uint64_t> STLMap; 
  
  /** \bref mapping from desc id of STVAL to STVAL instrn and desc id
      of RECVs awaiting forwards */
  map<uint64_t, set<uint64_t>> SVB; 
  /** \bref terminal load buffer, just to hold DNs waiting for mem
      response */
  map<uint64_t, DynamicNode*> TLBuffer; 

  // Decoupling-related stuff 
  /** \bref */
  int term_ld_count=0;
  /** \bref  comm buff size */
  int commBuff_size=64; 
  /** \bref  comm queue size */
  int commQ_size=512; 
  /** \bref  store address buffer size */
  int SAB_size=128; 
  /** \bref  max size of terminal load buffer */
  int term_buffer_size=32;
  /** \bref   */
  int commBuff_count=0;
  /** \bref   */
  int SAB_count=0;
  /** \bref  DESC_ID of instruction next to issue  */
  uint64_t SAB_issue_pointer=0; 
  /** \bref DESC ID of youngest STADDR instruction allowed to issue */
  uint64_t SAB_back=127; 
  /** \bref   */
  int commQ_count=0;
  /** \bref   */
  int SVB_count=0;
  /** \bref   */
  int SVB_size=128;
  /** \bref    */
  uint64_t SVB_back=127;
  /** \bref   */
  uint64_t SVB_issue_pointer=0;
  
  /** \bref   */
  unordered_map<uint64_t, int64_t> send_runahead_map;

  /** \bref map of runahead distance in cycles between when a send (or
      ld_produce) completes and a receive completes */
  map<uint64_t, int64_t> stval_runahead_map;

  /** \bref   */
  uint64_t cycles=0;
  /** \bref   */
  uint64_t last_send_id=0;
  /** \bref   */
  uint64_t last_stval_id=0;
  /** \bref   */
  uint64_t last_recv_id=0;
  /** \bref   */
  uint64_t last_staddr_id=0;
  /** \bref   */
  int latency=5;

  /** \bref   */
  set<uint64_t> debug_send_set;
  /** \bref   */
  set<uint64_t> debug_stval_set;

  /** \bref Inter-core commuincation queues.
      
   */
  DESCQ(Config *cfg) {
    commBuff_size=cfg->commBuff_size;
    commQ_size=cfg->commQ_size;
    term_buffer_size=cfg->term_buffer_size;
    latency=cfg->desc_latency;  
    SVB_size=cfg->SVB_size;
    SAB_size=cfg->SAB_size;
    SAB_back=SAB_size-1;
    SVB_back=SVB_size-1;
  }
  
  /** \bref Process all the ready execution.  
      
      If moves all the ready instructions from the #pq to the
      #execution_set. Then it go through the #execution_set, which is
      order with respect of the desc_id, and calls the execute()
      method on each one of them. This allows to s send out as many
      staddr instructions, that will become #SAB head. The atomic
      instruction should not be able to execute unless they acquire the
      lock.

      Then it process the terminal loads that have already accessed
      memory hierarchy. They are removed from the #TLBuffer and
      decrement #term_ld_count. If there is no more space in #commQ,
      then the memory operation is stalled.
   */
  void process();
  /** \bref Executes the instruction d.
      
      It is called in DESCQ::process only.
   */
  bool execute(DynamicNode* d);
  /** \bref Checks if there are resource limitations and if there is
      none, inserts respective instructions into their buffers 
      
      @parm d The instruction to be executed.
   */
  bool insert(DynamicNode* d, DynamicNode* forwarding_staddr);
  /** \bref Checks if d's address is present in the #SAB. If not,
      returns NULL.
      
      @parm d DynamicNode that will be comapred.
      @return The DynamicNode with the same adrress. 
   */
  DynamicNode* sab_has_dependency(DynamicNode* d);
  /** \bref 
      
   */
  void order(DynamicNode* d);
};
