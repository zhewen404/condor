#!/bin/bash
# gem5 testing

case $1 in
  0) bench=blackscholes;;
  1) bench=bodytrack;;
  2) bench=canneal;;
  3) bench=dedup;;
  4) bench=facesim;;
  5) bench=ferret;;
  6) bench=fluidanimate;;
  7) bench=freqmine;;
  8) bench=raytrace;;
  9) bench=streamcluster;;
  10) bench=swaptions;;
  11) bench=vips;;
  12) bench=x264;;
  *) echo "bad process id";; 
esac

gem5/my_scripts/fs/fs_test.sh

tar -czf testout.tar gem5/my_STATS/testout
