Name{number}
	device drivers - AdvanceMAME Device Drivers

	This file describes the video, sound, joystick, mouse
	and keyboard drivers used by the Advance programs.

SubSubIndex

Video Drivers
  Types of Drivers
	The video drivers can be divided in two different categories:

	* Generate drivers able to program directly the video board.
	* System drivers able to use only the available video modes.

    Generate drivers
	This set of drivers is able to program directly the video board
	to always generate a perfect video mode with the correct size
	and frequency.

	They always work in `fullscreen' output mode and generally
	support only a subset of video boards.

	To function correctly these drivers require a correct
	configuration of the `device_video_*' options.

	The `Generate' drivers are always the preferred choice.

    System drivers
	This set of drivers is able to use only the video modes
	available on the operating system.

	They can work in `fullscreen' mode, `window' mode and `zoom'
	output mode using the video board hardware acceleration to
	stretch the image. The default output mode is `window' if you
	run the program in a window manager, otherwise the output
	mode `fullscreen' is chosen.

	Please note that these drivers generally need to stretch the
	image losing a lot in image quality and speed.

	These drivers don't require any `device_video_*' options
	because they cannot control how the video modes are generated.

  Available Generate Drivers
	The following is the list of all the video drivers supported.

    fb - Generate Frame Buffer video (Linux)
	This driver works in Linux and is able to create video modes
	using the Linux Kernel Frame Buffer interface.

	All clocks, all RGB bit depths are available.

	This driver is able to use the Linux fb API to synchronize
	with the vertical sync of the video mode, but generally
	it isn't possible because the low-end drivers don't support
	this feature. Anyway, if you run the program as root it can
	use the standard VGA registers to detect the vsync.
	This is the major limitation compared with the svgalib driver.

	To allow any user to run the program as root, you can
	use the following commands to change the executable
	permissions:

		:chown root:root /usr/local/bin/advmame
		:chmod u+s /usr/local/bin/advmame

	To use this driver you must activate the Console Frame Buffer
	support in your Linux kernel.

	This driver is not available in X (when the environment DISPLAY
	variable is defined).

	The Frame Buffer device opened is `/dev/fb0' and `/dev/fb/0', you
	can change it setting the FRAMEBUFFER environment variable.

	The generic Frame Buffer `vesafb' driver cannot be used because
	it doesn't allow the creation of new video modes. If it's your
	only option you can use it through the SDL library.

    svgalib - Generate SVGA video (Linux) [OBSOLETE]
	This driver works in Linux and is able to use video modes obtained
	tweaking the hardware registers of the recognized SVGA boards using
	the SVGALIB library.

	All clocks, all RGB bit depths are available.
	This driver is generally able to synchronize with the vertical sync
	of the video mode.

	To use these modes your video board must be supported
	by a `svgalib' driver listed in the `cardlinx.txt' file.
	To use this driver you need to install the SVGALIB library
	version 1.9.x.

	To use this driver you need to correctly configure the
	SVGALIB HorizSync and VertRefresh options in the
	file /etc/vga/libvga.config file.
	You must use a range equal or a bit larger than the range
	specified with the AdvanceMAME `device_video_clock' option.

	This driver is not available in X (when the environment DISPLAY
	variable is defined).

	In the `contrib/svgalib' directory there are various
	SVGALIB patches to fix some problems with recent video
	boards, and to work with systems with kernel 2.6 and udev.

    svgaline - Generate SVGA video (DOS) [OBSOLETE]
	This driver works in DOS and is able to use video modes obtained
	tweaking the hardware registers of the recognized SVGA boards.

	All clocks, all RGB bit depths are available.

	To use these modes your video board must be supported
	by a `svgaline' driver listed in the `carddos.txt' file.

	This driver is completely independent of the VGA/VBE BIOS
	of your board.

    vbeline - Generate VBE video (DOS) [OBSOLETE]
	This driver works in DOS and is able to use video modes obtained
	tweaking the standard VBE (VESA) BIOS mode changing the hardware
	registers of the SVGA.

	All clocks, all RGB bit depths are available.

	To use these modes your video board must be supported
	by a `vbeline' driver listed in the `carddos.txt' file.

	These drivers work setting a video mode using the
	default VBE2 services and tweak some hardware SVGA
	registers to modify the obtained video mode.
	The driver `vbe3' is an exception. It requires the
	presence of a VBE3 BIOS to change the frequency of the
	desired video mode. Unfortunately the standard
	VBE3 services don't support a resolution size change.

	The resolution is changed modifying only the standard
	VGA registers. This hack may or not may works.
	Also the interlaced modes are only rarely supported
	by the various VBE3 BIOS because they are very rarely
	used in the standard PC monitors.

	If your video board isn't supported by any drivers and
	you don't have a VBE3 BIOS you can try installing a
	software VESA BIOS like the SciTech Display Doctor.

    vgaline - Generate VGA video (DOS) [OBSOLETE]
	This driver works in DOS and is able to use video modes obtained
	tweaking the hardware registers of the standard VGA.

	Only the standard VGA pixel clocks 6.29, 7.08, 12.59,
	14.16 MHz are available. Only 8 bit color modes. Only
	64 kBytes of video memory.

	This driver supports also text modes with pixel clocks
	12.59, 14.16, 25.17, 28.32 MHz.

	This driver is completely independent of the VGA BIOS
	of your board.

    svgawin - Generate SVGA video (Windows) [OBSOLETE]
	This driver works in Windows NT/2000/XP and is able to use video
	modes obtained tweaking the hardware registers of the recognized
	SVGA boards.

	All clocks, all RGB bit depths are available.

	To use these modes your video board must be supported
	by a `svgawin' driver listed in the `cardwin.txt' file.

	To use this driver you need to install the included SVGAWIN
	driver. Please read the `svgawin.txt' file carefully.

	This driver is experimental. At present it's only tested on
	Windows 2000 with a GeForce 2 board. It may not work will
	all the other boards.

  Available System Drivers
	The following is the list of all the System video drivers supported.

    sdl - System SDL video (Linux, Windows and Mac OS X)
	This driver works in Linux, Windows and Mac OS X and is able to
	use video modes reported by the SDL graphics library.

	It supports all RGB bit depths available and the YUV mode.

	The output in the YUV mode is generally accelerated, and can
	be used to scale the video output to an arbitrary size.
	You can enable this feature with the `-device_video_output overlay'
	option.

	You can change some options of this driver using the SDL specific
	environment variables described in the contrib/sdl/env.txt file.

    slang - System sLang text video (Linux)
	This driver works in Linux and is able to use current terminal text
	mode from the Linux sLang library.

    curses - System curses text video (Linux)
	This driver works in Linux and is able to use current terminal text
	mode from the Linux ncurses library.

    vbe - System VBE video (DOS) [OBSOLETE]
	This driver works in DOS and is able to use video modes reported
	by the VBE BIOS.

Sound Drivers
  Available Drivers
	The following is the list of all the sound drivers supported.

    alsa - ALSA sound (Linux)
	This driver works in Linux and it uses the ALSA sound library.

    oss - OSS sound (Linux)
	This driver works in Linux and it uses the OSS sound library.

    sdl - SDL sound (Linux, Windows and Mac OS X)
	This driver works in Linux, Windows and Mac OS X and it uses
	the SDL library.

	It isn't able to use the hardware volume control of the sound card.
	The volume changes are simulated reducing the sample values.

	It isn't able to precisely control the amount of bufferized samples.
	This means that it may add a small latency on the sound output.

	You can change some options of this driver using the SDL specific
	environment variables described in the contrib/sdl/env.txt file.

    seal - SEAL sound (DOS) [OBSOLETE]
	This driver works in DOS and it uses the SEAL sound library with
	some specific changes for MAME.

	The source patch and the library source can be downloaded from
	the MAME site:

		+http://www.mame.net/

    allegro - Allegro sound (DOS) [OBSOLETE]
	This driver works in DOS and it uses the Allegro library.

    vsync - VSYNC sound (DOS) [OBSOLETE]
	This driver works in DOS and it uses the VSync sound drivers
	from the VSyncMAME emulator.

	More info is in the VSyncMAME page:

		+http://vsynchmame.mameworld.net/

Input Drivers
  Available Keyboard Drivers
	The following is the list of all the keyboard drivers supported.

    event - Kernel Input-Event interface (Linux)
	This driver works in Linux and it uses the new style input-event
	interface of the Linux kernel.

	It supports more than one keyboard at the same time.

	You can change console with ALT+Fx. No other hotkeys are
	available. The hotkeys can be optionally disabled.

	For an emergency keyboard restore you can use the emergency
	Linux SysRq key. Check:

		:http:///usr/src/linux/Documentation/sysrq.txt

	Leds control is supported.

    raw - Kernel keyboard (Linux)
	This driver works in Linux and it uses directly the Linux kernel
	keyboard interface.

	It supports only one keyboard.

	You can change console with ALT+Fx and break the program
	with CTRL+C. No other hotkeys are available. The hotkeys can
	be optionally disabled.

	For an emergency keyboard restore you can use the emergency
	Linux SysRq key. Check:

		:http:///usr/src/linux/Documentation/sysrq.txt

	Leds control is supported.

    sdl - SDL keyboard (Linux, Windows and Mac OS X)
	This driver works in Linux, Windows and Mac OS X and it uses
	the SDL library.

	It supports only one keyboard.

	You can change some options of this driver using the SDL specific
	environment variables described in the contrib/sdl/env.txt file.

	In a Window Manager environment you can switch to fullscreen
	pressing ALT+ENTER.

	Leds control is not supported.

    svgalib - SVGALIB keyboard (Linux) [OBSOLETE]
	This driver works in Linux and it uses the SVGALIB library.

	It supports only one keyboard.

	You can change console with ALT+Fx and break the program
	with CTRL+C. No other hotkeys are available. The CTRL+C hotkey
	can be optionally disabled. The ALT+Fx hotkeys are always
	enabled.

	For an emergency keyboard restore you can use the emergency
	Linux SysRq key. Check:

		:http:///usr/src/linux/Documentation/sysrq.txt

	Leds control is not supported.

    allegro - Allegro keyboard (DOS) [OBSOLETE]
	This driver works in DOS and it uses the Allegro library.

	It supports only one keyboard.

	You can break the program pressing CTRL+C, CTRL+BREAK or ALT+CTRL+END.

	Leds control is supported.

  Available Joystick Drivers
	The following is the list of all the joystick drivers supported.

    event - Kernel Input-Event interface (Linux)
	This driver works in Linux and it uses the new style input-event
	interface of the Linux kernel.

	It supports more than one joystick or light-gun at the same time.

	For USB devices this driver doesn't require any configuration.
	It's able to autodetect all the present hardware.

	This driver is also able to correctly report the type of devices
	found. You should for example expects to have the gas pedal mapped
	on the gas control of the game.

	It can also be used with some custom devices connected at the
	Parallel Port. Details on how to build these custom interfaces are
	in the file:///usr/src/linux/Documentation/input/joystick-parport.txt
	file.

	It has a special support for the ACT Labs Lightgun to fix the wrong
	behavior of the light-gun when shooting out of screen.

	The joysticks are searched on the /dev/input/eventX devices.

	If you have a gameport joystick, the Linux Kernel Joystick driver
	may prevent a correct video vsync if the joystick polling is too slow.
	Generally it results in a missing frame every 5-10 seconds.

    raw - Kernel joystick (Linux)
	This driver works in Linux and it uses directly the Linux kernel
	joystick interface.

	It supports up to 4 joysticks at the same time.

	The joysticks are searched on the /dev/jsX and /dev/input/jsX devices.

	If you have a gameport joystick, the Linux Kernel Joystick driver
	may prevent a correct video vsync if the joystick polling is too slow.
	Generally it results in a missing frame every 5-10 seconds.

    sdl - SDL joystick (Linux, Windows and Mac OS X)
	This driver works in Linux, Windows and Mac OS X and it uses
	the SDL joystick interface.

	It supports more than one joystick at the same time.

	You can change some options of this driver using the SDL specific
	environment variables described in the contrib/sdl/env.txt file.

    svgalib - SVGALIB joystick (Linux) [OBSOLETE]
	This driver works in Linux and it uses the SVGALIB library.

	It supports up to 4 joysticks at the same time.

	The joysticks are searched on the /dev/jsX devices.

	If you have a gameport joystick, the Linux Kernel Joystick driver
	may prevent a correct video vsync if the joystick polling is too slow.
	Generally it results in a missing frame every 5-10 seconds.

    allegro - Allegro joystick (DOS) [OBSOLETE]
	This driver works in DOS and it uses the Allegro library.

	It supports only one joystick.

	Details on how to build the Parallel Port hardware interfaces for
	SNES, PSX, N64 and other pads are in the Allegro sources.

    lgrawinput - Light Gun (Windows XP)
	This driver works in Windows XP and it support lightguns
	using the Windows mouse interface.

	It supports more than one lightgun at the same time
	of the following types:

	* SMOG Lightgun (http://lightgun.splinder.com/)
	* Acts Labs Lightgun (http://www.act-labs.com/)

	The lightgun is automatically calibrated before
	every use. You must move the lightgun over the whole
	screen every time the program starts or changes video
	mode.

  Available Mouse Drivers
	The following is the list of all the mouse drivers supported.

    event - Kernel Input-Event interface (Linux)
	This driver works in Linux and it uses the new style input-event
	interface of the Linux kernel.

	It supports more than one mouse at the same time.

	For USB devices this driver doesn't require any configuration.
	It's able to autodetect all the present hardware.

	For serial mouse you must use the `inputattach' system
	utility to attach the serial line at the event interface.
	Generally this utility is available in the joystick
	calibration package of your distribution. In this case it's
	probably simpler to use the `raw' mouse driver.

	The mice are searched on the /dev/input/eventX devices.

    raw - Serial mouse (Linux)
	This driver works in Linux and it communicates directly with
	the configured serial mice. It also supports USB mice
	using the Linux mousedev module which maps mice to the
	/dev/input/mouseX devices.

	It supports more than one mouse at the same time.

	To use this driver you need to configure correctly the
	device_raw_* options to specify the mouse types and the mouse
	devices.

    sdl - SDL mouse (Linux, Windows and Mac OS X)
	This driver works in Linux, Windows and Mac OS X and it uses
	the SDL mouse interface.

	It supports only one mouse and only two axes.

	You can change some options of this driver using the SDL specific
	environment variables described in the contrib/sdl/env.txt file.

    svgalib - SVGALIB mouse (Linux) [OBSOLETE]
	This driver works in Linux and it uses the SVGALIB library.

	It supports only one mouse.

	To use this driver you need to configure correctly the
	SVGALIB mouse support in the file /etc/vga/libvga.config file.

    allegro - Allegro mouse (DOS) [OBSOLETE]
	This driver works in DOS and it uses the Allegro library.

	It supports up to 2 mice at the same time using the
	special `optimous' driver present in the `contrib/' directory.

    rawinput - Raw input interface (Windows)
	This driver works in Windows XP using the raw input interface.

	It supports more than one mouse at the same time.

	For any mouse up to two axes, one wheel and five buttons
	are supported.

	This driver is the preferred choice for Windows XP.

	Please note that this driver is not intented to be used in a window
	environment. The application takes the control of the mouse and
	doesn't allow to switch to other applications. It's mainly intented
	for a fullscreen environment.

    cpn - CPN custom driver interface (Windows)
	This driver works in Windows 2000/XP using the custom CPN
	mouse driver.

	It supports more than one mouse at the same time.

	For any mouse up to two axes, and three buttons are supported.

	The CPN mouse driver is available at:

		+http://cpnmouse.sourceforge.net/

	and in the `contrib/cpn' directory. Check the `install'
	and `unknown' files for install instructions.

	Please note that this driver is not intented to be used in a window 
	environment. The application takes the control of the mouse and 
	doesn't allow to switch to other applications. It's mainly intented 
	for a fullscreen environment.

Video Drivers Configuration
	The following are the video configuration options available for
	all the programs.

  Common Configuration Options
    device_video
	Selects the video driver to use.

	:device_video auto | (DEVICE[/MODEL])+

	Options:
		auto - Automatic detection of all the available drivers
			(default).

	The order of detection:
		DOS - svgaline, vbeline, vgaline, vbe.
		Linux - svgalib, fb, sdl, slang, curses.
		Mac OS X - sdl.
		Windows - svgawin, sdl.

	Options for Linux:
		svgalib - SVGA generated graphics modes with the
			SVGALIB 1.9.x library.
		fb - SVGA generated graphics modes with the Linux Console
			Frame Buffer.
		slang - Text video modes with the sLang library.
		curses - Text video modes with the ncurses library.
		sdl - SDL graphics and fake text modes.

	Options for Mac OS X:
		sdl - SDL graphics and fake text modes.

	Options for DOS:
		svgaline - SVGA generated graphics modes.
		svgaline/nv3_leg - SVGA legacy driver for nVidia boards.
			If the new driver doesn't work try this one.
		svgaline/savage_leg - SVGA legacy driver for S3 boards.
			If the new driver doesn't work try this one.
		vbeline - VBE generated graphics modes.
		vgaline - VGA generated text and graphics modes.
		vbe - VBE graphics modes.

	Options for Windows:
		svgawin - SVGA generated graphics modes with the
			SVGAWIN included library. To use this driver you
			need to install the `svgawin.sys' driver with the
			`svgawin.exe' command line utility.
		svgawin/nv3_leg - SVGA legacy driver for nVidia boards.
			If the new driver doesn't work try this one.
		svgawin/savage_leg - SVGA legacy driver for S3 boards.
			If the new driver doesn't work try this one.
		sdl - SDL graphics and fake text modes.

	Please note that to use the utilities `advv' and `advcfg' you
	must at least select a graphics and a text video driver. The
	available text video drivers are `vgaline' for DOS, `slang',
	`curses' for Linux and `sdl' for Windows.

	You can force the detection of a specific model of video board
	adding the name of the model driver after the driver name using
	the `/' separator. For example to force the `vbe3' model
	detection of the `vbeline' driver you must specify
	`vbeline/vbe3'.

	To get the list of all the available models try using the
	`help' model name. A short help will be printed.

	Please note that forcing a specific video driver is discouraged.
	Generally you don't need it.

	For a more complete description of the drivers check the
	previous `VIDEO DRIVER' section.

	Example to enable the `vbeline' and the `vgaline' drivers
	with auto-detection for DOS:
		:device_video vbeline vgaline

	Example to force the `vbeline/vbe3' driver and the `vgaline'
	driver for DOS:
		:device_video vbeline/vbe3 vgaline

	Example to enable the `fb' and `slang' driver for Linux:
		:device_video fb slang

    device_video_output
	Select the output mode.

	:device_video_output auto | window | fullscreen | overlay

	Options:
		auto - Automatically chosen (default).
		window - Use a window display.
		fullscreen - Use a fullscreen display.
		overlay - Use a YUV fullscreen overlay using the video board
			hardware acceleration to display and stretch it.
			This mode is available only in some environments, like
			xv in X Window and DirectX in Windows. The specific
			color format used is YUY2.

    device_video_overlaysize
	Select the favorite horizontal size to use with the `overlay'
	output mode. The program selects the nearest available video mode.

	device_video_overlaysize auto | SIZE

	Options:
		auto - Use the size of the current video mode (default).
			If not available tries 1280x1024.
		SIZE - The user favorite horizontal size.

	This option has effect only with the `overlay' output mode.

    device_video_cursor
	Select the mouse cursor mode.

	:device_video_cursor auto | off | on

	Options:
		auto - Automatically choose (default). The cursor
			is enabled in window modes, and disabled
			in fullscreen modes.
		off - Always off.
		on - Always on, only if the video mode support it.

  Generate Configuration Options
	The following are the common video configuration options
	available for all `generate' video drivers, i.e. all the
	video drivers with the exception of `sdl' and `vbe'.
	The `sdl' and `vbe' video drivers simply ignore these
	options.

    device_video_clock
	Specify the monitor frequency range in term of horizontal and
	vertical clocks. This option is MANDATORY.
	Generally these values are specified in the technical page of 
	your monitor manual. 

	:device_video_clock PIXEL_CLOCK / HORZ_CLOCK / VERT_CLOCK [; ...]

	Options:
		PIXEL_CLOCK - Pixel clock range in MHz.
			The lower value is the lower clock generable
			by your video board. The higher value is the
			video bandwidth of your monitor. If you
			don't know these values you can start
			with `5 - 100' which essentially enable any
			video mode.
		HORZ_CLOCK - Horizontal clock range in kHz.
		VERT_CLOCK- Vertical clock range in Hz.

	For any range you can specify a single value like `60' or a
	range of values like `50 - 60'. For multistandard TVs and monitors
	you can use more clock specifications separating them with `;'.

	For example:
		:device_video_clock 5 - 50 / 15.62 / 50 ; 5 - 50 / 15.73 / 60

	Check the `install.txt' file for examples and other information.

    device_video_modeline
	Define a video modeline. The modeline format is compatible with
	the format used by the Linux SVGALIB library and by the
	XFree Window system.

	:device_video_modeline Name CLOCK HDE HRS HRE HT VDE VRS VRE VT
	:	[-hsync] [-vsync] [+hsync] [+vsync] [doublescan] [interlace]

	Options:
		Name - Name of the video mode. You can use the quotes
			'"` for the names with spaces.
		CLOCK - Pixel clock in MHz
		HDE HRS HRE HT - Horizontal `Display End',
			`Retrace Start', `Retrace End', `Total'
		VDE VRS VRE VT - Vertical `Display End',
			`Retrace Start', `Retrace End', `Total'
		-hsync -vsync +hsync +vsync - Polarization mode.
		doublescan - Doublescan mode.
		interlace - Interlaced mode.

	Examples:
		:device_video_modeline tweak320x240 12.59 320 336 356 400 240 \
		:	249 254 262 doublescan -hsync -vsync

    device_video_format
	Select the format of the video modes to create.
	You can insert more than one of these option.
	
	:device_video_format HCLOCK HDE HRS HRE HT VDE VRS VRE VT

	Options:
		HCLOCK - Horizontal clock in Hz
		HDE HRS HRE HT VDE VRS VRE VT - Like the modeline option

	When a new modeline is created, AdvanceMAME uses a linear
	interpolation of the two formats with the nearest horizontal
	clock.

	The default value of this option is for an Arcade 15 kHz monitor:

		:15720 0.737 0.075 0.074 0.113 0.916 0.012 0.012 0.060

	and for an Arcade 25 kHz monitor:

		:25000 0.800 0.020 0.100 0.080 0.922 0.006 0.012 0.060

	and for a VGA 31.5 kHz monitor:

		:31500 0.800 0.020 0.120 0.060 0.914 0.019 0.004 0.063

	Which one of these defaults is used depends on the setting of the 
	`device_video_clock' option.

    device_video_singlescan/doublescan/interlace
	Limit the use of certain features.

	:device_video_singlescan yes | no
	:device_video_doublescan yes | no
	:device_video_interlace yes | no

	Options:
		yes - Permits the use of the feature if the
			low-end driver allows it (default).
		no - Disable completely the feature.

    device_color_palette8/br8/bgr15/bgr16/bgr24/bgr32/yuy2
	Limit the use of some bit depths. If you known that
	the program doesn't work well with a specific bit depth you
	can disable it.

	:device_color_palette8 yes | no
	:device_color_bgr8 yes | no
	:device_color_bgr15 yes | no
	:device_color_bgr16 yes | no
	:device_color_bgr24 yes | no
	:device_color_bgr32 yes | no
	:device_color_yuy2 yes | no

	Modes:
		palette8 - Palettized 8 bits mode.
		bgr8 - RGB 8 bits mode.
		bgr15 - RGB 15 bits mode.
		bgr16 - RGB 16 bits mode.
		bgr24 - RGB 24 bits mode.
		bgr32 - RGB 32 bits mode.
		yuy2 - YUV mode in the YUY2 format.

	Options:
		yes - Permits the use of the bit depth if the
			low-end driver allows it (default).
		no - Disable completely the bit depth.

    device_video_fastchange
	Enable or disable the fast video mode change. If enabled the
	current video mode is not reset before setting another video
	mode. The reset isn't generally required, but some
	limited DOS video BIOS need it. So, the fast change is disabled 
	for default.

	:device_video_fastchange yes | no

	Options:
		yes - Enable the fast video mode change.
		no - Disable the fast video mode change (default).

  fb Configuration Options
    device_hdmi/dpi_pclock_low
	Set the minimum pclock frequency used to drive the display. If a lower
	value is selected, transparently increases the video mode horizontal size
	until it reaches the allowed pixel clock.

	This option works only with Raspberry Pi, when using the FrameBuffer and
	the HDMI/DPI video interface.

	The use of this option is discouraged, it's present only for testing.

	:device_hdmi_pclock_low PCLOCK
	:device_dpi_pclock_low PCLOCK

	Options:
		PCLOCK - Pixel clock in Hz (default 0 for HDMI,
			and 31.25 MHz for DPI).

    device_fb_fastset
	Don't set the video mode if it's expected to be idential at the
	current one. This can be used to avoid a screen refresh
	when not required.

	:device_fb_fastset yes | no

	Options:
		yes - Don't set identical video mode.
		no - Always set the video mode (default).

  vbeline Configuration Options
	The following are the common video configuration options
	available only for the `vbeline' DOS video driver.

    device_vbeline_mode
	Select which `vbe' mode to use when generating `vbeline' modes.

	The use of this option is discouraged, it's present only for testing.

	:device_vbeline_mode smaller | bigger | ...

	Options:
		smaller - Use the biggest `vbe' mode contained in
			the `vbeline' mode (default).
		bigger - Use the smallest `vbe' mode which contains
			the `vbeline' mode.
		smaller_upto640 - Like `smaller' but not
			bigger than 640x480.
		bigger_upto640 - Like `bigger' but not
			bigger than 640x480.
		320 - Use always the 320x240 mode.
		400 - Use always the 400x300 mode.
		512 - Use always the 512x384 mode.
		640 - Use always the 640x480 mode.
		800 - Use always the 800x600 mode.

  svgaline Configuration Options
	The following are the common video configuration options
	available only for the `svgaline' DOS video driver.

    device_svgaline_skipboard
	Selects how many board skip in the video card detection. If you have
	more than a video card on your system you and the wrong one is
	used you can force to skip an arbitrary number of video boards.

	:device_svgaline_skipboard 0 | 1 | 2 | 3

	Options:
		0 - Don't skip any board (default).
		1 - Skip 1 boards.
		2 - Skip 2 boards.
		3 - Skip 3 boards.

	Examples:
		:device_svgaline_skipboard 1

    device_svgaline_divideclock
	Divide the pixelclock using the VGA sequencer. It should help to support 
	lower pixel clocks on some boards.

	The use of this option is discouraged, it's present only for testing.

	:device_svgaline_divideclock yes | no

	Options:
		yes - Divide the clock by 2.
		no - Don't divide the clock (default).
 
  svgawin Configuration Options
	The following are the common video configuration options
	available only for the `svgawin' Windows video driver.

    device_svgawin_skipboard
	Selects how many board skip in the video card detection. If you have
	more than a video card on your system you and the wrong one is
	used you can force to skip an arbitrary number of video boards.

	:device_svgawin_skipboard 0 | 1 | 2 | 3

	Options:
		0 - Don't skip any board (default).
		1 - Skip 1 boards.
		2 - Skip 2 boards.
		3 - Skip 3 boards.

	Examples:
		:device_svgawin_skipboard 1

    device_svgawin_stub
	Selects how the driver uses the Windows graphics.

	The use of this option is discouraged, it's present only for testing.

	Options:
		none - Don't use the Windows graphics support.
		window - Create a stub window before setting the video mode.
		fullscreen - Create a stub fullscreen window before setting
			the video mode (default).

    device_svgawin_divideclock
	Divides the pixelclock using the VGA sequencer. It should help to support
	lower pixel clocks on some boards.

	The use of this option is discouraged, it's present only for testing.

	:device_svgawin_divideclock yes | no

	Options:
		yes - Divide the clock by 2.
		no - Don't divide the clock (default).

Sound Drivers Configuration
    device_sound
	Specify the sound-card.

	:device_sound auto | none | DEVICE

	Options:
		none - No sound.
		auto - Automatic detection (default).

	Options for Linux:
		alsa - ALSA sound interface.
		oss - OSS sound interface.
		sdl - SDL sound interface.

	Options for Mac OS X:
		sdl - SDL sound interface.

	Options for DOS:
		seal - SEAL automatic detection.
		seal/sb - Sound Blaster.
		seal/pas - Pro Audio Spectrum.
		seal/gusmax - Gravis Ultrasound Max.
		seal/gus - Gravis Ultrasound.
		seal/wss - Windows Sound System.
		seal/ess - Ensoniq Soundscape.
		allegro - Allegro automatic detection.
		allegro/sb10 - Sound Blaster 1.0.
		allegro/sb15 - Sound Blaster 1.5.
		allegro/sb20 - Sound Blaster 2.0.
		allegro/sbpro - Sound Blaster Pro.
		allegro/sb16 - Sound Blaster 16.
		allegro/audio - Ensoniq AudioDrive.
		allegro/wss - Windows Sound System.
		allegro/ess - Ensoniq Soundscape.
		vsync/sb -  Sound Blaster.
		vsync/sbwin - Sound Blaster (Windows).
		vsync/ac97 - AC97.
		vsync/ac97win - AC97 (Windows).
		vsync/gusmax - Gravis Ultrasound Max.
		vsync/gus - Gravis Ultrasound.
		vsync/audio - Ensoniq AudioDrive.
		vsync/wss - Windows Sound System.
		vsync/ess - Ensoniq Soundscape.

	Options Windows:
		sdl - SDL sound interface.

  alsa Configuration Options
    device_alsa_device
	Select the alsa output device.

	:device_alsa_device DEVICE

	Options:
		DEVICE - Output device (default 'default').

	Other possible choices generally are 'hw:0,0' for using the
	frequency and format chosen directly by the hardware, or 'dmix'
	for allow concurrent access to other applications at the
	audio card.

	Example:
		:device_alsa_device dmix

	If you want to configure the ALSA library to remap the `default' device
	to the `dmix' device for all the applications, you can create
	the `.asoundrc' file in your home directory with the following content:

		:pcm.!default {
		:	type plug
		:	slave.pcm "dmixer"
		:}
		:
		:pcm.dmixer  {
		:	type dmix
		:	ipc_key 1024
		:	slave {
		:		pcm "hw:0,0"
		:		period_time 0
		:		period_size 1024
		:		buffer_size 16384
		:		rate 44100
		:	}
		:	bindings {
		:		0 0
		:		1 1
		:	}
		:}
		: 
		:ctl.dmixer {
		:	type hw
		:	card 0
		:}

	Note that the suggested `.asoundrc' on the ALSA web site has a lower
	`buffer_size' value (4096). For AdvanceMAME a bigger buffer is required.

    device_alsa_mixer
	Select the alsa mixer device.

	:device_alsa_mixer DEVICE

	Options:
		DEVICE - Mixer device. The special 'channel' value is
			used to adjust the volume changing the samples
			instead of using the card mixer. Other values
			like `default' are used to select the ALSA mixer.
			(default 'channel').

  sdl Configuration Options
    device_sdl_samples
	Select the size of the audio fragment of the SDL library.

	The use of this option is discouraged, it's present only for testing.

	device_sdl_samples 512 | 1024 | 2048 | 2560 | 3072 | 3584 | 4096 | 6144 | 8192

	Options:
		SAMPLES - Number of samples of an audio fragment
			(default 2048 in Windows, 512 otherwise).

	Lower values can be used to reduce the sound latency.
	Increase the value if your hear a choppy audio.

Input Drivers Configuration
    device_keyboard
	Selects the keyboard drivers.

	:device_keyboard auto | none

	Options:
		none - No keyboard.
		auto - Automatic detection (default).

	Options for Linux:
		event - Linux input-event interface.
		svgalib - SVGALIB keyboard interface.
		raw - Linux kernel keyboard interface.
		sdl - SDL keyboard interface. This driver is available
			only if the SDL video output is used.

	If you are using the SDL video driver you must also use the SDL
	keyboard driver.

	Options for Mac OS X:
		sdl - SDL keyboard interface.

	Options for DOS:
		allegro - Allegro keyboard interface.

	Options for Windows:
		sdl - SDL keyboard interface.

    device_joystick
	Selects the joystick driver.

	:device_joystick auto | none | DEVICE

	Options:
		none - No joystick (default).
		auto - Automatic detection.

	Options for Linux:
		event - Linux input-event interface.
		svgalib - SVGALIB joystick interface.
		raw - Linux kernel joystick interface.
		sdl - SDL joystick interface.

	Options for Mac OS X:
		sdl - SDL joystick interface.

	Options for DOS:
		allegro - Allegro automatic detection.
		allegro/standard - Standard joystick.
		allegro/dual - Dual joysticks.
		allegro/4button - 4-button joystick.
		allegro/6button - 6-button joystick.
		allegro/8button - 8-button joystick.
		allegro/fspro - CH Flightstick Pro.
		allegro/wingex - Logitech Wingman Extreme.
		allegro/sidewinder - Sidewinder.
		allegro/sidewinderag - Sidewinder Aggressive.
		allegro/gamepadpro - GamePad Pro.
		allegro/grip - GrIP.
		allegro/grip4 - GrIP 4-way.
		allegro/sneslpt1 - SNESpad LPT1.
		allegro/sneslpt2 - SNESpad LPT2.
		allegro/sneslpt3 - SNESpad LPT3.
		allegro/psxlpt1 - PSXpad LPT1.
		allegro/psxlpt2 - PSXpad LPT2.
		allegro/psxlpt3 - PSXpad LPT3.
		allegro/n64lpt1 - N64pad LPT1.
		allegro/n64lpt2 - N64pad LPT2.
		allegro/n64lpt3 - N64pad LPT3.
		allegro/db9lpt1 - DB9 LPT1.
		allegro/db9lpt2 - DB9 LPT2.
		allegro/db9lpt3 - DB9 LPT3.
		allegro/tgxlpt1 - TGX LPT1.
		allegro/tgxlpt2 - TGX LPT2.
		allegro/tgxlpt3 - TGX LPT3.
		allegro/segaisa - IF-SEGA/ISA.
		allegro/segapci - IF-SEGA2/PCI.
		allegro/segapcifast - IF-SEGA2/PCI.
		allegro/wingwarrior - Wingman Warrior.

	Options for Windows:
		sdl - SDL joystick interface.

	For the `allegro/psxlpt*', `allegro/db9lpt*', `allegro/tgxlpt*'
	drivers you can see the source file `advance/dos/jalleg.c'
	for instructions on how to build the lpt interface. See also
	the Allegro library source distribution.

    device_mouse
	Selects the mouse driver.

	:device_mouse auto | none

	Options:
		none - No mouse (default).
		auto - Automatic detection.

	Options for Linux:
		event - Linux input-event interface.
		svgalib - SVGALIB mouse interface.
		raw - Direct serial communication.
		sdl - SDL mouse interface.

	Options for Mac OS X:
		sdl - SDL mouse interface.

	Options for DOS:
		allegro - Allegro mouse interface.

	Options for Windows:
		sdl - SDL mouse interface.

  raw Configuration Options
    device_raw_mousetype[0,1,2,3]
	Select the type of the mouse.

	:device_raw_mousetype[0,1,2,3] auto | DEVICE

	Devices:
		auto - Auto detection. It works only sometimes, generally
			it's better to manually select the type.
		pnp - Plug And Play serial (3 buttons).
		ms - Microsoft Mouse serial (3 buttons).
		ms3 - Microsoft Intellimouse serial (3 buttons).
		ps2 - PS/2 (3 buttons).
		imps2 - Microsoft Intellimouse PS/2 (3 buttons).
		exps2 - Microsoft Intellimouse Explorer PS/2 (5 buttons).
		msc - Mouse System.
		mscgpm - Mouse System compatible with GPM.
		mman - Logitech MouseMan.
		mm - Logitech MM series
		logi - Logitech old protocol.
		bm - Bus Mouse.
		spaceball - Spacetec SpaceBall (6 buttons).
		wacomgraphire - Wacom Graphire tablet/mouse.
		drmousee4ds - Digital Research double-wheeled mouse.

	The Linux mice under /dev/input/* are always of type ps2,
	imps2 or exps2.

	Examples:
		:device_raw_mousetype[0] ps2
		:device_raw_mousetype[1] ms

    device_raw_mousedev[0,1,2,3]
	Select the mouse device to use.

	device_raw_mousedev[0,1,2,3] auto | DEVICE

	Options:
		auto - Automatically map to /dev/mouse or /dev/input/mouseX.
		DEVICE - Complete path of the mouse device.

	Examples:
		:device_raw_mousedev[0] /dev/input/mouse0
		:device_raw_mousedev[1] /dev/ttyS2

  lgrawinput Configuration Options
    device_lgrawinput_calibration
	Select the type of calibration of the lightgun.

	device_lgrawinput_calibration auto | none

	Options:
		auto - Automatically calibrates the lightgun before
			every use. You must move the lightgun over the
			whole screen every time the program starts or
			changes video mode (default).
		none - Never calibrates the lightguns, but it assumes
			that the returned values are already scaled in
			the range 0 - 65536.

Copyright
	This file is Copyright (C) 2003, 2004, 2005 Andrea Mazzoleni.

