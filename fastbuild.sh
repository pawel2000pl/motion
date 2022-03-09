#!/bin/bash

if [ -e "Makefile" ];
then
    make cleanall
fi;

automake
autoreconf -fiv
./configure
make clean && make
