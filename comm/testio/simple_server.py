import socket		# Import socket module
import sys
import threading
import collections	# for Counter()

HOST = socket.gethostname()	# Get local machine name
PORT = 8000			# Reserve a port for your service

s = socket.socket(socket.AF_INET,socket.SOCK_STREAM)	# Create a socket object
s.bind((HOST,PORT))	# Bind to the port
print 'Starting listen'
s.listen(5)		# backlog = max # of queued connections; Waiting for connection
print 'Starting listen'
c, addr = s.accept()	# Establish connection with client
print 'Connected to ', addr	# WEEDLE
data = int(c.recv(1))	# Receive id from client (e.g. 0x1F)
print 'Connected with ID ', data	#WEEDLE
