#!/bin/bash

g++ -std=c++17 -Wall -Wextra -O2 \
    colfm.cpp \
    -o colfm \
    $(pkg-config --cflags --libs Qt6Widgets Qt6Gui Qt6Core)
