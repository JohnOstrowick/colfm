#!/bin/sh

cp templates/colfm.desktop ~/.local/share/applications/colfm.desktop
cp templates/colfm_desktop.desktop ~/.local/share/applications/colfm_desktop.desktop
sudo ln -s /home/john/Github/_linux/colfm/colfm /usr/local/bin/colfm
sudo ln -s /home/john/Github/_linux/colfm/colfm_desktop /usr/local/bin/colfm_desktop
