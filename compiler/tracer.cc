#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <cassert>
#include <cmath>
#include <vector>
#include <fstream>
#include <mutex>
#include <stdarg.h>
#include "omp.h"
#include <string>
#include <iostream>
#include <sstream>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

#define MAX_THREADS 4096
using namespace std;
enum mess_type {CF, MEM, ACC, PB, END};
int files[MAX_THREADS];
int acc_files[MAX_THREADS];
/* TODO: find a better way to pass this parameter */
int nb_files;
// Files for partial barrier
ofstream PB_files[MAX_THREADS];

/* we have to record the number of threads, because the clean up is
   executed outside of the parallel riegion */
int nb_threads;
int buf[MAX_THREADS][1024];

mutex files_lock;

#ifdef DUMP_ASCI_INPUT
ofstream cf_s[MAX_THREADS];
ofstream mem_s[MAX_THREADS];
#endif



/** Gets the prefix for the output file  */
__attribute__((noinline))
extern "C"
char *get_dir_name(string run_dir, string kernel_type, string type) {
  if (omp_get_thread_num() >= MAX_THREADS) {
    cout << "ERROR: Unable to log for all threads! Increase MAX_THREADS in tracer.cc" << "\n";
    assert(0);
  }
  string file;
  int tid = omp_get_thread_num();
  file = run_dir + "/output_" + kernel_type + "_" + to_string(tid) + "/" + type;
  const char *tmp  = file.c_str();
  char *tmp2 = (char *) malloc((strlen(tmp) + 1) * sizeof(*tmp));
  strcpy(tmp2, tmp);
  
  return tmp2;
}

/** Writes the message into the pipe for a given tid. */
__attribute__((noinline))
extern "C"
void write_mess(int tid) {
  int *data = buf[tid];
  int bytes = 0;
  size_t size = 1024 * sizeof(int);
  int file_id;
  data[0] = tid;

#ifdef USE_FILES
  file_id = tid;
#else
  file_id = tid / ceil(nb_threads/nb_files);
#endif
  while((bytes = write(files[file_id], data, size)) != size)
    ;
}

__attribute__((noinline))
extern "C"
void insert_mess(int tid, int *data, mess_type type, string dir)
{
  int *_buf = buf[tid];
  int *cf = &_buf[_buf[1]*3 + 3];
  int *mem = &_buf[1024 - _buf[2]*5];
  int _data[5] = {-1, -1, -1, -1, -1};
  
  files_lock.lock();
  if (nb_files == 0) {
    fstream nb_files_file(dir + "/nb_files.txt" );
    nb_files_file >> nb_files;
    nb_threads = omp_get_num_threads();
#ifdef USE_FILES
    for(int i = 0; i < nb_threads; i++) {
      files[i] = open((dir + "/dyn_data.bin" + to_string(i)).c_str(),  O_WRONLY | O_CREAT | O_TRUNC, 0666);
      acc_files[i] = open((dir + "/acc_data.bin" + to_string(i)).c_str(),  O_WRONLY | O_CREAT | O_TRUNC, 0666);
#else
    for(int i = 0; i < nb_files; i++) {
      int file_id = tid / ceil(nb_threads/ nb_files);
      files[i] = open((dir + "/dyn_data.bin" + to_string(i)).c_str(),  O_WRONLY | O_NONBLOCK);
      acc_files[i] = open((dir + "/acc_data.bin" + to_string(i)).c_str(),  O_WRONLY | O_NONBLOCK);
#endif
      if(files[i] < 1) {
	perror("Opening pipe for writing from the native run");
	assert(false);
      }
    }
  }
  files_lock.unlock();
  
  switch(type) {
  case CF:
    if (cf + 3 > mem) {
      write_mess(tid);
      cf = &_buf[3];
      mem = &_buf[1024];
      _buf[1] = _buf[2] = 0;
    }
    memcpy((void *) cf, (void *) data, 3*sizeof(int));
    _buf[1]++;
    break;
  case MEM:
    if (mem - 5 < cf) {
      write_mess(tid);
      cf = &_buf[3];
      mem = &_buf[1024];
      _buf[1] = _buf[2] = 0;
    }    
    mem -= 5;
    memcpy((void *)mem, (void *) data, 5*sizeof(int));
    _buf[2]++;    
    break;
  // end
  case END:
    if(cf + 3 > mem - 5) {
      write_mess(tid);
      cf = &_buf[3];
      mem = &_buf[1024];
      _buf[1] = _buf[2] = 0;
    }    
    mem -= 5;
    memcpy((void *)cf,  (void *) _data, 3*sizeof(int));
    memcpy((void *)mem, (void *) _data, 5*sizeof(int));
    _buf[1]++;_buf[2]++;    
    write_mess(tid);
    break;
  // error
  default:
    assert(false);
  }
}

__attribute__((noinline))
extern "C"
void tracer_cleanup() {
#pragma omp single
  for (int i = 0; i < nb_threads; i++)  {
    insert_mess(i, NULL, END, "");
#ifdef DUMP_ASCI_INPUT
    cf_s[i]  << "-1,-1,-1" << endl;
    cf_s[i].flush();
    cf_s[i].close();
#endif
  }
}

__attribute__((noinline))
extern "C"
void printBranch(char* name, char *kernel_type, char *run_dir, int cond, char* n1, char *n2)
{
  int tid = omp_get_thread_num();
  int mess[3];
  char *target;

  target = cond == 0 ? n1 : n2;
  mess[0] = 1;
  mess[1] = atoi(name);
  mess[2] = atoi(target);

  insert_mess(tid, mess, CF, string(run_dir));

#ifdef DUMP_ASCI_INPUT
  if (!cf_s[tid].is_open()) 
    cf_s[tid].open(string(get_dir_name(run_dir, kernel_type, "ctrl.txt")) + ".asci", ofstream::out );
  cf_s[tid]  << "B," << name << "," << target << endl << flush;
  cf_s[tid].flush();
#endif
}
  
 __attribute__((noinline))
extern "C"
void printuBranch(char* name, char *kernel_type, char *run_dir, char *n1)
{
  int tid = omp_get_thread_num();
  int mess[3];

  mess[0] = 0;
  mess[1] = atoi(name);
  mess[2] = atoi(n1);
  insert_mess(tid, mess, CF, string(run_dir));
#ifdef DUMP_ASCI_INPUT
  if (!cf_s[tid].is_open()) {
    cf_s[tid].open(string(get_dir_name(run_dir, kernel_type, "ctrl.txt")) + ".asci", ofstream::out );
  }
  cf_s[tid]  << "U," << name << "," << n1 << endl << flush;
  cf_s[tid].flush();
#endif
}

__attribute__((noinline))
extern "C"
void printMem(char *name, char *kernel_type, char *run_dir, bool type, long long addr, int size)
{
  int tid = omp_get_thread_num();
  int mess[5];
  int *name_ptr = (int *) &mess[1];
  int  *_mem_ptr = (int *) &mess[2];
  long long *mem_ptr = (long long *) _mem_ptr;
  int  *size_ptr     = (int *) &mess[4];
  
  *mess = type == 0 ? 1 : 0;
  *name_ptr = atoi(name);
  *mem_ptr  = addr;
  *size_ptr = size;
  insert_mess(tid, mess, MEM, string(run_dir));
#ifdef DUMP_ASCI_INPUT
  if (!mem_s[tid].is_open()) {
    mem_s[tid].open(string(get_dir_name(run_dir, kernel_type, "mem.txt")) + ".asci", ofstream::out );
  }
  if(type) 
    mem_s[tid] << "S," << name << "," << addr << "," << size << "\n";
  else
    mem_s[tid] << "L," << name << "," << addr << "," << size << "\n";
  mem_s[tid].flush();
#endif
}
 
__attribute__((noinline))
extern "C"
void printSw(char *name, char *kernel_type, char *run_dir, int value, char *def, int n, ...)
{
  int tid = omp_get_thread_num();
  int mess[3];
  va_list vl;
  vector<int> vals;
  vector<char*> strs;
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
  for(int i=0; i<vals.size(); i++)
    if(vals.at(i) == value) {
      target = strs.at(i);
      found = true;
      break;
    }
  
  if(!found)
    target = def;
  
  mess[0] = 2;
  mess[1] = atoi(name);
  mess[2] = atoi(target);
  insert_mess(tid, mess, CF, string(run_dir));
}


__attribute__((noinline))
extern "C"
void write_acc_mess(int tid, string mess, enum mess_type type) {
   char data[512];
   int *int_data = (int *)data, bytes;
   const char *char_mess = mess.c_str();
   int file_id;
   int_data[0] = tid;
   int_data[1] = type;
   strcpy((char *) &int_data[2], char_mess);

#ifdef USE_FILES
   file_id = tid;
#else
   file_id = tid / ceil(nb_threads/nb_files);
#endif
   while((bytes = write(acc_files[file_id], data, 512)) != 512)
     ;
}

__attribute__((noinline))
extern "C"
void print_matmul(char *acc_kernel_name, char *kernel_type, char *run_dir, char* node_id, int rowsA, int colsA , int rowsB, int colsB, int batch_size, float *A, float *B, float *C)
{
  stringstream stream;
  stream  << acc_kernel_name << "," << node_id << "," << rowsA << ","<< colsA << ","<< rowsB << ","<< colsB << ","
			   << batch_size <<","<<reinterpret_cast<long long>(A)<<","<<reinterpret_cast<long long>(B)<<","
			   <<reinterpret_cast<long long>(C);
  write_acc_mess(omp_get_thread_num(),  stream.str(), ACC);
}

__attribute__((noinline))
extern "C"
void print_sdp(char *acc_kernel_name, char *kernel_type, char *run_dir,
	       char* node_id, int working_mode, int size,
	       float *in, float *filter, float *out)
{
  stringstream stream;
 
  stream << acc_kernel_name << "," << node_id << "," << working_mode << "," << size << "," << reinterpret_cast<long long>(in)<<","<<reinterpret_cast<long long>(filter)<<","<<reinterpret_cast<long long>(out);
  write_acc_mess(omp_get_thread_num(),  stream.str(), ACC);
}

 __attribute__((noinline))
extern "C"
void print_bias(char *acc_kernel_name, char *kernel_type, char *run_dir,
	       char* node_id, int batch, int size,
	       float *in, float *bias, float *out)
{
  stringstream stream;
 
  stream << acc_kernel_name << "," << node_id << "," << batch << "," << size << "," << reinterpret_cast<long long>(in)<<","<<reinterpret_cast<long long>(bias)<<","<<reinterpret_cast<long long>(out);
    write_acc_mess(omp_get_thread_num(),  stream.str(), ACC);
}

__attribute__((noinline))
extern "C"
void print_conv2d_layer(char *acc_kernel_name, char *kernel_type, char *run_dir, char* node_id , int batch, int in_channels, int in_height, int in_width, int out_channels, int filter_height, int filter_width, bool zero_pad, int vert_conv_stride, int horiz_conv_stride, bool pooling, int pool_height, int pool_width, int vertical_pool_stride, int horizontal_pool_stride, float *in, float *filters, float *out)
{
  stringstream stream;
 
  stream << acc_kernel_name << "," << node_id << "," <</*0*/ batch <<"," << /*1*/ in_channels << "," << /*2*/ in_height << "," << /*3*/ in_width << "," << /*4*/ out_channels << "," << /*5*/ filter_height << "," << /*6*/ filter_width << "," << /*7*/ zero_pad << "," << /*8*/ vert_conv_stride << "," << /*9*/ horiz_conv_stride << "," << /*10*/ pooling << "," << /*11*/ pool_height << "," << /*12*/ pool_width << "," << /*13*/ vertical_pool_stride << "," << /*14*/ horizontal_pool_stride <<"," << reinterpret_cast<long long>(in)<<","<<reinterpret_cast<long long>(filters)<<","<<reinterpret_cast<long long>(out);
  write_acc_mess(omp_get_thread_num(),  stream.str(), ACC);
}

__attribute__((noinline))
extern "C"
void print_dense_layer(char *acc_kernel_name, char *kernel_type, char *run_dir,
		       char* node_id, int batch, int in_channels, int out_channels,
		       float *in, float *filters, float *out)
{
  stringstream stream;
 
  stream << acc_kernel_name << "," << node_id << "," << /*0*/ batch << ","<< /*1*/ in_channels << ","<< /*2*/ out_channels <<"," <<reinterpret_cast<long long>(in)<<"," <<reinterpret_cast<long long>(filters)<<"," <<reinterpret_cast<long long>(out)<< "\n";
  write_acc_mess(omp_get_thread_num(),  stream.str(), ACC);
}

__attribute__((noinline))
extern "C"
void print_partial_barrier(char *kernel_type, char *run_dir, int fence_id, int nb_threads)
{
  stringstream stream;
 
  stream << fence_id << "\t" << nb_threads;
  write_acc_mess(omp_get_thread_num(),  stream.str(), PB);
}
