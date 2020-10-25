CC=gcc

all: isosonic_compensation

isosonic_compensation: main.o fft.o wave.o loudness.o isosonic.o
	$(CC) main.c fft.c wav.c loudness.c isosonic.c -lfftw3 -lm -o isosonic_compensation

main.o: main.c
	$(CC) -c main.c

fft.o: fft.c
	$(CC) -c fft.c

wav.o: wav.c
	$(CC) -c wav.c

loudness.o: loudness.c
	$(CC) -c loudness.c

isosonic.o: isosonic.c
	$(CC) -c isosonic.c

clean:
	 rm isosonic_compensation *.o