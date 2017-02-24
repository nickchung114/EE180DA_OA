import csv
import os
import numpy as np
#read from .csv 
#checking if file is there and if it isn't, ____ (something about waiting for it for the acc/gyro)
#and set variable equal to it 
 
#ifile = open('..\..\gait_tracking\currentPosition.csv', "rb")
#reader = csv.reader(ifile)
 
# for row in reader:
# 	currentPosition = row

# for i in xrange(len(currentPosition)):
# 	currentPosition[i] = int(currentPosition[i])

# ifile.close()


#as for acc/gyro data: have to get each line of data
ifile = open('csv\currentPosition.csv', "rb")
# ifile = open(os.path.join(dir,'csv','currentPosition.csv'), "rb")
reader = csv.reader(ifile)
for row in reader:
	currentPosition = row
for i in xrange(len(currentPosition)):
	currentPosition[i] = float(currentPosition[i])
ifile.close()

print currentPosition