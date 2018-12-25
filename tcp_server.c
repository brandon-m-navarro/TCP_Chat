// Created by bmnavarro@wpi.edu June 2018
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <sys/select.h>

#define maxConnect 5

int main() {
  // create a socket
  int server_socket = 0;
  int fdmax, i, checkSelect, checkBind, checkListen, checkSend, checkSockopt, counter;
  fd_set master; // create a master set
  fd_set clients; // create set for clients
  counter = maxConnect;

  // define the server address
  struct sockaddr_in server_address;
  struct sockaddr_in client_address;

  FD_ZERO(&master); // zero the set

  // (Domain, Type of socket, Protocol)
  server_socket = socket(AF_INET, SOCK_STREAM, 0);
  if(server_socket == -1){
    perror("socket");
    exit(1);
  }

  // bind the socket to our specified IP and port
  server_address.sin_family = AF_INET; // // knows what type of server_address
  server_address.sin_port = htons(9002); // htons() converts data format so that our struct can understand the port number
  server_address.sin_addr.s_addr = INADDR_ANY; // bind to any available address
  memset(server_address.sin_zero, '\0', sizeof server_address.sin_zero); // for padding, to make it the same size as struct sockaddr

  int yes = 1;
  checkSockopt = setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
  if(checkSockopt == -1){
    perror("setsockopt");
    exit(1);
  }

  checkBind = bind(server_socket, (struct sockaddr *) &server_address, sizeof(struct sockaddr));
  if(checkBind == -1){
    perror("unable to bind");
    exit(1);
  }

  // begin listening for clients attempting to connect
  checkListen = listen(server_socket, maxConnect);
  if(checkListen == -1){
    perror("listen");
    exit(1);
  }

  printf("%d more clients may connect\nListening for new clients...\n", counter);
  fflush(stdout);

  //printf("server socket: \t%d\n", server_socket);
  FD_SET(server_socket, &master); // sets the bit for the server_socket in the master socket set

  fdmax = server_socket; // fdmax is the range of sockets to be tested.
  int j; // for loop
  char update[1024]; // Holds message to send to clients keeping them updated with server activities
  // create loop so that new clients may connect to the server
  while(1){
    // create a copy of the master set because calling select() destroys the socket set
    clients = master;
    // select() indicates which of the specified sockets is ready for reading,
    // writing, or has an error pending. It halts the program until data arrives
    // on any of the client sockets because no timeout is specified.
    checkSelect = select(fdmax + 1, &clients, NULL, NULL, NULL);
    if(checkSelect == -1){
      perror("select");
      exit(1);
    }
    // if the program gets here, then a client is interacting with the server
    // set up a for loop to go through the list of sockets that are in the set
    for(i = 0; i <= fdmax; i++){
      // FD_ISSET returns a non-zero (1) if the socket exists in the set
      // if zero is returned, then the while loops starts again
      if(FD_ISSET(i, &clients)){
        // if the socket is connecting to the listening socket,
        // their decriptors should be the same
        if(i == server_socket){
          //printf("%d\n", server_socket);
          socklen_t addrlen;
          int client_socket;
          addrlen = sizeof(struct sockaddr_in);
          // accept connection
          /* accept extracts the first connection request on the queue of pending connections for the listening socket,
          sockfd, creates a new connected socket, and returns a new file descrip‐
          tor  referring  to that socket. */
          client_socket = accept(server_socket, (struct sockaddr *)&client_address, &addrlen);
          if(client_socket == -1){ // error with accepting connection to client
            perror("accept");
            exit(1);
          }else { // if there's no error. we can add the client to the set
          FD_SET(client_socket, &master);
          if(client_socket > fdmax){
            fdmax = client_socket; // update the range to include new socket
          }
          counter--;
          printf("New connection!\n  %d more clients may connect\nListening for new clients...\n", counter);
          sprintf(update, "*\n***A new client has joined***\n*\n");
          for(j = 0; j <= fdmax; j++){
            if (FD_ISSET(j, &master)){
              if(j != server_socket && j != i){
                checkSend = send(j, update, sizeof(update), 0);
                if(checkSend == -1){
                  perror("send");
                }
              }
            }
          }
        }
      }
      // if the client isn't connecting, then it must be sending data
      else {
        int bytes_rec, j;
        char recv_buf[1024], buf[1024];
        // recv() returns a negative value if an error occured
        // if it returns 0 then a stream socket peer has performed an orderly shutdown
        if((bytes_rec = recv(i, recv_buf, 1024, 0)) <= 0) {
          if(bytes_rec == 0){
            printf("socket %d hung up\n", i); // the client has terminated connection
            counter ++;
            printf("%d more clients may connect\nListening for new clients...\n", counter);
          }else {
            perror("recv");
          }
          close(i); // close the socket
          FD_CLR(i, &master); // clear the socket from the master set
          sprintf(update, "-\n***Socket %d left the server***\n-\n", i); // update now knows what socket disconnected
          for(j = 0; j <= fdmax; j++){
            if (FD_ISSET(j, &master)){
              if(j != server_socket){
                checkSend = send(j, update, sizeof(update), 0);
                if(checkSend == -1){
                  perror("send");
                }
              }
            }
          }
        }else { // recv returns the number of bytes recieved
          // loop through all clients in the master set
          for(j = 0; j <= fdmax; j++){
            if (FD_ISSET(j, &master)){ // if the bit is set for the socket
              if(j != server_socket && j != i){ // don't send message to server socket or the socket sending the message
              checkSend = send(j, recv_buf, bytes_rec, 0);
              if(checkSend == -1){
                perror("send");
              }
            }
          }
        }
      }
    }
  }
}
}
close(server_socket);
return 0;
}
