#!/bin/bash
# gem5 run shell script (baseline)

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
  *) 
    echo "bad process id" ;
    exit 1;; 
esac

case $2 in
  0) sim=simsmall;;
  1) sim=simlarge;;
  2) sim=simmedium;;
  *) 
    echo "bad sim option" ;
    exit 1;; 
esac

echo "cp /staging/zpan52/parsec.tar.gz ./"
cp /staging/zpan52/parsec.tar.gz ./
echo "tar -xzvf parsec.tar.gz"
tar -xzvf parsec.tar.gz

echo "cd gem5"
cd gem5
echo "my_scripts/fs/ckpt_resume.sh 64 ${bench} ${sim} 1"
my_scripts/fs/ckpt_resume.sh 64 ${bench} ${sim} 1
echo "cd .."
cd ..

timestamp=$(date +%Y%m%d-"%H%M%S")
echo "tar -czf ${bench}_${sim}_64_1_$timestamp.tar gem5/my_STATS/${bench}_${sim}_64_1"
tar -czf ${bench}_${sim}_64_1_$timestamp.tar gem5/my_STATS/${bench}_${sim}_64_1

echo "rm parsec.tar.gz parsec"
rm parsec.tar.gz parsec
