# curve processing

Create a set of transfer functions from the Fletcher-Munson's curves.

	clear && rm -fr carve/* && ./curve_process && ./gnuplot.sh

Erasing the content of directory **carve/** is necessary to avoid the concatenation of the .csv files.


## Note

- **gnuplot** likes column dataset, so a transpose might be necessary.
- Use **csvtool** to transpose the **input.csv** file.
- **curve_process.c** exports .csv files at each step of the processing in **carve/** directory.
- **gnuplot.sh** is an automated script that creates plots of provided .csv file in **carve/** directory.