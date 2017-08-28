# Isosonic Compensation

The program currently support 16bits PCM .wav file with any sample rate.
Listening level of 80dB SPL and +, there will be no compensation.
Listening level lower than 80dB SPL, there will be a compensation.
For now, there are good results for listening levels between 60 and 80 dB SPL.
The transfer function set needs to be improved (and extreme values limited) to avoid distortion.


# Compilation et Execution (FFTW3 & gcc required)

Compilation

=> reset && gcc -Wall main.c gui.c fft.c wave.c loudness.c -lfftw3 -lm -o dsp.bin

Execution

=> reset && ./dsp.bin ../input_file ../output_file *buffer_size* *listening_level*

A buffer size of 4096 and a listening level of 80 is recommended for now.

The transfer function set is a .csv file that should remain in the root directory of the program.


# Simplified flowchart

- Input parameters
- Header read/write
- Processing loop
  - Data read
    - circshift
    - fft r2c
    - complex product with the transfer function
    - ifft c2r
    - circshift
  - Data write
- Update file size
- Free the memory

