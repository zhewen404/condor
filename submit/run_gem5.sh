#!/bin/bash
# gem5 build shell script

mkdir gem5/m5out

gem5/build/X86/gem5.opt \
gem5/configs/example/gem5_library/x86-parsec-benchmarks.py \
--benchmark dedup \
--size simsmall

tar -czf m5out.tar gem5/m5out
tar -czf disk-image.tar disk-image