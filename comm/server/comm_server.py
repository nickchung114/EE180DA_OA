# WEEDLE is for debugging :)

#####################################################################
############################# RESOURCES #############################
#####################################################################

# http://stackoverflow.com/questions/10861556/how-listening-to-a-socket-works
# http://stackoverflow.com/questions/489036/how-does-the-socket-api-accept-function-work
# https://www.tutorialspoint.com/python/python_networking.htm
# http://stackoverflow.com/questions/7828867/how-to-efficiently-compare-two-unordered-lists-not-sets-in-python
# http://www.slideshare.net/dabeaz/an-introduction-to-python-concurrency
# http://stackoverflow.com/a/36906785
# http://stackoverflow.com/questions/15268088/printf-format-float-with-padding
# http://stackoverflow.com/a/4071407
# http://stackoverflow.com/a/386177
# http://stackoverflow.com/questions/1483429/how-to-print-an-error-in-python
# http://stackoverflow.com/questions/2967194/open-in-python-does-not-create-a-file-if-it-doesnt-exist
# http://stackoverflow.com/questions/339007/nicest-way-to-pad-zeroes-to-string
# http://stackoverflow.com/questions/9271464/what-does-the-file-variable-mean-do/9271617#9271617
# https://docs.python.org/2/tutorial/inputoutput.html#reading-and-writing-files
#####################################################################
############################# LIBRARIES #############################
#####################################################################

import socket		# Import socket module
import sys
import threading	# multithreading for multiple users
import collections	# for Counter()
import csv		# writing to .csv's
import os		# relative file paths
import subprocess 
import winsound
import re		# regex expressions

#####################################################################
######################## VARIABLE DECLARATIONS ######################
#####################################################################

HOST = socket.gethostname()	# Get local machine name
PORT = 5000			# Reserve a port for your service
EXPECTED_USERS = 1		# Number of users
FOOT_MSG_PAD = 18		# 15 + 3 (negative & zero & decimal)
FOOT_MSG_LEN = FOOT_MSG_PAD*6 + 1 + 2*6 + 1	# The last byte is newline
MAX_NUM_SAMPLES = 256

hIDtoSocket = {}
fIDtoSocket = {}
dict_rm_ws = {'\\n' : '', '\n' : '', '\r\n' : '', ' ' : ''}

dir = os.path.abspath(os.path.dirname('__file__'))
   
#####################################################################
####################### FUNCTION DECLARATIONS #######################
#####################################################################

# http://stackoverflow.com/questions/6116978/python-replace-multiple-strings
# https://gist.github.com/bgusach/a967e0587d6e01e889fd1d776c5f3729#file-multireplace-py-L5
def multireplace(string, replacements):
	"""
	Given a string and a replacement map, it returns the replaced string.
	:param str string: string to execute replacements on
	:param dict replacements: replacement dictionary {value to find: value to replace}
	:rtype: str
	"""
	# Place longer ones first to keep shorter substrings from matching where the longer ones should take place
	# For instance given the replacements {'ab': 'AB', 'abc': 'ABC'} against the string 'hey abc', it should produce
	# 'hey ABC' and not 'hey ABc'
	substrs = sorted(replacements, key=len, reverse=True)

	# Create a big OR regex that matches any of the substrings to replace
	regexp = re.compile('|'.join(map(re.escape, substrs)))

	# For each match, look up the new string in the replacements
	return regexp.sub(lambda match: replacements[match.group(0)], string)

# returns true if the lists contain the same number of each unique element
def compare_hashable_lists(s,t):
	return collections.Counter(s) == collections.Counter(t)

# OBJECTIVE (for Thread 3)
# request note from hand IMU
# establish a listener
# block/wait
# play a sound based on (instrument, note)
def hand_main(my_id, instrument, Note_old) # will need to add a variable Note_old to foot_main.
	print 'Starting hand_main with client ID', my_id	# WEEDLE
	# SEND AN INTERRUPT
	hIDtoSocket[my_id].send('ahem')
	# ESTABLISH A LISTENER
	# WAIT
	# RECEIVE CLASSIFIED DATA (i.e. NOTE)
	data1 = hIDtoSocket[my_id].recv(1)
	#data2 = data1.split(,)
	#data3 = [int(e) for e in data2]
	#pitch = data3[1]
	pitch = int(data1)
	# PLAY SOUND
	NoteArray = ['zero','Bassoon_c.wav','Bassoon_d.wav','Bassoon_e.wav','Bassoon_f.wav','Bassoon_g.wav',
                'French_c.wav','French_d.wav','French_e.wav','French_f.wav','French_g.wav',
                'Guitar_C.wav','Guitar_d.wav','Guitar_e.wav','Guitar_f.wav','Guitar_g.wav',
                'Violin_c.wav','Violin_d.wav','Violin_e.wav','Violin_f.wav','Violin_g.wav']
	if Note_old is < 11 or Note_old > 15 #Stop playing the previous note if it is a wind or instrument or violin.
		winsound.PlaySound(NoteArray[Note_old], (winsound.SND_FILENAME  winsound.SND_PURGE))
	Note = pitch + 5*instrument
	winsound.PlaySound(NoteArray[Note], (winsound.SND_FILENAME  winsound.SND_ASYNC))
	conn.sendall(bytes('Word', 'UTF-8'))
	Note_old = Note
	print 'Exiting hand_main with client ID', my_id	# WEEDLE
	
# OBJECTIVE (for Thread 2)
# poll streaming data
# gait tracking
# if a stomp is detected by the foot IMU
# 	classify location into an instrument
# 	spawn a thread (Thread 3) for corresponding hand client
def foot_main(my_id):
	print 'Starting foot_main with client ID', my_id	# WEEDLE
	fIDtoSocket[my_id].send("hello")	# let foot client know we're ready
	
	counter = 0
	currFileInd = 0
	fileName = ''.join(['batch',chr(my_id+65),'{0:02d}'.format(currFileInd),'_CalInertialAndMag.csv'])
	writePath = os.path.join(dir, 'csv', fileName)
	print 'Writing to',writePath
	f = open(writePath,'a+')
	#writer = csv.writer(f, quoting=csv.QUOTE_ALL)
	writer = csv.writer(f)
	
	test_counter = 0
	NUM_ITERATIONS_FOR_TESTING = MAX_NUM_SAMPLES*2
	Note_old = 0
	
	p = subprocess.Popen("testingpy2mat.bat", shell=True)
	#testingpy2mat.bat file should include: "matlab" -nodisplay -nosplash -nodesktop -r "run('[Path to script]\Script_Batched.m');exit;"


	while True:
		# receiving orientation (accel + gyro) and stomped
		data = fIDtoSocket[my_id].recv(FOOT_MSG_LEN)
		# print 'Data I received:', data	# WEEDLE
		test_counter += 1
		if test_counter > NUM_ITERATIONS_FOR_TESTING:
			print 'Finished',str(NUM_ITERATIONS_FOR_TESTING),'iterations'
			break
		#"""
		try:
			#data = [float(x.strip(' \n\r\n')) for x in data.split(',')]
			data = [float(multireplace(x,dict_rm_ws)) for x in data.split(',')]
		except Exception as e: 
			print e
			print [repr(x.strip(' \n\r\n')) for x in data.split(',')]
			print '\n\n\n\n~~~'
			break
		#print [repr(d) for d in data]	# WEEDLE
		#print data
		#"""
		
		
		# STORE INFORMATION INTO A .csv FILE
		if counter >= MAX_NUM_SAMPLES:
			print 'Closing',writePath
			f.close()
			# originally str(currFileInd)
			currFileInd = (currFileInd + 1) % 100
			fileName = ''.join(['batch',chr(my_id+65),'{0:02d}'.format(currFileInd),'_CalInertialAndMag.csv'])
			writePath = os.path.join(dir, 'csv', fileName)
			print 'Writing to',writePath
			f = open(writePath,'w+b')
			#writer = csv.writer(f, quoting=csv.QUOTE_ALL)
			writer = csv.writer(f)
			counter = 0
		writer.writerow([counter] + data[1:] + [0,0,0])
		counter += 1
		print counter
		# GET THE CURRENT POSITION
		ifile = open(os.path.join(dir,'csv','currentPosition.csv'), "rb")
		reader = csv.reader(ifile)
		for row in reader:
			currentPosition = row
		for i in xrange(len(currentPosition)):
			currentPosition[i] = float(currentPosition[i])
		ifile.close()
		
		Note_old = 0
		if stomped:
		#	CLASSIFY LOCATION INTO AN INSTRUMENT'
			instrument = 0
			threading.Thread(target=hand_main,args=(my_id,instrument,Note_old,)).start()
	print 'Closing',writePath
	f.close();
	print 'Exiting foot_main with client ID ', my_id	# WEEDLE
	
#####################################################################
##################### MAIN THREAD (1) EXECUTION #####################
#####################################################################

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
	# print 'Started thread for foot client', x	# WEEDLE

print 'Thread 1 execution complete'
