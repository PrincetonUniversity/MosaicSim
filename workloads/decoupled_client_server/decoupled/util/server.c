// From https://www.geeksforgeeks.org/socket-programming-cc/

// TODO: Because we're just doing local sockets on unix, we can probably
// use unix style sockets to accelerate this.

// Server side C/C++ program to demonstrate Socket programming

#include <unistd.h> 
#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h>
#include "decouple.h"
#define PORT 8080

int supply_sock, compute_sock;
char buffer_rec[1024] = {0};
char buffer_send[1024] = {0}; 
int main(int argc, char const *argv[]) 
{
  
    int server_fd, new_socket, valread; 
    struct sockaddr_in address; 
    int opt = 1; 
    int addrlen = sizeof(address); 

    char *hello = "Hello from server"; 
       
    // Creating socket file descriptor 
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) 
    { 
        perror("socket failed"); 
        exit(EXIT_FAILURE); 
    } 
       
    // Forcefully attaching socket to the port 8080 
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, 
                                                  &opt, sizeof(opt))) 
    { 
        perror("setsockopt"); 
        exit(EXIT_FAILURE); 
    } 
    address.sin_family = AF_INET; 
    address.sin_addr.s_addr = INADDR_ANY; 
    address.sin_port = htons( PORT ); 
       
    // Forcefully attaching socket to the port 8080 
    if (bind(server_fd, (struct sockaddr *)&address,  
                                 sizeof(address))<0) 
    { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    }

    //Connect to both of the sockets
    for (int i = 0; i < 2; i++) {
      if (listen(server_fd, 3) < 0) { 
        perror("listen"); 
        exit(EXIT_FAILURE); 
      } 

      if ((new_socket = accept(server_fd, (struct sockaddr *)&address,  
                               (socklen_t*)&addrlen))<0)  { 
        perror("accept"); 
        exit(EXIT_FAILURE); 
      }
      read(new_socket , buffer_rec, 1024);
      
      if (strstr(buffer_rec, SUPPLY_STR) != NULL) {
        supply_sock = new_socket;
      }
      else {
        compute_sock = new_socket;
      }
      printf("hello from %s\n", buffer_rec);
      send(new_socket, buffer_send, 1024, 0); 
    }
    
    while(1) {
      
      memset(buffer_rec, '\0', 1024); 
      int bytes_read = read(supply_sock, buffer_rec, 1024);

      //printf("READ %s\n", buffer_rec);

      if (strstr(buffer_rec, PROD_CON_TYPE) != NULL) {
        //prod-con type
        printf("executing produce/consume interaction\n");

        //read the supply value
        int rec_int;
        read(supply_sock, &rec_int, sizeof(rec_int));
        int converted = ntohl(rec_int);
        //printf("got the number %d\n", converted);
        int to_send = htonl(converted);

        //send it to the compute side
        write(compute_sock, &to_send, sizeof(to_send));
      }
      else if (strstr(buffer_rec, STORE_TYPE) != NULL) {
        printf("executing store interaction\n");
        // store-type

        int rec_int;
        //read the value from the compute side
        read(compute_sock, &rec_int, sizeof(rec_int));
        int converted = ntohl(rec_int);
        //printf("got the number %d\n", converted);

        int to_send = htonl(converted);

        //send it to the supply side
        write(supply_sock, &to_send, sizeof(to_send));
      }
      else if (bytes_read != 0) {
        printf("ERROR %s\n", buffer_rec);
      }
    }    
    return 0; 
}
