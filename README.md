# Isosonic_Compensation
~ A dynamic audio compensation of the isosonic curves in terms of frequency and level ~

Most audio material are mixed and masterized by the sound engineer at a level of approximately 80dB SPL/1m. The isosonic curves of Fletcher and Munson taught us that our perception of sound is not linear and vary with the frequency and the level. We can therefore deduce that if we listen to the audio material at a level lower than 80dB SPL/1m, we won't have the same frequency balance as the one the sound engineer originally mixed. This program is the first step toward the creation of an "automated loudness compensation" which could solve this problem. Instead of using a convolution (very slow for this kind of work), we are taking the Fourier transform of the audio signal and then we realize a complex product of the audio spectrum with a transfer function derived from the isosonic curves.

isosonic_compensation contain the main program which is able to read/write a PCM .wav file and compensate the isosonic curves for a specific listening level.

curve_processing contain a small program which creates a set of transfer functions from the Fletcher & Munson curves. For visualisation purpose, gnuplot is used to plot the different steps.

To Do:
- Apply a Hann windows in order to remove the windowing artefacts (regular click).
- Normalize the output to -3dB FS max, TruePeak if possible
- Improve the quality of the transfer function set
- Improve the execution speed, no more fseek()
