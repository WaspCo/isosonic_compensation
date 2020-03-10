# isosonic compensation

The program currently supports 16bits PCM .wav file with any sample rate.
Listening level of 80dB SPL and +, there will be no compensation.
Listening level lower than 80dB SPL, there will be a compensation.
For now, there are good results for listening levels between 60 and 80 dB SPL.
The transfer function set needs to be improved (and extreme values limited) to avoid distortions.


## Compilation et Execution (FFTW3 & gcc required)

To compile:

    clear && gcc -Wall main.c gui.c fft.c wave.c loudness.c -lfftw3 -lm -o dsp.bin

To execute:

    clear && ./dsp.bin [input_file] [output_file] [buffer_size] [listening_level]

A buffer size of 4096 is recommended. The transfer function set is a .csv file that is assumed to be in the root directory of the program.


## Flow

- input parameters
- .wav header read/write
- processing loop
  - data read
    - circshift
    - fft r2c
    - complex product with the transfer function
    - ifft c2r
    - circshift
  - data write
- update file size
- free memory

