import socket		# Import socket module
import sys
import struct

#HOST = socket.gethostname()	# Get local machine name
HOST = ''
PORT = 8000			# Reserve a port for your service

print 'Making socket'
s = socket.socket(socket.AF_INET,socket.SOCK_STREAM)	# Create a socket object

print 'Binding socket to port', PORT
s.bind((HOST,PORT))	# Bind to the port

print 'Listening...'
s.listen(5)		# backlog = max # of queued connections; Waiting for connection

print 'Accepting...'
c, addr = s.accept()	# Establish connection with client

print 'Connected to', addr

print 'Waiting for id'
# Note that recv(N) receives a MAX of N bytes, and doesn't block (by default)
data = c.recv(4)	# Receive id from client (e.g. 0x1F)
print 'Connected with ID: 0x%X' % struct.unpack("<I", data)[0]
