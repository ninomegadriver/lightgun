#!/bin/bash

LDFLAGS="-lwiringPi  -lbcm_host -lvcos -lvchiq_arm -lvchostif" CFLAGS="-w -mcpu=arm1176jzf-s -mfloat-abi=hard -mfpu=vfp -mtune=arm1176jzf-s -O3" ./configure --prefix=/home/pi/advmame --enable-vc --with-vc-prefix=/usr --disable-oss --disable-alsa --disable-sdl --disable-sdl2 --disable-ncurses --enable-alsa && \
LDFLAGS="-lwiringPi  -lbcm_host -lvcos -lvchiq_arm -lvchostif" CFLAGS="-w -mcpu=arm1176jzf-s -mfloat-abi=hard -mfpu=vfp -mtune=arm1176jzf-s -O3" make -j4
