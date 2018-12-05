#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h>
#include "assert.h"
#include "stub.h"

#define PORT 8080

int desc_sock = 0;

int ID = 0;

char desc_buffer_send[1024] = {0};
char desc_buffer_rec[1024] = {0};

typedef union {
  int i;
  float f;
 } fi32;

typedef union {
  uint64_t i;
  double f;
 } fi64;



char desc_compute_consume_i8() {
  assert(0);
}

int desc_compute_consume_i32() {
  int to_ret = 0;
  read(desc_sock, &to_ret, sizeof(to_ret));
  return ntohl(to_ret);
}

uint64_t desc_compute_consume_i64() {
  uint64_t to_ret = 0;
  read(desc_sock, &to_ret, sizeof(to_ret));
  uint64_t tmp = ntohll(to_ret);
  //printf("got the i64 value %lu\n", tmp);
  return tmp;
}


void * desc_compute_consume_ptr() {
  void * to_ret = NULL;
  read(desc_sock, &to_ret, sizeof(to_ret));
  void *tmp = ntohll_ptr(to_ret);
  //printf("got the addr value %lu\n", tmp);
  return tmp;  
}

float desc_compute_consume_float() {
  fi32 tmp;
  tmp.i = desc_compute_consume_i32();
  return tmp.f;
}

double desc_compute_consume_double() {
  fi64 tmp;
  tmp.i = desc_compute_consume_i64();
  return tmp.f;
}

void desc_compute_produce_i8(char x) {
  assert(0);
}

void desc_compute_produce_i32(int x) {
  int to_send = htonl(x);
  write(desc_sock, &to_send, sizeof(to_send));
}

void desc_compute_produce_i64(uint64_t x) {
  uint64_t to_send = htonll(x);
  //printf("sending i64: %lu\n",x);
  write(desc_sock, &to_send, sizeof(to_send));
}

void desc_compute_produce_ptr(void *x) {
  void * to_send = htonll_ptr(x);
  //printf("sending addr: %lu\n",x);
  write(desc_sock, &to_send, sizeof(to_send));
}

void desc_compute_produce_float(float x) {
  fi32 tmp;
  tmp.f = x;
  desc_compute_produce_i32(tmp.i);
}

void desc_compute_produce_double(double x) {
  fi64 tmp;
  tmp.f = x;
  desc_compute_produce_i64(tmp.i);
}

// Supply

void desc_supply_produce_i8(char val) {
  assert(0); // Not supported yet. Although shouldn't be difficult to add when/if needed
}

void desc_supply_produce_i32(int val) {
  strcpy(desc_buffer_send, SUPPLY_PROD_TYPE_32);
  send(desc_sock, desc_buffer_send, 1024, 0);
  int to_send =  val;
  int converted = htonl(to_send);
  write(desc_sock, &converted, sizeof(converted));
}

void desc_supply_produce_i64(uint64_t val) {
  strcpy(desc_buffer_send, SUPPLY_PROD_TYPE_64);
  send(desc_sock, desc_buffer_send, 1024, 0);
  uint64_t to_send = val;
  //printf("sending addr: %lu\n",to_send);
  uint64_t converted = htonll(to_send);
  write(desc_sock, &converted, sizeof(converted)); 
}

void desc_supply_produce_ptr(void *addr) {
  strcpy(desc_buffer_send, SUPPLY_PROD_TYPE_64);
  send(desc_sock, desc_buffer_send, 1024, 0);
  void * to_send = addr;
  //printf("sending addr: %lu\n",to_send);
  void * converted = htonll_ptr(to_send);
  write(desc_sock, &converted, sizeof(converted));
}

void desc_supply_produce_float(float val) {
  fi32 tmp;
  tmp.f = val;
  desc_supply_produce_i32(tmp.i);
}

void desc_supply_produce_double(double val) {
  fi64 tmp;
  tmp.f = val;
  desc_supply_produce_i64(tmp.i);
}

// supply consume
void desc_supply_consume_i8(char *addr) {
  assert(0);
}

void desc_supply_consume_i32(int *addr) {
  strcpy(desc_buffer_send, SUPPLY_CONS_TYPE_32);
  send(desc_sock, desc_buffer_send, 1024, 0);

  int to_store = 0;
  read(desc_sock, &to_store, sizeof(to_store));
  *addr = ntohl(to_store);
}

void desc_supply_consume_i64(uint64_t *addr) {
  strcpy(desc_buffer_send, SUPPLY_CONS_TYPE_64);
  send(desc_sock, desc_buffer_send, 1024, 0);
  
  uint64_t to_store = 0;
  read(desc_sock, &to_store, sizeof(to_store));
  uint64_t tmp = ntohll(to_store);
  //printf("got the i64 value %lu\n", tmp);
  *addr = tmp;

}

void desc_supply_consume_ptr(void **addr) {
  strcpy(desc_buffer_send, SUPPLY_CONS_TYPE_64);
  send(desc_sock, desc_buffer_send, 1024, 0);

  void * to_store = NULL;
  read(desc_sock, &to_store, sizeof(to_store));
  void * tmp = ntohll_ptr(to_store);
  //printf("got the addr value %lu\n", tmp);
  *addr = tmp;
}

void desc_supply_consume_float(float *addr) {

  strcpy(desc_buffer_send, SUPPLY_CONS_TYPE_32);
  send(desc_sock, desc_buffer_send, 1024, 0);

  int to_store = 0;
  read(desc_sock, &to_store, sizeof(to_store));
  to_store = ntohl(to_store);
  fi32 tmp;
  tmp.i = to_store;
  *addr = tmp.f;
}

void desc_supply_consume_double(double *addr) {
  strcpy(desc_buffer_send, SUPPLY_CONS_TYPE_64);
  send(desc_sock, desc_buffer_send, 1024, 0);
  
  uint64_t to_store = 0;
  read(desc_sock, &to_store, sizeof(to_store));
  to_store = ntohll(to_store);
  //printf("got the i64 value %lu\n", tmp);
  fi64 tmp;
  tmp.i = to_store;
  *addr = tmp.f;
}


// init and cleanup
void desc_init(int option) {
  
  struct sockaddr_in address; 
  struct sockaddr_in serv_addr; 
  if ((desc_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) { 
    printf("\n Socket creation error \n"); 
    exit(1); 
  }

  ID = option;
  
  memset(&serv_addr, '0', sizeof(serv_addr)); 
  
  serv_addr.sin_family = AF_INET; 
  serv_addr.sin_port = htons(PORT); 
  
  // Convert IPv4 and IPv6 addresses from text to binary form 
  if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0) { 
    printf("\nInvalid address/ Address not supported \n"); 
    exit(1); 
  } 
  
  if (connect(desc_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) { 
    printf("\nConnection Failed \n"); 
    exit(1); 
  }
  
  if (option == SUPPLY) {
    strcpy(desc_buffer_send, SUPPLY_STR);
  }
  if (option == COMPUTE) {
    strcpy(desc_buffer_send, COMPUTE_STR);
  }
  
  // Tell the server who we are and wait for a response
  send(desc_sock, desc_buffer_send, 1024, 0);
  read(desc_sock, desc_buffer_rec, 1024);

  printf("connected to desc server!\n");
}

void desc_cleanup() {
  int err = 0;
  if (ID == SUPPLY) {
    strcpy(desc_buffer_send, SUPPLY_FINISH);
  }
  else if (ID == COMPUTE) {
    strcpy(desc_buffer_send, COMPUTE_FINISH);
  }
  else {
    printf("error: unrecognized ID: %d\n", ID);
    exit(1);
  }
  printf("sending finished!\n");
  send(desc_sock, desc_buffer_send, 1024, 0);
  
  // Just to wait for ack from server
  read(desc_sock, desc_buffer_rec, 1024); 
  if (close(desc_sock) < 0) {
    printf("\nerror closing socket!\n");
    exit(1);
  }  
}
