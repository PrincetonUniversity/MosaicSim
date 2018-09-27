#include <stdio.h> 
#include <stdlib.h> 
#include <sys/socket.h> 
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>

int desc_sock = 0;

char desc_buffer_send[1024] = {0};
char desc_buffer_rec[1024] = {0};

#define SUPPLY 0
#define SUPPLY_STR "SUPPLY\0"

#define COMPUTE 1
#define COMPUTE_STR "COMPUTE\0"

#define PROD_CON_TYPE "PROD_CON_TYPE\0"
#define STORE_TYPE "STORE_TYPE\0"

int desc_init(int option, int portnum) {
  struct sockaddr_in address; 
  struct sockaddr_in serv_addr; 
  if ((desc_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) { 
    printf("\n Socket creation error \n"); 
    return -1; 
  } 
  
  memset(&serv_addr, '0', sizeof(serv_addr)); 
  
  serv_addr.sin_family = AF_INET; 
  serv_addr.sin_port = htons(portnum); 
  
  // Convert IPv4 and IPv6 addresses from text to binary form 
  if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0) { 
    printf("\nInvalid address/ Address not supported \n"); 
    return -1; 
  } 
  
  if (connect(desc_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) { 
    printf("\nConnection Failed \n"); 
    return -1; 
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
  return 0;
}

void desc_cleanup() {
}

void desc_join() {
}

// For the access slice
void desc_produce_i32(int x) {

  //Signal what type of interaction
  strcpy(desc_buffer_send, PROD_CON_TYPE);
  send(desc_sock, desc_buffer_send, 1024, 0);

  int to_send = x;
  int converted = htonl(to_send);
  //Figure out how to send integer
  write(desc_sock, &converted, sizeof(converted));
  //read(desc_sock, desc_buffer_rec, 1024);

}

void desc_store_addr_i32(int *x) {

  //Signal what type of interaction
  strcpy(desc_buffer_send, STORE_TYPE);
  send(desc_sock, desc_buffer_send, 1024, 0);

  int to_store = 0;
  read(desc_sock, &to_store, sizeof(to_store));
  //figure out how to read
  *x = ntohl(to_store);
  //send(desc_sock, desc_buffer_send, 1024, 0);
  
}

// For the compute slice
int desc_consume_i32() {
  int to_ret = 0;
  read(desc_sock, &to_ret, sizeof(to_ret));
  //printf("read %d\n", ntohl(to_ret));
  return ntohl(to_ret);
       
}

void desc_store_val_i32(int x) {
  //figure out how to send x
  int to_send = htonl(x);  
  write(desc_sock, &to_send, sizeof(to_send));
}

