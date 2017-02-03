# WEEDLE is for debugging :)

#################
### RESOURCES ###
#################

# http://stackoverflow.com/questions/10861556/how-listening-to-a-socket-works
# http://stackoverflow.com/questions/489036/how-does-the-socket-api-accept-function-work
# https://www.tutorialspoint.com/python/python_networking.htm
# http://stackoverflow.com/questions/7828867/how-to-efficiently-compare-two-unordered-lists-not-sets-in-python
# http://www.slideshare.net/dabeaz/an-introduction-to-python-concurrency

#################
### LIBRARIES ###
#################

import socket		# Import socket module
import sys
import threading
import collections	# for Counter()

######################################
### CONSTANT VARIABLE DECLARATIONS ###
######################################

HOST = socket.gethostname()	# Get local machine name
PORT = 8000			# Reserve a port for your service
EXPECTED_USERS = 1		# Number of users

#####################################
### MUTABLE VARIABLE DECLARATIONS ###
#####################################

hIDtoSocket = {}
fIDtoSocket = {}

#############################
### FUNCTION DECLARATIONS ###
#############################

# returns true if the lists contain the same number of each unique element
def compare_hashable_lists(s,t):
	return collections.Counter(s) == collections.Counter(t)

# OBJECTIVE (for Thread 3)
# request note from hand IMU
# establish a listener
# block/wait
# play a sound based on (instrument, note)
def hand_main(my_id, instrument):
	print 'Starting hand_main with client ID ', my_id	# WEEDLE
	# SEND AN INTERRUPT
	# ESTABLISH A LISTENER
	# WAIT
	# RECEIVE CLASSIFIED DATA (i.e. NOTE)
	# PLAY SOUND
	print 'Exiting hand_main with client ID ', my_id	# WEEDLE
	
# OBJECTIVE (for Thread 2)
# poll streaming data
# gait tracking
# if a stomp is detected by the foot IMU
# 	classify location into an instrument
# 	spawn a thread (Thread 3) for corresponding hand client
def foot_main(my_id):
	print 'Starting foot_main with client ID ', my_id	# WEEDLE
	while True:
		# receiving orientation (accel + gyro) and stomped
		data = fIDtoSocket[my_id].recv(4096)
		print 'I received stuff!'	# WEEDLE
		
		# FIGURE OUT HOW TO GET THE SEVEN INTS OUT FROM DATA
		
		# STORE INFORMATION INTO A .csv FILE
		# GET THE CURRENT POSITION FROM A FILE
		
		# if stomped:
		#	CLASSIFY LOCATION INTO AN INSTRUMENT
		#	threading.Thread(target=hand_main,args=(my_id,instrument,)).start()
	print 'Exiting foot_main with client ID ', my_id	# WEEDLE
	
#################################
### MAIN THREAD (1) EXECUTION ###
#################################

# OBJECTIVE (for Thread 1)
# listen for new clients
# determine which imu each client is
# spawn a thread (Thread 2) for each foot client

print 'Starting main'	# WEEDLE

counter = 0
while counter < EXPECTED_USERS*2:
	s = socket.socket(socket.AF_INET,socket.SOCK_STREAM)	# Create a socket object
	s.bind((HOST,PORT))	# Bind to the port
	print 'Starting listen'
	s.listen(1)		# backlog = max # of queued connections; Waiting for connection
	print 'Starting listen'
	c, addr = s.accept()	# Establish connection with client
	print 'Connected to ', addr	# WEEDLE
	data = bytearray(c.recv(2))[0]	# Receive id from client (e.g. 0x1F)
	print 'Connected with type/ID ', data	#WEEDLE

	client_type = data >> 8;	# 1 -> foot; 0 -> hand
	client_ID = data & 0x0F;
	if client_type == 1:
		if fIDtoSocket.has_key(client_ID):	# maybe this should stop
			print 'Already connected with foot client ', client_ID
			continue
		fIDtoSocket[client_ID] = c	# i really hope this is passed-by-value
		print "Connected with foot client ", client_ID
	elif client_type == 0:
		if hIDtoSocket.has_key(client_ID):	# maybe this should stop
			print 'Already connected with hand client ', client_ID
			continue
		hIDtoSocket[client_ID] = c
		print "Connected with hand client ", client_ID
	else:
		print "Invalid client ID"
		sys.exit()
	counter += 1

if not compare_hashable_lists(fIDtoSocket.keys(),hIDtoSocket.keys()):
	print 'Failed to connect to all foot-hand pairs'
	unmatched = [x for x in fIDtoSocket.keys() if x not in hIDtoSocket.keys()]
	print 'Listing all unmatched foot clients ', unmatched
	unmatched = [x for x in hIDtoSocket.keys() if x not in fIDtoSocket.keys()]
	print 'Listing all unmatched hand clients ', unmatched
	sys.exit()

for x in fIDtoSocket.keys():
	# assuming that each thread still maintains access to the two dictionaries
	threading.Thread(target=foot_main,args=(x,)).start()
	print 'Started thread for foot client ', x	# WEEDLE

print 'Thread 1 execution complete'