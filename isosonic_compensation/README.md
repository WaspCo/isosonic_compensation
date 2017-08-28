# Isosonic Compensation

The program currently support 16bits PCM .wav file with every sample rate.
Listening level of 80dB SPL and +, there will be no compensation
Listening level lower than 80dB SPL, there will be a compensation
For now, there are good results for listening levels between 60 and 80 dB SPL.
The transfer function set need to be improved (and extreme values limited) to avoid distortion.

Here is a short flowchart:

- Input parameters
- Header read/write
- Processing loop
  - data read
    - fft r2c
    - circshift
    - complex product with the transfer function
    - circshift
    - ifft c2r
  - data write
- update file size
- free the memory


# Compilation et Execution (FFTW3 & gcc required)

Compilation

=> reset && gcc -Wall main.c gui.c fft.c wave.c loudness.c -lfftw3 -lm -o dsp.bin

Execution

=> reset && ./dsp.bin ../input_file ../output_file *buffer_size* *listening_level*

A buffer size of 4096 and a listening level of 80 is recommended for now.

The transfer function set is a .csv file that should remain in the root directory of the program.
