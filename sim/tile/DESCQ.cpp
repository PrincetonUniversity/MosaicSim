#include "DESCQ.hpp"

void DESCQ::order(DynamicNode* d) {
  if(d->n->typeInstr == SEND) {
    d->desc_id = last_send_id;
    last_send_id++;
  }
  if(d->n->typeInstr == LD_PROD || d->atomic) {
    d->desc_id = last_send_id;
    last_send_id++;
  }
  else if(d->n->typeInstr == STVAL) {
    stval_map.insert(make_pair(last_stval_id,d));
    d->desc_id = last_stval_id;
    last_stval_id++;
  }
  else if (d->n->typeInstr == RECV) {
    d->desc_id = last_recv_id;
    last_recv_id++;
  }
  else if (d->n->typeInstr == STADDR) {
    d->desc_id = last_staddr_id;
    last_staddr_id++;
  }
}

DynamicNode* DESCQ::sab_has_dependency(DynamicNode* d) {
  //note: SAB is sorted by desc_id, which also sorts by program order
  for(auto it=SAB.begin(); it!=SAB.end();++it) {
    DynamicNode* store_d = it->second; 
    if(*d < *store_d) { //everything beyond this will be younger
      return NULL;
    }
    else if(store_d->addr==d->addr) { //older store with matching address
      return store_d;     
    }
  }
  return NULL;
}

bool DESCQ::execute(DynamicNode* d) {
  //if you're atomic, I wanna do an if
  if(d->atomic) {    
    d->core->access(d);
    d->mem_status=PENDING;
    TLBuffer.insert({d->desc_id,d});    
    return true;
  } else if(d->n->typeInstr==LD_PROD) {    
    if(d->mem_status==NONE || d->mem_status==DESC_FWD || d->mem_status==FWD_COMPLETE) {     
      d->c->insertQ(d);
      return true;
    } else { //pending here
      TLBuffer.insert({d->desc_id,d});    
      return true;
    }
  } else if (d->n->typeInstr==SEND) {
    assert(d->mem_status==NONE);
    d->c->insertQ(d);
    return true; 
  } else if (d->n->typeInstr==RECV) {
    if(commQ.find(d->desc_id)==commQ.end() || !commQ[d->desc_id]->completed) { //RECV too far ahead
      return false;
    }
    if(commQ[d->desc_id]->mem_status==FWD_COMPLETE || commQ[d->desc_id]->mem_status==NONE) { //data is ready in commQ from a forward or completed terminal load or regular produce
      if(commQ[d->desc_id]->mem_status==FWD_COMPLETE) { //St-to-Ld forwarding was used        
        assert(STLMap.find(d->desc_id)!=STLMap.end());
        uint64_t stval_desc_id=STLMap[d->desc_id];
        
        assert(SVB.find(stval_desc_id)!=SVB.end());
        SVB[stval_desc_id].erase(d->desc_id); //remove desc_id of ld_prod. eventually, it'll be empty, which allows STVAL to complete
        STLMap.erase(d->desc_id); //save space       
      }
      commQ_count--; //free up commQ entry
      commQ.erase(d->desc_id);
      d->c->insertQ(d);
      return true;     
    }    
  } else if (d->n->typeInstr==STVAL) {
    d->mem_status=FWD_COMPLETE; //indicate that it's ready to forward
    //can't complete until corresponding staddr is at front of SAB

    if(SVB.find(d->desc_id)!=SVB.end()) {
      //loop through all ld_prod and mark their mem_status as fwd_complete, so recv can get the value
      for(auto it = SVB[d->desc_id].begin(); it != SVB[d->desc_id].end(); ++it ) {
        assert(commQ.find(*it)!=commQ.end());
        commQ[*it]->mem_status=FWD_COMPLETE;       
      }
    }
    
    if(SAB.size()==0) { //STADDR is still behind (rare, but possible)  
      return false;
    }
    
    uint64_t f_desc_id=SAB.begin()->first; //get the desc_id of front of SAB
    if(d->desc_id==f_desc_id && (SVB.find(d->desc_id)==SVB.end() || SVB[d->desc_id].size()==0)) { //STADDR is head of SAB and nothing to forward to RECV instr     
      SVB_count--;
      SVB_back++;
      SVB.erase(d->desc_id); //Luwa: just added
      d->c->insertQ(d);
      return true;     
    }
  } else if (d->n->typeInstr==STADDR) { //make sure staddrs complete after corresponding send    
    if(stval_map.find(d->desc_id)==stval_map.end()) {
      return false;
    }
    //auto sab_front=SAB.begin();
        
    //note: corresponding sval would only have completed when this becomes front of SAB
    if(stval_map.at(d->desc_id)->completed) {
      auto sab_front=SAB.begin();
      assert(d==sab_front->second);
      //assert it's at front
      
      SAB.erase(d->desc_id); //free entry so future term loads can't find it
      SAB_count--;
      SAB_back++; //allow younger instructions to be able to enter SAB
      d->core->access(d); //now that you've gotten the value, access memory 
      d->c->insertQ(d); 
      return true;      
    }
  }
  return false;
}

//checks if there are resource limitations
//if not, inserts respective instructions into their buffers
bool DESCQ::insert(DynamicNode* d, DynamicNode* forwarding_staddr) {
  bool canInsert=true;
  if(d->n->typeInstr==LD_PROD || d->atomic) {
    //TODO: REINTRODUCE_STATS
    /* sim->commQSizes.push_back(commQ_count); */
    /* if (sim->commQMax < commQ_count) { */
    /*   sim->commQMax = commQ_count; */
    /* }   */
    if(d->mem_status==PENDING || d->atomic) { //must insert in TLBuff
      //TODO: REINTRODUCE_STATS
      /* sim->termBuffSizes.push_back(term_ld_count); */
      /* if (sim->termBuffMax < term_ld_count) { */
      /*   sim->termBuffMax = term_ld_count; */
      /* } */
      if(term_ld_count==term_buffer_size || commQ_count==commQ_size) {  //check term ld buff size 
        canInsert=false;        
      }
      else {
        commQ_count++; //corresponding recv will decrement this count, signifying removing/freeing the entry from commQ
        term_ld_count++; //allocate space on TLBuff but don't insert yet until execute() to account for descq latency
      }
    } else { // insert in commQ directly        
      if(commQ_count==commQ_size) { //check commQ size
        canInsert=false;
      }
      else {
        if(d->mem_status==DESC_FWD) {
          //here, connect the queues          
          assert(STLMap.find(d->desc_id)==STLMap.end());
          STLMap[d->desc_id]=forwarding_staddr->desc_id; //this allows corresponding RECV to find desc id of STVAL to get data from
          
          //next, we set the set desc ids of pending forwards that an STVAL has to wait for before completing
          SVB[forwarding_staddr->desc_id].insert(d->desc_id);
        }
        else { //e.g., LSQ_FWD
          d->mem_status=NONE;
        }
        commQ_count++;
        assert(commQ.find(d->desc_id)==commQ.end());
        commQ[d->desc_id]=d;
      }
    }
  }
  else if(d->n->typeInstr==SEND) { 
    //TODO: REINTRODUCE_STATS
    /* sim->commQSizes.push_back(commQ_count);     */
    /* if (sim->commQMax < commQ_count) { */
    /*   sim->commQMax = commQ_count; */
    /* } */
    if(commQ_count==commQ_size) { //check commQ size
      canInsert=false;
    }
    else {
      commQ_count++;
      assert(commQ.find(d->desc_id)==commQ.end());
      commQ[d->desc_id]=d;
    }
  }
  else if(d->n->typeInstr==RECV) {    
    //no resource constraints here
  }
  else if(d->n->typeInstr==STADDR) {
    //TODO: REINTRODUCE_STATS
    /* sim->SABSizes.push_back(SAB_count); */
    /* if (sim->SABMax < SAB_count) { */
    /*   sim->SABMax = SAB_count; */
    /* } */
    if(d->desc_id!=SAB_issue_pointer || d->desc_id>SAB_back ||  SAB_count==SAB_size) { //check SAB size, check that it's not younger than youngest allowed staddr instruction, based on SAB size, force in order issue/dispatch of staddrs
      canInsert=false;
    }
    else {
      SAB.insert({d->desc_id,d});
      SAB_count++;
      SAB_issue_pointer++;
    }
  }
  else if(d->n->typeInstr==STVAL) {
    //TODO: REINTRODUCE_STATS
    /* sim->SVBSizes.push_back(SVB_count); */
    /* if (sim->SVBMax < SVB_count) { */
    /*   sim->SVBMax = SVB_count; */
    /* } */
    if(d->desc_id!=SVB_issue_pointer || d->desc_id>SVB_back ||  SVB_count==SVB_size) { //check SVB size, check that it's not younger than youngest allowed stval instruction, based on SVB size, force in order issue/dispatch of stvals
      canInsert=false;
    }
    else {
      SVB_count++;
      SVB_issue_pointer++;
    }
  }
  if(canInsert) {
    pq.push(make_pair(d, cycles + latency));
  }
  return canInsert;
}


void DESCQ::process() {
  while(true) {
    if (pq.empty() || pq.top().second >= cycles) {
      break;
    }
    execution_set.insert(pq.top().first); //insert, sorted by desc_id
    pq.pop();
  }

  //go through execution set sorted in desc id
  //this also allows us to send out as many staddr instructions (that become SAB head) back to back
  //should not be able to execute atomic instruction if can't get lock
  for(auto it=execution_set.begin(); it!=execution_set.end();) {
    if(execute(*it)) {
      it=execution_set.erase(it); //gets next iteration
    } else {
      ++it;
    }
  }
  
  //process terminal loads
  //has already accessed mem hierarchy, which calls callback to remove from TLBuffer and decrement term_ld_count
  //must stall if no space in comm Q or still waiting for mem results
  for(auto it=TLBuffer.begin(); it!=TLBuffer.end();) {   
    DynamicNode* d=it->second;
    
    if(d->mem_status==PENDING) { //still awaiting mem response   
      ++it; 
    } else { //can insert in commQ      
      d->mem_status=NONE;
      it=TLBuffer.erase(it); //get next item
      assert(commQ.find(d->desc_id)==commQ.end());
      commQ[d->desc_id]=d;
      d->c->insertQ(d);     
      term_ld_count--; //free up space in term ld buff
    }
  }
  cycles++;
}

