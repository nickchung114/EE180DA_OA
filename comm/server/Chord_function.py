def chord(noteList, instrument):
	"Plays a list of notes together"
	NoteArray = ['zero','Bassoon_c.wav','Bassoon_d.wav','Bassoon_e.wav','Bassoon_f.wav','Bassoon_g.wav',
			'French_c.wav','French_d.wav','French_e.wav','French_f.wav','French_g.wav',
			'Guitar_C.wav','Guitar_d.wav','Guitar_e.wav','Guitar_f.wav','Guitar_g.wav',
			'Violin_c.wav','Violin_d.wav','Violin_e.wav','Violin_f.wav','Violin_g.wav']
	for pitch in noteList:
		Note = pitch + 5*instrument
		winsound.PlaySound(NoteArray[Note], (winsound.SND_FILENAME  winsound.SND_ASYNC))
	return;
