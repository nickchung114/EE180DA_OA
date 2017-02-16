#################################################
############## VARIABLE DECLARATIONS ############
#################################################



# Start MATLAB script
# from mlabwrap import mlab
# mlab.path(mlab.path(), 'C:\Users\gabri\OneDrive\Documents\180DA\Gait-Tracking-With-x-IMU-master\Gait Tracking With x-IMU')
# mlab.Script_ourscript_2

# http://stackoverflow.com/questions/13311415/run-a-matlab-script-from-python-pass-args
#scrpt = 'run("C:\Users\gabri\OneDrive\Documents\180DA\Gait-Tracking-With-x-IMU-master\Script_ourscript_1.m")'
#mtlb = '"C:\Program Files\MATLAB\R2013a\\bin\matlab.exe"'    


import subprocess 
batpath = r"../comm/server/" #r"C:\\Users\\gabri\\OneDrive\\Documents\\180DA\\Gait-Tracking-With-x-IMU-master\\"
#p = subprocess.Popen("testingpy2mat.bat", cwd=batpath, shell=True)
p = subprocess.Popen("./testingpy2mat.sh", cwd=batpath, shell=True)

#p = subprocess.Popen("testingpy2mat.bat", cwd=r"C:\\Users\\gabri\\Documents\\180DA\\", shell=True)

#p = subprocess.Popen([mtlb,  '-nodisplay', '-nosplash', '-nodesktop','-r', scrpt])
#p = subprocess.Popen("C:\Users\gabri\Documents\180DA\\testingpy2mat.bat",shell=True)
#f = open('C:\\Users\\gabri\\Documents\\180DA\\testingpy2mat.bat')
#print f.readline()
#This works in cmd "C:\Program Files\MATLAB\R2013a\bin\matlab.exe" -nodisplay -nosplash -nodesktop -r "run('C:\Users\gabri\OneDrive\Documents\180DA\Gait-Tracking-With-x-IMU-master\Script_ourscript_1.m');exit;"
