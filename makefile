CC=gcc

all: isosonic_compensation

isosonic_compensation: main.o fft.o wav.o loudness.o isosonic.o allocation.o
	$(CC) -O0 -g main.c fft.c wav.c loudness.c isosonic.c allocation.c -lfftw3 -lm -o isosonic_compensation

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

allocation.o: allocation.c
	$(CC) -c allocation.c

clean:
	 rm isosonic_compensation out *.o