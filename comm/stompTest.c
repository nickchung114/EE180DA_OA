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

#define ACCEL_X_OFFSET	0 //0.075707
#define ACCEL_X_SCALE	1
#define ACCEL_Y_OFFSET	0
#define ACCEL_Y_SCALE	1
#define ACCEL_Z_OFFSET	1 
#define ACCEL_Z_SCALE	1//0.997808

#define NUM_NOTES	5


float update_run_avg(float *curr_avg, float num, int curr_avg_len) {
	*curr_avg = ((*curr_avg)*curr_avg_len + num)/(curr_avg_len+1);
	return *curr_avg;
}

void error(const char *msg)
{
	perror(msg);
	exit(0);
}

int main(int argc, char *argv[])
{
	//set up 9DOF
	data_t accel_data, gyro_data, mag_data;
	data_t gyro_offset;
	int16_t temperature;
	float a_res, g_res, m_res;
	float norm; 
	mraa_i2c_context accel, gyro, mag;
	accel_scale_t a_scale = A_SCALE_4G;
	gyro_scale_t g_scale = G_SCALE_245DPS;
	mag_scale_t m_scale = M_SCALE_2GS;
	float x_angle;
	//int counter=1;
	float cali_xyz[3]={0,0,0};
	
	FILE *fp = fopen("test.csv", "w+");

	accel = accel_init();
	set_accel_scale(accel, a_scale);	
	a_res = calc_accel_res(a_scale);
	gyro = gyro_init();
	set_gyro_scale(gyro, g_scale);
	g_res = calc_gyro_res(g_scale);
	gyro_offset = calc_gyro_offset(gyro, g_res);
	printf("Ready.\n");
	int stomp = 0;
	int stompLastPlayed = 0;
	float energy[10];
	int i = 0;
	for(i = 9; i >= 0; i--)
	{
			energy[i] = 0;
	}
	while(1)
	{
		stomp = 0;
		//Read sensor data
		accel_data = read_accel(accel, a_res);
		gyro_data = read_gyro(gyro, g_res); //How do you use gyro_offset?
		int hasDip = 0;
		for(i = 9; i > 0; i--)
		{
			energy[i] = energy[i-1];
			if(energy[i] < 0.55)
				hasDip = 1;
		}
		energy[0] = sqrt(accel_data.x*accel_data.x + accel_data.y*accel_data.y + accel_data.z*accel_data.z);
		if(energy[0] >2 && hasDip && stompLastPlayed == 0)
		{
			stomp = 1;
			printf("Stomp with energy %f\n",energy[0]);
			stompLastPlayed = 20;
		}
		fprintf(fp, "%f, %f, %f, %f, %f, %f, %f\n",energy[0], accel_data.x, accel_data.y, accel_data.z, gyro_data.x, gyro_data.y, gyro_data.z);
		if(stompLastPlayed > 0)
		{
			stompLastPlayed -= 1;
		}
		usleep(20000);
		
	}
	// clean up the file descriptors
	return 0;
}