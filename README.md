# audio_isosonic_processing
~ A dynamic audio compensation of the isosonic curves in terms of frequency and level ~

isosonic_compensation contain the main program which is able to read/write a PCM .wav file and compensate the isosonic curves for a specific listening level.

curve_processing contain a small program which creates a set of transfer functions from the Fletcher & Munson curves.

To Do:
- Apply a Hann windows in order to remove the windowing artefacts
- Normalize the output to -3dB FS max, TruePeak if possible
- Improve the quality of the transfer function set
- Improve the execution speed, no more fseek()
