set terminal tkcanvas
set output "/home/simlab/Desktop/spectrometer/gcan" 
set xrange [350:1000]
set yrange [:]
plot '/home/simlab/Desktop/spectrometer/.tempspectrum' using 2:4 w l notitle 
