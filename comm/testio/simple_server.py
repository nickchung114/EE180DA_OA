import socket		# Import socket module
import sys

HOST = socket.gethostname()	# Get local machine name
PORT = 8000			# Reserve a port for your service

print 'Making socket'
s = socket.socket(socket.AF_INET,socket.SOCK_STREAM)	# Create a socket object

print 'Binding socket to port ', PORT
s.bind((HOST,PORT))	# Bind to the port

print 'Listening...'
s.listen(5)		# backlog = max # of queued connections; Waiting for connection

print 'Accepting...'
c, addr = s.accept()	# Establish connection with client

print 'Connected to ', addr	# WEEDLE
data = int(c.recv(1))	# Receive id from client (e.g. 0x1F)

print 'Connected with ID ', data	#WEEDLE
