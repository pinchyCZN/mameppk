// license:BSD-3-Clause
// copyright-holders:Aaron Giles
//============================================================
//
//  video.c - Win32 video handling
//
//============================================================

// standard windows headers
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// MAME headers
#include "emu.h"
#include "emuopts.h"
#include "osdepend.h"
#include "video/vector.h"
#include "render.h"
#include "rendutil.h"
#include "ui/ui.h"
#include "uiinput.h"
#ifdef USE_SCALE_EFFECTS
#include "osdscale.h"
#endif /* USE_SCALE_EFFECTS */

// MAMEOS headers
#include "winmain.h"
#include "video.h"
#include "window.h"
#include "input.h"
#include "strconv.h"
#include "config.h"

#ifdef MAME_AVI
#include "Avi.h"
static struct MAME_AVI_STATUS AviStatus;
static char *avi_filename;
#define win_video_window win_window_list->hwnd
#endif /* MAME_AVI */

#ifdef KAILLERA
#include "ui_temp.h"
extern int kPlay;
#endif /* KAILLERA */


//============================================================
//  CONSTANTS
//============================================================


//============================================================
//  GLOBAL VARIABLES
//============================================================

osd_video_config video_config;

// monitor info
osd_monitor_info *osd_monitor_info::list = NULL;

#ifdef USE_SCALE_EFFECTS
static int cur_scale_xsize;
static int cur_scale_ysize;
#endif /* USE_SCALE_EFFECTS */

//============================================================
//  LOCAL VARIABLES
//============================================================


//============================================================
//  PROTOTYPES
//============================================================

static void init_monitors(void);

static void check_osd_inputs(running_machine &machine);

static float get_aspect(const char *defdata, const char *data, int report_error);
static void get_resolution(const char *defdata, const char *data, osd_window_config *config, int report_error);


//============================================================
//  video_init
//============================================================

// FIXME: Temporary workaround
static osd_window_config   windows[MAX_WINDOWS];        // configuration data per-window

bool windows_osd_interface::video_init()
{
	int index;

	// extract data from the options
	extract_video_config();

	// set up monitors first
	init_monitors();

	// initialize the window system so we can make windows
	window_init();

#ifdef MAME_AVI
	if (avi_filename)
		AviStartCapture(NULL, avi_filename, &AviStatus);
#endif /* MAME_AVI */

	// create the windows
	windows_options &options = downcast<windows_options &>(machine().options());
	for (index = 0; index < video_config.numscreens; index++)
		win_window_info::create(machine(), index, osd_monitor_info::pick_monitor(options, index), &windows[index]);
	if (video_config.mode != VIDEO_MODE_NONE)
		SetForegroundWindow(win_window_list->m_hwnd);

	return true;
}


//============================================================
//  video_exit
//============================================================

void windows_osd_interface::video_exit()
{
#ifdef MAME_AVI
	if (GetAviCapture())
		AviEndCapture();

	if (avi_filename)
	{
		free(avi_filename);
		avi_filename = NULL;
	}
#endif /* MAME_AVI */

	window_exit();

	// free all of our monitor information
	while (osd_monitor_info::list != NULL)
	{
		osd_monitor_info *temp = osd_monitor_info::list;
		osd_monitor_info::list = temp->m_next;
		global_free(temp);
	}
}



win_monitor_info::win_monitor_info(const HMONITOR handle, const char *monitor_device, float aspect)
	: osd_monitor_info(&m_handle, monitor_device, aspect), m_handle(handle)
{
	refresh();
}

win_monitor_info::~win_monitor_info()
{
}

//============================================================
//  winvideo_monitor_refresh
//============================================================

void win_monitor_info::refresh()
{
	BOOL result;

	// fetch the latest info about the monitor
	m_info.cbSize = sizeof(m_info);
	result = GetMonitorInfo(m_handle, (LPMONITORINFO)&m_info);
	assert(result);
	char *temp = utf8_from_tstring(m_info.szDevice);

	if (temp) strncpy(m_name, temp, sizeof(m_name));

	osd_free(temp);

	m_pos_size = RECT_to_osd_rect(m_info.rcMonitor);
	m_usuable_pos_size = RECT_to_osd_rect(m_info.rcWork);
	m_is_primary = ((m_info.dwFlags & MONITORINFOF_PRIMARY) != 0);
	(void)result; // to silence gcc 4.6
}



//============================================================
//  sdlvideo_monitor_get_aspect
//============================================================

float osd_monitor_info::aspect()
{
	// FIXME: returning 0 looks odd, video_config is bad
	if (video_config.keepaspect)
	{
		return m_aspect / ((float)m_pos_size.width() / (float)m_pos_size.height());
	}
	return 0.0f;
}

//============================================================
//  winvideo_monitor_from_handle
//============================================================

osd_monitor_info *win_monitor_info::monitor_from_handle(HMONITOR hmonitor)
{
	osd_monitor_info *monitor;

	// find the matching monitor
	for (monitor = osd_monitor_info::list; monitor != NULL; monitor = monitor->m_next)
		if (*((HMONITOR *)monitor->oshandle()) == hmonitor)
			return monitor;
	return NULL;
}



//============================================================
//  update
//============================================================

void windows_osd_interface::update(bool skip_redraw)
{
	// ping the watchdog on each update
	winmain_watchdog_ping();

	// if we're not skipping this redraw, update all windows
	if (!skip_redraw)
	{
#ifdef USE_SCALE_EFFECTS
		extern int win_scale_res_changed;
		win_scale_res_changed = 0;

		if (scale_effect.xsize != cur_scale_xsize || scale_effect.ysize != cur_scale_ysize)
		{
			win_scale_res_changed = 1;
			cur_scale_xsize = scale_effect.xsize;
			cur_scale_ysize = scale_effect.ysize;
		}
#endif /* USE_SCALE_EFFECTS */
//      profiler_mark(PROFILER_BLIT);
		for (win_window_info *window = win_window_list; window != NULL; window = window->m_next)
			window->update();
//      profiler_mark(PROFILER_END);
	}

	// poll the joystick values here
	winwindow_process_events(machine(), TRUE, FALSE);
	wininput_poll(machine());
	check_osd_inputs(machine());
	// if we're running, disable some parts of the debugger
	if ((machine().debug_flags & DEBUG_FLAG_OSD_ENABLED) != 0)
		debugger_update();
}





//============================================================
//  monitor_enum_callback
//============================================================

BOOL CALLBACK win_monitor_info::monitor_enum_callback(HMONITOR handle, HDC dc, LPRECT rect, LPARAM data)
{
	osd_monitor_info ***tailptr = (osd_monitor_info ***)data;
	osd_monitor_info *monitor;
	MONITORINFOEX info;
	BOOL result;

	// get the monitor info
	info.cbSize = sizeof(info);
	result = GetMonitorInfo(handle, (LPMONITORINFO)&info);
	assert(result);
	(void)result; // to silence gcc 4.6

	// guess the aspect ratio assuming square pixels
	float aspect = (float)(info.rcMonitor.right - info.rcMonitor.left) / (float)(info.rcMonitor.bottom - info.rcMonitor.top);

	// allocate a new monitor info
	char *temp = utf8_from_tstring(info.szDevice);
	// copy in the data
	monitor = global_alloc(win_monitor_info(handle, temp, aspect));
	osd_free(temp);

	// hook us into the list
	**tailptr = monitor;
	*tailptr = &monitor->m_next;

	// enumerate all the available monitors so to list their names in verbose mode
	return TRUE;
}


//============================================================
//  init_monitors
//============================================================

static void init_monitors(void)
{
	osd_monitor_info **tailptr;

	// make a list of monitors
	osd_monitor_info::list = NULL;
	tailptr = &osd_monitor_info::list;
	EnumDisplayMonitors(NULL, NULL, win_monitor_info::monitor_enum_callback, (LPARAM)&tailptr);

	// if we're verbose, print the list of monitors
	{
		osd_monitor_info *monitor;
		for (monitor = osd_monitor_info::list; monitor != NULL; monitor = monitor->m_next)
		{
			osd_printf_verbose(_WINDOWS("Video: Monitor %p = \"%s\" %s\n"), monitor->oshandle(), monitor->devicename(), monitor->is_primary() ? _WINDOWS("(primary)") : "");
		}
	}
}


//============================================================
//  pick_monitor
//============================================================

osd_monitor_info *osd_monitor_info::pick_monitor(windows_options &options, int index)
{
	osd_monitor_info *monitor;
	const char *scrname, *scrname2;
	int moncount = 0;
	float aspect;

	// get the screen option
	scrname = options.screen();
	scrname2 = options.screen(index);

	// decide which one we want to use
	if (strcmp(scrname2, "auto") != 0)
		scrname = scrname2;

	// get the aspect ratio
	aspect = get_aspect(options.aspect(), options.aspect(index), TRUE);

	// look for a match in the name first
	if (scrname != NULL && (scrname[0] != 0))
	{
		for (monitor = osd_monitor_info::list; monitor != NULL; monitor = monitor->next())
		{
			moncount++;
			if (strcmp(scrname, monitor->devicename()) == 0)
				goto finishit;
		}
	}

	// didn't find it; alternate monitors until we hit the jackpot
	index %= moncount;
	for (monitor = osd_monitor_info::list; monitor != NULL; monitor = monitor->next())
		if (index-- == 0)
			goto finishit;

	// return the primary just in case all else fails
	for (monitor = osd_monitor_info::list; monitor != NULL; monitor = monitor->next())
		if (monitor->is_primary())
			goto finishit;

	// FIXME: FatalError?
finishit:
	if (aspect != 0)
	{
		monitor->set_aspect(aspect);
	}
	return monitor;
}


//============================================================
//  check_osd_inputs
//============================================================

static void check_osd_inputs(running_machine &machine)
{
	// check for toggling fullscreen mode
	if (ui_input_pressed(machine, IPT_OSD_1))
		winwindow_toggle_full_screen();

	// check for taking fullscreen snap
	if (ui_input_pressed(machine, IPT_OSD_2))
		winwindow_take_snap();

	// check for taking fullscreen video
	if (ui_input_pressed(machine, IPT_OSD_3))
		winwindow_take_video();

	// check for taking fullscreen video
	if (ui_input_pressed(machine, IPT_OSD_4))
		winwindow_toggle_fsfx();
}



//============================================================
//  extract_video_config
//============================================================

void windows_osd_interface::extract_video_config()
{
	const char *stemp;

#ifdef USE_SCALE_EFFECTS
	stemp = options().value(OPTION_SCALE_EFFECT);

	if (stemp)
	{
		scale_decode(stemp);

		if (scale_effect.effect)
			osd_printf_verbose(_WINDOWS("Using %s scale effect\n"), scale_desc(scale_effect.effect));
	}
#endif /* USE_SCALE_EFFECTS */

	// global options: extract the data
	video_config.windowed      = options().window();
	video_config.prescale      = options().prescale();
	video_config.filter        = options().filter();
	video_config.keepaspect    = options().keep_aspect();
	video_config.numscreens    = options().numscreens();

	// if we are in debug mode, never go full screen
	if (machine().debug_flags & DEBUG_FLAG_OSD_ENABLED)
		video_config.windowed = TRUE;

	// per-window options: extract the data
	const char *default_resolution = options().resolution();
	get_resolution(default_resolution, options().resolution(0), &windows[0], TRUE);
	get_resolution(default_resolution, options().resolution(1), &windows[1], TRUE);
	get_resolution(default_resolution, options().resolution(2), &windows[2], TRUE);
	get_resolution(default_resolution, options().resolution(3), &windows[3], TRUE);

	// video options: extract the data
	stemp = options().video();
	if (strcmp(stemp, "d3d") == 0)
		video_config.mode = VIDEO_MODE_D3D;
	else if (strcmp(stemp, "auto") == 0)
		video_config.mode = VIDEO_MODE_D3D;
	else if (strcmp(stemp, "ddraw") == 0)
		video_config.mode = VIDEO_MODE_DDRAW;
	else if (strcmp(stemp, "gdi") == 0)
		video_config.mode = VIDEO_MODE_GDI;
	else if (strcmp(stemp, "bgfx") == 0)
		video_config.mode = VIDEO_MODE_BGFX;
	else if (strcmp(stemp, "none") == 0)
	{
		video_config.mode = VIDEO_MODE_NONE;
		if (options().seconds_to_run() == 0)
			osd_printf_warning(_WINDOWS("Warning: -video none doesn't make much sense without -seconds_to_run\n"));
	}
#if (USE_OPENGL)
	else if (strcmp(stemp, "opengl") == 0)
		video_config.mode = VIDEO_MODE_OPENGL;
#endif
	else
	{
		osd_printf_warning(_WINDOWS("Invalid video value %s; reverting to gdi\n"), stemp);
		video_config.mode = VIDEO_MODE_GDI;
	}
	video_config.waitvsync     = options().wait_vsync();
	video_config.syncrefresh   = options().sync_refresh();
	video_config.triplebuf     = options().triple_buffer();
	video_config.switchres     = options().switch_res();

	// ddraw options: extract the data
	video_config.hwstretch     = options().hwstretch();

	if (video_config.prescale < 1 || video_config.prescale > 3)
	{
		osd_printf_warning("Invalid prescale option, reverting to '1'\n");
		video_config.prescale = 1;
	}
	#if (USE_OPENGL)
		// default to working video please
		video_config.forcepow2texture = options().gl_force_pow2_texture();
		video_config.allowtexturerect = !(options().gl_no_texture_rect());
		video_config.vbo         = options().gl_vbo();
		video_config.pbo         = options().gl_pbo();
		video_config.glsl        = options().gl_glsl();
		if ( video_config.glsl )
		{
			int i;

			video_config.glsl_filter = options().glsl_filter();

			video_config.glsl_shader_mamebm_num=0;

			for(i=0; i<GLSL_SHADER_MAX; i++)
			{
				stemp = options().shader_mame(i);
				if (stemp && strcmp(stemp, OSDOPTVAL_NONE) != 0 && strlen(stemp)>0)
				{
					video_config.glsl_shader_mamebm[i] = (char *) malloc(strlen(stemp)+1);
					strcpy(video_config.glsl_shader_mamebm[i], stemp);
					video_config.glsl_shader_mamebm_num++;
				} else {
					video_config.glsl_shader_mamebm[i] = NULL;
				}
			}

			video_config.glsl_shader_scrn_num=0;

			for(i=0; i<GLSL_SHADER_MAX; i++)
			{
				stemp = options().shader_screen(i);
				if (stemp && strcmp(stemp, OSDOPTVAL_NONE) != 0 && strlen(stemp)>0)
				{
					video_config.glsl_shader_scrn[i] = (char *) malloc(strlen(stemp)+1);
					strcpy(video_config.glsl_shader_scrn[i], stemp);
					video_config.glsl_shader_scrn_num++;
				} else {
					video_config.glsl_shader_scrn[i] = NULL;
				}
			}
		} else {
			int i;
			video_config.glsl_filter = 0;
			video_config.glsl_shader_mamebm_num=0;
			for(i=0; i<GLSL_SHADER_MAX; i++)
			{
				video_config.glsl_shader_mamebm[i] = NULL;
			}
			video_config.glsl_shader_scrn_num=0;
			for(i=0; i<GLSL_SHADER_MAX; i++)
			{
				video_config.glsl_shader_scrn[i] = NULL;
			}
		}

	#endif /* USE_OPENGL */

#ifdef MAME_AVI
	memset(&AviStatus, 0, sizeof(AviStatus));
	avi_filename = NULL;

	if (strlen(options().value("avi_avi_filename")) > 0)
	{
		avi_filename                        = astring_from_utf8(options().value("avi_avi_filename"));
		AviStatus.def_fps                   = options().float_value("avi_def_fps");
		AviStatus.fps                       = options().float_value("avi_fps");
		AviStatus.frame_skip                = options().int_value("avi_frame_skip");
		AviStatus.frame_cmp                 = options().bool_value("avi_frame_cmp");
		AviStatus.frame_cmp_pre15           = options().bool_value("avi_frame_cmp_pre15");
		AviStatus.frame_cmp_few             = options().bool_value("avi_frame_cmp_few");
		AviStatus.width                     = options().int_value("avi_width");
		AviStatus.height                    = options().int_value("avi_height");
		AviStatus.depth                     = options().int_value("avi_depth");
		AviStatus.orientation               = options().int_value("avi_orientation");
		AviStatus.rect.m_Top                = options().int_value("avi_rect_top");
		AviStatus.rect.m_Left               = options().int_value("avi_rect_left");
		AviStatus.rect.m_Width              = options().int_value("avi_rect_width");
		AviStatus.rect.m_Height             = options().int_value("avi_rect_height");
		AviStatus.interlace                 = options().bool_value("avi_interlace");
		AviStatus.interlace_odd_number_field  = options().bool_value("avi_interlace_odd_field");
		AviStatus.avi_filesize              = options().int_value("avi_avi_filesize");
		AviStatus.avi_savefile_pause        = options().bool_value("avi_avi_savefile_pause");
		AviStatus.avi_width                 = options().int_value("avi_avi_width");
		AviStatus.avi_height                = options().int_value("avi_avi_height");
		AviStatus.avi_depth                 = options().int_value("avi_avi_depth");
		AviStatus.avi_rect.m_Top            = options().int_value("avi_avi_rect_top");
		AviStatus.avi_rect.m_Left           = options().int_value("avi_avi_rect_left");
		AviStatus.avi_rect.m_Width          = options().int_value("avi_avi_rect_width");
		AviStatus.avi_rect.m_Height         = options().int_value("avi_avi_rect_height");
		AviStatus.avi_smooth_resize_x       = options().bool_value("avi_avi_smooth_resize_x");
		AviStatus.avi_smooth_resize_y       = options().bool_value("avi_avi_smooth_resize_y");

		AviStatus.wav_filename              = (char *)options().value("avi_wav_filename");
		AviStatus.audio_type                = options().int_value("avi_audio_type");
		AviStatus.audio_channel             = options().int_value("avi_audio_channel");
		AviStatus.audio_samples_per_sec     = options().int_value("avi_audio_samples_per_sec");
		AviStatus.audio_bitrate             = options().int_value("avi_audio_bitrate");
		AviStatus.avi_audio_record_type     = options().int_value("avi_audio_record_type");
		AviStatus.avi_audio_channel         = options().int_value("avi_avi_audio_channel");
		AviStatus.avi_audio_samples_per_sec = options().int_value("avi_avi_audio_samples_per_sec");
		AviStatus.avi_audio_bitrate         = options().int_value("avi_avi_audio_bitrate");
		AviStatus.avi_audio_cmp             = options().bool_value("avi_audio_cmp");
		
		AviStatus.hour                      = options().int_value("avi_hour");
		AviStatus.minute                    = options().int_value("avi_minute");
		AviStatus.second                    = options().int_value("avi_second");
	}
#endif /* MAME_AVI */
}



//============================================================
//  get_aspect
//============================================================

static float get_aspect(const char *defdata, const char *data, int report_error)
{
	int num = 0, den = 1;

	if (strcmp(data, OSDOPTVAL_AUTO) == 0)
	{
		if (strcmp(defdata,OSDOPTVAL_AUTO) == 0)
			return 0;
		data = defdata;
	}
	if (sscanf(data, "%d:%d", &num, &den) != 2 && report_error)
		osd_printf_error(_WINDOWS("Illegal aspect ratio value = %s\n"), data);
	return (float)num / (float)den;
}


//============================================================
//  get_resolution
//============================================================

static void get_resolution(const char *defdata, const char *data, osd_window_config *config, int report_error)
{
	config->width = config->height = config->depth = config->refresh = 0;
	if (strcmp(data, OSDOPTVAL_AUTO) == 0)
	{
		if (strcmp(defdata, OSDOPTVAL_AUTO) == 0)
			return;
		data = defdata;
	}
	if (sscanf(data, "%dx%d@%d", &config->width, &config->height, &config->refresh) < 2 && report_error)
		osd_printf_error(_WINDOWS("Illegal resolution value = %s\n"), data);
}
