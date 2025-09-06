#!/bin/bash

sudo apt update
sudo apt install libimobiledevice6 libimobiledevice-utils ifuse gvfs-backends gvfs-fuse
mkdir -p ~/iPhone
idevicepair pair
ifuse ~/iPhone
xdg-open ~/iPhone/DCIM
