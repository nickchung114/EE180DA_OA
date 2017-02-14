#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <mraa/i2c.h>
#include "LSM9DS0.h"

// *** TODO read in config file
//#define DEBUG
#define BATCH_SIZE 256
#define MAX_FILES 1
#define MAX_NUM_FILES 20 // only makes sense if MAX_FILES is 1
#define SAMP_RATE 256
#define INIT_SEC 2
#define MAX_BUF 128 // Maximum string size

int main() {
  data_t accel_data, gyro_data, mag_data;
  data_t gyro_offset;
  float a_res, g_res, m_res;
  mraa_i2c_context accel, gyro, mag;
  //accel_scale_t a_scale = A_SCALE_4G;
  accel_scale_t a_scale = A_SCALE_8G;
  //gyro_scale_t g_scale = G_SCALE_245DPS;
  gyro_scale_t g_scale = G_SCALE_2000DPS;
  mag_scale_t m_scale = M_SCALE_2GS;

  char dir_name[MAX_BUF];
  struct stat st = {0};
  time_t t = time(NULL);
  struct tm *now = localtime(&t);
  char file_name[MAX_BUF];

  int i = 0;
  int j;
  int file_num = 0;
  int init_sam = (SAMP_RATE*INIT_SEC)+1;
  FILE *fp;
  
  strftime(dir_name, MAX_BUF, "data_%F_%H-%M-%S", now);

  if (stat(dir_name, &st) == -1) mkdir(dir_name, 0755);
  printf("Directory name: %s\n", dir_name);

  //initialize sensors, set scale, and calculate resolution.
  accel = accel_init();
  set_accel_scale(accel, a_scale);	
  a_res = calc_accel_res(a_scale);
	
  gyro = gyro_init();
  set_gyro_scale(gyro, g_scale);
  g_res = calc_gyro_res(g_scale);
	
  mag = mag_init();
  set_mag_scale(mag, m_scale);
  m_res = calc_mag_res(m_scale);

  gyro_offset = calc_gyro_offset(gyro, g_res);

#ifdef DEBUG
  printf("GYRO OFFSETS x: %f y: %f z: %f\n",
	 gyro_offset.x, gyro_offset.y, gyro_offset.z);
#endif

  //Read the sensor data and print them.
  printf("STARTING TO COLLECT DATA\n");

#ifdef DEBUG
  printf("\n\t\tAccelerometer\t\t\t||");
  printf("\t\t\tGyroscope\t\t\t||");
  printf("\t\t\tMagnetometer\n");
#endif
  
  while ((!MAX_FILES) || (MAX_FILES && file_num < MAX_NUM_FILES)) {
    sprintf(file_name, "%s/data%02d_CalInertialAndMag.csv",
	    dir_name, file_num);
    fp = fopen(file_name, "w");
    for (j = 0; j < BATCH_SIZE; j++) {
      if (i%1000 == 0) printf("%i points\n", i);
      if (i == init_sam) printf("INITIALIZATION PERIOD DONE\n");
      accel_data = read_accel(accel, a_res);
      gyro_data = read_gyro(gyro, g_res);
      mag_data = read_mag(mag, m_res);

#ifdef DEBUG
      printf("X: %f\t Y: %f\t Z: %f\t||", accel_data.x, accel_data.y, accel_data.z);
      printf("\tX: %f\t Y: %f\t Z: %f\t||", gyro_data.x - gyro_offset.x, gyro_data.y - gyro_offset.y, gyro_data.z - gyro_offset.z);
      printf("\tX: %f\t Y: %f\t Z: %f\n", mag_data.x, mag_data.y, mag_data.z);
#endif

      fprintf(fp, "%i,%f,%f,%f,%f,%f,%f,%f,%f,%f\n", j+1,
	      gyro_data.x - gyro_offset.x, gyro_data.y - gyro_offset.y, gyro_data.z - gyro_offset.z,
	      accel_data.x, accel_data.y, accel_data.z,
	      mag_data.x, mag_data.y, mag_data.z);
      i++;
    }

    usleep(1000000/SAMP_RATE);
    fclose(fp);
    file_num++;
  }
  return 0;	
}
