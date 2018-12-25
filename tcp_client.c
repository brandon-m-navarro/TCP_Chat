// created by bmnavarro@wpi.edu June 2018
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdbool.h>

int main() {

  // create a socket
  int network_socket = 0;
  int fdmax, i, checkSelect, checkSend;
  fd_set master; // create a master set
  fd_set clients; // create set for clients
  int recent = 1; // used to track if the client is new to the server

  // define the server address
  struct sockaddr_in server_address;

  network_socket = socket(AF_INET, SOCK_STREAM, 0); // (Domain, Type of socket, Protocol)
  if(network_socket == -1){
    perror("socket");
    exit(1);
  }
  //
  server_address.sin_family = AF_INET; // knows what type of server_address
  server_address.sin_port = htons(9002); // htons() converts data format so that our struct can understand the port number
  server_address.sin_addr.s_addr = INADDR_ANY; // bind to any available address
  memset(server_address.sin_zero, '\0', sizeof(server_address.sin_zero)); // for padding, to make it the same size as struct sockaddr

  // connect with server socket
  int connection_status = connect(network_socket, (struct sockaddr*) &server_address, sizeof(server_address));
  // check for error with connection
  if(connection_status == -1){
    perror("connect");
    exit(-1);
  }else{
    printf("Connected to remote socket \n");
    //printf("Welcome to the server! Please enter a username.\n");
  }
  FD_ZERO(&master); // zero the sets
  FD_ZERO(&clients);
  //printf("network socket: \t%d\n", network_socket);

  FD_SET(0, &master);  // add zero to the set so that we can watch for input
  FD_SET(network_socket, &master); // add the connected socket
  //printf("network socket: \t%d\n", network_socket);
  fdmax = network_socket; // fdmax is the range, so add update it to include the only socket in the set

  // Need to print a welcome message to the client when they first connect to the server
  // Give clients the ability to choose a username upon connecting

  while(1){
    clients = master;
    char username[256];
    // select() indicates which of the specified sockets is ready for reading,
    // writing, or has an error pending. It halts the program until data arrives
    // on any of the client sockets because no timeout is specified.
    checkSelect = select(fdmax + 1, &clients, NULL, NULL, NULL);
    if(checkSelect == -1){
      perror("select");
      exit(1);
    }

    for(i = 0; i <= fdmax; i++){
      // FD_ISSET returns a non-zero if the socket exists in the set
      // if zero is returned, then the while loops starts again
      if(FD_ISSET(i, &clients)){
        char send_buf[1024];
        char recv_buf[1024];
        //char user_buf[10];
        int bytes_rec;
        if(i == 0){ // if i is zero, then check for input
          if(recent){
            puts("Please enter a username\n");
            fgets(send_buf, 256, stdin);
            checkSend = send(network_socket, send_buf, strlen(send_buf), 0);
            if(checkSend == -1){
              perror("send");
            }
            recent = 0;
          }
          fgets(send_buf, 1024, stdin); // wait for user input
          if(strcmp(send_buf, "leave\n") == 0) { // if the user types leave, leave
            exit(1);
          }else{ // otherwise, attempt to send
            checkSend = send(network_socket, send_buf, strlen(send_buf), 0);
            if(checkSend == -1){
              perror("send");
            }
          }
        }else { // if the client isn't sending a message, they must be recieving a message

          bytes_rec = recv(network_socket, recv_buf, 1024, 0);
          if(bytes_rec == -1){
            perror("recv");
          }
          recv_buf[bytes_rec] = '\0'; // place null terminating character at end of string

          printf("%s\n", recv_buf);
          fflush(stdout);
        }
      }
    }
  }
  //printf("Clients have left\n");
  close(network_socket);
  return 0;
}
