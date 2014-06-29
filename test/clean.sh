#!/bin/sh

for dir in case-*; do
  cd $dir
  make clean
  cd ..
done
