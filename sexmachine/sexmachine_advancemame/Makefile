#############################################################################
# Automatic configuration
#
# Setup by the ./configure script. If you want to use the manual
# configuration edit the Makefile.usr file and NOT this file.
#

VERSION=3.9-50-g384e646
prefix=/home/pi/advmame
datadir=${datarootdir}
datarootdir=${prefix}/share
sysconfdir=${prefix}/etc
exec_prefix=${prefix}
bindir=${exec_prefix}/bin
mandir=${prefix}/man
docdir=${prefix}/doc
CONF_SYSTEM=unix
CONF_HOST=linux
CONF_BUILD=linux
CONF_LIB_DIRECT=yes
CONF_LIB_ZLIB=yes
CONF_LIB_EXPAT=no
CONF_LIB_SVGALIB=no
CONF_LIB_FB=yes
CONF_LIB_VC=yes
CONF_LIB_ALSA=yes
CONF_LIB_OSS=no
CONF_LIB_SDL=no
CONF_LIB_FREETYPE=yes
CONF_LIB_SVGAWIN=no
CONF_LIB_PTHREAD=yes
CONF_LIB_SLANG=no
CONF_LIB_NCURSES=no
CONF_LIB_KRAW=yes
CONF_LIB_JRAW=yes
CONF_LIB_MRAW=yes
CONF_LIB_KEVENT=yes
CONF_LIB_JEVENT=yes
CONF_LIB_MEVENT=yes
CONF_LIB_MRAWINPUT=no
CONF_LIB_MCPN=no
CONF_CFLAGS_ARCH= -DUSE_LSB
CONF_CFLAGS_OPT=-w -mcpu=arm1176jzf-s -mfloat-abi=hard -mfpu=vfp -mtune=arm1176jzf-s -O3
CONF_LDFLAGS=-lwiringPi -lbcm_host -lvcos -lvchiq_arm -lvchostif
CONF_LIBS= -lm
CONF_DEBUGGER=no
CONF_DEBUG=no
CONF_PERF=no
CONF_DEFS=-DHAVE_CONFIG_H
CONF_TINY=no

#############################################################################
# Extra configuration common for ./configure and manual

# Enable the creation of the map files
ifndef CONF_MAP
CONF_MAP=no
endif

# Name of the architecture. Used in the distribution file names.
ifndef CONF_ARCH
ifeq ($(CONF_PERF),yes)
CONF_ARCH=perf
else
ifeq ($(CONF_DEBUG),yes)
CONF_ARCH=debug
else
CONF_ARCH=blend
endif
endif
endif

############################################################################
# Tools configuration for ./configure

srcdir=.
# Don't add the prefix @. This command must be used also in a shell script
INSTALL=/usr/bin/install -c
CC=@gcc
CXX=@g++
LD=@gcc
LDXX=@g++
AR=@ar
ASM=@
RC=@
LN_S=@ln -s
MD=-@mkdir -p
RM=@rm -f
ECHO=@echo
CC_FOR_BUILD=@gcc
LD_FOR_BUILD=@gcc
CXX_FOR_BUILD=@g++
LDXX_FOR_BUILD=@g++
EXE=
EXE_FOR_BUILD=
SDLCFLAGS=
SDLLIBS=
FREETYPECFLAGS=-I/usr/include/freetype2 -I/usr/include/libpng16
FREETYPELIBS=-lfreetype
VCCFLAGS=-I/usr/include
VCLIBS=-L/usr/lib -lbcm_host -lvcos -lvchiq_arm
ASMFLAGS=-f elf
CFLAGS_FOR_BUILD=-O0 -DUSE_COMPILER_GNUC -DUSE_OBJ_ELF -DUSE_OS_UNIX
INSTALL_PROGRAM_DIR = $(INSTALL) -d -m 755
INSTALL_MAN_DIR = $(INSTALL) -d -m 755
INSTALL_DATA_DIR = $(INSTALL) -d -m 755
INSTALL_PROGRAM = $(INSTALL) -c -m 755
INSTALL_MAN = $(INSTALL) -c -m 644
INSTALL_DATA = $(INSTALL) -c -m 644

#############################################################################
# Root makefile

include $(srcdir)/root.mak


