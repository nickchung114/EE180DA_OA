#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <mraa/i2c.h>
#include "LSM9DS0.h"
#include <math.h>

void error(const char *msg)
{
	perror(msg);
	exit(0);
}

/*
 * Setup IMU
 * Setup client's socket and connect with server
 * Send ID to server and wait for server to be ready
 */
int main(int argc, char *argv[])
{
	//fflush(stdout);
	
	//>>>> IMU CODE FROM ~/tuts/7-imu/EXAMPLE.C >>>>//
	// set up imu
	data_t accel_data, gyro_data;
	data_t gyro_offset;
	float a_res, g_res;
	mraa_i2c_context accel, gyro;
	accel_scale_t a_scale = A_SCALE_4G;
	gyro_scale_t g_scale = G_SCALE_245DPS;
	
	accel = accel_init();
	set_accel_scale(accel, a_scale);	
	a_res = calc_accel_res(a_scale);
	
	gyro = gyro_init();
	set_gyro_scale(gyro, g_scale);
	g_res = calc_gyro_res(g_scale);
	
	gyro_offset = calc_gyro_offset(gyro,g_res);
	printf("x: %f\ty: %f\tz: %f\n", gyro_offset.x, gyro_offset.y, gyro_offset.z);
	//<<<< IMU CODE FROM ~/tuts/7-imu/EXAMPLE.C <<<<//
	
	
	//>>>> SOCKET-CLIENT CODE FROM ~/tuts/6-TCP_Comm/client.c >>>>//
	int client_socket_fd, portno, n;
	struct sockaddr_in serv_addr;
	struct hostent *server;
	char buffer[256];
		
	// Read command line arguments, need to get the host IP address and port
	if (argc < 3) {
		fprintf(stderr,"usage %s hostname port\n", argv[0]);
		exit(0);
	}

	// Convert the arguments to the appropriate data types
	portno = atoi(argv[2]);
	
	// setup the socket (IPv4, TCP)
	client_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	
	// check if the socket was created successfully. If it wasnt, display an error and exit
	if(client_socket_fd < 0) {
		error("ERROR opening socket\n");
	}

	// check if the IP entered by the user is valid 
	server = gethostbyname(argv[1]);
	if (server == NULL) {
		fprintf(stderr,"ERROR: no such host\n");
		exit(0);
	}
	
	// clear out the serv_addr buffer
	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	// set up the socket 
	serv_addr.sin_family = AF_INET;	
	memcpy((char *)&serv_addr.sin_addr.s_addr, (char *)server->h_addr, server->h_length);
	serv_addr.sin_port = htons(portno);
	
	printf("Connecting to server...\n"); //WEEDLE
	// try to connect to the server
	if (connect(client_socket_fd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0){ 
		error("ERROR connecting");
	}
	printf("Connected with server...\n"); //WEEDLE
	//<<<< SOCKET-CLIENT CODE FROM ~/tuts/6-TCP_Comm/client.c <<<<//
	
	
	//SENDING ID TO SERVER
	uint16_t id = 0x10;
	n = write(client_socket_fd,&id,2);
	printf("Sent my ID to server.\n"); //WEEDLE

	
	//>>>> SOCKET-CLIENT CODE FROM ~/tuts/6-TCP_Comm/client.c >>>>//
	// n contains how many bytes were received by the server
	if (n < 0) {
		error("ERROR writing to socket\n");
		return 1;
	}
	//<<<< SOCKET-CLIENT CODE FROM ~/tuts/6-TCP_Comm/client.c <<<<//
	
	
	// READ A CONFIRMATION FROM SERVER
	n = read(client_socket_fd,buffer,255);
	if(n < 0) {
		error("ERROR reading from socket\n");
	}
	printf("Beginning to write...\n"); //WEEDLE

	
	// BEGIN INFINITE LOOP FOR SENDING DATA TO SERVER
	int stomp = 0, stompLastPlayed = 0;
	float energy[50];
	int i = 0;
	for(i = 49; i >= 0; i--)
	{
			energy[i] = 0;
	}
	float ax,ay,az,gx,gy,gz;
	while(1)
	{
		// Read sensor data
		accel_data = read_accel(accel, a_res);
		gyro_data = read_gyro(gyro, g_res);
		ax = accel_data.x;
		ay = accel_data.y;
		az = accel_data.z;
		gx = gyro_data.x - gyro_offset.x;
		gy = gyro_data.y - gyro_offset.y;
		gz = gyro_data.z - gyro_offset.z;
		
		int min = 1;
		for(i = 49; i > 0; i--)
		{
			energy[i] = energy[i-1];
			if(energy[i] < min)
				min = energy[i];
		}

		energy[0] = sqrt(ax*ax + ay*ay + az*az);
		stomp = 0;
		
		if(energy[0] - min > 3.2 && stompLastPlayed == 0)
		{
			stomp = 1;
			printf("Stomp with energy %f\n",energy[0]);
			stompLastPlayed = 25;
		}
		printf("%f, %f, %f, %f, %f, %f, %f\n",stomp,ax,ay,az,gx,gy,gz);
		dprintf(client_socket_fd,"%f, %f, %f, %f, %f, %f, %f\n",stomp,ax,ay,az,gx,gy,gz);
		printf("wrote to server\n");
		if(stompLastPlayed > 0)
		{
			stompLastPlayed -= 1;
		}
		sleep(1/256);
	}
	// clean up the file descriptors
	close(client_socket_fd);
	return 0;
}
