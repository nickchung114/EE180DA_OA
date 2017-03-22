#include <math.h>
#include <mraa/i2c.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

#include "LSM9DS0.h"

#define NUM_NOTES	7
#define START_ANGLE	(-110)
#define ANGLE_RANGE	220
#define TURN_THR	400.0f
#define TURN_SAMP_RATE	50
#define MILLION		1000000
#define TIMEOUT		10
#define CLOCK_TO_MS	50
#define DEBUG

mraa_i2c_context gyro;
float g_res;

float update_run_avg(float *curr_avg, float num, int curr_avg_len) {
	*curr_avg = ((*curr_avg)*curr_avg_len + num)/(curr_avg_len+1);
	return *curr_avg;
}

void error(const char *msg) {
	perror(msg);
	exit(1);
}

void *edgeProcessing(void *argstruct){
	int turns = 0;
	int i,sign,dir;
	clock_t now, start;
	int stoppedFlag = 0;
	data_t gyro_data;
	printf("Starting edgeProcessing...\n");
	//clock_t now, start = clock(), pc = start/CLOCK_TO_S;

	start = clock();
	//while(turns < 10) {
	//while((clock() - start)/CLOCKS_PER_SEC < TIMEOUT){
	while (1) {
		for(i = 0; i < 4; i++) {
			//printf("Stage %i\n", i);
			start = clock();
			sign = 1-2*(1&i);
			gyro_data = read_gyro(gyro,g_res);
			//printf("~~~%d~~~\n",i);
			while (sign*abs(gyro_data.x)<sign*TURN_THR) {
				now = (clock() - start)/CLOCK_TO_MS;
				if(now >= 200) {
					//printf("Aborted at stage %i\n", i);
					stoppedFlag = 1;
					i = 4;
					break;
				}
				usleep(MILLION/TURN_SAMP_RATE);
				gyro_data = read_gyro(gyro,g_res);
			}
			//if (!stoppedFlag) printf("    %.1f\n", gyro_data.x);
			if(i == 0) {
				dir = gyro_data.x >= TURN_THR ? 1 : -1;
			}
		}
		if(!stoppedFlag) {
			turns += dir;
			printf("Turn Is Now %d\n",turns); 
		}
		stoppedFlag = 0;
	}
	return NULL;
}

int main(int argc, char *argv[]) {
	//set up 9DOF
	data_t gyro_offset;
	float a_res;
	float norm; 
	mraa_i2c_context accel;
	accel_scale_t a_scale = A_SCALE_4G;
	gyro_scale_t g_scale = G_SCALE_500DPS;
	data_t accel_data, gyro_data;

	//Read the sensor data and print them.
	float x_angle;
	//float run_xyz_avg[3]={0,0,0};
	//int counter=1;
	float cali_xyz[3]={0,0,0};
	int classify = 0, j;
	FILE *fp = fopen("gyro.csv", "w");

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
	pthread_t tid;
	pthread_create(&tid, NULL, edgeProcessing, NULL);
	
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
		if (x_angle < START_ANGLE) {
/* #ifdef DEBUG */
/* 			printf("* "); */
/* #endif */
			classify = 1;
		}
		else if (x_angle > START_ANGLE + ANGLE_RANGE) {
/* #ifdef DEBUG */
/* 			printf("* "); */
/* #endif */
			classify = NUM_NOTES;
		}
		else {
			for(j = 1; j <= NUM_NOTES; j++) {
				if(x_angle < -90 + j*180/NUM_NOTES) {
					classify = j;
					break;
				}
			}
/* #ifdef DEBUG */
/* 			printf("  "); */
/* #endif */
		}
#ifdef DEBUG
		//printf("GX: %.3f\tGY: %.3f\tGZ: %.3f\n", gyro_data.x,gyro_data.y,gyro_data.z);
		fprintf(fp, "%f,%f,%f\n",
				gyro_data.x-gyro_offset.x,
				gyro_data.y-gyro_offset.y,
				gyro_data.z-gyro_offset.z);
		//printf("ACCX: %.3f\tACCY: %.3f\tACCZ: %.3f\tANGLE: %f\tCLASS: %d\n",accel_data.x,accel_data.y,accel_data.z,x_angle, classify);
#endif
		//printf("%.10f\t%.10f\t%.10f\n", gyro_data.x, gyro_data.y, gyro_data.z);
		usleep(MILLION/TURN_SAMP_RATE);
	}
	fclose(fp);
	return 0;
}

