#include "DynamicNode.h"
#include "LoadStoreQ.h"
#include "Core.h"

LoadStoreQ::LoadStoreQ(Core* thecore) {
  core=thecore;
}

void LoadStoreQ::resolveAddress(DynamicNode *d) {
  if(d->type == LD)
    unresolved_ld_set.erase(d);
  else
    unresolved_st_set.erase(d);
}
void LoadStoreQ::insert(DynamicNode *d) {
  if(d->type == LD) {
    lq.push_back(d);
    if(lm.find(d->addr) == lm.end())
      lm.insert(make_pair(d->addr, set<DynamicNode*,DynamicNodePointerCompare>()));
    lm.at(d->addr).insert(d);
    unresolved_ld_set.insert(d);
  }
  else {
    sq.push_back(d);
    if(sm.find(d->addr) == sm.end())
      sm.insert(make_pair(d->addr, set<DynamicNode*,DynamicNodePointerCompare>()));
    sm.at(d->addr).insert(d);
    unresolved_st_set.insert(d);
  }
}

bool LoadStoreQ::checkSize(int num_ld, int num_st) {
  int ld_need = lq.size() + num_ld - size;
  int st_need = sq.size() + num_st - size; 
  int ld_ct = 0;
  int st_ct = 0;
  if(ld_need <= 0 && st_need <= 0)
    return true;
  if(ld_need > 0) {
    for(auto it = lq.begin(); it!= lq.end(); ++it) {
      if((*it)->completed)
        ld_ct++;
      else
        break;
      if(ld_ct == ld_need)
        break;
    }
  }
  if(st_need > 0) {
    for(auto it = sq.begin(); it!= sq.end(); ++it) {
      if((*it)->completed)
        st_ct++;
      else
        break;
      if(st_ct == st_need)
        break;
    }
  }
  if(ld_ct >= ld_need && st_ct >= st_need) {
    for(int i=0; i<ld_need; i++) {
      DynamicNode *d = lq.front();
      lm.at(d->addr).erase(d);
      if(lm.at(d->addr).size() == 0)
        lm.erase(d->addr);
      lq.pop_front();
    }
    for(int i=0; i<st_need; i++) {
      DynamicNode *d = sq.front();
      sm.at(d->addr).erase(d);
      if(sm.at(d->addr).size() == 0)
        sm.erase(d->addr);
      sq.pop_front();  
    }
    stat.update("lsq_insert_success");
    return true;
  }
  else {
    stat.update("lsq_insert_fail");
    return false; 
  }
}

bool LoadStoreQ::check_unresolved_load(DynamicNode *in) {
  if(unresolved_ld_set.size() == 0)
    return false;
  DynamicNode *d = *(unresolved_ld_set.begin());
  if(*d < *in || *d == *in)
    return true;
  else
    return false;
}

bool LoadStoreQ::check_unresolved_store(DynamicNode *in) {

  if(unresolved_st_set.size() == 0)
    return false;
  DynamicNode *d = *(unresolved_st_set.begin());

  if(core->local_cfg.mem_speculate) {
    for(auto it=unresolved_st_set.begin(); it!=unresolved_st_set.end();it++) {
      if (*in < **it || *in == **it) {        
        return false;
      }
      if (in->addr == (*it)->addr)
        return true;
    }

    return false;
  }

    
  if(*d < *in || *d == *in) {    
    return true;
  }
  else {
    
    return false;
  }
}

//for perfect, loop through all unresolved stores, if no conflicting
//address, return false for unresolved

int LoadStoreQ::check_load_issue(DynamicNode *in, bool speculation_enabled) {
  bool check = check_unresolved_store(in);
  if(check && !speculation_enabled)
    return -1;
  if(sm.find(in->addr) == sm.end()) {
    if(check)
      return 0;
    else
      return 1;
  }
  set<DynamicNode*, DynamicNodePointerCompare> &s = sm.at(in->addr);
  for(auto it = s.begin(); it!= s.end(); ++it) {
    DynamicNode *d = *it;
    if(*in < *d)
      return 1;
    else if(!d->completed)
      return -1;
  }
  return 1;
}

bool LoadStoreQ::check_store_issue(DynamicNode *in) {
  bool skipStore = false;
  bool skipLoad = false;
  if(check_unresolved_store(in))
    return false;

  if(check_unresolved_load(in))
    return false;
  if(sm.at(in->addr).size() == 1)
    skipStore = true;
  if(lm.find(in->addr) == lm.end())
    skipLoad = true;
  if(!skipStore) {
    set<DynamicNode*, DynamicNodePointerCompare> &s = sm.at(in->addr);
    for(auto it = s.begin(); it!= s.end(); ++it) {
      DynamicNode *d = *it;
      if(*in < *d || *d == *in)
        break;
      else if(!d->completed)
        return 0;
    }
  }
  if(!skipLoad) {
    set<DynamicNode*, DynamicNodePointerCompare> &s = lm.at(in->addr);
    for(auto it = s.begin(); it!= s.end(); ++it) {
      DynamicNode *d = *it;
      if(*in < *d)
        break;
      else if(!d->completed)
        return 0;
    }
  }
  return true;
}

int LoadStoreQ::check_forwarding (DynamicNode* in) {
  bool speculative = false;    
  if(sm.find(in->addr) == sm.end())
    return -1;
  set<DynamicNode*, DynamicNodePointerCompare> &s = sm.at(in->addr);
  

  DynamicNode test = *in;
  auto it = s.rbegin();
  auto uit = unresolved_st_set.rbegin();

  
  /*cout << "before id check \n";
  (*uit)->print("possible failing", -10);
  if(*in<*(*uit))
    {
      cout << "check the id \n";      
    }
  cout << "after id check \n";
  */



  //luwa, below shoud be removed
  /* while(true) {
    (*in).print("arg", -10);
    (*(*uit)).print("entry", -10);
    cout << "Node Id is \n";
    cout << (*in).n->id << "\n";

    cout << "Node Id is \n";
    cout << (*(*uit)).n->id << "\n";
    
    if(!(*in < *(*uit) && uit != unresolved_st_set.rend())) {      
      break;
    }
    uit++;
  }
  */
  //find first *just* older dynamic node with unresolved store
  while(uit != unresolved_st_set.rend() && *in < *(*uit))
    uit++;

  //find first *just* older, completed dynamic node that accesses the address in question
  while(it != s.rend() && *in < *(*it)) {   
    it++;
  }
  //can't find someone to forward data cuz no older, completed node with matching address
  if(it == s.rend())
    return -1;

  //previous version
  //unresolved one is older than relevant one with the address
  //but shouldn't it be the other way around??
  //shouldn't you get the forward from the most recent store, which is the younger between *(*it) and *(*uit)? That is, closest to *in. 

  /*
  if(uit != unresolved_st_set.rend() && *(*uit) < *(*it)) {
    speculative = true;
  } 
  */
  //corrected version
  //here, unresolved store *(*uit) is closest to *in. Technically, we don't know
  //the address is irrelevant yet and that *(*it) is the relevant one, so it would be speculative. 
  if(uit != unresolved_st_set.rend() &&   *(*it) < *(*uit)) { //unresolved is younger, should be getting from them if address matched, which you don't know
    speculative = true;
  }
  
  DynamicNode *d = *it;
  if(d->completed && !speculative)
    return 1;
  else if(d->completed && speculative) //just get from most recent completed with addr anyway
    return 0;
  else if(!d->completed)
    return -1;
  return -1;
}

std::vector<DynamicNode*> LoadStoreQ::check_speculation (DynamicNode* in) {
  std::vector<DynamicNode*> ret;
  if(lm.find(in->addr) == lm.end())
    return ret;
  set<DynamicNode*, DynamicNodePointerCompare> &s = lm.at(in->addr);
  for(auto it = s.begin(); it!= s.end(); ++it) {
    DynamicNode *d = *it;
    if(*in < *d)
      continue;
    if(d->speculated) {
      d->speculated = false;
      ret.push_back(d);
    }
  }
  return ret;
}
