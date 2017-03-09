#include <errno.h>
#include <math.h>
#include <mraa/i2c.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include "LSM9DS0.h"

#define DEBUG

// point := sample
#define PACK_PTS 16 // number of pts per packet. Should divide BATCH_PTS
#define SAMP_RATE 256
#define INIT_SEC 2
#define SAMP_SIZE ((3*2)+1) // (num dimensions * |{accel,gyro}|) + |{stomp}|
#define PACK_NUM 10 // Size of buffer of packets b/c non-blocking I/O
#define PACK_SIZE (PACK_PTS*SAMP_SIZE)
#define BUFF_SIZE (PACK_NUM*PACK_SIZE)
#define BUFF_BYTES (BUFF_SIZE*sizeof(float)) // size of buffer in bytes
#define PRINT_PERIOD 1024

void error(const char *msg) {
  perror(msg);
  exit(1);
}

/*
 * Setup IMU
 * Setup client's socket and connect with server
 * Send ID to server and wait for server to be ready
 */
int main(int argc, char *argv[]) {
  //>>>> IMU CODE FROM ~/tuts/7-imu/example.c >>>>//
  // setup imu
  data_t accel_data, gyro_data;
  data_t gyro_offset;
  float a_res, g_res;
  mraa_i2c_context accel, gyro;
  accel_scale_t a_scale = A_SCALE_8G;
  gyro_scale_t g_scale = G_SCALE_2000DPS;

  uint32_t stomp = 0, stompLastPlayed = 0;
  float energy = 0,ax,ay,az,gx,gy,gz;
  char *data;

  //>>>> SOCKET-CLIENT CODE FROM ~/tuts/6-TCP_Comm/client.c >>>>//
  int client_socket_fd, portno, n, sent, tot;
  unsigned char conf;
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

  // Read command line arguments, need to get the host IP address and port
  if (argc < 3) {
    fprintf(stderr, "usage %s hostname port\n", argv[0]);
    exit(1);
  }

  // Convert the arguments to the appropriate data types
  portno = atoi(argv[2]);

  // setup the socket (IPv4, TCP)
  client_socket_fd = socket(PF_INET, SOCK_STREAM, 0);

  // check if the socket was created successfully. If it wasnt, display an error and exit
  if(client_socket_fd < 0) {
    error("ERROR opening socket\n");
  }

  // check if the IP entered by the user is valid
  server = gethostbyname(argv[1]);
  if (server == NULL) {
    fprintf(stderr,"ERROR: no such host\n");
    exit(1);
  }
	
  // clear out the serv_addr buffer
  memset((char *) &serv_addr, 0, sizeof(serv_addr));
  // set up the socket
  serv_addr.sin_family = AF_INET;
  memcpy((char *)&serv_addr.sin_addr.s_addr, (char *)server->h_addr, server->h_length);
  serv_addr.sin_port = htons(portno);

  // try to connect to the server
  if (connect(client_socket_fd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0){
    error("ERROR connecting");
  }

  printf("Connected with server...\n");
  //<<<< SOCKET-CLIENT CODE FROM ~/tuts/6-TCP_Comm/client.c <<<<//

  //SENDING ID TO SERVER
  // TODO: Make the ID a command-line arg
  uint16_t id = 0x0100; // (client_type, client_id)
  sent = 0;
  while (sent < 2) {
    n = write(client_socket_fd,&id,2);

    //>>>> SOCKET-CLIENT CODE FROM ~/tuts/6-TCP_Comm/client.c >>>>//
    // n contains how many bytes were received by the server
    if (n < 0) {
      error("ERROR writing to socket\n");
    }
    //<<<< SOCKET-CLIENT CODE FROM ~/tuts/6-TCP_Comm/client.c <<<<//

    sent += n;
  }
  printf("Sent my ID to server.\n");
	
  // READ A CONFIRMATION FROM SERVER
  n = read(client_socket_fd,&conf,1);
  if (n < 0) {
    error("ERROR reading from socket\n");
  }
  if (n != 1) { // failed to confirm
    error("ERROR received refusal from server\n");
  }

  /* Set socket to non-blocking */
  if ((flags = fcntl(client_socket_fd, F_GETFL, 0)) < 0) {
    error("ERROR getting socket flags");
  }
  if (fcntl(client_socket_fd, F_SETFL, flags | O_NONBLOCK) < 0) {
    error("ERROR setting socket to non-blocking");
  }

  printf("Now beginning sensor initialization\n");
  accel = accel_init();
  set_accel_scale(accel, a_scale);	
  a_res = calc_accel_res(a_scale);
	
  gyro = gyro_init();
  set_gyro_scale(gyro, g_scale);
  g_res = calc_gyro_res(g_scale);
	
  gyro_offset = calc_gyro_offset(gyro, g_res);
  //printf("x: %f\ty: %f\tz: %f\n", gyro_offset.x, gyro_offset.y, gyro_offset.z);
  //<<<< IMU CODE FROM ~/tuts/7-imu/example.c <<<<//

  printf("Beginning to write...\n");
  printf("\n    \t\tAccelerometer\t\t\t||");
  printf("\t\t\tGyroscope\t\t\t||");
  printf("\t\t\tSTOMP\n");

  // BEGIN INFINITE LOOP FOR SENDING DATA TO SERVER
  while (1) {
    for (i = 0; i < PACK_PTS; i++) {
      if (num_pts == init_sam) printf("INITIALIZATION PERIOD DONE\n");
      
      // Read sensor data
      accel_data = read_accel(accel, a_res);
      gyro_data = read_gyro(gyro, g_res);
      ax = accel_data.x;
      ay = accel_data.y;
      az = accel_data.z;
      gx = gyro_data.x - gyro_offset.x;
      gy = gyro_data.y - gyro_offset.y;
      gz = gyro_data.z - gyro_offset.z;

      // TODO: incorporate daguan's energy detection algo
      energy = sqrt(ax*ax + ay*ay + az*az);
      stomp = 0;
      if (energy > 3 && stompLastPlayed == 0) {
	  stomp = 1;
	  stompLastPlayed = 20;
      }

      if (num_pts%PRINT_PERIOD == 0) {
	printf("%i points\n", num_pts);
	printf("    send_idx: %d    curr_idx: %d\n", send_idx, curr_idx);
	printf("    X: %f\t Y: %f\t Z: %f\t||", accel_data.x, accel_data.y, accel_data.z);
	printf("\tX: %f\t Y: %f\t Z: %f\t||", gyro_data.x - gyro_offset.x, gyro_data.y - gyro_offset.y, gyro_data.z - gyro_offset.z, num_pts);
	printf("\t%d p: %d\n", stomp, num_pts);
      }

#ifdef DEBUG
      printf("X: %f\t Y: %f\t Z: %f\t||", accel_data.x, accel_data.y, accel_data.z);
      printf("\tX: %f\t Y: %f\t Z: %f\t||", gyro_data.x - gyro_offset.x, gyro_data.y - gyro_offset.y, gyro_data.z - gyro_offset.z, num_pts);
      printf("\t%d p: %d\n", stomp, num_pts);
#endif

      curr_it = curr_idx/sizeof(float); // all elements are 4 bytes
      buffer[curr_it] = gyro_data.x - gyro_offset.x;
      buffer[curr_it+1] = gyro_data.y - gyro_offset.y;
      buffer[curr_it+2] = gyro_data.z - gyro_offset.z;
      buffer[curr_it+3] = accel_data.x;
      buffer[curr_it+4] = accel_data.y;
      buffer[curr_it+5] = accel_data.z;
      buffer[curr_it+6] = stomp;

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
    } // packet done

    // write batch to server
    n = write(client_socket_fd, ((char*)buffer)+send_idx,
	      ((curr_idx>send_idx)?
	       (curr_idx-send_idx):
	       (BUFF_BYTES-send_idx)));

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

    /* if (asprintf(&data,"%d, %18.15lf, %18.15lf, %18.15lf, %18.15lf, %18.15lf, %18.15lf\n",stomp,ax,ay,az,gx,gy,gz) < 0) { */
    /*   error("Did not make string properly.\n"); */
    /* } */
    /* printf(data); */
    /* dprintf(client_socket_fd,data); */
    /* printf("wrote to server\n"); */
      
    if (stompLastPlayed > 0) {
      stompLastPlayed -= 1;
    }
    
    /* sleep(1/256); */
    usleep(1000000/SAMP_RATE);
  }
  
  // clean up the file descriptors
  close(client_socket_fd);
  return 0;
}
