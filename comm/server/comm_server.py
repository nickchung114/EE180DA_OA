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

import collections	# for Counter()
import csv		# writing to .csv's
import os		# relative file paths
import re		# regex expressions
import socket		# Import socket module
import struct
import subprocess
import sys
import threading	# multithreading for multiple users

if not(sys.platform == "linux" or sys.platform == "linux2"):
        import winsound
else:
        import pyaudio
        import wave

        CHUNK = 1024
        
#####################################################################
######################## VARIABLE DECLARATIONS ######################
#####################################################################

#HOST = socket.gethostname()	# Get local machine name
HOST = ''
# TODO make this a command-line argument
PORT = 5000			# Reserve a port for your service

# TODO need a separate thread waiting for new connections rather than waiting for EXPECTED_USERS
EXPECTED_USERS = 1		# Number of users
#FOOT_MSG_PAD = 18		# 15 + 3 (negative & zero & decimal)
STOMP_LEN = 1			# unsigned int
#FOOT_MSG_LEN = STOMP_LEN + FOOT_MSG_PAD*6 + 2*6 + 1	# The last byte is newline
FOOT_MSG_LEN = STOMP_LEN + 6 # stomp + (dimensions * |{accel,gyro}|
FOOT_MSG_BYTES = FOOT_MSG_LEN*4 # everything is 4 bytes
MAX_NUM_SAMPLES = 256 # ie: BATCH_PTS
MAX_FILES = 1 # flag that says whether or not to limit num of files (for testing)
MAX_NUM_FILES = 20
NUM_ITERATIONS_FOR_TESTING = MAX_NUM_SAMPLES*MAX_NUM_FILES
FILE_MAX = 100

# TODO maybe want smth cleaner? not critical though
hIDtoSocket = {}
fIDtoSocket = {}
dict_rm_ws = {'\\n' : '', '\n' : '', '\r\n' : '', ' ' : ''}

dir = os.path.abspath(os.path.dirname('__file__'))
GAIT_RELPATH = '../../gait_tracking'
GAIT_PATH = os.path.abspath(GAIT_RELPATH)
GAIT_DATA = 'data'

num_stomps = 0

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
def hand_main(my_id, instrument, Note_old): # will need to add a variable Note_old to foot_main.
	print 'Starting hand_main with client ID', my_id, 'from stomp', num_stomps
        sock = hIDtoSocket[my_id]

	# SEND AN INTERRUPT
        sock.send(struct.pack('<B',1))

	# ESTABLISH A LISTENER
	# WAIT
	# RECEIVE CLASSIFIED DATA (i.e. NOTE)
	data1 = ''
        while len(data1) < 4:
                data1 += sock.recv(4-len(data1))
	#data2 = data1.split(,)
	#data3 = [int(e) for e in data2]
	#pitch = data3[1]
	pitch = struct.unpack('<i', data1)[0]
	# PLAY SOUND
	NoteArray = ['zero','Bassoon_C.wav','Bassoon_D.wav','Bassoon_E.wav','Bassoon_F.wav','Bassoon_G.wav',
                'French_C.wav','French_D.wav','French_E.wav','French_F.wav','French_G.wav',
                'Guitar_C.wav','Guitar_D.wav','Guitar_E.wav','Guitar_F.wav','Guitar_G.wav',
                'Violin_C.wav','Violin_D.wav','Violin_E.wav','Violin_F.wav','Violin_G.wav']
        Note = pitch + 5*instrument
        # Stop playing the previous note if it is a wind or instrument or violin.
        if not(sys.platform == "linux" or sys.platform == "linux2"):
                if Note_old < 11 or Note_old > 15:
                        winsound.PlaySound(NoteArray[Note_old], (winsound.SND_FILENAME | winsound.SND_PURGE))
	        winsound.PlaySound(NoteArray[Note], (winsound.SND_FILENAME | winsound.SND_ASYNC))
	# conn.sendall(bytes('Word', 'UTF-8'))
        else:
                # Refer to: http://people.csail.mit.edu/hubert/pyaudio/
                p = pyaudio.PyAudio()
                wf = wave.open(os.path.join('wav',NoteArray[Note]), 'rb')
                stream = p.open(format=p.get_format_from_width(wf.getsampwidth()),
                                channels=wf.getnchannels(),
                                rate=wf.getframerate(),
                                output=True)
                data = wf.readframes(CHUNK)
                while data != '':
                        stream.write(data)
                        data = wf.readframes(CHUNK)
                        
                stream.stop_stream()
                stream.close()
                p.terminate()
                
	Note_old = Note
        
	print 'Exiting  hand_main with client ID', my_id, 'from stomp', num_stomps
	
# OBJECTIVE (for Thread 2)
# poll streaming data
# gait tracking (ie: write out to filesystem)
# if a stomp is detected by the foot IMU
# 	classify location into an instrument
# 	spawn a thread (Thread 3) for corresponding hand client
def foot_main(my_id):
	print 'Starting foot_main with client ID', my_id
        sock = fIDtoSocket[my_id]
	#fIDtoSocket[my_id].send("hello")	# let foot client know we're ready
        sock.send(struct.pack('<B',1))
	
	counter = MAX_NUM_SAMPLES
	currFileInd = -1
	Note_old = 0
	
	test_counter = 0
	
        # TODO get this working more elegantly. for now just have matlab running at the same time for linux
        # if sys.platform == "linux" or sys.platform == "linux2":
        #         #p = subprocess.Popen(['./testingpy2mat.sh'])
	# else: # windows
        if not(sys.platform == "linux" or sys.platform == "linux2"): # windows
		p = subprocess.Popen("testingpy2mat.bat", shell=True)
		#testingpy2mat.bat file should include: "matlab" -nodisplay -nosplash -nodesktop -r "run('[Path to script]\Script_Batched.m');exit;"

	while True:
		# receiving orientation (accel + gyro) and stomped
		data = ''
		while len(data) < FOOT_MSG_BYTES:
			data += sock.recv(FOOT_MSG_BYTES-len(data))
		# print 'Data I received:', data
		# rawdata = data
		
		test_counter += 1
		if test_counter > NUM_ITERATIONS_FOR_TESTING and MAX_FILES:
			print 'Finished',str(NUM_ITERATIONS_FOR_TESTING),'iterations'
			break

                # UNCOMMENT the following if sending data as a string
		# try:
		# 	#data = [float(x.strip(' \n\r\n')) for x in data.split(',')]
		# 	data = [float(multireplace(x,dict_rm_ws)) for x in data.split(',')]
		# except Exception as e: 
		# 	print e
		# 	print [repr(x.strip(' \n\r\n')) for x in data.split(',')]
		# 	print '\n\n\n\n~~~'
		# 	break
                
		#print [repr(d) for d in data]
		#print data
		
		# STORE INFORMATION INTO A .csv FILE
		if counter >= MAX_NUM_SAMPLES:
                        # TODO maybe want to do this in a clearer way? not critical though
                        if currFileInd >= 0:
				print 'Closing',writePath
				f.close()
			# originally str(currFileInd)
			currFileInd = (currFileInd + 1) % FILE_MAX
			#fileName = ''.join(['batch',chr(my_id+65),'{0:02d}'.format(currFileInd),'_CalInertialAndMag.csv'])
                        fileName = 'id{0:02d}batch{1:02d}_CalInertialAndMag.csv'.format(my_id, currFileInd)
                        
			#writePath = os.path.join(dir, 'csv', fileName)
                        writePath = os.path.join(GAIT_PATH, GAIT_DATA, fileName)
                        
			if os.path.isfile(writePath):
				# processing occurs slower than read
				print "ERROR:", writePath, "has not yet been processed by MATLAB."
                                # Filesystem ring buffer full! panic!
				break
			print 'Writing to:',writePath
                        # TODO this should be done using "with open..."
			f = open(writePath,'w+b')
			#writer = csv.writer(f, quoting=csv.QUOTE_ALL)
			writer = csv.writer(f)
			counter = 0
                (gyroX, gyroY, gyroZ, accelX, accelY, accelZ, stomp) = struct.unpack('<fffffff', data)
		#writer.writerow([counter] + data[1:] + [0,0,0])
                writer.writerow([counter, gyroX, gyroY, gyroZ, accelX, accelY, accelY, accelZ, 0, 0, 0])
		counter += 1
                
		# print counter, data[0]
		# print rawdata
		if stomp != 0 and stomp != 1:
			print "ERROR: stomp value invalid:", stomp
			break
		
		Note_old = 0
		if testingfoot == 1: 
			stomp = 1
		if stomp:
			print 'THERE WAS A STOMP'
			# GET THE CURRENT POSITION
			ifile = open(os.path.join(GAIT_PATH,'currentPosition.csv'), "rb")
			reader = csv.reader(ifile)
			for row in reader:
				currentPosition = row
			for i in xrange(len(currentPosition)):
				currentPosition[i] = float(currentPosition[i])
			ifile.close()

			# CLASSIFY LOCATION INTO AN INSTRUMENT
			instrument = 0
			if currentPosition[0] <= 0 and currentPosition[1] > 0:
				instrument = 1
			elif currentPosition[0] <=0 and currentPosition[1] <= 0:
				instrument = 2
			elif currentPosition[0] > 0 and currentPosition[1] > 0:
				instrument = 3
			threading.Thread(target=hand_main,args=(my_id,instrument,Note_old,)).start()
	try:
		print 'Closing:',writePath
		f.close();
	except:
		print 'Please klean gait_tracking/data directory'
	print 'Exiting foot_main with client ID:', my_id
	
#####################################################################
##################### MAIN THREAD (1) EXECUTION #####################
#####################################################################

# OBJECTIVE (for Thread 1)
# listen for new clients
# determine which imu each client is
# spawn a thread (Thread 2) for each foot client

if __name__ == "__main__":
        counter = 0
        testingfoot = 1

	s = socket.socket()	# Create a socket object
	s.bind((HOST,PORT))	# Bind to the port
	s.listen(2*EXPECTED_USERS) # backlog = max # of queued connections
                
        # TODO This should be an infinite loop in a separate thread
        while counter < EXPECTED_USERS*2:
	        # Waiting for connection
	        print 'Accepting...'
	        c, addr = s.accept()	# Establish connection with client
	        print 'Connected to', addr
                data = ''
                while len(data) < 2:
	                #data = bytearray(c.recv(2))[0]	# Receive id from client (e.g. 0x1F)
                        data += c.recv(2-len(data))
	        #print 'Received', data

	        #client_type = data >> 4;	# 1 -> foot; 0 -> hand
	        #client_ID = data & 0x0F;
                (client_ID, client_type) = struct.unpack('BB',data)
                print 'CLIENT TYPE', client_type
                print 'CLIENT ID', client_ID
	        if client_type == 1:
		        if fIDtoSocket.has_key(client_ID):
			        print 'Already connected with foot client', client_ID
                                # TODO send back refusal to client so it can end gracefully and close this socket
                                continue	# maybe this should stop
                        fIDtoSocket[client_ID] = c	# i really hope this is passed-by-value YES
                        print 'Connected with foot client', client_ID
	        elif client_type == 0:
		        if hIDtoSocket.has_key(client_ID):
			        print 'Already connected with hand client ', client_ID
                                # TODO send back refusal to client so it can end gracefully and close this socket
			        continue	# maybe this should stop
		        hIDtoSocket[client_ID] = c
		        print 'Connected with hand client', client_ID
	        else:
		        print "Invalid client ID"
		        # TODO send back refusal to client so it can end gracefully and close this socket
                        continue
	        if testingfoot == 1:
		        break
	        counter += 1

        # At this point all foot-hand clients should be paired
        if testingfoot == 0:
	        if not compare_hashable_lists(fIDtoSocket.keys(),hIDtoSocket.keys()):
		        print 'Failed to connect to all foot-hand pairs'
		        unmatched = [x for x in fIDtoSocket.keys() if x not in hIDtoSocket.keys()]
		        print 'Listing all unmatched foot clients', unmatched
		        unmatched = [x for x in hIDtoSocket.keys() if x not in fIDtoSocket.keys()]
		        print 'Listing all unmatched hand clients', unmatched
		        sys.exit()

        for x in fIDtoSocket.keys():
	        # assuming that each thread still maintains access to the two dictionaries YES
                threading.Thread(target=foot_main,args=(x,)).start()
	        # print 'Started thread for foot client', x
