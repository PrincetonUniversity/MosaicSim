#include <iostream>
#include <stdlib.h>
#include <cassert>
#include <vector>
#include <fstream>
#include <sstream>
#include <stdarg.h>
#include "omp.h"
#include <string>

//Hack to make this work.
#define MAX_THREADS 1024

std::ofstream f1[MAX_THREADS]; //for mem.txt
std::ofstream f2[MAX_THREADS]; // for ctrl.txt
std::ofstream f3[MAX_THREADS]; // for acc.txt

__attribute__((noinline))
const char * get_dir_name(std::string run_dir, std::string kernel_type, std::string type) {
  if (omp_get_thread_num() >= MAX_THREADS) {
    std::cout << "ERROR: Unable to log for all threads! Increase MAX_THREADS in tracer.cc" << "\n";
    assert(0);
  }
  std::ostringstream ret;
  ret << run_dir << "/output_" << kernel_type << "_" << omp_get_thread_num() << "/" << type;
  return ret.str().c_str();
}

__attribute__((noinline))
extern "C"
void printBranch(char* name, char *kernel_type, char *run_dir, int cond, char* n1, char *n2)
{
  if(!f1[omp_get_thread_num()].is_open()) {
    //f1.open("output/ctrl.txt", std::ofstream::out | std::ofstream::trunc);
    f1[omp_get_thread_num()].open(get_dir_name(run_dir, kernel_type, "ctrl.txt"), std::ofstream::out | std::ofstream::trunc);
  }
	char *target;
	if(cond == 0)
		target = n1;
	else
		target = n2;
	f1[omp_get_thread_num()] << "B,"<<name << "," << target << "\n";
	//std::cout << "Branch ["<< name << "]: " << cond << " / " << target <<  "\n";	
  //f1[omp_get_thread_num()].close();
}

__attribute__((noinline))
extern "C"
void printuBranch(char* name, char *kernel_type, char *run_dir, char *n1)
{
  if(!f1[omp_get_thread_num()].is_open()) {
    //f1.open("output/ctrl.txt", std::ofstream::out | std::ofstream::trunc);
    f1[omp_get_thread_num()].open(get_dir_name(run_dir, kernel_type, "ctrl.txt"), std::ofstream::out | std::ofstream::trunc);
  }
  if(!f1[omp_get_thread_num()].is_open())
    assert(false);
  f1[omp_get_thread_num()] << "U,"<< name << "," << n1 << "\n";
  //std::cout << "Branch ["<< name << "]: " << cond << " / " << target <<  "\n";	
  //f1[omp_get_thread_num()].close();
}

__attribute__((noinline))
extern "C"
void printMem(char *name, char *kernel_type, char *run_dir, bool type, long long addr, int size)
{
  if(!f2[omp_get_thread_num()].is_open()) {
    //f2.open("output/mem.txt", std::ofstream::out | std::ofstream::trunc);
    f2[omp_get_thread_num()].open(get_dir_name(run_dir, kernel_type, "mem.txt"), std::ofstream::out | std::ofstream::trunc);
  }
	
  if(type == 0)
    //std::cout << "Load ["<< name << "]: " << type << " / " << addr << " / " << size <<  "\n";	
    f2[omp_get_thread_num()] << "L,"<< name << "," << addr << ","<< size <<"\n";
  else if(type == 1)
    f2[omp_get_thread_num()] << "S,"<< name << "," << addr << ","<< size <<"\n";
  //std::cout << "Store ["<< name << "]: " << type << " / " << addr << " / " << size <<  "\n";	
  //f2[omp_get_thread_num()].close();
}

__attribute__((noinline))
extern "C"
void print_matmul(char *acc_kernel_name, char *kernel_type, char *run_dir, char* node_id, int rowsA, int colsA , int rowsB, int colsB, int batch_size)
{
  if(!f3[omp_get_thread_num()].is_open()) {
    
    f3[omp_get_thread_num()].open(get_dir_name(run_dir, kernel_type, "acc.txt"), std::ofstream::out | std::ofstream::trunc);
  }
  //std::cout << "printing acc now " << rowsA << ","<< colsA << rowsB << ","<< colsB <<"\n";
 
  f3[omp_get_thread_num()] << acc_kernel_name << "," << node_id << "," << rowsA << ","<< colsA << ","<< rowsB << ","<< colsB << "," << batch_size << "\n";
}

__attribute__((noinline))
extern "C"
void print_sdp(char *acc_kernel_name, char *kernel_type, char *run_dir, char* node_id, int working_mode, int size)
{
  if(!f3[omp_get_thread_num()].is_open()) {
    
    f3[omp_get_thread_num()].open(get_dir_name(run_dir, kernel_type, "acc.txt"), std::ofstream::out | std::ofstream::trunc);
  }

 
  f3[omp_get_thread_num()] << acc_kernel_name << "," << node_id << "," << working_mode << ","<< size << "\n";
}

__attribute__((noinline))
extern "C"
void print_conv2d_layer(char *acc_kernel_name, char *kernel_type, char *run_dir, char* node_id, int in_channels, int in_height, int in_width, int out_channels, int filter_height, int filter_width, bool zero_pad, int vert_conv_stride, int horiz_conv_stride, bool pooling, int pool_height, int pool_width, int vertical_pool_stride, int horizontal_pool_stride)
{
  if(!f3[omp_get_thread_num()].is_open()) {
    
    f3[omp_get_thread_num()].open(get_dir_name(run_dir, kernel_type, "acc.txt"), std::ofstream::out | std::ofstream::trunc);
  }
 
  f3[omp_get_thread_num()] << acc_kernel_name << "," << node_id << "," << in_channels << "," << in_height << "," << in_width << "," << out_channels << "," << filter_height << "," << filter_width << "," << zero_pad << "," << vert_conv_stride << "," << horiz_conv_stride << "," << pooling << "," << pool_height << "," << pool_width << "," << vertical_pool_stride << "," << horizontal_pool_stride << "\n";
}

__attribute__((noinline))
extern "C"
void print_dense_layer(char *acc_kernel_name, char *kernel_type, char *run_dir, char* node_id, int batch, int in_channels, int out_channels)
{
  if(!f3[omp_get_thread_num()].is_open()) {
    
    f3[omp_get_thread_num()].open(get_dir_name(run_dir, kernel_type, "acc.txt"), std::ofstream::out | std::ofstream::trunc);
  }
 
  f3[omp_get_thread_num()] << acc_kernel_name << "," << node_id << "," << batch << ","<< in_channels << ","<< out_channels << "\n";
}

__attribute__((noinline))
extern "C"
void printSw(char *name, char *kernel_type, char *run_dir, int value, char *def, int n, ...)
{
  if(!f1[omp_get_thread_num()].is_open()) {
    //f1.open("output/ctrl.txt", std::ofstream::out | std::ofstream::trunc);
    f1[omp_get_thread_num()].open(get_dir_name(run_dir, kernel_type, "ctrl.txt"), std::ofstream::out | std::ofstream::trunc);
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
  //f1[omp_get_thread_num()].close();
}
