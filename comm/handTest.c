#include <math.h>
#include <mraa/i2c.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "LSM9DS0.h"

#define ACCEL_X_OFFSET	0 //0.075707
#define ACCEL_X_SCALE	1
#define ACCEL_Y_OFFSET	0
#define ACCEL_Y_SCALE	1
#define ACCEL_Z_OFFSET	1 
#define ACCEL_Z_SCALE	1 //0.997808

#define NUM_NOTES	7
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
	data_t accel_data, gyro_data, mag_data;
	data_t gyro_offset;
	int16_t temperature;
	float a_res, g_res, m_res;
	float norm; 
	mraa_i2c_context accel, gyro, mag;
	accel_scale_t a_scale = A_SCALE_4G;
	gyro_scale_t g_scale = G_SCALE_245DPS;
	mag_scale_t m_scale = M_SCALE_2GS;
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
	gyro_offset = calc_gyro_offset(gyro, g_res);	
	
	/*
	 * TODO: need to multi-thread this
	 */
	while (1) {
		//code for classification.
		accel_data = read_accel(accel, a_res);
		gyro_data = read_gyro(gyro, g_res);
		
		cali_xyz[0] = accel_data.x;
		//cali_xyz[1] = accel_data.y;
		cali_xyz[2] = accel_data.z;

		x_angle = atan2(cali_xyz[2],cali_xyz[0]);
		x_angle = -(x_angle*180/M_PI-90) + 5.5;
		x_angle -= x_angle > 180 ? 360 : 0;
		if (x_angle < START_ANGLE || x_angle > START_ANGLE + ANGLE_RANGE) {
			printf("out of bounds\n");
			usleep(1000000/10);
			continue;
		}
		for(j = 1; j <= NUM_NOTES; j++) {
			if(x_angle <= START_ANGLE + j*ANGLE_RANGE/NUM_NOTES) {
				classify = j;
				break;
			}
		}
		printf("GX: %.3f\tGY: %.3f\tGZ: %.3f\n", gyro_data.x,gyro_data.y,gyro_data.z);
		//printf("ACCX: %.3f\tACCY: %.3f\tACCZ: %.3f\tANGLE: %f\tCLASS: %d\n",accel_data.x,accel_data.y,accel_data.z,x_angle, classify);
		usleep(1000000/10);
	}
	return 0;
}

