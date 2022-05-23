#!/bin/bash
# gem5 build shell script

scons build/X86/gem5.opt -sQ -j$(nproc)

tar -czf build.tar build/