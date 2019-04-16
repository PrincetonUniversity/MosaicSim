#include <iostream>
#include <stdlib.h>
#include <cassert>
#include <vector>
#include <fstream>
#include <sstream>
#include <stdarg.h>
#include "omp.h"

//Hack to make this work.
#define MAX_THREADS 1024

std::ofstream f1[MAX_THREADS];
std::ofstream f2[MAX_THREADS];

const char * get_dir_name(char* kernel_type, const char * type) {
  if (omp_get_thread_num() >= MAX_THREADS) {
    std::cout << "ERROR: Unable to log for all threads! Increase MAX_THREADS in tracer.cc" << "\n";
    assert(0);
  }
  std::ostringstream ret;
  ret << "output_" << kernel_type << "_" << omp_get_thread_num() << "/" << type;
  return ret.str().c_str();
}

__attribute__((noinline)) void printBranch(char* name, char *kernel_type, int cond, char* n1, char *n2)
{
  if(!f1[omp_get_thread_num()].is_open()) {
    //f1.open("output/ctrl.txt", std::ofstream::out | std::ofstream::trunc);
    f1[omp_get_thread_num()].open(get_dir_name(kernel_type, "ctrl.txt"), std::ofstream::out | std::ofstream::trunc);
  }
	char *target;
	if(cond == 0)
		target = n1;
	else
		target = n2;
	f1[omp_get_thread_num()] << "B,"<<name << "," << target << "\n";
	//std::cout << "Branch ["<< name << "]: " << cond << " / " << target <<  "\n";	
}

__attribute__((noinline)) void printuBranch(char* name, char *kernel_type, char *n1)
{
  if(!f1[omp_get_thread_num()].is_open()) {
    //f1.open("output/ctrl.txt", std::ofstream::out | std::ofstream::trunc);
    f1[omp_get_thread_num()].open(get_dir_name(kernel_type, "ctrl.txt"), std::ofstream::out | std::ofstream::trunc);
  }
  if(!f1[omp_get_thread_num()].is_open())
    assert(false);
  f1[omp_get_thread_num()] << "U,"<< name << "," << n1 << "\n";
  //std::cout << "Branch ["<< name << "]: " << cond << " / " << target <<  "\n";	
}

__attribute__((noinline)) void printMem(char *name, char *kernel_type, bool type, long long addr, int size)
{
  if(!f2[omp_get_thread_num()].is_open()) {
    //f2.open("output/mem.txt", std::ofstream::out | std::ofstream::trunc);
    f2[omp_get_thread_num()].open(get_dir_name(kernel_type, "mem.txt"), std::ofstream::out | std::ofstream::trunc);
  }
	
  if(type == 0)
    //std::cout << "Load ["<< name << "]: " << type << " / " << addr << " / " << size <<  "\n";	
    f2[omp_get_thread_num()] << "L,"<< name << "," << addr << ","<< size <<"\n";
  else if(type == 1)
    f2[omp_get_thread_num()] << "S,"<< name << "," << addr << ","<< size <<"\n";
  //std::cout << "Store ["<< name << "]: " << type << " / " << addr << " / " << size <<  "\n";	
}

__attribute__((noinline)) void printSw(char *name, char *kernel_type, int value, char *def, int n, ...)
{
  if(!f1[omp_get_thread_num()].is_open()) {
    //f1.open("output/ctrl.txt", std::ofstream::out | std::ofstream::trunc);
    f1[omp_get_thread_num()].open(get_dir_name(kernel_type, "ctrl.txt"), std::ofstream::out | std::ofstream::trunc);
  }
  va_list vl;
  std::vector<int> vals;
  std::vector<char*> strs;
  char* target;
  va_start(vl,n);
  for(int i=0; i<n; i++) {
    int val = va_arg(vl, int);
    char *bbname = va_arg(vl, char*);
    vals.push_back(val);
    strs.push_back(bbname);
  }
  va_end(vl);
  bool found = false;
  for(int i=0; i<vals.size(); i++) {
    if(vals.at(i) == value) {
      target = strs.at(i);
      found = true;
      break;
    }
  }
  if(!found)
    target = def;
  f1[omp_get_thread_num()] << "S," <<name << "," << target <<"\n";
  //std::cout << "Switch [" << name << "]: " << value << " / " << target << "\n";
}
