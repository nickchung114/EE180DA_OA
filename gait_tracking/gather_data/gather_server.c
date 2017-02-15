// Gather server on 9DOF Edison
//     Gets data from client
//     Writes to files in ring buffer manner

#include <stdio.h>

#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define BATCH_SIZE 32
#define MAX_FILES 1
#define MAX_NUM_FILES 1 // only makes sense if MAX_FILES is 1
#define MAX_BUF 128 // Maximum string size
#define SAMP_SIZE (3*2) // num dimensions * |{accel,gyro}|
#define BUFF_SIZE (BATCH_SIZE*SAMP_SIZE) // batch size * sample size

void error(const char *msg) {
  perror(msg);
  exit(0);
}

int main(int argc, char *argv[]) {
  int server_socket_fd, client_socket_fd, portno;
  socklen_t clilen;
  float buffer[BUFF_SIZE];
  struct sockaddr_in serv_addr, cli_addr;
  int n;

  char dir_name[MAX_BUF];
  struct stat st = {0};
  time_t t = time(NULL);
  struct tm *now = localtime(&t);
  char file_name[MAX_BUF];

  int j;
  int file_num = 0;
  char dummy = 0x00;
  FILE *fp;

  // error check command line arguments
  if (argc < 2) {
    fprintf(stderr, "USAGE: %s <port>\n", argv[0]);
    exit(1);
  }

  // setup directory + files
  strftime(dir_name, MAX_BUF, "data_%F_%H-%M-%S", now);
  if (stat(dir_name, &st) == -1) mkdir(dir_name, 0755);
  printf("Directory name: %s\n", dir_name);

  // setup socket
  server_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_socket_fd < 0) {	
    error("ERROR opening socket");
  }

  // setup server information
  memset((char *) &serv_addr, 0, sizeof(serv_addr));
  portno = atoi(argv[1]);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);

  // bind the socket to an address
  if (bind(server_socket_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
    error("ERROR on binding");
  }

  // listen for incoming connections
  // accept at most 5 connections before refusing them
  listen(server_socket_fd, 5);
  clilen = sizeof(cli_addr);

  printf("Waiting for client...\n");
  // block until there is a new connection. When one is received, accept it
  client_socket_fd = accept(server_socket_fd, (struct sockaddr *) &cli_addr, &clilen);
  if (client_socket_fd < 0) {
    error("ERROR on accept");
  }  

  // clear the buffer
  memset(buffer, 0, BUFF_SIZE*sizeof(float));

  while ((!MAX_FILES) || (MAX_FILES && file_num < MAX_NUM_FILES)) {
    sprintf(file_name, "%s/data%02d_CalInertialAndMag.csv",
	    dir_name, file_num);
    fp = fopen(file_name, "w");

    // read what the client sent to the server and store it in "buffer"
    n = read(client_socket_fd, buffer, BUFF_SIZE*sizeof(float));
    if (n < 0) {
      error("ERROR reading from socket");
    }

    for (j = 0; j < BATCH_SIZE; j++) {
      fprintf(fp, "%i,%f,%f,%f,%f,%f,%f\n", j+1,
	      buffer[j*SAMP_SIZE], buffer[(j*SAMP_SIZE)+1], buffer[(j*SAMP_SIZE)+2],
	      buffer[(j*SAMP_SIZE)+3], buffer[(j*SAMP_SIZE)+4], buffer[(j*SAMP_SIZE)+5]);

    }

    printf("Completed file %d\n", file_num);
    fclose(fp);
    file_num++;
  }

  // ping client to finish
  n = write(client_socket_fd, &dummy, 1);
  if (n < 0) {
    error("ERROR pinging client to finish");
  }
  
  close(client_socket_fd);
  close(server_socket_fd);
  return 0; 
}
