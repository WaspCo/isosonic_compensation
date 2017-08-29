# --- Creation of a transfer function set from the isosonic curves ---


# Demo

cd ../
reset && rm -fr carve/* && ./curve_process && ./gnuplot.sh

=> Erasing the content of carve/ is necessary for the .csv not to concatenate


# Note

=> Gnuplot likes column dataset, so a transpose can be necessary ...
	=> csvtool transpose input.csv > output.csv
=> curve_process.c export .csv files in carve/ directory
=> gnuplot.sh is an automated script which creates plots of the .csv in carve/ directory





