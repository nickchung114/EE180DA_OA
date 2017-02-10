# WEEDLE is for debugging :)

#################
### RESOURCES ###
#################

# http://stackoverflow.com/questions/10861556/how-listening-to-a-socket-works
# http://stackoverflow.com/questions/489036/how-does-the-socket-api-accept-function-work
# https://www.tutorialspoint.com/python/python_networking.htm
# http://stackoverflow.com/questions/7828867/how-to-efficiently-compare-two-unordered-lists-not-sets-in-python
# http://www.slideshare.net/dabeaz/an-introduction-to-python-concurrency

#################################################
################### LIBRARIES ###################
#################################################

import socket		# Import socket module
import sys
import threading
import collections	# for Counter()
import csv
import os
import matlab.engine


#################################################
############## VARIABLE DECLARATIONS ############
#################################################

HOST = socket.gethostname()	# Get local machine name
PORT = 5000			# Reserve a port for your service
EXPECTED_USERS = 1		# Number of users

hIDtoSocket = {}
fIDtoSocket = {}

# Start MATLAB script
eng = matlab.engine.start_matlab()
eng.Script_batched()

#################################################
############# FUNCTION DECLARATIONS #############
#################################################

# returns true if the lists contain the same number of each unique element
def compare_hashable_lists(s,t):
	return collections.Counter(s) == collections.Counter(t)

# OBJECTIVE (for Thread 3)
# request note from hand IMU
# establish a listener
# block/wait
# play a sound based on (instrument, note)
def hand_main(my_id, instrument):
	print 'Starting hand_main with client ID', my_id	# WEEDLE
	# SEND AN INTERRUPT
	# ESTABLISH A LISTENER
	# WAIT
	# RECEIVE CLASSIFIED DATA (i.e. NOTE)
	# PLAY SOUND
	print 'Exiting hand_main with client ID', my_id	# WEEDLE
	
# OBJECTIVE (for Thread 2)
# poll streaming data
# gait tracking
# if a stomp is detected by the foot IMU
# 	classify location into an instrument
# 	spawn a thread (Thread 3) for corresponding hand client
def foot_main(my_id):
	print 'Starting foot_main with client ID', my_id	# WEEDLE
	currFileInd = 0; 
	while True:
		# receiving orientation (accel + gyro) and stomped
		data = fIDtoSocket[my_id].recv(10)
		print 'Data I received:', data	# WEEDLE
		
		# FIGURE OUT HOW TO GET THE SIX INTS OUT FROM DATA
		# ax is a string 
		# data = strcat(ax,',',ay,',',az,',',gx,',',gy,',',gz,',',mx,',',my,',',mz)
		
		# STORE INFORMATION INTO A .csv FILE
		fileName = ''.join(['batch',str(currFileInd),'_CalInertialAndMag.csv'])
		currFileInd = (currFileInd + 1) % 100;
		# http://stackoverflow.com/a/36906785
		dir = os.path.dirname(__file__)
		writePath = os.path.join(dir, 'csv', fileName)
		
		f = open(writePath,'w+')
		writer = csv.writer(fileName, quoting=csv.QUOTE_ALL)
		writer.writerow(data)
		f.close();
		# TELL MATLAB TO PROCESS THE INFORMATION EVERY T SECONDS
		# GET THE CURRENT POSITION
		
		# if stomped:
		#	CLASSIFY LOCATION INTO AN INSTRUMENT
		#	threading.Thread(target=hand_main,args=(my_id,instrument,)).start()
	print 'Exiting foot_main with client ID ', my_id	# WEEDLE
	
#################################################
########### MAIN THREAD (1) EXECUTION ###########
#################################################

# OBJECTIVE (for Thread 1)
# listen for new clients
# determine which imu each client is
# spawn a thread (Thread 2) for each foot client

print 'Starting main'	# WEEDLE

counter = 0
while counter < EXPECTED_USERS*2:
	s = socket.socket()	# Create a socket object
	s.bind((HOST,PORT))	# Bind to the port
	s.listen(5)		# backlog = max # of queued connections
				# Waiting for connection
	print 'Accepting...'	# WEEDLE
	c, addr = s.accept()	# Establish connection with client
	print 'Connected to', addr	# WEEDLE
	data = bytearray(c.recv(2))[0]	# Receive id from client (e.g. 0x1F)
	print 'Received', data 
	client_type = data >> 4;	# 1 -> foot; 0 -> hand
	client_ID = data & 0x0F;
	if client_type == 1:
		if fIDtoSocket.has_key(client_ID):
			print 'Already connected with foot client', client_ID
			continue	# maybe this should stop
		fIDtoSocket[client_ID] = c	# i really hope this is passed-by-value
		print 'Connected with foot client', client_ID
	elif client_type == 0:
		if hIDtoSocket.has_key(client_ID):
			print 'Already connected with hand client ', client_ID
			continue	# maybe this should stop
		hIDtoSocket[client_ID] = c
		print 'Connected with hand client', client_ID
	else:
		print "Invalid client ID"
		sys.exit()
	counter += 1

if not compare_hashable_lists(fIDtoSocket.keys(),hIDtoSocket.keys()):
	print 'Failed to connect to all foot-hand pairs'
	unmatched = [x for x in fIDtoSocket.keys() if x not in hIDtoSocket.keys()]
	print 'Listing all unmatched foot clients', unmatched
	unmatched = [x for x in hIDtoSocket.keys() if x not in fIDtoSocket.keys()]
	print 'Listing all unmatched hand clients', unmatched
	sys.exit()

for x in fIDtoSocket.keys():
	# assuming that each thread still maintains access to the two dictionaries
	threading.Thread(target=foot_main,args=(x,)).start()
	print 'Started thread for foot client', x	# WEEDLE

print 'Thread 1 execution complete'
