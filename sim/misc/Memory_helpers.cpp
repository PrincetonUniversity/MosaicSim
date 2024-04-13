#include "Memory_helpers.hpp"
#include <sstream>


vector<string> split(const string &s, char delim) {
  stringstream ss(s);
  string item;
  vector<string> tokens;

  while (getline(ss, item, delim)) 
    tokens.push_back(item);

  return tokens;
}

AccBlock::AccBlock (uint64_t addr, int size, double factor, int cacheline_size, int cycle):
  issued(0), finished(0),
  cacheline_size(cacheline_size),
  arried_at(cycle)
{
  int rest;
  start = addr;
  // the last address is one word behind
  end = addr+size-4;
  
  nb_elem = (end - start) / cacheline_size;
  nb_total_elem = nb_elem * factor;
}

bool AccBlock::contains(uint64_t addr) {
  return ( (addr >= start) && (addr <= end) );
}

bool AccBlock::completed() {
  return finished == nb_total_elem;
}

bool AccBlock::completed_requesting() {
  return issued == nb_total_elem;
}

uint64_t AccBlock::next_to_send() {
  if (issued == nb_total_elem)
    return 0;
  else
    return (uint64_t) start + ((issued%nb_elem) * cacheline_size);
}

void AccBlock::sent() {
  issued++;
}

void AccBlock::transaction_complete() {
  finished++;
}
