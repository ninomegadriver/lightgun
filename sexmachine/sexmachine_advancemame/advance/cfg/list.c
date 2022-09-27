/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 1999, 2000, 2001, 2002, 2003 Andrea Mazzoleni
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

const char* MONITOR[] = {
	"# TVs",
#ifdef DEBUG /* disabled on release as it's not the recommended mode for HDTV */
	"Generic:HDTV 1980x1080:10-180/30-80/50-70",
#endif
	"Generic:PAL TV (European):4-100/15.62/50",
	"Generic:PAL/NTSC TV (European):4-100/15.62/50;5-100/15.73/60",
	"Generic:NTSC TV (USA):4-100/15.73/60",
	"# Arcade Monitors",
	"Generic:Arcade Monitor Standard CGA Resolution 15 kHz:4-100/15.75/60",
	"Generic:Arcade Monitor Medium EGA Resolution 25 kHz:4-100/24.84/60",
	"Generic:Arcade Monitor Extended Resolution 16 kHz:4-100/16.5/53",
	"Hantarex:Polo 15 kHz:4-100/15.75/60",
	"Neotec:NT-1415/1000/1915/2500/2700/3300/3500 Standard CGA Resolution:4-100/15.75/60",
	"Neotec:NT-1426/1100/1725/1926/2501/2701/3301/3501 Medium EGA Resolution:4-100/24.84/60",
	"Neotec:NT-1431/1200/1531/1731/1931 VGA Resolution:4-100/31.5/50-60",
	"Neotec:NT-431HR SVGA Resolution:4-100/31.5-38/45-90",
	"Neotec:NT-448/548 SVGA Resolution:4-100/31.5-48/50-100",
	"Neotec:NT-1565/1765 SVGA Resolution:4-100/31.5-65/50-120",
	"Neotec:NT-785M SVGA Resolution:4-100/31.5-85/50-120",
	"Wells Gardner:K7200/K7400 Universal CGA Monitor:4-100/15.75/60",
	"Wells Gardner:K7600 CGA/EGA Monitor:4-100/15.75/60;5-100/25/60",
	"Wells Gardner:K7500 EGA Monitor:4-100/25/47-63",
	"Wells Gardner:U3100 VGA Monitor:4-100/31.5/56-87;5-100/35.5/56-87",
	"Wells Gardner:D9200 Digital CGA to VGA:4-100/15.75/60;5-100/25/60;5-100/31.5/60-90",
	"# PC Monitors",
	"Generic:PC VGA 640x480:4-150/31.5/55-130",
	"Generic:PC SVGA 800x600 (Super):4-150/30.5-50/55-90",
	"Generic:PC XVGA 1024x768 (Extended):4-150/30.5-70/55-130",
	"Generic:PC SXVGA 1280x1024 (Super Extended):4-180/30.5-120/55-130",
	"Generic:PC WSXVGA 1600x1024 (Wide Super Extended):4-180/30.5-120/55-130",
	"Generic:PC HDTV 1980x1080 (Full HD):10-180/30-80/50-70",
	"Generic:PC UXVGA 1600x1200 (Utra Extended):4-250/30.5-120/55-130",
	"Generic:PC WUXVGA 1920x1200 (Wide Utra Extended):4-250/30.5-120/55-130",
	0
};

