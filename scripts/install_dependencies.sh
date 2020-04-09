#!/bin/bash

# g++ compiler
# (required by qt5 and igraph)
sudo apt-get install build-essential

# (required by qt5-multimedia)
sudo apt-get install libpulse-dev

# ffmpeg
sudo apt-get install ffmpeg

# qt5
sudo apt-get install qt5-default
sudo apt-get install qtmultimedia5-dev
sudo apt-get install libqt5multimedia5-plugins

# armadillo
sudo apt-get install libarmadillo-dev

# opencv
sudo apt-get install libopencv-dev

# cplex
# chmod +x dependencies/cplex/cplex_studio1251.linux-x86-64.bin
# sudo ./dependencies/cplex/cplex_studio1251.linux-x86-64.bin

# igraph
sudo apt-get install libxml2-dev
cd dependencies/igraph
tar xzvf igraph-0.7.1.tar.gz 
cd igraph-0.7.1
./configure
make
make check
sudo make install
cd ../../
