#!/bin/bash

## usage: install-webmirror.sh install user
##   or
##        install-webmirror.sh uninstall system
## etc.

c="$1"
p="$2"
if [ -z "$c" ] ; then 
	c='install'
fi
if [ -z "$p" ] ; then
	p="user"
fi

xdg-icon-resource "$c" --mode "$p" --size 22 ../../../icons/red-22.png webmirror.png
xdg-icon-resource "$c" --mode "$p" --size 22 ../../../icons/red-22.png webmirror-menu.png
xdg-icon-resource "$c" --mode "$p" --size 22 ../../../icons/red-22.png webmirror-admintool.png
xdg-icon-resource "$c" --mode "$p" --size 64 ../../../icons/red-64.png webmirror.png
xdg-icon-resource "$c" --mode "$p" --size 64 ../../../icons/red-64.png webmirror-menu.png
xdg-icon-resource "$c" --mode "$p" --size 64 ../../../icons/red-64.png webmirror-admintool.png

#xdg-desktop-menu "$c" "$p" --vendor shinythings ./webmirror.desktop
#xdg-desktop-menu "$c" "$p" --vendor shinythings ./webmirror-admin.desktop
#xdg-desktop-menu "$c" "$p" --vendor shinythings ./webmirror.directory
#xdg-desktop-menu "$c" "$p" --vendor shinythings ./webmirror.menu

xdg-desktop-menu "$c" --mode "$p" --noupdate ./webmirror.desktop
xdg-desktop-menu "$c" --mode "$p" --noupdate ./webmirror-admin.desktop
xdg-desktop-menu "$c" --mode "$p" --noupdate ./webmirror.directory
xdg-desktop-menu "$c" --mode "$p" --noupdate ./webmirror.menu

xdg-desktop-menu forceupdate "$p"
