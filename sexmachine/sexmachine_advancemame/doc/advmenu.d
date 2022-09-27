Name{number}
	advmenu - AdvanceMENU Frontend

Synopsis
	:advmenu [-default] [-remove] [-cfg FILE]
	:	[-log] [-version] [-help]

Description
	AdvanceMENU is a front-end to for AdvanceMAME, AdvanceMESS,
	and any other emulator.

	Simply run it in the same directory of the emulator and press
	`f1' to get the help screen or `~' for the main menu.

	Press `tab' to change the display mode. Press `space' to change
	the preview mode.

	To run a game press `enter'. Press `esc' to exit.

	The major features are:

	* Simply download and run. Copy the executable and run it!
	* Auto update of the rom info.
	* Vertical and horizontal orientation.
	* Support for any TV/Arcade Monitor like AdvanceMAME but it's
		good also for a normal PC monitor.
	* Static and Animated image and clip preview (PNG/PCX/ICO/MNG).
		Up to 192 images at the same time!
	* Sound preview. (MP3/WAV). You can select a special sound for
		every game played when the cursor move on it.
	* Sound backgrounds (MP3/WAV). Play your favorite songs or
		radio records in background.
	* Sound effects (MP3/WAV) for key press, program start, game
		start, program exit...
	* Support for zipped images and sounds archives.
	* Screen-saver. A slide show of the game images.
	* Selectable background and help images with translucency.

SubSubIndex

Options
	-default
		Add to the configuration file all the missing options
		with default values.

	-remove
		Remove from the configuration file all the options
		with default values.

	-cfg FILE
		Select an alternate configuration file. In Linux and Mac
		OS X the you can prefix the file name with "./" to
		load it from the current directory.

	-log
		Create the file `advmenu.log' with a lot of internal
		information. Very useful for bug report.

	-verbose
		Print some startup information.

	-version
		Print the version number, the low-level device drivers
		supported and the configuration directories.

	-help
		Print a short command line help.

	In Linux and Mac OS X you can also use `--' before options instead of `-'.
	In DOS and Windows you can also use `/'.

Emulators
	The program supports many type of emulators. The emulators
	AdvanceMAME, AdvanceMESS, MAME, SDLMAME, DMAME,
	DMESS and DRAINE are directly supported and the only thing
	you should do is to run the AdvanceMENU program in the same
	directory of the emulator.

	All the other emulators are supported with the emulator
	type `generic'.

  generic - Generic emulator
	For the `generic' emulator no additional rom information is
	needed. Only the name and the size of the rom files are used.

	You should specify all emulator information and directories
	with the `emulator' and `emulator_*' options in the
	`advmenu.rc' file.

	You need to use at least the `emulator' and `emulator_roms'
	options to inform AdvanceMENU how to run the emulator and where
	to find the roms.

	For example:
		:emulator "snes9x" generic "c:\game\snes9x\snes9x.exe" "%f"
		:emulator_roms "snes9x" "c:\game\snes9x\roms"
		:emulator_roms_filter "snes9x" "*.smc;*.sfc;*.fig;*.1"

		:emulator "zsnes" generic "c:\game\zsnes\zsnes.exe" "-e -m roms\%f"
		:emulator_roms "zsnes" "c:\game\zsnes\roms"
		:emulator_roms_filter "zsnes" "*.smc;*.sfc;*.fig;*.1"

	The various %s, %f, %p, ... macros are explained in the `emulator'
	option description.

	The roms are searched in the path specified with the
	`emulator_roms' option in the `advmenu.rc' file. For every file
	found (with any extension) a game is added in the menu.
	You can filter the files with the `emulator_roms_filter' option.

	All the snapshots are searched in the directories specified with
	the `emulator_*' options using the same name of the rom file.

	If you want, you can manually write a MAME like information file
	and name it as `ENUNAME.lst'. This file has the same format
	of the output of the `-listinfo' MAME command.
	Actually only the information `game', `name', `description', `year',
	`manufacturer', `cloneof' are used.
	Please note that this file is used only to add information at the
	existing games. The games present in this file are not automatically
	added at the game list.

  advmame - AdvanceMAME
	For the `advmame' emulator type the roms information is
	gathered from the file `ENUNAME.xml'. If this file doesn't
	exist, it's created automatically with emulator `-listxml'
	command.

	The directories specified in the `dir_rom' option in the
	`advmame.rc' file are used to detect the list of the
	available roms. In the DOS and Windows versions of the
	program the `advmenu.rc' file is searched in the same directory
	of the emulator. In the Unix version it's searched in the
	`HOME/.advance' directory.

	The directory specified in `dir_snap' is used to
	detect the list of available snapshots.

  advmess - AdvanceMESS
	For the `advmess' emulator the rom information is gathered
	from the file `EMUNAME.xml'. If this file doesn't exist,
	it's created automatically with emulator `-listxml' command.

	The directories specified in the `dir_rom' option in the
	`advmess.rc' file are used to detect the list of the
	available bioses.

	All the directories listed in the option `dir_image' are
	read and all the files found in the `machine' directories
	are inserted as software if the extension is recognized as a
	valid device extension for the current `machine' or if it's a
	`zip' file.
	For example if the `dir_image' options is `c:\software',
	AdvanceMENU scans the directories `c:\software\ti99_4a',
	`c:\software\sms', `c:\software\gameboy'...
	Files in the main directory `c:\software' are NOT checked.

	When you select to run a zip file, the zip file is opened and
	all the files in the zip with a valid name and with a recognized
	device extension are added at the AdvanceMESS command line.
	A file is considered to have a valid name if has the same name
	of the zip of if it has the name of the zip with an additional
	char. For example in the file `alpiner.zip' the files
	`alpiner.bin', `alpinerc.bin' and `alpinerg.bin' have a valid
	name. This feature can be used to group all the required roms
	to play a software in a single zip file.

	The file extension is also used to correctly select the device
	type when calling AdvanceMESS.

	The directory specified in `dir_snap' is used to detect the
	list of available snapshots.
	At any exit of AdvanceMESS if a new snapshot is created, this
	file is moved to the correct `snap\system' directory renaming
	it as the software started.
	For example, suppose that you run the `ti99_4a' system with the
	software `alpiner'. If you press F12 during the emulation, the
	file `snap\ti99_4a.png' is created. When you return to
	AdvanceMENU the file is moved automatically to
	`snap\ti99_4a\alpiner.png'.

  mame - Windows MAME
	For the `mame' emulator the roms information is gathered from
	the file `EMUNAME.xml'. If this file doesn't exist, it's created
	automatically with emulator `-listxml' command.

	The directories specified in the `rompath' option in the
	`mame.ini' file are used to detect the list of the available
	roms.

	The directory specified in `snapshot_directory' is
	used to detect the list of available snapshots.

  dmame - DOS MAME
	For the `dmame' emulator the roms information is gathered
	from the file `EMUNAME.xml'. If this file doesn't exist, it's
	created automatically with emulator `-listxml' command.

	The directories specified in the `rompath' option in the
	`mame.cfg' file are used to detect the list of the available
	roms.

	The directory specified in `snap' is used to
	detect the list of available snapshots.

  dmess - DOS MESS
	For the `dmess' emulator the roms information is gathered
	from the file `EMUNAME.xml'. If this file doesn't exist, it's
	created automatically with emulator `-listxml' command.

	The directories specified in the `biospath' option in the
	`mess.cfg' file are used to detect the list of the available
	bioses.

	All the directories listed in the option `softwarepath' are
	read and all the `zip' files found in the `subsystem'
	directories are inserted as software.
	For example if the `softwarepath' options is `c:\software',
	AdvanceMENU scans the directories `c:\software\ti99_4a',
	`c:\software\sms', `c:\software\gameboy'...
	Zips in the main directory `c:\software' are NOT checked.

	When you select one of these entries the zip is opened and is
	searched the first file with the same name of the zip but
	different extension. This file is used as the argument of the
	`-cart' option when running `mess'.
	AdvanceMENU is NOT able to use other supports like `-flop'.

	All the aliases present if the `mess.cfg' are inserted as
	software entries. When you select one of these entries the
	`mess' option `-alias' is used to start the game.

	You can set an arbitrary description on an alias specification
	adding it on the same line of the alias after the comment char
	'#' using this format:

		:ALIAS = ALIAS_DEF # Description | YEAR | MANUFACTURER

	For example:
		:[ti99_4a]
		:ti-inva = -cart ti-invac.bin -cart ti-invag.bin \
		:	# Invaders | 1982 | Texas Instrument

	At any exit of the emulator if a new snapshot is created, this file
	is moved to the correct `snap\system' directory renaming it as
	the software started.
	For example, suppose that you run the `ti99_4a' system with the
	software `alpiner'. If you press F12 during the emulation, the
	file `snap\ti99_4a.png' is created. When you return to
	AdvanceMENU the file is moved automatically to
	`snap\ti99_4a\alpiner.png'.

  draine - DOS Raine
	For the `draine' emulator the roms information is gathered
	from the file `EMUNAME.lst'. If this file doesn't exist, it's
	created automatically with emulator `-gameinfo' command.

	All the directories specified in the `rom_dir_*' options are
	used to detect the list of the available roms.

	The directory specified in `screenshots' is used to detect the
	list of available snapshots.

Configuration
	The file `advmenu.rc' is used to save the current state of the
	front-end. It's read at startup and saved at exit. You can
	prevent the automatic save at the exit with the `config' option.

	In DOS and Windows the configuration options are read from the
	file `advmenu.rc' in the current directory.

	In Linux and Mac OS X the configuration options are read from the
	files `advmame.rc' and `advmess.rc' in the $host, $data and
	the $home directory.
	The $host directory is `$SYSCONFDIR', where $SYSCONFDIR is the
	`sysconfdir' directory configured with the `configure' script.
	The default is `/usr/local/etc'.
	The $data directory is `$DATADIR/advance', where $DATADIR is the
	`datadir' directory configured with the `configure' script.
	The default is `/usr/local/share'.
	The $home directory is `$ADVANCE', where $ADVANCE is the value of the
	ADVANCE environment variable when the program is run.
	If the ADVANCE environment variable is missing the $home directory
	is `$HOME/.advance' where $HOME is the value of the HOME environment
	variable.
	If both the ADVANCE and HOME environment variables are missing the
	$data directory became also the $home directory.

	The priority of the options is in the order: $host, $home and $data.

	The $home directory is also used to write all the information
	by the program. The files in the $host and $data directory are only read.

	You can include an additional configuration files with the `include'
	option. In DOS and Windows the files are searched in the current directory.
	In Linux and Mac OS X the files are searched in the $home directory if
	they are expressed as a relative path. You can force the search in the
	current directory prefixing the file with `./'.
	To include more than one file you must divide the names with `;' in
	DOS and Windows, and with `:' in Linux and Mac OS X.

	You can force the creation of a default configuration file with the
	command line option `-default'.

	In DOS and Windows the directory name separator is `\' and the
	multi-directory separator is `;'. In Linux and Mac OS X the directory
	name separator is `/' and the multi-directory separator is `:'.

  Global Configuration Options
	This section describes the global options used to
	customize the the program.

	A subset of the configuration options are saved per
	emulator basis to allow different configurations for
	different emulators. Please note that these specific emulator
	configurations are not activated if you select to show
	more than one emulator at time.
	In this case only the default configuration is used.
	Specifically these special options are `mode', `sort',
	`preview', `group_include' and `type_include'.

    config
	Selects if and when the configuration modified by the user at
	runtime should be saved.

	:config save_at_exit | restore_at_exit | restore_at_idle

	Options:
		save_at_exit - Save any changes before exiting
			(default).
		restore_at_exit - Don't save the changes. At the next
			run, restore the previous configuration.
		restore_at_idle - Restore the previous configuration
			after the `idle' time.

	You can manually save the configuration at runtime from the
	main menu.

    emulator
	Selects the emulators to list in the menu. You can specify more than
	one emulator.

	WARNING! Before playing with this option, you should do a
	backup copy of your current `advmenu.rc' because when you remove
	an emulator, the game information for that emulator (like
	the time played) is lost.

	:emulator "EMULATOR" (generic | advmame | advmess | mame | dmame
	:	| dmess | draine) "[-]EXECUTABLE" "ARGUMENTS"

	Options:
		EMULATOR - The name for the emulator. It must be
			different for every emulator defined.
		generic - It's a generic emulator.
		advmame - It's the AdvanceMAME emulator.
		advmess - It's the AdvanceMESS emulator.
		mame - It's the Window MAME emulator.
		dmame - It's the DOS MAME emulator.
		dmess - It's the DOS MESS emulator.
		draine - It's the DOS Raine emulator.
		[-]EXECUTABLE - The executable path of the emulator.
			In DOS and Windows you can also use a batch (.bat)
			file, but this prevent the automatic generation of
			the listing file which must be generated manually.
			You can put a `-' in front of the file path
			to ignore any error returned by the executable.
		ARGUMENTS - The arguments to be passed to the emulator.
			The arguments are required only for the `generic'
			emulator. For the others, AdvanceMENU automatically
			adds the required arguments to run a
			game. However, you may wish to add extra
			arguments.

	In the emulator arguments some macros are substituted
	with some special values:
		%s - Expanded as the game name. For example "pacman".
		%p - Expanded as the complete path of the rom. For
			example "c:\emu\roms\pacman.zip".
		%f - Expanded as the rom name with the extension. For
			example "pacman.zip".
		%o[R0,R90,R180,R270] - Expanded as one of the R* string,
			depending on the current menu orientation.
			Note that you cannot use space in the R* string.
			For example "%o[,-ror,-flipx,-rol] %o[,,-flipy,]"
			correctly rotate the AdvanceMAME emulator.

	For the `generic' emulator type you need use the % macros
	to tell at the emulator which game run. For all the other emulator
	types this information is automatically added by AdvanceMENU.

	Examples for DOS and Windows:
		:emulator "AdvanceMAME" advmame "advmame\advmame.exe" \
		:	"%o[,-ror,-flipx,-rol] %o[,,-flipy,]"
		:emulator "MAME" mame "mame\mame.exe" "-nohws"
		:emulator "MESS" dmess "mess\mess.exe" ""
		:emulator "Raine" draine "raine\raine.exe" ""
		:emulator "Custom Raine" draine "raine\raine2.bat" ""
		:emulator "SNes9x" generic "c:\game\snes9x\snes9x.exe" "%f"
		:emulator "ZSNes" generic "c:\game\zsnes\zsnes.exe" "-e -m roms\%f"

	Examples for Linux and Mac OS X:
		:emulator "AdvanceMAME" advmame "advmame" \
		:	"%o[,-ror,-flipx,-rol] %o[,,-flipy,]"

    emulator_roms/roms_filter/altss/flyers/cabinets/icons/titles
	Selects additional directories for the emulators. These
	directories are used in addition to any other directory
	defined in the emulator config file. The preview images and
	sounds files are also searched also in any `.zip' file present
	in these directories.

	:emulator_roms "EMULATOR" "LIST"
	:emulator_roms_filter "EMULATOR" "LIST"
	:emulator_altss "EMULATOR" "LIST"
	:emulator_flyers "EMULATOR" "LIST"
	:emulator_cabinets "EMULATOR" "LIST"
	:emulator_marquees "EMULATOR" "LIST"
	:emulator_icons "EMULATOR" "LIST"
	:emulator_titles "EMULATOR" "LIST"

	Commands:
		roms - List of directories used for the roms. This
			option is used only for the `generic' emulator
			type. All the other emulators use the
			emulator-specific config file to set the rom
			path.
		roms_filter - List of pattern for the file to list.
			An empty pattern means all files.
		altss - Snapshot directory, used for snap images and
			sounds. If possible, the directories
			specified in the emulator configuration file
			are also used.
		flyers - Flyers directory.
		cabinets - Cabinets directory.
		marquees - Marquees directory.
		icons - Icons directory.
		titles - Titles directory.

	Options:
		EMULATOR - The name for the emulator. Must be the same
			name of a defined emulator
		LIST - List of directories or patterns. In DOS and Windows
			use the `;' char as separator. In Linux and
			Mac OS X use the `:' char.

	Examples for DOS and Windows:
		:emulator_roms "SNes9x" "c:\game\snes9x\roms;c:\game\zsnes\roms2"
		:emulator_roms_filter "SNes9x" "*.smc;*.sfc;*.fig;*.1"
		:emulator_flyers "SNes9x" "c:\game\zsnes\fly"
		:emulator_cabinets "SNes9x" "c:\game\zsnes\cab"
		:emulator_marquees "SNes9x" "c:\game\zsnes\mar"
		:emulator_roms "ZSNes" "c:\game\zsnes\roms"
		:emulator_roms_filter "ZSNes" "*.smc;*.sfc;*.fig;*.1"

    mode
	Selects the menu listing mode.

	:[EMULATOR/]mode full | full_mixed | text | list | list_mixed
	:	| tile_tiny | tile_small | tile_normal
	:	| tile_big | tile_enormous | tile_giant
	:	| tile_icon | tile_marquee

	Options:
		EMULATOR/ - Nothing for the default value, or an emulator
			name for a specific emulator option.
		full - Full screen preview.
		full_mixed - Full screen preview with 4 images.
		text - Game list.
		list - Game list and preview of the selected game (default).
		list_mixed - Game list and 4 preview of the selected game.
		tile_tiny - Show the preview of 3x2 games.
		tile_small - Show the preview of 4x3 games.
		tile_normal - Show the preview of 5x6 games.
		tile_big - Show the preview of 8x6 games.
		tile_enormous - Show the preview of 12x9 games.
		tile_giant - Show the preview of 16x12 games.
		tile_icon - Special mode for icon preview.
		tile_marquee - Special mode for marquee preview.

    mode_skip
	Disables some menu modes when you press `tab'.

	:mode_skip (full | full_mixed | text | list | list_mixed
	:	| tile_small | tile_normal | tile_big | tile_enormous
	:	| tile_giant | tile_icon | tile_marquee)*

	Options:
		SKIP - Multiple selections of disabled modes. Use
			an empty list to enable all the modes.

	Examples:
		:mode_skip tile_giant
		:mode_skip full full_mixed list tile_small tile_giant
		:mode_skip

    sort
	Selects the sort order of the games displayed.

	:[EMULATOR/]sort parent | name | time | smart_time | play | year | manufacturer
	:	| type | group | size | resolution | info

	Options:
		EMULATOR/ - Nothing for the default value, or an emulator
			name for a specific emulator option.
		parent - Game parent name (default).
		emulator - Emulator name.
		name - Game name.
		time - Total Time played.
		smart_time - Time played ignoring the first 20 minutes.
		play - Total Play times.
		timeperplay - Time per play.
		year - Game year release.
		manufacturer - Game manufacturer.
		type - Game type.
		group - Game group.
		size - Size of the game rom.
		resolution - Resolution of the game display.
		info - External information imported with the
			`info_import' option.

    preview
	Selects the type of the images displayed.

	:[EMULATOR/]preview snap | titles | flyers | cabinets

	Options:
		EMULATOR/ - Nothing for the default value, or an emulator
			name for a specific emulator option.
		snap - Files in the `snap' and `altss' dir.
		flyers - Files in the `flyers' dir.
		cabinets - Files in the `cabinets' dir.
		titles - Files in the `titles' dir.

	The `icons' and `marquees' images can be selected with the
	special `mode' options `tile_icon' and `tile_marquee'.

    preview_expand
	Enlarges the screen area used by the vertical games on horizontal
	tile (and horizontal games in vertical tile).

	:preview_expand FACTOR

	Options:
		FACTOR - Expansion float factor from 1.0 to 3.0
			(default 1.15)

	Examples:
		:preview_expand 1.15

    preview_default_*
	Selects the default images. When an image for the selected game
	is not found, a default image can be displayed.

	:preview_default "FILE"
	:preview_default_snap "FILE"
	:preview_default_flyer "FILE"
	:preview_default_cabinet "FILE"
	:preview_default_icon "FILE"
	:preview_default_marquee "FILE"
	:preview_default_title "FILE"

	Commands:
		default - Selects the default image for all preview
			modes.
		default_TAG - Selects the default image for a single
			preview mode.

	Options:
		FILE - The complete PATH of the image.

	Examples:
		:preview_default "C:\MAME\DEFAULT.PNG"
		:preview_default_marquee "C:\MAME\DEFMAR.PNG"
		:preview_default_icon "C:\MAME\DEFICO.ICO"

    icon_space
	Selects the space size between icons. The `icon' mode is
	available only if you set the option `emulator_icons' in the
	emulator config file.

	:icon_space SPACE

	Options:
		SPACE - The number of pixel between icons (default 43).

	In the icon display the game title is displayed in multiple rows
	if there is enough space.

    merge
	Selects the expected format of your romset. It's used to test
	the existence of the correct zips needed to run the games.

	:merge none | differential | parent | any | disable

	Options:
		none - Every clone zip contains all the needed roms.
		differential - Every clone zip contains only
			the unique roms (default).
		parent - All the roms are in the parent zip.
		any - Any of the above, use this if you have
			a rom set that is organized poorly.
		disable - Check disabled.

    game
	Contains various information of the know games.
	A `game' option is added automatically at the configuration
	files for any rom found. It's used to keep some game
	information like the play time.

	:game "EMULATOR/GAME" "GROUP" "TYPE" TIME PLAY "DESC"

	Options:
		EMULATOR - Name of the emulator.
		GAME - Short name of the game, generally the
			rom name without the extension.
		GROUP - Name of the group of the game or empty "".
		TYPE - Name of the type of the game or empty "".
		TIME - Time played in seconds.
		PLAY - Number of play.
		DESC - User description or empty "".

	The GROUP, TYPE and DESC argument overwrite any
	other value imported with the `group_import', `type_import',
	and `desc_import' options. The imported values take effect
	only if the user GROUP, TYPE and DESC are empty.

	Examples:
		:game "advmame/puckman" "Very Good" "Arcade" \
		:	1231 21 "Pac-Man Japanese"
		:game "advmame/1943" "" "" 121 4 "1943 !!"

  Display Configuration Options
	This section describes the options used to customize the display.

    device_video_*
	These options are used to customize the video drivers.

	All the `device_video_*' options defined in the `advdev.txt' file can
	be used.

	If you use a `System' video driver, you don't need to set these
	options. They are mostly ignored.

	With a `Generate' video drivers these options are used to select
	and create the correct video mode. If missing the settings for a
	standard Multisync SVGA monitor are used.

    display_size
	Selects the desired size of the video mode.

	:display_size auto | WIDTHxHEIGHT

	Options:
		auto - Try to autodetect the best size. Usually the current
			one (default).
		WIDTHxHEIGHT - Size in pixels of the video mode. The nearest
			available video mode is chosen (default 1280x1024).

	Examples:
		:display_size 1600x1200

    display_resizeeffect
	Control the scaling transformation applied to screenshots.

	:display_resizeeffect auto | none | scalex | scalek | hq | xbr

	Options:
		auto - Selects automatically the best effect (default).
		none - Simply removes or duplicates lines as required.
		scalex - It adds the missing pixels matching the
			original bitmap pattern.
			It uses a 3x3 mapping analysis with 4 comparisons.
			It doesn't interpolate pixels and it compares colors
			for equality.
		scalek - It adds the missing pixels matching the
			original bitmap pattern.
			It uses a 3x3 mapping analysis with 4 comparisons.
			It interpolates pixels and it compares colors
			for equality.
		hq - It adds the missing pixels matching the
			original bitmap pattern.
			It uses a 3x3 mapping analysis with 8 comparisons.
			It interpolates pixels and it compares colors
			for distance.
		xbr - It adds the missing pixels matching the
			original bitmap pattern.
			It uses a 5x5 mapping analysis with a gradient estimation.
			It interpolates pixels and it compares colors
			for distance.

    display_restoreatgame
	Selects whether to reset the video mode before running the
	emulator.

	:[EMULATOR/]display_restoreatgame yes | no

	Options:
		EMULATOR/ - Nothing for the default value, or an emulator
			name for a specific emulator option.
		yes - Reset the video mode (default).
		no - Maintain the current graphics mode.

    display_restoreatexit
	Selects whether to reset the video mode before exiting.

	:display_restoreatexit yes | no

	Options:
		yes - Reset the video mode (default).
		no - Maintain the current graphics mode.

    display_orientation
	Selects the desired orientation of the screen.

	:display_orientation (flip_xy | mirror_x | mirror_y)*

	Options:
		mirror_x - Mirror in the horizontal direction.
		mirror_y - Mirror in the vertical direction.
		flip_xy - Swap the x and y axes.

	Examples:
		:display_orientation flip_xy mirror_x

    display_brightness
	Selects the image brightness factor.

	:display_brightness FACTOR

	Options:
		FACTOR - Brightness float factor (default 1.0).

	Examples:
		:display_brightness 0.9

    display_gamma
	Selects the image gamma correction factor.

	:display_gamma FACTOR

	Options:
		FACTOR - Gamma float factor (default 1.0).

	Examples:
		:display_gamma 0.9

  Sound Configuration Options
	This section describes the options used to customize the sound.

    device_sound_*
	These options are used to customize the audio drivers.

	All the `device_sound_*' options defined in the `advdev.txt' file can
	be used.

    sound_volume
	Sets the global sound volume.

	:sound_volume VOLUME

	Options:
		VOLUME - The volume attenuation in dB (default -3).
			The attenuation is a negative value from -40 to 0.

	Examples:
		:sound_volume -5

    sound_latency
	Sets the audio latency. The latency is the delay between a
	sound is generated and before you hear it.
	You generally want a small latency, but a bigger latency
	avoids sound clicks in case the audio reproduction is too slow.

	:sound_latency TIME

	Options:
		TIME - Latency in seconds from 0.01 to 2.0.
			(default 0.1)

	Increase the value if your hear a choppy audio.

    sound_buffer
	Sets the size of the look-ahead audio buffer for decoding.

	:sound_buffer TIME

	Options:
		TIME - Buffer size in seconds from 0.05 to 2.0.
			(default 0.1)

	Increase the value if your hear a choppy audio.

    sound_foreground_EVENT
	Selects the sounds played in foreground for the various events.

	:sound_foreground_begin none | default | FILE
	:sound_foreground_end none | default | FILE
	:sound_foreground_key none | default | FILE
	:sound_foreground_start none | default | FILE
	:sound_foreground_stop none | default | FILE

	Commands:
		begin - Sound played at AdvanceMENU startup.
		end - Sound played at AdvanceMENU exit.
		start - Sound played at emulator startup.
		stop - Sound played at emulator exit.
		key - Sound played when a key is pressed.

	Options:
		none - No sound.
		default - Use the default sound.
		FILE - Path of the sound file (.wav or .mp3).

    sound_background_EVENT
	Selects the sounds played in background for the various events.

	:sound_background_begin none | FILE
	:sound_background_end none | FILE
	:sound_background_start none | FILE
	:sound_background_stop none | FILE
	:sound_background_loop none | default | FILE

	Commands:
		begin - Sound played at AdvanceMENU startup.
		end - Sound played at AdvanceMENU exit.
		start - Sound played at emulator startup.
		stop - Sound played at emulator exit.
		loop - Sound played in loop if no other background
			sound is available.

	Options:
		none - No sound
		default - Use the default sound
		FILE - Path of the sound file (.wav or .mp3)

    sound_background_loop_dir
	Selects the background music directory to search for MP3 and WAV
	files. Music tracks will be played in random order.

	Multiple directories may be specified by separating each with a
	semicolon `;' in DOS and Windows, with a double-colon `:' in Linux
	and Mac OS X.

	Note that this directory must be used only for your music.
	The emulated game recordings, played when the cursor is moved on
	the game, are stored in the snap directory defined in the emulator
	configuration file or with the `emulator_altss' option.

	:sound_background_loop_dir "DIR"

	Options:
		DIR - Directory for .mp3 and .wav files.

	Examples:
		:sound_background_loop_dir C:\MP3\POP;C:\MP3\ROCK

  Input Configuration Options
	This section describes the options used to customize the user
	input.

    device_keyboard/joystick/mouse
	These options are used to customize the input drivers.

	All the `device_keyboard/joystick/mouse_*' options defined in
	the `advdev.txt' file can be used.

	As default the mouse and the joystick support is disabled.
	To enable it you must add the options:

		:device_mouse auto
		:device_joystick auto

	in your advmenu.rc file.

    mouse_delta
	Selects the mouse/trackball sensitivity. Increase the value for
	a slower movement. Decrease it for a faster movement.

	:mouse_delta STEP

	Options:
		STEP - Mouse/trackball position step (default 100).

  User Interface
	This section describes the options used to customize the user
	interface.

    ui_autocalib
	Enables or disables the auto joystick calibration menu. If enabled and no joystick
	is found, the calibration menu automatically starts.

	If any keypress is detected this mechanism is automatically disabled, and you
	have to start the calibration menu manually.

	This is intended for a joustick only configuration.

	:ui_autocalib yes | no

    ui_text/bar_font
	Selects a font file for normal text and the title bar.
	The formats: TrueType (TTF), GRX, PSF and RAW are supported.
	You can find a collection of fonts in the `contrib' directory.

	:ui_text_font auto | "FILE"
	:ui_bar_font auto | "FILE"

	Options:
		auto - Use the built-in font (default).
		FILE - Font file path.

	The TrueType (TTF) format is supported only if the program is
	compiled with the FreeType2 library.

    ui_text/bar_size
	Selects the font size, if the specified font is scalable.
	The size is expressed in number of rows and columns of text in the
	screen.

	:ui_text_size auto | ROWS [COLS]
	:ui_bar_size auto | ROWS [COLS]

	Options:
		auto - Automatically compute the size (default).
		ROWS - Number of text rows.
		COLS - Number of text columns. If omitted is computed from
			the number of rows.

    ui_background
	Defines a background image in .PNG or MNG format. The image is
	stretched to fit the screen.

	ui_background FILE | none

	Options:
		none - No image (default).
		FILE - File in .PNG or .MNG format to load.

	For .MNG files only the first frame is used.

    ui_exit
	Defines an exit image/clip in .PNG or .MNG format displayed
	at the frontexit exit. The image is stretched to fit the screen.

	ui_exit FILE | none

	Options:
		none - No image (default).
		FILE - File in .PNG or .MNG format to load.

    ui_startup
	Defines a startup image/clip in .PNG or .MNG format displayed
	at the frontend startup. The image is stretched to fit the screen.

	ui_startup FILE | none

	Options:
		none - No image (default).
		FILE - File in .PNG or .MNG format to load.

    ui_help
	Defines an help image/clip in .PNG or .MNG format displayed when the
	user press F1. The image is stretched to fit the screen.

	ui_help FILE | none

	Options:
		none - No image (default).
		FILE - File in .PNG or .MNG format to load.

    ui_gamemsg
	One line message displayed when a game is chosen. The
	message is displayed only if the option `display_restoreatgame' is
	set to `no'.

	:ui_gamemsg "MESSAGE"

	Options:
		MESSAGE - Message to display (default "Run Game").
			To prevent the display of the message use the
			empty string "".

	Examples:
		:ui_gamemsg "Avvio il gioco..."

    ui_game
	Selects the preview type to display when a game is run. The
	preview is displayed only if the option `display_restoreatgame' is
	set to `no'.

	:ui_game none | snap | flyers | cabinets | titles

	Options:
		none - Don't display any preview.
		snap, flyers, cabinets, titles - Display the
			specified preview. (default snap).

    ui_skiptop/bottom/left/right
	Defines the border area of the screen not used by the menu.
	Generally it's the part of the screen used by the background image.
	If a `ui_background' image is specified these values refer at image
	size before stretching, otherwise they refer at the current video
	mode size.

	ui_skiptop N
	ui_skipbottom N
	ui_skipleft N
	ui_skipright N

	Options:
		N - Number of pixel to skip (default 0).

    ui_skiphorz/vert
	Defines the inner horizontal and vertical area between tiles.

	ui_skiphorz auto | N
	ui_skipvert auto | N

	Options:
		auto - Auto select depending on the tile mode and font size (default).
		N - Number of pixel to skip.

    ui_topbar/bottombar/scrollbar/outline
	Enables or disables the top, bottom and scroll information bars.
	The outline is the thin box around backdrops.

	ui_topbar yes | no
	ui_bottombar yes | no
	ui_scrollbar yes | no
	ui_outline yes | no

    ui_topname
	Puts the game name at the top instead of at the bottom.

	ui_topname yes | no

    ui_name
	Enables or disables the display of the game name in the screensaver.

	ui_name yes | no

    ui_color
	Selects the user interface colors.

	:ui_color TAG FOREGROUND BACKGROUND

	Tags:
		menu_item - Game menu entry.
		menu_hidden - Game menu hidden entry.
		menu_tag - Game menu highlight entry.
		menu_item_select - Game menu selected entry.
		menu_hidden_select - Game menu hidden selected entry.
		menu_tag_select - Game menu selected highlight.
		bar - Top and bottom bars.
		bar_tag - Top and bottom bars highlight.
		bar_hidden - Top and bottom bars hidden text.
		grid - Scrollbar marker and generic background color.
		overscan - Overscan area controlled by skip_top/bottom/left/right.
		backdrop - Backdrop outline and backdrop border/missing.
		help - Help.
		help_tag - Help highlight.
		submenu_bar - Submenu title.
		submenu_item - Submenu entry.
		submenu_item_select - Submenu selected entry.
		submenu_hidden - Submenu hidden entry.
		submenu_hidden_select - Submenu selected hidden entry.
		icon - Icon outline and missing icon.
		cursor - Flashing cursor. Use equal foreground and background to
			avoid flashing.

	Options:
		FOREGROUND - Foreground color in RRGGBB
			hex format. For example FF0000 is red,
			00FF00 is green and 0000FF is blue.
		BACKGROUND - Background color. Like foreground color.

    ui_clip
	Selects how play the video clips.

	:ui_clip none | single | singleloop | multi | multiloop | multiloopall

	Options:
		none - No clip.
		single - Play only one clip and only one time.
		singleloop - Play only one clip continuously. The sound is
			not looped.
		multi - Play all the clips.
		multiloop - Play all the clips, and loop the clip on the
			cursor. The sound is not looped.
		multiloopall - Play all the clips, and loop all the clips.
			The sound is not looped (default).

    ui_translucency
	Selects the translucency of the user interface.

	ui_translucency FACTOR

	Options:
		FACTOR - Translucency factor from 0 to 1
			(default 0.6).

	The translucency has effect only if you have a background
	image.

    ui_command
	Defines the user commands. These commands are executed as
	shell scripts. The video mode is not changed, so they must be
	silent.

	ui_command "MENU" SCRIPT

	Options:
		MENU - Name of the menu entry.
		SCRIPT - Commands to execute. If you need to insert more
			command rows you can end the line with the \ char.

	In the script text some macro are substituted with information of
	the selected game:
		%s - The game name. For example "pacman".
		%p - The complete path of the rom. For
			example "c:\emu\roms\pacman.zip".
		%f - The rom name with the extension. For
			example "pacman.zip".

	If no game is selected the macros aren't substituted.

	If the script exits with an error code, a message is displayed.

	Examples:
		:ui_command "Delete Hiscore" \
		:	rm ~/.advance/hi/%s.hi
		:ui_command "Enable GamePad" \
		:	rmmod analog \
		:	sleep 1 \
		:	modprobe analog js=gamepad

    ui_command_menu
	Selects the name of the menu entry for the commands submenu.

	ui_command_menu MENU

	Options:
		MENU - Name of the menu entry (default "Command").

    ui_command_error
	Selects the message to display if a command fails.

	ui_command_error MSG

	Options:
		MSG - Message to display (default "Error running the command").

    ui_menukey
	Enables or disables the key names in the menu entries.

	ui_menukey yes | no

    ui_console
	Changes the user interface behavior for the use on a game
	console system.

	ui_console yes | no

	In console mode the menu is reduced to contains only the
	minimal commands, and the sound volume is propagated to the
	emulators if possible.

  Input Configuration Options
	This section describes the options used to customize the user
	input.

    device_keyboard/joystick/mouse_*
	These options are used to customize the input drivers.

	All the `device_keyboard/joystick/mouse_*' options defined in
	the `advdev.txt' file can be used.

    input_hotkey
	Enables or disables the recognition of the special OS keyboard
	sequences.

	:input_hotkey yes | no

	Options:
		no - No hot key recognition.
		yes - Hot key recognition (default).

	In DOS the hotkey recognized are:
		CTRL+ALT+DEL - Reset.
		CTRL+ALT+END - Quit.
		CTRL+BREAK (Pause) - Break.

	In Linux the hotkey recognized generally are:
		CTRL+C - Break.
		ALT+Fx - Change virtual console.

    lock
	Locks or unlocks the user interface. When locked, the user can
	only browse and run games. Options can't be changed and the user
	cannot exit with the generic 'esc' event.

	You can anyway allow the user to exit or shutdown, changing the
	`misc_exit' option.

	:lock yes | no

	Options:
		yes - Locked mode activate.
		no - Locked mode deactivate (default).

    event_assign
	Customizes the input keyboard codes that trigger menu
	events.

	:event_assign EVENT EXPRESSION

	Events:
		up, down, left, right - Movement.
		home, end, pgup, pgdn - Movement.
		enter - Main action, start.
		esc - Back action, exit & cancel.
		space - Change action, select & deselect.
		ins - Select all.
		del - Deselect all.
		menu - The main menu.
		sort - Changes the sort mode.
		mode - Change the display mode.
		preview - Change the preview mode.
		emulator - The emulator menu.
		help - Show a little help.
		group - Select a game group.
		type - Select a game type.
		exclude - Exclude some games with filters.
		setgroup - Select the group of the current game
		settype - Select the type of the current game
		runclone - Run a game clone.
		exit - Exit from the application.
		shutdown - Exit and shutdown the machine.
		command - The file command menu.
		rotate - Rotate the screen of 90�.
		lock - Lock/unlock the user interface.
		mute - Mute/unmute the audio.
		calib - Joystick calibration menu.
		volume - The volume menu.
		difficulty - The difficulty menu.

	Options:
		EXPRESSION - Definition of the button expression that
			generates the event. It's a combination of
			the key, joystick or mouse button and of the operators `not', `or'.
			The `and' operator is implicit between consecutive codes.
			To not assign any input at the event, use the 'none' keyword.
			To completely disable the event, and to make it to disappear
			from the menus, use the 'hidden' keyword.
		KEY -  The available key names are:
			a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r,
			s, t, u, v, w, x, y, z, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
			0_pad, 1_pad, 2_pad, 3_pad, 4_pad, 5_pad, 6_pad,
			7_pad, 8_pad, 9_pad, f1, f2, f3, f4, f5, f6, f7, f8,
			f9, f10, f11, f12, esc, backquote, minus, equals,
			backspace, tab, openbrace, closebrace, enter,
			semicolon, quote, backslash, less, comma, period,
			slash, space, insert, del, home, end, pgup, pgdn, left,
			right, up, down, slash_pad, asterisk_pad, minus_pad,
			plus_pad, period_pad, enter_pad, prtscr, pause,
			lshift, rshift, lcontrol, rcontrol, lalt, ralt,
			lwin, rwin, menu, scrlock, numlock, capslock.
			You can also specify a keyboard scancode, like:
			scan127, scan128, ...
		JOYSTICK - The available joystick buttons are:
			joy_a, joy_b, joy_c, joy_x, joy_y, joy_z, joy_tl (top left),
			joy_tr (top right), joy_tl2 (second top left),
			joy_tr2 (second top right), joy_start, joy_mode,
			joy_select, joy_thumbl (thumb left), joy_thumbr (thumb right),
			joy_gear_down, joy_gear_up.
			You can also use ordinal values if the driver doesn't
			export the button names:
			joy_button0, joy_button1, joy_button2, joy_button3, ...
		MOUSE - The available mouse buttons are:
			mouse_left, mouse_right, mouse_middle, mouse_side,
			mouse_extra, mouse_forward, mouse_back.
			You can also use ordinal values if the driver doesn't
			export the button names:
			mouse_button0, mouse_button1, mouse_button2, ...

	Examples:
		:event_assign enter lcontrol or enter or joy_a or mouse_left
		:event_assign menu space or joy_mode or mouse_right
		:event_assign command none
		:event_assign close hidden

	The utility `advk' can be used to shown key scancodes and names.
	The utilities `advj' and `advm' can be used to shown the joystick and
	mouse button names.

    event_repeat
	Selects the repeat rate of the various events.

	:event_repeat FIRST_TIME NEXT_TIME

	Options:
		FIRST_TIME - Time of the first repeat in ms.
		NEXT_TIME - Time of the next repeats in ms.

    event_mode
	Selects whether to wait for a complete screen update before
	processing the next event.

	:event_mode wait | fast

	Options:
		wait - The screen is completely redrawn before processing
			the next event.
		fast - If an event is waiting, the screen drawing
			is interrupted (default).

    event_alpha
	Disables the alphanumeric keys for fast moving.
	If you have a keyboard encoder or a keyboard hack with some
	buttons remapped to alphanumeric keys, it's useful to disable
	them.

	:event_alpha yes | no

	Options:
		yes - Enable (default).
		no - Disable.

  Other Configuration Options

    idle_start
	Automatically starts a random game after some time of inactivity.
	You can also configure the AdvanceMAME option `input_idleexit'
	in the file `advmame.rc' to create a continuous demo mode.

	:idle_start START_TIMEOUT REPEAT_TIMEOUT

	Options:
		START_TIMEOUT - Number of seconds to wait for the
			first run. 0 means never (default never).
		REPEAT_TIMEOUT - Number of seconds to wait for the
			next run. 0 means never (default never).

	Examples:
		:idle_start 400 60

    idle_screensaver
	Selects the start time of the default screen saver. The screensaver
	is a slide show of the available snapshots.

	:idle_screensaver START_TIMEOUT REPEAT_TIMEOUT

	Options:
		START_TIMEOUT - Number of seconds to wait for the
			first run. 0 means never (default 60).
		REPEAT_TIMEOUT - Number of seconds to wait for the
			next run. 0 means never (default 14).

	Examples:
		:idle_screensaver 120 14

    idle_screensaver_preview
	Selects the preview type to use in the screensaver. Like
	the preview option.

	:idle_screensaver_preview none | exit | shutdown | play
	:	| snap | flyers | cabinets | titles

	Options:
		none - Shutdown the monitor using the VESA/PM services
			if available. Otherwise use a black image.
		exit - Exit from the application.
		shutdown - Shutdown the machine.
		snap, flyers, cabinets, titles - Start a mute slide show
			of the specified image type.
		play - Start a snap slide show using the animated
			snapshots and the game sounds (default).

    group/type
	Selects the available `group' and `type' category names and
	which of them to show.

	:group "STRING"
	:type "STRING"
	:[EMULATOR/]group_include "STRING"
	:[EMULATOR/]type_include "STRING"

	Commands:
		group, type - Define a category.
		group_include, type_include - Show a category.

	Options:
		EMULATOR/ - Nothing for the default value, or an emulator
			name for a specific emulator option.
		STRING - name of the category

    group/type/desc/info_import
	Selects the automatic import of the groups, types, descriptions
	and extra information from an external file. The extra info are
	additional information displayed for every game.

	The file formats supported are CATVER, CATLIST/CAT32, MacMAME and NMS.
	The files are read in the current directory in DOS and Windows
	and in the $home directory in Linux and Mac OS X.

	WARNING! These options DON'T OVERRIDE any user explicit
	choices made with the `game' option.

	:desc_import (catver | catlist | mac | nms) "EMULATOR" "FILE" ["SECTION"]
	:info_import (catver | catlist | mac | nms) "EMULATOR" "FILE" ["SECTION"]
	:group_import (catver | catlist | mac | nms) "EMULATOR" "FILE" ["SECTION"]
	:type_import (catver | catlist | mac | nms) "EMULATOR" "FILE" ["SECTION"]

	Commands:
		desc_import - Imports the game names shown in the menu.
		info_import - Imports additional information printed on
			bottom bar of the menu.
		group_import - Imports the group names of the game.
		type_import - Imports the type names of the game.

	Options:
		none - Don't import.
		catver - Import in the CATVER format. In this format you must
			also specify the section to load, usually it's "Category".
		catlist - Import in the CATLIST/CAT32 format.
		mac - Import in the MacMAME format.
		nms - Import in the NMS format.
		EMULATOR - The emulator tag name as specified in
			the `emulator' option.
		FILE - The file name.
		SECTION - The section name (only for the `ini' format).

	Examples:
		:group_import catver "advmame" "catver.ini" "Category"
		:info_import catver "advmame" "catver.ini" "VerAdded"
		:group_import catlist "advmame" "catlist.ini"
		:type_import mac "advmame" "Genre 37b14.txt"
		:desc_import nms "raine" "raine.nms"

	The CATVER/CATLIST files can be downloaded from:

		+http://www.progettoemma.net/?catlist

	or:

		+http://www.progettosnaps.net/renameset/

    misc_exit
	Selects the exit mode.

	:misc_exit none | esc | exit | shutdown | all

	Options:
		none - Exit is disabled.
		esc - Exit is possible with the 'esc' event [ESC]
			if the interface is not locked (default).
		exit - Exit is possible with the 'exit' event [ALT-X].
		shutdown - Exit is possible with the 'shutdown' event [CTRL-ESC].
		all - All the exit events work.
		menu - All the exit events work but you get a menu confirmation.

	To lock the interface use the 'lock' option.

    misc_quiet
	Disables the copyright text message at the startup.

	:misc_quiet yes | no

Formats Supported
	This is the list of the file formats supported by AdvanceMENU.

	Images:
		PNG - The PNG format.
		PCX - The PCX format.
		ICO - The ICON format.

	Clips:
		MNG - The MNG-VLC (Very Low Complexity) sub format
			without transparency and alpha channel.
			Or the sub-formats generated by AdvanceMAME or
			by the `advmng' compression utility.

	Sounds:
		MP3 - The MP3 format.
		WAV - The WAV format with a sample size of 16 bit.

	Fonts:
		TTF - The TrueType format (with the FreeType2 library).
		RAW - The RAW format.
		PSF - The PSF format.
		GRX - The GRX format.

	Archives:
		ZIP - The ZIP format.

Signals
	The program intercepts the following signals:

		SIGQUIT - Exit normally.
		SIGTERM, SIGINT, SIGALRM - Exit restoring only the output devices.
		SIGHUP - Restart the program.

Copyright
	This file is Copyright (C) 2003, 2004, 2005, 2012 Andrea Mazzoleni, Randy Schnedler.

