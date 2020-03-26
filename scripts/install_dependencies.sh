#!/bin/bash

# g++ compiler
# (required by qt5 and igraph)
sudo apt-get install build-essential

# ffmpeg plugin for gstreamer 0.10 in Ubuntu 14.04
# (required by qt5-multimedia)
# sudo add-apt-repository ppa:mc3man/trusty-media   # Ubuntu 14.04
# sudo add-apt-repository ppa:mc3man/gstffmpeg-keep   # Ubuntu 16.04
# sudo apt-get update
# sudo apt-get install gstreamer0.10-ffmpeg

# ffmpeg plugin for gstreamer 1.0 in Ubuntu 16.04
# (required by qt5-multimedia)
# sudo apt-get install gstreamer1.0-libav
sudo apt-get install libpulse-dev

# ffmpeg
sudo apt-get install ffmpeg

# sox
sudo apt-get install sox
sudo apt-get install libsox-fmt-all

# qt5
# sudo apt-get install qt5-default
# sudo apt-get install qtmultimedia5-dev
# sudo apt-get install libqt5multimedia5-plugins

# armadillo
sudo apt-get install libarmadillo-dev

# opencv
sudo apt-get install libopencv-dev

# cplex
chmod +x dependencies/cplex/cplex_studio1251.linux-x86-64.bin
sudo ./dependencies/cplex/cplex_studio1251.linux-x86-64.bin

# igraph
sudo apt-get install libxml2-dev

cd dependencies/igraph/igraph-0.7.1/
automake -f -c
autoconf -f
./configure
make
make check
sudo make install
echo "export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib" >> ~/.bashrc
cd -

# alizé
sudo apt-get install aclocal autoconf automake libtool

cd dependencies/alize/ALIZE/
./configure
make
make check
make install

cd ../LIA_RAL/
aclocal
automake --add-missing
autoconf
./configure
make

cd ../../../

cp dependencies/alize/LIA_RAL/bin/EnergyDetector spkDiarization/bin/
cp dependencies/alize/LIA_RAL/bin/IvExtractor spkDiarization/bin/
cp dependencies/alize/LIA_RAL/bin/NormFeat spkDiarization/bin/
cp dependencies/alize/LIA_RAL/bin/TotalVariability spkDiarization/bin/
cp dependencies/alize/LIA_RAL/bin/TrainWorld spkDiarization/bin/
