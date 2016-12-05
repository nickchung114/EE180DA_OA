#include <stdio.h>
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

int main() {
	data_t accel_data, gyro_data, mag_data;
	data_t gyro_offset;
	int16_t temperature;
	float a_res, g_res, m_res;
	float norm; 
	mraa_i2c_context accel, gyro, mag;
	accel_scale_t a_scale = A_SCALE_4G;
	gyro_scale_t g_scale = G_SCALE_245DPS;
	mag_scale_t m_scale = M_SCALE_2GS;

	//initialize sensors, set scale, and calculate resolution.
	accel = accel_init();
	set_accel_scale(accel, a_scale);	
	a_res = calc_accel_res(a_scale);
	
	printf("Accelerometer\n");
	
	//Read the sensor data and print them.
	float x_angle;
	//float run_xyz_avg[3]={0,0,0};
	//int counter=1;
	float cali_xyz[3]={0,0,0};
	int classify = 0, j;
	while(1) {
		accel_data = read_accel(accel, a_res);
		
		cali_xyz[0] = accel_data.x;
		//cali_xyz[1] = accel_data.y;
		cali_xyz[2] = accel_data.z;

		x_angle = cali_xyz[0]*90;
		if(cali_xyz[2] < 0)
			x_angle = 180-x_angle;
		if(x_angle < -90 || x_angle > 270)
			x_angle = 360-x_angle;
		
		for(j = 1; j <= NUM_NOTES; j++) {
			if(x_angle < -90 + j*180/NUM_NOTES) {
				classify = j;
				break;
			}
		}
		printf("Stationary XZ Angle: %f\t\t Class: %d\n",x_angle, classify);		
		/*
		printf("X: %f\tY: %f\tZ: %f\tXrun: %f\tYrun: %f\tZrun: %f\n",  
			cali_xyz[0],
		        cali_xyz[1],	
			cali_xyz[2], 
			update_run_avg(&run_xyz_avg[0],cali_xyz[0],counter),
			update_run_avg(&run_xyz_avg[1],cali_xyz[1],counter),
		      	update_run_avg(&run_xyz_avg[2],cali_xyz[2],counter));
		counter++;	
		*/
		
		usleep(100000);
	}	
	return 0;	
}

