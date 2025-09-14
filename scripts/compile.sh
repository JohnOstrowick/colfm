#!/bin/bash

echo killing instances
killall colfm
killall colfm_desktop
echo "building main app"
g++ -std=c++17 -Wall -Wextra -O2 \
    colfm.cpp \
    -o colfm \
    $(pkg-config --cflags --libs Qt6Widgets Qt6Gui Qt6Core Qt6Network)

echo "building desktop"
g++ -std=c++17 colfm_desktop.cpp -o colfm_desktop $(pkg-config --cflags --libs Qt6Widgets) -lX11 -lQt6Network
