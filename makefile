all: isosonic_compensation

isosonic_compensation: main.o
	gcc main.c fft.c wave.c loudness.c -lfftw3 -lm -o isosonic_compensation
     
clean:
	 rm isosonic_compensation