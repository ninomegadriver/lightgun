#!/bin/bash

LDFLAGS="-lbcm_host -lvcos -lvchiq_arm -lvchostif" CFLAGS="-w -mcpu=arm1176jzf-s -mfloat-abi=hard -mfpu=vfp -mtune=arm1176jzf-s -O3" make -j4
