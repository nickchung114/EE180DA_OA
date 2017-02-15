def hand_main(my_id, instrument, Note_old) # will need to add a variable Note_old to foot_main.
	print 'Starting hand_main with client ID', my_id	# WEEDLE
	# SEND AN INTERRUPT
	hIDtoSocket[my_id].send(ahem)
	# ESTABLISH A LISTENER
	# WAIT
	# RECEIVE CLASSIFIED DATA (i.e. NOTE)
	data1 = hIDtoSocket[my_id].recv(1024)
	data2 = data1.split(,)
	data3 = [int(e) for e in data2]
	pitch = data3[1]
	
	# PLAY SOUND
	NoteArray = ['zero','Bassoon_c.wav','Bassoon_d.wav','Bassoon_e.wav','Bassoon_f.wav','Bassoon_g.wav',
                'French_c.wav','French_d.wav','French_e.wav','French_f.wav','French_g.wav',
                'Guitar_C.wav','Guitar_d.wav','Guitar_e.wav','Guitar_f.wav','Guitar_g.wav',
                'Violin_c.wav','Violin_d.wav','Violin_e.wav','Violin_f.wav','Violin_g.wav']
	if Note_old  11 or Note_old  15 #Stop playing the previous note if it is a wind or instrument or violin.
		winsound.PlaySound(NoteArray[Note_old], (winsound.SND_FILENAME  winsound.SND_PURGE))
	Note = pitch + 5*instrument
	winsound.PlaySound(NoteArray[Note], (winsound.SND_FILENAME  winsound.SND_ASYNC))
	conn.sendall(bytes('Word', 'UTF-8'))
	Note_old = Note;
	print 'Exiting hand_main with client ID', my_id	# WEEDLE
