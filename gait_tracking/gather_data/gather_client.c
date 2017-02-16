// Gather client on 9DOF Edison
//     Gets data from sensors
//     Sends to server

#include <stdio.h>

#include <mraa/i2c.h>
#include "LSM9DS0.h"

#include <errno.h>
#include <netdb.h> 
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

//#define DEBUG

// point := sample
//#define BATCH_PTS 256 // num pts per batch (ie: pts per file)
#define PACK_PTS 16 // number of pts per packet. Should divide BATCH_PTS
#define SAMP_RATE 256
#define INIT_SEC 2
#define SAMP_SIZE (3*2) // num dimensions * |{accel,gyro}|
#define PACK_NUM 10 // Size of buffer of packets b/c non-blocking I/O
#define PACK_SIZE (PACK_PTS*SAMP_SIZE)
#define BUFF_SIZE (PACK_NUM*PACK_SIZE)
#define BUFF_BYTES (BUFF_SIZE*sizeof(float)) // size of buffer in bytes
#define PRINT_PERIOD 1024

void error(const char *msg) {
  perror(msg);
  exit(0);
}

int main(int argc, char *argv[]) {
  data_t accel_data, gyro_data;
  data_t gyro_offset;
  float a_res, g_res;
  mraa_i2c_context accel, gyro;
  accel_scale_t a_scale = A_SCALE_8G;
  gyro_scale_t g_scale = G_SCALE_2000DPS;

  int client_socket_fd, portno, n;
  struct sockaddr_in serv_addr;
  struct hostent *server;
  int flags;
  float buffer[BUFF_SIZE];
  int send_idx = 0, curr_idx = 0;
  int next_it, send_it, curr_it; // helper variables for converting idx (byte) to it (float)

  int num_pts = 0;
  int i;
  int init_sam = (SAMP_RATE*INIT_SEC)+1; // number of samples during init period
  int quit = 0; // quit flag
  char stop; // dummy buffer for stop ping

  // Read command line arguments, need to get the host IP address and port
  if (argc < 3) {
    fprintf(stderr, "USAGE: %s <hostname> <port>\n", argv[0]);
    exit(1);
  }

  // Convert the arguments to the appropriate data types
  portno = atoi(argv[2]);

  // setup the socket
  // technically PF_INET. Refer to:
  //     http://stackoverflow.com/questions/6729366/what-is-the-difference-between-af-inet-and-pf-inet-in-socket-programming
  //client_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  client_socket_fd = socket(PF_INET, SOCK_STREAM, 0);
  
  // check if the socket was created successfully. If it wasnt, display an error and exit
  if(client_socket_fd < 0) {
    error("ERROR opening socket");
  }

  // check if the IP entered by the user is valid 
  server = gethostbyname(argv[1]);
  if (server == NULL) {
    fprintf(stderr,"ERROR, no such host\n");
    exit(0);
  }

  // clear our the serv_addr buffer
  memset((char *) &serv_addr, 0, sizeof(serv_addr));
  // set up the socket 
  serv_addr.sin_family = AF_INET;
  memcpy((char *)&serv_addr.sin_addr.s_addr, (char *)server->h_addr, server->h_length);
  serv_addr.sin_port = htons(portno);

  // try to connect to the server
  if (connect(client_socket_fd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0){ 
    error("ERROR connecting");
  }

  // make non-blocking. Refer to:
  //     http://developerweb.net/viewtopic.php?id=3000
  /* Set socket to non-blocking */
  if ((flags = fcntl(client_socket_fd, F_GETFL, 0)) < 0) {
    error("ERROR getting socket flags");
  }
  if (fcntl(client_socket_fd, F_SETFL, flags | O_NONBLOCK) < 0) {
    error("ERROR setting socket to non-blocking");
  }

  printf("Socket connected to server!\n");
  printf("Now beginning sensor initialization\n");
  
  // initialize sensors, set scale, and calculate resolution.
  accel = accel_init();
  set_accel_scale(accel, a_scale);	
  a_res = calc_accel_res(a_scale);
	
  gyro = gyro_init();
  set_gyro_scale(gyro, g_scale);
  g_res = calc_gyro_res(g_scale);
	
  gyro_offset = calc_gyro_offset(gyro, g_res);

  printf("GYRO OFFSETS x: %f y: %f z: %f\n",
	 gyro_offset.x, gyro_offset.y, gyro_offset.z);

  //Read the sensor data and print them.
  printf("STARTING TO COLLECT DATA\n");
  printf("\n    \t\tAccelerometer\t\t\t||");
  printf("\t\t\tGyroscope\t\t\t\n");

  while (1) {
    for (i = 0; i < PACK_PTS; i++) {
      n = read(client_socket_fd, &stop, 1);
      if (n < 0) {
	if (!(errno == EWOULDBLOCK || errno == EAGAIN))
	  error("ERROR reading from client socket");
	// else no stop ping yet
      } else {
#ifdef DEBUG
	printf("RECEIVED STOP PING\n");
#endif
	quit = 1;
	break;
      }

      if (num_pts == init_sam) printf("INITIALIZATION PERIOD DONE\n");
      accel_data = read_accel(accel, a_res);
      gyro_data = read_gyro(gyro, g_res);

      if (num_pts%PRINT_PERIOD == 0) {
	printf("%i points\n", num_pts);
	printf("    send_idx: %d    curr_idx: %d\n", send_idx, curr_idx);
	printf("    X: %f\t Y: %f\t Z: %f\t||", accel_data.x, accel_data.y, accel_data.z);
	printf("\tX: %f\t Y: %f\t Z: %f p: %d\t\n", gyro_data.x - gyro_offset.x, gyro_data.y - gyro_offset.y, gyro_data.z - gyro_offset.z, num_pts);
      }

#ifdef DEBUG
      printf("X: %f\t Y: %f\t Z: %f\t||", accel_data.x, accel_data.y, accel_data.z);
      printf("\tX: %f\t Y: %f\t Z: %f p: %d\t\n", gyro_data.x - gyro_offset.x, gyro_data.y - gyro_offset.y, gyro_data.z - gyro_offset.z, num_pts);
#endif

      curr_it = curr_idx/sizeof(float);
      buffer[curr_it] = gyro_data.x - gyro_offset.x;
      buffer[curr_it+1] = gyro_data.y - gyro_offset.y;
      buffer[curr_it+2] = gyro_data.z - gyro_offset.z;
      buffer[curr_it+3] = accel_data.x;
      buffer[curr_it+4] = accel_data.y;
      buffer[curr_it+5] = accel_data.z;
      
      num_pts++;
      next_it = (curr_it + SAMP_SIZE)%BUFF_SIZE;
      send_it = send_idx/sizeof(float); // truncate integer
      if ((next_it >= send_it) &&
	  (next_it <= (send_it+SAMP_SIZE-1))) {
	// note that curr_idx always advances SAMP_SIZE at a time
	// went around ring buffer. panic!
	fprintf(stderr, "Ring buffer is full. Abort!\n");
	exit(1);
      }
      curr_idx = next_it*sizeof(float);
    } // batch done

    // stop writing packets if received stop ping
    if (quit) break;

    // write batch to server
    n = write(client_socket_fd, ((char*)buffer)+send_idx,
	      ((curr_idx>send_idx)?
	       (curr_idx-send_idx):
	       (BUFF_BYTES-send_idx)));
    // n contains how many bytes were written to the socket
    // if n is less than 0, then there was an error
    // Refer to:
    //     http://stackoverflow.com/questions/3153939/properly-writing-to-a-nonblocking-socket-in-c
    //     http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html
    if (n < 0) {
      if (errno == EWOULDBLOCK || errno == EAGAIN) {
	fprintf(stderr, "FAILED WRITE for byte %d. Currently at byte %d\n",
		send_idx, curr_idx);
      } else {
	error("ERROR writing to socket");
      }
    } else { // successful write
      send_idx = (send_idx+n)%BUFF_BYTES;
    }
    
#ifdef DEBUG
    printf("send_idx: %d    curr_idx: %d\n", send_idx, curr_idx);
#endif
    usleep(1000000/SAMP_RATE);
  }

  close(client_socket_fd);
  return 0;
}
