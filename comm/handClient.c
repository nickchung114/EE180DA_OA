#include <math.h>
#include <mraa/i2c.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "LSM9DS0.h"

#define ACCEL_X_OFFSET	0 //0.075707
#define ACCEL_X_SCALE	1
#define ACCEL_Y_OFFSET	0
#define ACCEL_Y_SCALE	1
#define ACCEL_Z_OFFSET	1 
#define ACCEL_Z_SCALE	1 //0.997808

#define NUM_NOTES	5
#define START_ANGLE	(-110)
#define ANGLE_RANGE	220

int turns = 0;

float update_run_avg(float *curr_avg, float num, int curr_avg_len) {
	*curr_avg = ((*curr_avg)*curr_avg_len + num)/(curr_avg_len+1);
	return *curr_avg;
}

void error(const char *msg) {
	perror(msg);
	exit(1);
}

int main(int argc, char *argv[]) {
	//set up 9DOF
	//data_t accel_data, gyro_data, mag_data;
	data_t accel_data, gyro_data;
	data_t gyro_offset;
	//int16_t temperature;
	//float a_res, g_res, m_res;
	float a_res, g_res;
	float norm; 
	mraa_i2c_context accel, gyro, mag;
	accel_scale_t a_scale = A_SCALE_4G;
	gyro_scale_t g_scale = G_SCALE_245DPS;
	//mag_scale_t m_scale = M_SCALE_2GS;

	//Read the sensor data and print them.
	float x_angle;
	//float run_xyz_avg[3]={0,0,0};
	//int counter=1;
	float cali_xyz[3]={0,0,0};
	int classify = 0, j;

	accel = accel_init();
	set_accel_scale(accel, a_scale);	
	a_res = calc_accel_res(a_scale);

	gyro = gyro_init();
	set_gyro_scale(gyro, g_scale);
	g_res = calc_gyro_res(g_scale);
	gyro_offset = calc_gyro_offset(gyro,g_res);

	//Set up server connection
	int client_socket_fd, portno, n, sent, tot;
	struct sockaddr_in serv_addr;
	struct hostent *server;
	unsigned char dummy;
		
	// Read command line arguments, need to get the host IP address and port
	if (argc < 3) {
		fprintf(stderr,"usage %s hostname port\n", argv[0]);
		exit(1);
	}

	// Convert the arguments to the appropriate data types
	portno = atoi(argv[2]);

	/* setup the socket
	 *	AF_INET 	->	IPv4
	 *	SOCK_STREAM 	->	TCP
	 */
	client_socket_fd = socket(AF_INET, SOCK_STREAM, 0);

	// check if the socket was created successfully. If it wasnt, display an error and exit
	if(client_socket_fd < 0) {
		error("ERROR opening socket\n");
	}

	// check if the IP entered by the user is valid 
	server = gethostbyname(argv[1]);
	if (server == NULL) {
		fprintf(stderr,"ERROR, no such host\n");
		exit(1);
	}

	// clear our the serv_addr buffer
	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	// set up the socket 
	serv_addr.sin_family = AF_INET;	
	memcpy((char *)&serv_addr.sin_addr.s_addr, (char *)server->h_addr, server->h_length);
	serv_addr.sin_port = htons(portno);

	// try to connect to the server
	if (connect(client_socket_fd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0){ 
		error("ERROR connecting\n");
	}

	/*
	// get user input
	printf("Please enter the message: ");
	memset(buffer, 0 ,256);
	fgets(buffer, 255, stdin); // the part that actually gets the user input

	// send user input to the server
	n = write(client_socket_fd,buffer,strlen(buffer));
	*/
	uint16_t id = 0x0000; // (client_type, client_id)
	sent = 0;
	while (sent < 2) {
		n = write(client_socket_fd,&id,2);

		// n contains how many bytes were received by the server
		// if n is less than 0, then there was an error
		if (n < 0) {
			error("ERROR writing to socket\n");
		}
		sent += n;
	}
	printf("Sent my ID to server. Now waiting...\n");

	//Wait for server pings.
	//int continue = 1;
	while (1) {
		tot = 0;
		while (tot < 1) {
			n = read(client_socket_fd, &dummy, 1); // hang until received ping
			if (n < 0) {
			error("ERROR reading from socket\n");
			}
			tot += n;
		}
		printf("Received ping\n");
			
		//code for classification.
		accel_data = read_accel(accel, a_res);
			
		cali_xyz[0] = accel_data.x;
		//cali_xyz[1] = accel_data.y;
		cali_xyz[2] = accel_data.z;

		/*
		x_angle = cali_xyz[0]*90;
		if(cali_xyz[2] < 0)
		x_angle = 180-x_angle;
		if(x_angle < -90 || x_angle > 270)
		x_angle = 360-x_angle;
		*/
		x_angle = atan2(cali_xyz[2],cali_xyz[0]);
		x_angle = -(x_angle*180/M_PI-90) + 5.5;
		x_angle -= x_angle > 180 ? 360 : 0;
		if (x_angle < START_ANGLE) {
			printf("* ");
			classify = 1;
		}
		else if (x_angle > START_ANGLE + ANGLE_RANGE) {
			printf("* ");
			classify = NUM_NOTES;
		}
		else {
			for(j = 1; j <= NUM_NOTES; j++) {
				if(x_angle < -90 + j*180/NUM_NOTES) {
					classify = j;
					break;
				}
			}
			printf("  ");
		}
		printf("ACCX: %.3f, ACCY: %.3f, ACCZ: %.3f, ANGLE: %f,CLASS: %d\n",accel_data.x,accel_data.y,accel_data.z,x_angle, classify);

		//dprintf(client_socket_fd,"%f,%d\n",x_angle, classify); //Stationary XZ Angle, classification

		sent = 0;
		//dprintf(client_socket_fd,"%d",classify);
		while (sent < 4) {
			n = write(client_socket_fd, &classify, 4);

			if (n < 0) {
				error("ERROR writing to socket");
			}
			sent += n;
		}
	}
	// clean up the file descriptors
	close(client_socket_fd);
	return 0;
}
