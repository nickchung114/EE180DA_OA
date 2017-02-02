import winsound
import socket
from timeit import default_timer as timer

print('Started')
NoteArray = ['zero','Bassoon_c.wav','Bassoon_d.wav','Bassoon_e.wav','Bassoon_f.wav','Bassoon_g.wav',
                'French_c.wav','French_d.wav','French_e.wav','French_f.wav','French_g.wav',
                'Guitar_C.wav','Guitar_d.wav','Guitar_e.wav','Guitar_f.wav','Guitar_g.wav',
                'Violin_c.wav','Violin_d.wav','Violin_e.wav','Violin_f.wav','Violin_g.wav']

# make socket object ;; AF_INET -> IPv4 ;; SOCK_STREAM -> TCP
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)


#HOST = '0.0.0.0'		# Symbolic name meaning all available interfaces
HOST = socket.gethostname()	# Get local machine name
PORT = 8000			# Arbitrary non-privileged port
BUFFER_SIZE = 1			# should be 1 as long as we're sending one int at a time

s.bind((HOST, PORT))
s.listen(1)
conn, addr = s.accept()
print("{} {}".format('Current client at IP address', addr))

start = timer()

counter = 0;
while 1:
    # conn.recv(x) receives a sequences of bytes
    # .decode('utf-8') converts that into a string, and .rstrip removes newlines
    #data = (conn.recv(BUFFER_SIZE)).decode('utf-8').rstrip('\n')
    # takes the ints (encoded as bytes) and returns int
    data = conn.recv(BUFFER_SIZE)
    counter = counter + 1
	#if not data:   # checks if what we received was 'nothing'
    #   # ASSUMPTION 2: THE CLIENT CONTINUE WRITING
    #    continue
    data = int.from_bytes(data, byteorder='big')
    if data == 0:
        break
    #print(data, type(data))
    #winsound.PlaySound(NoteArray[Note], (winsound.SND_FILENAME | winsound.SND_ASYNC))
    #conn.sendall(bytes('Word', 'UTF-8'))
print("{}".format(timer() - start))
print(counter)
conn.close()