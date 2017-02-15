#include <stdio.h>
#include <mraa/i2c.h>
#include "LSM9DS0.h"
#include <math.h>

int main(int argc, char *argv[])
{
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
	
	gyro_offset = calc_gyro_offset(gyro, g_res);

	while(1)
	{
		accel_data = read_accel(accel, a_res);
		gyro_data = read_gyro(gyro, g_res);
		ax = accel_data.x;
		ay = accel_data.y;
		az = accel_data.z;
		gx = gyro_data.x - gyro_offset.x;
		gy = gyro_data.y - gyro_offset.y;
		gz = gyro_data.z - gyro_offset.z;
		
		printf("%f, %f, %f, %f, %f, %f\n", ax, ay, az, gx, gy, gz);
	}
	return 0;
}
