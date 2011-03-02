/***************************************************************************

    emuopts.c

    Options file and command line management.

    Copyright Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#include "emu.h"
#include "emuopts.h"

#include <ctype.h>

#ifdef MAMEMESS
#define MESS
#endif /* MAMEMESS */



/***************************************************************************
    BUILT-IN (CORE) OPTIONS
***************************************************************************/

const options_entry mame_core_options[] =
{
	/* unadorned options - only a single one supported at the moment */
	{ "<UNADORNED0>",                NULL,        0,                 NULL },

	/* config options */
	{ NULL,                          NULL,        OPTION_HEADER,     "CORE CONFIGURATION OPTIONS" },
	{ "readconfig;rc",               "1",         OPTION_BOOLEAN,    "enable loading of configuration files" },
	{ "writeconfig;wc",				 "0",		  OPTION_BOOLEAN,	 "writes configuration to (driver).ini on exit" },
#ifdef DRIVER_SWITCH
	{ "driver_config",               "all",       0,                 "switch drivers"},
#endif /* DRIVER_SWITCH */

	/* seach path options */
	{ NULL,                          NULL,        OPTION_HEADER,     "CORE SEARCH PATH OPTIONS" },
	{ "rompath;rp;biospath;bp",      "roms",      0,                 "path to ROMsets and hard disk images" },
	{ "hashpath;hash_directory;hash","hash",      0,                 "path to hash files" },
	{ "samplepath;sp",               "samples",   0,                 "path to samplesets" },
	{ "artpath;artwork_directory",   "artwork",   0,                 "path to artwork files" },
	{ "ctrlrpath;ctrlr_directory",   "ctrlr",     0,                 "path to controller definitions" },
	{ "inipath",                     "ini",       0,                 "path to ini files" },
	{ "fontpath",                    ".",         0,                 "path to font files" },
	{ "cheatpath",                   "cheat",     0,                 "path to cheat files" },
	{ "crosshairpath",               "crosshair", 0,                 "path to crosshair files" },
	{ "langpath",                    "lang",      0,                 "path to localized languages and datafiles" },
#ifdef USE_IPS
	{ "ipspath",                     "ips",       0,                 "path to ips files" },
#endif /* USE_IPS */

	/* output directory options */
	{ NULL,                          NULL,        OPTION_HEADER,     "CORE OUTPUT DIRECTORY OPTIONS" },
	{ "cfg_directory",               "cfg",       0,                 "directory to save configurations" },
	{ "nvram_directory",             "nvram",     0,                 "directory to save nvram contents" },
	{ "memcard_directory",           "memcard",   0,                 "directory to save memory card contents" },
	{ "input_directory",             "inp",       0,                 "directory to save input device logs" },
	{ "state_directory",             "sta",       0,                 "directory to save states" },
	{ "snapshot_directory",          "snap",      0,                 "directory to save screenshots" },
	{ "diff_directory",              "diff",      0,                 "directory to save hard drive image difference files" },
	{ "comment_directory",           "comments",  0,                 "directory to save debugger comments" },
#ifdef USE_HISCORE
	{ "hiscore_directory",           "hi",        0,                 "directory to save hiscores" },
#endif /* USE_HISCORE */
#ifdef MAME_AVI
	{ "avi_directory",               "avi",       0,                 "directory to save avi video file" },
#endif /* MAME_AVI */

	/* filename options */
	{ NULL,                          NULL,        OPTION_HEADER,     "CORE FILENAME OPTIONS" },
#ifdef CMD_LIST
	{ "command_file",               "command.dat",0,                 "command list database name" },
#endif /* CMD_LIST */
#ifdef USE_HISCORE
	{ "hiscore_file",               "hiscore.dat",0,                 "high score database name" },
#endif /* USE_HISCORE */

	/* state/playback options */
	{ NULL,                          NULL,        OPTION_HEADER,     "CORE STATE/PLAYBACK OPTIONS" },
	{ "state",                       NULL,        0,                 "saved state to load" },
	{ "autosave",                    "0",         OPTION_BOOLEAN,    "enable automatic restore at startup, and automatic save at exit time" },
	{ "playback;pb",                 NULL,        0,                 "playback an input file" },
	{ "record;rec",                  NULL,        0,                 "record an input file" },
#ifdef KAILLERA
	{ "playbacksub;pbsub",           NULL,        0,                 "playbacksub an input file" },
	{ "recordsub;recsub",            NULL,        0,                 "recordsub an input file" },
	{ "auto_record_name;at_rec_name",NULL,        0,                 "auto record filename" },
#endif /* KAILLERA */
	{ "mngwrite",                    NULL,        0,                 "optional filename to write a MNG movie of the current session" },
	{ "aviwrite",                    NULL,        0,                 "optional filename to write an AVI movie of the current session" },
	{ "wavwrite",                    NULL,        0,                 "optional filename to write a WAV file of the current session" },
	{ "snapname",                    "%g/%i",     0,                 "override of the default snapshot/movie naming; %g == gamename, %i == index" },
	{ "snapsize",                    "auto",      0,                 "specify snapshot/movie resolution (<width>x<height>) or 'auto' to use minimal size " },
	{ "snapview",                    "internal",  0,                 "specify snapshot/movie view or 'internal' to use internal pixel-aspect views" },
	{ "burnin",                      "0",         OPTION_BOOLEAN,    "create burn-in snapshots for each screen" },

	/* performance options */
	{ NULL,                          NULL,        OPTION_HEADER,     "CORE PERFORMANCE OPTIONS" },
	{ "autoframeskip;afs",           "0",         OPTION_BOOLEAN,    "enable automatic frameskip selection" },
	{ "frameskip;fs(0-10)",          "0",         0,                 "set frameskip to fixed value, 0-10 (autoframeskip must be disabled)" },
	{ "seconds_to_run;str",          "0",         0,                 "number of emulated seconds to run before automatically exiting" },
	{ "throttle",                    "1",         OPTION_BOOLEAN,    "enable throttling to keep game running in sync with real time" },
	{ "sleep",                       "1",         OPTION_BOOLEAN,    "enable sleeping, which gives time back to other applications when idle" },
	{ "speed(0.01-100)",             "1.0",       0,                 "controls the speed of gameplay, relative to realtime; smaller numbers are slower" },
	{ "refreshspeed;rs",             "0",         OPTION_BOOLEAN,    "automatically adjusts the speed of gameplay to keep the refresh rate lower than the screen" },

	/* rotation options */
	{ NULL,                          NULL,        OPTION_HEADER,     "CORE ROTATION OPTIONS" },
	{ "rotate",                      "1",         OPTION_BOOLEAN,    "rotate the game screen according to the game's orientation needs it" },
	{ "ror",                         "0",         OPTION_BOOLEAN,    "rotate screen clockwise 90 degrees" },
	{ "rol",                         "0",         OPTION_BOOLEAN,    "rotate screen counterclockwise 90 degrees" },
	{ "autoror",                     "0",         OPTION_BOOLEAN,    "automatically rotate screen clockwise 90 degrees if vertical" },
	{ "autorol",                     "0",         OPTION_BOOLEAN,    "automatically rotate screen counterclockwise 90 degrees if vertical" },
	{ "flipx",                       "0",         OPTION_BOOLEAN,    "flip screen left-right" },
	{ "flipy",                       "0",         OPTION_BOOLEAN,    "flip screen upside-down" },

	/* artwork options */
	{ NULL,                          NULL,        OPTION_HEADER,     "CORE ARTWORK OPTIONS" },
	{ "artwork_crop;artcrop",        "0",         OPTION_BOOLEAN,    "crop artwork to game screen size" },
	{ "use_backdrops;backdrop",      "1",         OPTION_BOOLEAN,    "enable backdrops if artwork is enabled and available" },
	{ "use_overlays;overlay",        "1",         OPTION_BOOLEAN,    "enable overlays if artwork is enabled and available" },
	{ "use_bezels;bezel",            "1",         OPTION_BOOLEAN,    "enable bezels if artwork is enabled and available" },

	/* screen options */
	{ NULL,                          NULL,        OPTION_HEADER,     "CORE SCREEN OPTIONS" },
	{ "brightness(0.1-2.0)",         "1.0",       0,                 "default game screen brightness correction" },
	{ "contrast(0.1-2.0)",           "1.0",       0,                 "default game screen contrast correction" },
	{ "gamma(0.1-3.0)",              "1.0",       0,                 "default game screen gamma correction" },
	{ "pause_brightness(0.0-1.0)",   "0.65",      0,                 "amount to scale the screen brightness when paused" },
	{ "effect",                      "none",      0,                 "name of a PNG file to use for visual effects, or 'none'" },
#ifdef USE_SCALE_EFFECTS
	{ "scale_effect",                "none",      0,                 "image enhancement effect" },
#endif /* USE_SCALE_EFFECTS */

	/* vector options */
	{ NULL,                          NULL,        OPTION_HEADER,     "CORE VECTOR OPTIONS" },
	{ "antialias;aa",                "1",         OPTION_BOOLEAN,    "use antialiasing when drawing vectors" },
	{ "beam",                        "1.0",       0,                 "set vector beam width" },
	{ "flicker",                     "0",         0,                 "set vector flicker effect" },

	/* sound options */
	{ NULL,                          NULL,        OPTION_HEADER,     "CORE SOUND OPTIONS" },
	{ "sound",                       "1",         OPTION_BOOLEAN,    "enable sound output" },
	{ "samplerate;sr(1000-1000000)", "48000",     0,                 "set sound output sample rate" },
	{ "samples",                     "1",         OPTION_BOOLEAN,    "enable the use of external samples if available" },
	{ "volume;vol",                  "0",         0,                 "sound volume in decibels (-32 min, 0 max)" },
#ifdef USE_VOLUME_AUTO_ADJUST
	{ "volume_adjust",               "0",         OPTION_BOOLEAN,    "enable/disable volume auto adjust" },
#endif /* USE_VOLUME_AUTO_ADJUST */

	/* input options */
	{ NULL,                          NULL,        OPTION_HEADER,     "CORE INPUT OPTIONS" },
	{ "coin_lockout;coinlock",       "1",         OPTION_BOOLEAN,    "enable coin lockouts to actually lock out coins" },
	{ "ctrlr",                       NULL,        0,                 "preconfigure for specified controller" },
	{ "mouse",                       "0",         OPTION_BOOLEAN,    "enable mouse input" },
	{ "joystick;joy",                "1",         OPTION_BOOLEAN,    "enable joystick input" },
	{ "lightgun;gun",                "0",         OPTION_BOOLEAN,    "enable lightgun input" },
	{ "multikeyboard;multikey",      "0",         OPTION_BOOLEAN,    "enable separate input from each keyboard device (if present)" },
	{ "multimouse",                  "0",         OPTION_BOOLEAN,    "enable separate input from each mouse device (if present)" },
	{ "steadykey;steady",            "0",         OPTION_BOOLEAN,    "enable steadykey support" },
	{ "offscreen_reload;reload",     "0",         OPTION_BOOLEAN,    "convert lightgun button 2 into offscreen reload" },
	{ "joystick_map;joymap",         "auto",      0,                 "explicit joystick map, or auto to auto-select" },
	{ "joystick_deadzone;joy_deadzone;jdz",      "0.3",  0,          "center deadzone range for joystick where change is ignored (0.0 center, 1.0 end)" },
	{ "joystick_saturation;joy_saturation;jsat", "0.85", 0,          "end of axis saturation range for joystick where change is ignored (0.0 center, 1.0 end)" },
	{ "natural;nat",				 "0",		  OPTION_BOOLEAN,	 "specifies whether to use a natural keyboard or not" },
	{ "uimodekey;umk",      		 "auto",	  0,    			 "specifies the key used to toggle between full and partial UI mode" },
#ifdef MAMEUIPLUSPLUS
	{ "forceuse_dinput",             "0",         OPTION_BOOLEAN,    "force use direct input" },
#endif /* MAMEUIPLUSPLUS */


	/* input autoenable options */
	{ NULL,                          NULL,        OPTION_HEADER,     "CORE INPUT AUTOMATIC ENABLE OPTIONS" },
	{ "paddle_device;paddle",        "keyboard",  0,                 "enable (none|keyboard|mouse|lightgun|joystick) if a paddle control is present" },
	{ "adstick_device;adstick",      "keyboard",  0,                 "enable (none|keyboard|mouse|lightgun|joystick) if an analog joystick control is present" },
	{ "pedal_device;pedal",          "keyboard",  0,                 "enable (none|keyboard|mouse|lightgun|joystick) if a pedal control is present" },
	{ "dial_device;dial",            "keyboard",  0,                 "enable (none|keyboard|mouse|lightgun|joystick) if a dial control is present" },
	{ "trackball_device;trackball",  "keyboard",  0,                 "enable (none|keyboard|mouse|lightgun|joystick) if a trackball control is present" },
	{ "lightgun_device",             "keyboard",  0,                 "enable (none|keyboard|mouse|lightgun|joystick) if a lightgun control is present" },
	{ "positional_device",           "keyboard",  0,                 "enable (none|keyboard|mouse|lightgun|joystick) if a positional control is present" },
	{ "mouse_device",                "mouse",     0,                 "enable (none|keyboard|mouse|lightgun|joystick) if a mouse control is present" },

	/* debugging options */
	{ NULL,                          NULL,        OPTION_HEADER,     "CORE DEBUGGING OPTIONS" },
	{ "log",                         "0",         OPTION_BOOLEAN,    "generate an error.log file" },
	{ "verbose;v",                   "0",         OPTION_BOOLEAN,    "display additional diagnostic information" },
	{ "update_in_pause",             "0",         OPTION_BOOLEAN,    "keep calling video updates while in pause" },
	{ "debug;d",                     "0",         OPTION_BOOLEAN,    "enable/disable debugger" },
	{ "debugscript",                 NULL,        0,                 "script for debugger" },
	{ "debug_internal;di",           "0",         OPTION_BOOLEAN,    "use the internal debugger for debugging" },

	/* misc options */
	{ NULL,                          NULL,        OPTION_HEADER,     "CORE MISC OPTIONS" },
	{ "bios",                        NULL,        0,                 "select the system BIOS to use" },
	{ "cheat;c",                     "0",         OPTION_BOOLEAN,    "enable cheat subsystem" },
	{ "skip_gameinfo",               "0",         OPTION_BOOLEAN,    "skip displaying the information screen at startup" },
	{ "uifont",                      "default",   0,                 "specify a font to use" },
#ifdef CONFIRM_QUIT
	{ "confirm_quit",                "1",         OPTION_BOOLEAN,    "quit game with confirmation" },
#endif /* CONFIRM_QUIT */
#ifdef PLAYBACK_END_PAUSE
	{ "playback_end_pause",          "0",         OPTION_BOOLEAN,    "automatic pause when playback ended" },
#endif /* PLAYBACK_END_PAUSE */
#ifdef TRANS_UI
	{ "ui_transparency",             "215",       0,                 "transparency in-game UI [0-255]" },
#endif /* TRANS_UI */
#ifdef USE_IPS
	{ "ips",                         NULL,        0,                 "ips datafile name"},
#endif /* USE_IPS */
	{ "ramsize;ram",				 NULL,		  OPTION_DRIVER_ONLY,"size of RAM (if supported by driver)" },
#ifdef MAMEUIPLUSPLUS
	{ "disp_autofire_status",        "1",         OPTION_BOOLEAN,    "display autofire status" },
#endif /* MAMEUIPLUSPLUS */

#ifdef UI_COLOR_DISPLAY
	/* palette options */
	{ NULL,                          NULL,        OPTION_HEADER,     "CORE PALETTE OPTIONS" },
	{ "main_background",             "16,16,48",    0,               "main background color" },
	{ "cursor_sel_text",             "255,255,255", 0,               "cursor text color (selected)" },
	{ "cursor_sel_background",       "60,120,240",  0,               "cursor background color (selected)" },
	{ "cursor_hov_text",             "120,180,240", 0,               "cursor text color (floating)" },
	{ "cursor_hov_background",       "32,32,0",     0,               "cursor background color (floating)" },
	{ "button_red",                  "255,64,64",   0,               "button color (red)" },
	{ "button_yellow",               "255,238,0",   0,               "button color (yellow)" },
	{ "button_green",                "0,255,64",    0,               "button color (green)" },
	{ "button_blue",                 "0,170,255",   0,               "button color (blue)" },
	{ "button_purple",               "170,0,255",   0,               "button color (purple)" },
	{ "button_pink",                 "255,0,170",   0,               "button color (pink)" },
	{ "button_aqua",                 "0,255,204",   0,               "button color (aqua)" },
	{ "button_silver",               "255,0,255",   0,               "button color (silver)" },
	{ "button_navy",                 "255,160,0",   0,               "button color (navy)" },
	{ "button_lime",                 "190,190,190", 0,               "button color (lime)" },
#endif /* UI_COLOR_DISPLAY */

	/* language options */
	{ NULL,                          NULL,        OPTION_HEADER,     "CORE LANGUAGE OPTIONS" },
	{ "language;lang",               "auto",      0,                 "select translation language" },
	{ "use_lang_list",               "1",         OPTION_BOOLEAN,    "enable/disable local language game list" },

	/* image device options */
	//mamep: stop adding device options until array drivers[] is ready
	{ OPTION_ADDED_DEVICE_OPTIONS,	 "1",		  OPTION_BOOLEAN | OPTION_INTERNAL,	"image device-specific options have been added" },

#ifdef MAME_AVI
	// avi options
	{ NULL,                           NULL,   OPTION_HEADER,  "AVI RECORD OPTIONS" },
	{ "avi_avi_filename",             NULL,   0,              "avi options(avi_filename)" },
	{ "avi_def_fps",                  "60.0", 0,              "avi options(def_fps)" },
	{ "avi_fps",                      "60.0", 0,              "avi options(fps)" },
	{ "avi_frame_skip",               "0",    0,              "avi options(frame_skip)" },
	{ "avi_frame_cmp",                "0",    OPTION_BOOLEAN, "avi options(frame_cmp)" },
	{ "avi_frame_cmp_pre15",          "0",    OPTION_BOOLEAN, "avi options(frame_cmp_pre15)" },
	{ "avi_frame_cmp_few",            "0",    OPTION_BOOLEAN, "avi options(frame_cmp_few)" },
	{ "avi_width",                    "0",    0,              "avi options(width)" },
	{ "avi_height",                   "0",    0,              "avi options(height)" },
	{ "avi_depth",                    "16",   0,              "avi options(depth)" },
	{ "avi_orientation",              "0",    0,              "avi options(orientation)" },
	{ "avi_rect_top",                 "0",    0,              "avi options(rect_top)" },
	{ "avi_rect_left",                "0",    0,              "avi options(rect_left)" },
	{ "avi_rect_width",               "0",    0,              "avi options(rect_width)" },
	{ "avi_rect_height",              "0",    0,              "avi options(rect_height)" },
	{ "avi_interlace",                "0",    OPTION_BOOLEAN, "avi options(interlace)" },
	{ "avi_interlace_odd_field",      "0",    OPTION_BOOLEAN, "avi options(interlace_odd_field)" },
	{ "avi_avi_filesize",             "0",    0,              "avi options(avi_filesize)" },
	{ "avi_avi_savefile_pause",       "0",    OPTION_BOOLEAN, "avi options(avi_savefile_pause)" },
	{ "avi_avi_width",                "0",    0,              "avi options(avi_width)" },
	{ "avi_avi_height",               "0",    0,              "avi options(avi_height)" },
	{ "avi_avi_depth",                "16",   0,              "avi options(avi_depth)" },
	{ "avi_avi_rect_top",             "0",    0,              "avi options(avi_rect_top)" },
	{ "avi_avi_rect_left",            "0",    0,              "avi options(avi_rect_left)" },
	{ "avi_avi_rect_width",           "0",    0,              "avi options(avi_rect_width)" },
	{ "avi_avi_rect_height",          "0",    0,              "avi options(avi_rect_height)" },
	{ "avi_avi_smooth_resize_x",      "0",    OPTION_BOOLEAN, "avi options(avi_smooth_resize_x)" },
	{ "avi_avi_smooth_resize_y",      "0",    OPTION_BOOLEAN, "avi options(avi_smooth_resize_y)" },

	{ "avi_wav_filename",             NULL,   0,              "avi options(wav_filename)" },
	{ "avi_audio_type",               "0",    0,              "avi options(audio_type)" },
	{ "avi_audio_channel",            "0",    0,              "avi options(audio_channel)" },
	{ "avi_audio_samples_per_sec",    "0",    0,              "avi options(audio_samples_per_sec)" },
	{ "avi_audio_bitrate",            "0",    0,              "avi options(audio_bitrate)" },
	{ "avi_audio_record_type",        "0",    0,              "avi options(audio_record_type)" },
	{ "avi_avi_audio_channel",        "0",    0,              "avi options(avi_audio_channel)" },
	{ "avi_avi_audio_samples_per_sec","0",    0,              "avi options(avi_audio_samples_per_sec)" },
	{ "avi_avi_audio_bitrate",        "0",    0,              "avi options(avi_audio_bitrate)" },
	{ "avi_audio_cmp",                "0",    OPTION_BOOLEAN, "avi options(audio_cmp)" },

	{ "avi_hour",                     "0",    0,              "avi options(hour)" },
	{ "avi_minute",                   "0",    0,              "avi options(minute)" },
	{ "avi_second",                   "0",    0,              "avi options(second)" },
#endif /* MAME_AVI */
	{ NULL }
};



/***************************************************************************
    CORE IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    memory_error - report a memory error
-------------------------------------------------*/

static void memory_error(const char *message)
{
	fatalerror("%s", message);
}



/*-------------------------------------------------
    mame_puts_info
    mame_puts_warning
    mame_puts_error
-------------------------------------------------*/

static void mame_puts_info(const char *s)
{
	mame_printf_info("%s", s);
}

static void mame_puts_warning(const char *s)
{
	mame_printf_warning("%s", s);
}

static void mame_puts_error(const char *s)
{
	mame_printf_error("%s", s);
}

/*-------------------------------------------------
    image_get_device_option - accesses a device
    option
-------------------------------------------------*/

const char *image_get_device_option(device_image_interface *image)
{
	const char *result = NULL;

	if (options_get_bool(image->device().machine->options(), OPTION_ADDED_DEVICE_OPTIONS))
	{
		/* access the option */
		result = options_get_string_priority(image->device().machine->options(),  image->image_config().instance_name(), OPTION_PRIORITY_DRIVER_INI);
	}
	return result;
}

/*-------------------------------------------------
    image_add_device_options - add all of the device
    options for a specified device
-------------------------------------------------*/

void image_add_device_options(core_options *opts, const game_driver *driver)
{
	int index = 0;
	const device_config_image_interface *image = NULL;

	/* create the configuration */
	machine_config config(*driver);

	/* enumerate our callback for every device */
	/* loop on each device instance */
	for (bool gotone = config.m_devicelist.first(image); gotone; gotone = image->next(image))
	{
		options_entry entry[2];
		astring dev_full_name;

		/* first device? add the header as to be pretty */
		if (index == 0)
		{
			memset(entry, 0, sizeof(entry));
			entry[0].description = "IMAGE DEVICES";
			entry[0].flags = OPTION_HEADER;
			options_add_entries(opts, entry);
		}

		/* retrieve info about the device instance */
		dev_full_name.printf("%s;%s", image->instance_name(), image->brief_instance_name());

		/* add the option */
		memset(entry, 0, sizeof(entry));
		entry[0].name = dev_full_name;
		options_add_entries(opts, entry, TRUE);

		index++;
	}

	/* record that we've added device options */
	options_set_bool(opts, OPTION_ADDED_DEVICE_OPTIONS, TRUE, OPTION_PRIORITY_CMDLINE);
}

/*-------------------------------------------------
    image_driver_name_callback - called when we
    parse the driver name, so we can add options
    specific to that driver
-------------------------------------------------*/

static void image_driver_name_callback(core_options *opts, const char *arg)
{
	const game_driver *driver;

	/* only add these options if we have not yet added them */
	if (!options_get_bool(opts, OPTION_ADDED_DEVICE_OPTIONS))
	{
		driver = driver_get_name(arg);
		if (driver != NULL)
		{
			image_add_device_options(opts, driver);
		}
	}
}


/*-------------------------------------------------
    mame_options_init - create core MAME options
-------------------------------------------------*/

core_options *mame_options_init(const options_entry *entries)
{
	/* create MAME core options */
	core_options *opts = options_create(memory_error);

	/* set up output callbacks */
	options_set_output_callback(opts, OPTMSG_INFO, mame_puts_info);
	options_set_output_callback(opts, OPTMSG_WARNING, mame_puts_warning);
	options_set_output_callback(opts, OPTMSG_ERROR, mame_puts_error);

	options_add_entries(opts, mame_core_options);
	if (entries != NULL)
		options_add_entries(opts, entries);

	/* we need to dynamically add options when the device name is parsed */
	options_set_option_callback(opts, OPTION_GAMENAME, image_driver_name_callback);
	return opts;
}
