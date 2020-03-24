#!/bin/bash

rm *~
rm moc_*
rm *.o
rm Makefile

qmake
make
