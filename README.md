# isosonic compensation
~ A dynamic audio compensation of the isosonic curves ~

This is my bachelor's degree final project.

## Introduction

Most audio materials are edited, mixed and eventually mastered by a sound engineer at a standard level of approximately 80dB SPL (typically at 1 meter). This level is high enough to allow the sound engineer to perceive the whole dynamic range and details of the audio material while remaining low enough to prevent any hearing damages. Of all the possible ways a sound engineer could shape the sound, balancing the frequency spectrum correctly is of great importance, and a difference of 3dB on a particular frequency band might greatly influence the result. 

Final audio materials are played at different levels depending on the context. In a night club, it might be 110dB SPL. Using headphones, probably 70dB SPL. At home or at the office as an audio background however, it will range from 20db SPL to 60db SPL. We know that the human ear does not have a flat spectral sensitivity. We usually approximate our sensitivity to relative intensities and frequencies as logarithmic, but it is more complex than that. Equal-loudness contour curves were created to characterize precisely the human ear sensitivity to relative frequencies. The most famous is the [Fletcher-Munson](https://en.wikipedia.org/wiki/Equal-loudness_contour) set of curves determined experimentally by Harvey Fletcher and Wilden A. Munson in 1933.

<p style="width:400px; margin: auto;" >
  <img src="./asset/fletcher_munson.png">
</p>

Final audio materials are played at different levels depending on the context. In a night club, it might be 110dB SPL. Using headphones, probably 70dB SPL. At home or at work as an audio background however, it will range from 20db SPL to 60db SPL. We know that humans do not have a flat spectral sensitivity of frequency against amplitude. We usually approximate our sensitivity to relative intensities and frequencies as logarithmic, but it is more complex than that. Equal-loudness contour curves were created to characterize precisely the human ear sensitivity to relative frequencies. The most famous is the [Fletcher-Munson](https://en.wikipedia.org/wiki/Equal-loudness_contour) set of curves determined experimentally by Harvey Fletcher and Wilden A. Munson in 1933.


## Dynamic compensation of the isosonic curves

A dynamic compensation of the isosonic curves is proposed. We proved the feasibility of our approach using a simple C program. Accepting an uncompressed .wav audio file as input and listening level, this program outputs an audio file whose spectrum has been corrected. Using a fast Fourier transform (FFTW library) and an appropriate sliding window, we take the product of the audio file's spectrum with an appropriate transfer function. This is known to be much faster than convoluting the signal with the inverse Fourier transform of the transfer function.

<p style="width:500px; margin: auto;" >
  <img src="./asset/fourier_convolution.png">
</p>


## Results

The correction is applied at different levels on a white noise input file. By analyzing the spectrum of the output file, we observed that the correction was successfully applied to the audio file. However, using a music audio file we observed that the stronger the correction (typically for very low listening levels), the more distortion was added. We believe that the amount of distortion created could be reduced by crafting better transfer functions, and by choosing a smooth window for the Fourier transform. However, distortion is unavoidable when strong equalization is applied to any audio material.


## Content

**isosonic_compensation** contains the main program which can read/write a PCM .wav file and compensate the isosonic curves for a specific listening level. **curve_processing** contains a small program that creates a set of transfer functions from the Fletcher & Munson curves. For visualization purposes, **gnuplot** is used to plot the different steps. **sample** contains 3 audio files, and **asset** is for any image used in this readme.


## Possible improvements

- Apply Hann windows to remove the windowing artifacts (regular click) and reduce distortion.
- Normalize the output to -3dB FS max, TruePeak if possible.
- Improve the quality of the transfer function set.
- Improve the execution speed by getting rid of fseek() statements.