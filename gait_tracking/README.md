# GAIT-TRACKING LOCALIZATION
`Quaternions`, `AHRS.m`, `SixDofAnimation.m`, and `ximu_matlab_library` are adapted from X-IMU's Gait-tracking code

`split_files_N` directories are split versions of `Datasets/straightLine_CalInertialAndMag.csv`, where batches are size N

`straightLine_truePos.mat` is the position output from the original X-IMU processing of the `straightLine` dataset

`straightLine_trueXXX_RT.mat` is the XXX output from the modified real-time version of the localization algo for the `straightLine` dataset

`Script_batched.m` is the main real-time Matlab gait-tracking algo

## gather_data
A small framework to demo the localization in real-time

To use:
* Run `gather_client` on the Edison 9-DOF
* Run `gather_server` on the laptop, which receives data from `gather_client`
* `gather_server` writes data out to filesystem interface w/ Matlab
* Matlab calculates position and sends to Plotly for real-time visualization

`gather.c` is a less-powerful program that runs on the edison and simply writes out data to a series of files

## Datasets
`straightLine`, `stairsAndCorridor`, and `spiralStairs` are adapted from X-IMU

`*.BIN` is smth from the original X-IMU datasets. idk what they do

`XXX00_CalInertialAndMag.csv` is just `XXX_CalInertialAndMag.csv` without the column headers (since Matlab RT gait_tracking doesn't want them)
