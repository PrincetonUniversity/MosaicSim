#pragma once

#include "../common.hpp"

using namespace std;

class DynamicNode;
class DynamicNodePointerCompare;
class Core;

/** \brief   */
class LoadStoreQ {
public:
  /** \brief   */
  deque<DynamicNode*> lq;
  /** \brief   */
  deque<DynamicNode*> sq;
  /** \brief   */
  unordered_map<uint64_t, set<DynamicNode*, DynamicNodePointerCompare>> lm;
  /** \brief program order sorted set of dynamic nodes (loads) that access an address   */
  unordered_map<uint64_t, set<DynamicNode*, DynamicNodePointerCompare>> sm;
  /** \brief FLag for memory spculation  */
  bool mem_speculate;
  /** \brief program order sorted set of dynamic nodes (stores) that access an address   */
  set<DynamicNode*, DynamicNodePointerCompare> us;
  /** \brief   */
  set<DynamicNode*, DynamicNodePointerCompare> ul;
  /** \brief   */
  set<DynamicNode*,DynamicNodePointerCompare> unresolved_st_set;
  /** \brief program order sorted set of dynamic nodes (stores) whose addresses are unresolved   */
  set<DynamicNode*,DynamicNodePointerCompare> unresolved_ld_set;

  /** \brief   */
  int size;
  /** \brief   */
  bool set_processed = false;

  /** \brief   */
  LoadStoreQ(bool speculate);
  /** \brief   */
  void resolveAddress(DynamicNode *d);
  /** \brief   */
  void insert(DynamicNode *d);
  /** \brief   */
  bool checkSize(int num_ld, int num_st);
  /** \brief   */
  bool check_unresolved_load(DynamicNode *in);
  /** \brief   */
  bool check_unresolved_store(DynamicNode *in);
  /** \brief   */
  int  check_load_issue(DynamicNode *in, bool speculation_enabled);
  /** \brief   */
  bool check_store_issue(DynamicNode *in);
  /** \brief   */
  int  check_forwarding (DynamicNode* in);
  /** \brief   */
  void remove(DynamicNode* d);
  /** \brief   */
  vector<DynamicNode*> check_speculation (DynamicNode* in);
};
