// license:BSD-3-Clause
// copyright-holders:Nicola Salmoria, Aaron Giles, Nathan Woods
/*********************************************************************

    ui.c

    Functions used to handle MAME's user interface.

*********************************************************************/

#ifdef KAILLERA
#define WIN32_LEAN_AND_MEAN
#define _WIN32_IE 0x0501
#include <windows.h>
#endif /* KAILLERA */

#include "emu.h"
#include "emuopts.h"
#include "video/vector.h"
#include "machine/laserdsc.h"
#include "render.h"
#include "cheat.h"
#include "rendfont.h"
#include "uiinput.h"
#include "ui/ui.h"
#include "ui/cheatopt.h"
#include "ui/mainmenu.h"
#include "ui/miscmenu.h"
#include "ui/filemngr.h"
#include "ui/sliders.h"
#include "ui/viewgfx.h"
#include "imagedev/cassette.h"
#ifdef CMD_LIST
#include "cmddata.h"
#endif /* CMD_LIST */
#ifdef USE_SHOW_TIME
#include <time.h>
#endif /* USE_SHOW_TIME */

#ifdef MAME_AVI
extern int bAviRun;
#endif /* MAME_AVI */

#ifdef KAILLERA
#include "kailleraclient.h"
#include "KailleraChat.h"
#include "ui_temp.h"
extern int kPlay;
int quiting; //kt
#endif /* KAILLERA */

#ifdef MAMEMESS
#define MESS
#endif /* MAMEMESS */


/***************************************************************************
    CONSTANTS
***************************************************************************/

enum
{
	LOADSAVE_NONE,
	LOADSAVE_LOAD,
	LOADSAVE_SAVE
};

//mamep: to render as fixed-width font
enum
{
	CHAR_WIDTH_HALFWIDTH = 0,
	CHAR_WIDTH_FULLWIDTH,
	CHAR_WIDTH_UNKNOWN
};


/***************************************************************************
    LOCAL VARIABLES
***************************************************************************/

// list of natural keyboard keys that are not associated with UI_EVENT_CHARs
static const input_item_id non_char_keys[] =
{
	ITEM_ID_ESC,
	ITEM_ID_F1,
	ITEM_ID_F2,
	ITEM_ID_F3,
	ITEM_ID_F4,
	ITEM_ID_F5,
	ITEM_ID_F6,
	ITEM_ID_F7,
	ITEM_ID_F8,
	ITEM_ID_F9,
	ITEM_ID_F10,
	ITEM_ID_F11,
	ITEM_ID_F12,
	ITEM_ID_NUMLOCK,
	ITEM_ID_0_PAD,
	ITEM_ID_1_PAD,
	ITEM_ID_2_PAD,
	ITEM_ID_3_PAD,
	ITEM_ID_4_PAD,
	ITEM_ID_5_PAD,
	ITEM_ID_6_PAD,
	ITEM_ID_7_PAD,
	ITEM_ID_8_PAD,
	ITEM_ID_9_PAD,
	ITEM_ID_DEL_PAD,
	ITEM_ID_PLUS_PAD,
	ITEM_ID_MINUS_PAD,
	ITEM_ID_INSERT,
	ITEM_ID_DEL,
	ITEM_ID_HOME,
	ITEM_ID_END,
	ITEM_ID_PGUP,
	ITEM_ID_PGDN,
	ITEM_ID_UP,
	ITEM_ID_DOWN,
	ITEM_ID_LEFT,
	ITEM_ID_RIGHT,
	ITEM_ID_PAUSE,
	ITEM_ID_CANCEL
};

/***************************************************************************
    GLOBAL VARIABLES
***************************************************************************/

#ifdef UI_COLOR_DISPLAY
static rgb_t uifont_colortable[MAX_COLORTABLE];
#endif /* UI_COLOR_DISPLAY */
static rgb_t ui_bgcolor;
static render_texture *bgtexture;
static bitmap_argb32 *bgbitmap;

static int multiline_text_box_visible_lines;
static int multiline_text_box_target_lines;

//mamep: to render as fixed-width font
static int draw_text_fixed_mode;
static int draw_text_scroll_offset;

static int message_window_scroll;
static int scroll_reset;

#ifdef TRANS_UI
static int ui_transparency;
#endif /* TRANS_UI */

#ifdef USE_SHOW_TIME
static int show_time = 0;
static int Show_Time_Position;
#endif /* USE_SHOW_TIME */

// messagebox buffer
static std::string messagebox_text;
static std::string messagebox_poptext;
static rgb_t messagebox_backcolor;

// slider info
static slider_state *slider_list;
static slider_state *slider_current;


/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

// slider controls
static slider_state *slider_alloc(running_machine &machine, const char *title, INT32 minval, INT32 defval, INT32 maxval, INT32 incval, slider_update update, void *arg);
static slider_state *slider_init(running_machine &machine);
static INT32 slider_volume(running_machine &machine, void *arg, std::string *str, INT32 newval);
static INT32 slider_mixervol(running_machine &machine, void *arg, std::string *str, INT32 newval);
static INT32 slider_adjuster(running_machine &machine, void *arg, std::string *str, INT32 newval);
static INT32 slider_overclock(running_machine &machine, void *arg, std::string *str, INT32 newval);
static INT32 slider_refresh(running_machine &machine, void *arg, std::string *str, INT32 newval);
static INT32 slider_brightness(running_machine &machine, void *arg, std::string *str, INT32 newval);
static INT32 slider_contrast(running_machine &machine, void *arg, std::string *str, INT32 newval);
static INT32 slider_gamma(running_machine &machine, void *arg, std::string *str, INT32 newval);
static INT32 slider_xscale(running_machine &machine, void *arg, std::string *str, INT32 newval);
static INT32 slider_yscale(running_machine &machine, void *arg, std::string *str, INT32 newval);
static INT32 slider_xoffset(running_machine &machine, void *arg, std::string *str, INT32 newval);
static INT32 slider_yoffset(running_machine &machine, void *arg, std::string *str, INT32 newval);
static INT32 slider_overxscale(running_machine &machine, void *arg, std::string *str, INT32 newval);
static INT32 slider_overyscale(running_machine &machine, void *arg, std::string *str, INT32 newval);
static INT32 slider_overxoffset(running_machine &machine, void *arg, std::string *str, INT32 newval);
static INT32 slider_overyoffset(running_machine &machine, void *arg, std::string *str, INT32 newval);
static INT32 slider_flicker(running_machine &machine, void *arg, std::string *str, INT32 newval);
static INT32 slider_beam(running_machine &machine, void *arg, std::string *str, INT32 newval);
static char *slider_get_screen_desc(screen_device &screen);
#ifdef MAME_DEBUG
static INT32 slider_crossscale(running_machine &machine, void *arg, std::string *str, INT32 newval);
static INT32 slider_crossoffset(running_machine &machine, void *arg, std::string *str, INT32 newval);
#endif


/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

//-------------------------------------------------
//  is_breakable_char - is a given unicode
//  character a possible line break?
//-------------------------------------------------

INLINE int is_breakable_char(unicode_char ch)
{
	// regular spaces and hyphens are breakable
	if (ch == ' ' || ch == '-')
		return TRUE;

	// In the following character sets, any character is breakable:
	//  Hiragana (3040-309F)
	//  Katakana (30A0-30FF)
	//  Bopomofo (3100-312F)
	//  Hangul Compatibility Jamo (3130-318F)
	//  Kanbun (3190-319F)
	//  Bopomofo Extended (31A0-31BF)
	//  CJK Strokes (31C0-31EF)
	//  Katakana Phonetic Extensions (31F0-31FF)
	//  Enclosed CJK Letters and Months (3200-32FF)
	//  CJK Compatibility (3300-33FF)
	//  CJK Unified Ideographs Extension A (3400-4DBF)
	//  Yijing Hexagram Symbols (4DC0-4DFF)
	//  CJK Unified Ideographs (4E00-9FFF)
	if (ch >= 0x3040 && ch <= 0x9fff)
		return TRUE;

	// Hangul Syllables (AC00-D7AF) are breakable
	if (ch >= 0xac00 && ch <= 0xd7af)
		return TRUE;

	// CJK Compatibility Ideographs (F900-FAFF) are breakable
	if (ch >= 0xf900 && ch <= 0xfaff)
		return TRUE;

	return FALSE;
}

//mamep: check fullwidth character.
//mame core does not support surrogate pairs U+10000-U+10FFFF
INLINE int is_fullwidth_char(unicode_char uchar)
{
	switch (uchar)
	{
	// Chars in Latin-1 Supplement
	// font width depends on your font
	case 0x00a7:
	case 0x00a8:
	case 0x00b0:
	case 0x00b1:
	case 0x00b4:
	case 0x00b6:
	case 0x00d7:
	case 0x00f7:
		return CHAR_WIDTH_UNKNOWN;
	}

	// Greek and Coptic
	// font width depends on your font
	if (uchar >= 0x0370 && uchar <= 0x03ff)
		return CHAR_WIDTH_UNKNOWN;

	// Cyrillic
	// font width depends on your font
	if (uchar >= 0x0400 && uchar <= 0x04ff)
		return CHAR_WIDTH_UNKNOWN;

	if (uchar < 0x1000)
		return CHAR_WIDTH_HALFWIDTH;

	// Halfwidth CJK Chars
	if (uchar >= 0xff61 && uchar <= 0xffdc)
		return CHAR_WIDTH_HALFWIDTH;

	// Halfwidth Symbols Variants
	if (uchar >= 0xffe8 && uchar <= 0xffee)
		return CHAR_WIDTH_HALFWIDTH;

	return CHAR_WIDTH_FULLWIDTH;
}



/***************************************************************************
    CORE IMPLEMENTATION
***************************************************************************/
#ifdef UI_COLOR_DISPLAY
/*-------------------------------------------------
    setup_palette - set up the ui palette
-------------------------------------------------*/

void ui_manager::setup_palette()
{
	static struct
	{
		const char *name;
		int color;
		UINT8 defval[3];
	} palette_decode_table[] =
	{
		{ OPTION_SYSTEM_BACKGROUND,     SYSTEM_COLOR_BACKGROUND,  { 16,16,48 } },
		{ OPTION_CURSOR_SELECTED_TEXT,  CURSOR_SELECTED_TEXT,     { 255,255,255 } },
		{ OPTION_CURSOR_SELECTED_BG,    CURSOR_SELECTED_BG,       { 60,120,240 } },
		{ OPTION_CURSOR_HOVER_TEXT,     CURSOR_HOVER_TEXT,        { 120,180,240 } },
		{ OPTION_CURSOR_HOVER_BG,       CURSOR_HOVER_BG,          { 32,32,0 } },
		{ OPTION_BUTTON_RED,            BUTTON_COLOR_RED,         { 255,64,64 } },
		{ OPTION_BUTTON_YELLOW,         BUTTON_COLOR_YELLOW,      { 255,238,0 } },
		{ OPTION_BUTTON_GREEN,          BUTTON_COLOR_GREEN,       { 0,255,64 } },
		{ OPTION_BUTTON_BLUE,           BUTTON_COLOR_BLUE,        { 0,170,255 } },
		{ OPTION_BUTTON_PURPLE,         BUTTON_COLOR_PURPLE,      { 170,0,255 } },
		{ OPTION_BUTTON_PINK,           BUTTON_COLOR_PINK,        { 255,0,170 } },
		{ OPTION_BUTTON_AQUA,           BUTTON_COLOR_AQUA,        { 0,255,204 } },
		{ OPTION_BUTTON_SILVER,         BUTTON_COLOR_SILVER,      { 255,0,255 } },
		{ OPTION_BUTTON_NAVY,           BUTTON_COLOR_NAVY,        { 255,160,0 } },
		{ OPTION_BUTTON_LIME,           BUTTON_COLOR_LIME,        { 190,190,190 } },
		{ NULL }
	};

	int i;

#ifdef TRANS_UI
	ui_transparency = 255;

	ui_transparency = machine().options().int_value(OPTION_UI_TRANSPARENCY);
	if (ui_transparency < 0 || ui_transparency > 255)
	{
		osd_printf_error(_("Illegal value for %s = %s\n"), OPTION_UI_TRANSPARENCY, machine().options().value(OPTION_UI_TRANSPARENCY));
		ui_transparency = 215;
	}
#endif /* TRANS_UI */

	for (i = 0; palette_decode_table[i].name; i++)
	{
		const char *value = machine().options().value(palette_decode_table[i].name);
		int col = palette_decode_table[i].color;
		int r = palette_decode_table[i].defval[0];
		int g = palette_decode_table[i].defval[1];
		int b = palette_decode_table[i].defval[2];
		int rate;

		if (value)
		{
			int pal[3];

			if (sscanf(value, "%d,%d,%d", &pal[0], &pal[1], &pal[2]) != 3 ||
				pal[0] < 0 || pal[0] >= 256 ||
				pal[1] < 0 || pal[1] >= 256 ||
				pal[2] < 0 || pal[2] >= 256 )
			{
				osd_printf_error(_("error: invalid value for palette: %s\n"), value);
				continue;
			}

			r = pal[0];
			g = pal[1];
			b = pal[2];
		}

		rate = 0xff;
#ifdef TRANS_UI
		if (col == UI_BACKGROUND_COLOR)
			rate = ui_transparency;
		else
		if (col == CURSOR_SELECTED_BG)
		{
			rate = ui_transparency / 2;
			if (rate < 128)
				rate = 128; //cursor should be visible
		}
#endif /* TRANS_UI */

		uifont_colortable[col] = rgb_t(rate, r, g, b);
	}
}
#endif /* UI_COLOR_DISPLAY */



static const UINT32 mouse_bitmap[] = {
	0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,
	0x09a46f30,0x81ac7c43,0x24af8049,0x00ad7d45,0x00a8753a,0x00a46f30,0x009f6725,0x009b611c,0x00985b14,0x0095560d,0x00935308,0x00915004,0x00904e02,0x008f4e01,0x008f4d00,0x008f4d00,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,
	0x00a16a29,0xa2aa783d,0xffbb864a,0xc0b0824c,0x5aaf7f48,0x09ac7b42,0x00a9773c,0x00a67134,0x00a26b2b,0x009e6522,0x009a5e19,0x00965911,0x0094550b,0x00925207,0x00915004,0x008f4e01,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,
	0x009a5e18,0x39a06827,0xffb97c34,0xffe8993c,0xffc88940,0xedac7c43,0x93ad7c44,0x2dac7c43,0x00ab793f,0x00a87438,0x00a46f30,0x00a06827,0x009c611d,0x00985c15,0x0095570e,0x00935309,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,
	0x00935308,0x00965810,0xcc9a5e19,0xffe78a21,0xfffb9929,0xfff49931,0xffd88e39,0xffb9813f,0xc9ac7c43,0x66ad7c44,0x0cac7a41,0x00a9773c,0x00a67134,0x00a26b2b,0x009e6522,0x009a5e19,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,
	0x008f4e01,0x00904e02,0x60925106,0xffba670a,0xfff88b11,0xfff98f19,0xfff99422,0xfff9982b,0xffe89434,0xffc9883c,0xf3ac7a41,0x9cad7c44,0x39ac7c43,0x00ab7a40,0x00a87539,0x00a56f31,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,
	0x008e4d00,0x008e4d00,0x098e4d00,0xea8f4d00,0xffee7f03,0xfff68407,0xfff6870d,0xfff78b15,0xfff78f1d,0xfff79426,0xfff49730,0xffd98d38,0xffbc823f,0xd2ac7c43,0x6fad7c44,0x12ac7b42,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,
	0x008e4d00,0x008e4d00,0x008e4c00,0x8a8e4c00,0xffc46800,0xfff37e00,0xfff37f02,0xfff38106,0xfff3830a,0xfff48711,0xfff48b19,0xfff58f21,0xfff5942b,0xffe79134,0xffcb863b,0xf9ac7a41,0xa5ac7c43,0x3fac7c43,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,
	0x008e4d00,0x008e4d00,0x008e4c00,0x218d4c00,0xfc8e4c00,0xffee7a00,0xfff07c00,0xfff17c00,0xfff17d02,0xfff17e04,0xfff18008,0xfff2830d,0xfff28614,0xfff38a1c,0xfff38f25,0xfff2932e,0xffd98b37,0xffbc813e,0xdbac7c43,0x78ad7c44,0x15ac7b42,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,
	0x008e4d00,0x008e4d00,0x008e4d00,0x008e4c00,0xb18d4c00,0xffcf6b00,0xffed7900,0xffed7900,0xffee7900,0xffee7a01,0xffee7a01,0xffee7b03,0xffee7c06,0xffef7e0a,0xffef8110,0xfff08618,0xfff08a20,0xfff18f2a,0xffe78f33,0xffcc863b,0xfcab7a40,0xaeac7c43,0x4bac7c43,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,
	0x008f4d00,0x008e4d00,0x008e4d00,0x008e4c00,0x488d4c00,0xffa85800,0xffe97500,0xffea7600,0xffea7600,0xffeb7600,0xffeb7600,0xffeb7600,0xffeb7701,0xffeb7702,0xffeb7804,0xffec7a07,0xffec7d0d,0xffec8013,0xffed851c,0xffee8a25,0xffee8f2e,0xffd98937,0xffbe813d,0xe4ab7a40,0x81ab7a40,0x1ba9763b,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,
	0x008f4d00,0x008e4d00,0x008e4d00,0x008e4c00,0x008d4c00,0xdb8d4c00,0xffd86c00,0xffe77300,0xffe77300,0xffe87300,0xffe87300,0xffe87300,0xffe87300,0xffe87300,0xffe87401,0xffe87401,0xffe87503,0xffe97606,0xffe9780a,0xffe97c10,0xffea7f16,0xffeb831d,0xffeb8623,0xffe48426,0xffc67725,0xffa5661f,0xb7985c15,0x54935309,0x038e4d00,0x00ffffff,0x00ffffff,0x00ffffff,
	0x008f4d00,0x008e4d00,0x008e4d00,0x008e4d00,0x008e4c00,0x6f8d4c00,0xffb25b00,0xffe36f00,0xffe47000,0xffe47000,0xffe57000,0xffe57000,0xffe57000,0xffe57000,0xffe57000,0xffe57000,0xffe57000,0xffe57000,0xffe57101,0xffe57000,0xffe47000,0xffe16e00,0xffde6c00,0xffd86900,0xffd06600,0xffc76200,0xffaa5500,0xff8a4800,0xea743f00,0x5a7a4200,0x00ffffff,0x00ffffff,
	0x008f4d00,0x008f4d00,0x008e4d00,0x008e4d00,0x008e4c00,0x0f8d4c00,0xf38d4c00,0xffdc6a00,0xffe16d00,0xffe16d00,0xffe26d00,0xffe26d00,0xffe26d00,0xffe26d00,0xffe26d00,0xffe16d00,0xffe06c00,0xffde6b00,0xffd96900,0xffd16500,0xffc76000,0xffb95900,0xffab5200,0xff9c4b00,0xff894300,0xff6b3600,0xf9512c00,0xa5542d00,0x3c5e3200,0x00ffffff,0x00ffffff,0x00ffffff,
	0x008f4d00,0x008f4d00,0x008e4d00,0x008e4d00,0x008e4c00,0x008d4c00,0x968d4c00,0xffbc5d00,0xffde6a00,0xffde6a00,0xffde6a00,0xffdf6a00,0xffdf6a00,0xffdf6a00,0xffde6a00,0xffdc6800,0xffd66600,0xffcc6100,0xffbf5b00,0xffaf5300,0xff9d4a00,0xff8a4200,0xff6d3500,0xff502900,0xe7402300,0x7b3f2200,0x15442500,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,
	0x008f4d00,0x008f4d00,0x008f4d00,0x008e4d00,0x008e4d00,0x008e4c00,0x2a8d4c00,0xff9b5000,0xffda6600,0xffdb6700,0xffdb6700,0xffdc6700,0xffdc6700,0xffdb6700,0xffd96500,0xffd16200,0xffc25b00,0xffad5100,0xff974700,0xff7f3c00,0xff602f00,0xff472500,0xbd3d2100,0x513d2100,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,
	0x008f4d00,0x008f4d00,0x008f4d00,0x008e4d00,0x008e4d00,0x008e4c00,0x008e4c00,0xc08d4c00,0xffc35c00,0xffd76300,0xffd76300,0xffd86300,0xffd86300,0xffd76300,0xffd06000,0xffc05800,0xffa54c00,0xff7f3b00,0xff582c00,0xf03f2200,0x903c2000,0x2a3e2100,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,
	0x008f4d00,0x008f4d00,0x008f4d00,0x008f4d00,0x008e4d00,0x008e4d00,0x008e4c00,0x548d4c00,0xffa55200,0xffd35f00,0xffd46000,0xffd46000,0xffd46000,0xffd25e00,0xffc65900,0xffac4e00,0xff833c00,0xe7472600,0x693c2000,0x0c3d2100,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,
	0x008f4d00,0x008f4d00,0x008f4d00,0x008f4d00,0x008e4d00,0x008e4d00,0x008e4c00,0x038d4c00,0xe48d4c00,0xffc95a00,0xffd15d00,0xffd15d00,0xffd15d00,0xffcb5a00,0xffb95200,0xff984300,0xff5f2e00,0x723f2200,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,
	0x008f4d00,0x008f4d00,0x008f4d00,0x008f4d00,0x008e4d00,0x008e4d00,0x008e4d00,0x008e4c00,0x7b8d4c00,0xffad5200,0xffce5a00,0xffce5a00,0xffcd5900,0xffc35500,0xffaa4a00,0xff853a00,0xf9472600,0x15432400,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,
	0x008f4d00,0x008f4d00,0x008f4d00,0x008f4d00,0x008f4d00,0x008e4d00,0x008e4d00,0x008e4c00,0x188d4c00,0xf98e4c00,0xffc95600,0xffcb5700,0xffc75500,0xffb94f00,0xff9b4200,0xff6c3100,0xab442500,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,
	0x008f4d00,0x008f4d00,0x008f4d00,0x008f4d00,0x008f4d00,0x008e4d00,0x008e4d00,0x008e4d00,0x008e4c00,0xa58d4c00,0xffb35000,0xffc75300,0xffc05000,0xffac4800,0xff8b3a00,0xff542a00,0x45462500,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,
	0x008f4d00,0x008f4d00,0x008f4d00,0x008f4d00,0x008f4d00,0x008f4d00,0x008e4d00,0x008e4d00,0x008e4c00,0x398d4c00,0xff994d00,0xffc24f00,0xffb74b00,0xff9e4000,0xff763200,0xde472600,0x03492800,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,
	0x008f4d00,0x008f4d00,0x008f4d00,0x008f4d00,0x008f4d00,0x008f4d00,0x008e4d00,0x008e4d00,0x008e4c00,0x008e4c00,0xcf8d4c00,0xffb24b00,0xffab4500,0xff8d3900,0xff5e2b00,0x7e452500,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,
	0x008f4d00,0x008f4d00,0x008f4d00,0x008f4d00,0x008f4d00,0x008f4d00,0x008e4d00,0x008e4d00,0x008e4d00,0x008e4c00,0x638d4c00,0xff984800,0xffa03f00,0xff7e3200,0xfc492800,0x1b472600,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,
	0x008f4d00,0x008f4d00,0x008f4d00,0x008f4d00,0x008f4d00,0x008f4d00,0x008f4d00,0x008e4d00,0x008e4d00,0x008e4c00,0x098b4b00,0xed824600,0xff903800,0xff692c00,0xb4462600,0x004c2900,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,
	0x008f4d00,0x008f4d00,0x008f4d00,0x008f4d00,0x008f4d00,0x008f4d00,0x008f4d00,0x008e4d00,0x008e4d00,0x008e4c00,0x008a4a00,0x8a7e4400,0xff793500,0xff572900,0x51472600,0x00542d00,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,
	0x008f4d00,0x008f4d00,0x008f4d00,0x008f4d00,0x008f4d00,0x008f4d00,0x008f4d00,0x008f4d00,0x008e4d00,0x008d4c00,0x00884900,0x247a4200,0xfc633500,0xe74f2a00,0x034d2900,0x005e3300,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,
	0x008f4d00,0x008f4d00,0x008f4d00,0x008f4d00,0x008f4d00,0x008f4d00,0x008f4d00,0x008f4d00,0x008e4d00,0x008d4c00,0x00884900,0x00794100,0xb4643600,0x87552e00,0x00593000,0x006b3900,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,
	0x008f4d00,0x008f4d00,0x008f4d00,0x008f4d00,0x008f4d00,0x008f4d00,0x008f4d00,0x008f4d00,0x008f4d00,0x008d4c00,0x00884900,0x007c4300,0x486d3b00,0x24643600,0x00693800,0x00774000,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,
	0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,
	0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff
};


//-------------------------------------------------
//  ctor - set up the user interface
//-------------------------------------------------

ui_manager::ui_manager(running_machine &machine)
	: m_machine(machine)
{
#ifdef UI_COLOR_DISPLAY
	setup_palette();
#endif /* UI_COLOR_DISPLAY */
	build_bgtexture();
	ui_bgcolor = UI_BACKGROUND_COLOR;

	// initialize the other UI bits
	ui_menu::init(machine);
	ui_gfx_init(machine);

#ifdef CMD_LIST
	datafile_init(machine, &machine.options());
#endif /* CMD_LIST */

#ifdef USE_SHOW_TIME
	show_time = 0;
	Show_Time_Position = 0;
#endif /* USE_SHOW_TIME */


	// reset instance variables
	m_font = NULL;
	m_handler_callback = NULL;
	m_handler_param = 0;
	m_single_step = false;
	m_showfps = false;
	m_showfps_end = false;
	m_show_profiler = false;
	m_popup_text_end = 0;
	m_use_natural_keyboard = false;
	m_mouse_arrow_texture = NULL;

	// more initialization
	set_handler(handler_messagebox, 0);
	m_non_char_keys_down = auto_alloc_array(machine, UINT8, (ARRAY_LENGTH(non_char_keys) + 7) / 8);
	m_mouse_show = machine.system().flags & GAME_CLICKABLE_ARTWORK ? true : false;

	// request a callback upon exiting
	machine.add_notifier(MACHINE_NOTIFY_EXIT, machine_notify_delegate(FUNC(ui_manager::exit), this));

	// retrieve options
	m_use_natural_keyboard = machine.options().natural_keyboard();
	bitmap_argb32 *ui_mouse_bitmap = auto_alloc(machine, bitmap_argb32(32, 32));
	UINT32 *dst = &ui_mouse_bitmap->pix32(0);
	memcpy(dst,mouse_bitmap,32*32*sizeof(UINT32));
	m_mouse_arrow_texture = machine.render().texture_alloc();
	m_mouse_arrow_texture->set_bitmap(*ui_mouse_bitmap, ui_mouse_bitmap->cliprect(), TEXFORMAT_ARGB32);
}


//-------------------------------------------------
//  exit - clean up ourselves on exit
//-------------------------------------------------

void ui_manager::exit()
{
	// free the mouse texture
	machine().render().texture_free(m_mouse_arrow_texture);
	m_mouse_arrow_texture = NULL;

	// free the font
	if (m_font != NULL)
	{
		machine().render().font_free(m_font);
		m_font = NULL;
	}
}


//-------------------------------------------------
//  initialize - initialize ui lists
//-------------------------------------------------

void ui_manager::initialize(running_machine &machine)
{
	// initialize the on-screen display system
	slider_list = slider_current = slider_init(machine);
}


//-------------------------------------------------
//  set_handler - set a callback/parameter
//  pair for the current UI handler
//-------------------------------------------------

UINT32 ui_manager::set_handler(ui_callback callback, UINT32 param)
{
	m_handler_callback = callback;
	m_handler_param = param;
	return param;
}

#ifdef UI_COLOR_DISPLAY
rgb_t ui_manager::get_rgb_color(rgb_t color)
{
	if (color < MAX_COLORTABLE)
		return uifont_colortable[color];

	return color;
}
#endif /* UI_COLOR_DISPLAY */




//-------------------------------------------------
//  display_startup_screens - display the
//  various startup screens
//-------------------------------------------------

void ui_manager::display_startup_screens(bool first_time, bool show_disclaimer)
{
	const int maxstate = 4;
	int str = machine().options().seconds_to_run();
	bool show_gameinfo = !machine().options().skip_gameinfo();
	bool show_warnings = !machine().options().skip_gameinfo();
	bool show_mandatory_fileman = !machine().options().skip_gameinfo();
	int state;

	// disable everything if we are using -str for 300 or fewer seconds, or if we're the empty driver,
	// or if we are debugging
	if (!first_time || (str > 0 && str < 60*5) || &machine().system() == &GAME_NAME(___empty) || (machine().debug_flags & DEBUG_FLAG_ENABLED) != 0)
		show_gameinfo = show_warnings = show_disclaimer = show_mandatory_fileman = FALSE;

	#if defined(EMSCRIPTEN)
	// also disable for the JavaScript port since the startup screens do not run asynchronously
	show_gameinfo = show_warnings = show_disclaimer = FALSE;
	#endif

#ifdef KAILLERA
	if (kPlay)
		show_gameinfo = show_warnings = show_disclaimer = FALSE;
#endif /* KAILLERA */

	// loop over states
	set_handler(handler_ingame, 0);
	for (state = 0; state < maxstate && !machine().scheduled_event_pending() && !ui_menu::stack_has_special_main_menu(); state++)
	{
		// default to standard colors
		messagebox_backcolor = UI_BACKGROUND_COLOR;

		// pick the next state
		switch (state)
		{
			case 0:
				if (show_disclaimer && disclaimer_string(messagebox_text).length() > 0)
					set_handler(handler_messagebox_ok, 0);
				break;

			case 1:
				if (show_warnings && warnings_string(messagebox_text).length() > 0)
				{
					set_handler(handler_messagebox_ok, 0);
					if (machine().system().flags & (GAME_WRONG_COLORS | GAME_IMPERFECT_COLORS | GAME_REQUIRES_ARTWORK | GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND | GAME_IMPERFECT_KEYBOARD | GAME_NO_SOUND))
						messagebox_backcolor = UI_YELLOW_COLOR;
					if (machine().system().flags & (GAME_NOT_WORKING | GAME_UNEMULATED_PROTECTION | GAME_MECHANICAL))
						messagebox_backcolor = UI_RED_COLOR;
				}
				break;

			case 2:
				if (show_gameinfo && game_info_astring(messagebox_text).length() > 0)
					set_handler(handler_messagebox_anykey, 0);
				break;

			case 3:
				if (show_mandatory_fileman && image_mandatory_scan(machine(), messagebox_text).length() > 0)
				{
					std::string warning;
					warning.assign("This driver requires images to be loaded in the following device(s): ").append(messagebox_text.substr(0, messagebox_text.length() - 2));
					ui_menu_file_manager::force_file_manager(machine(), &machine().render().ui_container(), warning.c_str());
				}
				break;
		}

		// clear the input memory
		machine().input().reset_polling();
		while (machine().input().poll_switches() != INPUT_CODE_INVALID) ;

		// loop while we have a handler
		while (m_handler_callback != handler_ingame && !machine().scheduled_event_pending() && !ui_menu::stack_has_special_main_menu())
		{
			machine().manager().web()->serve();
			machine().video().frame_update();
		}

		// clear the handler and force an update
		set_handler(handler_ingame, 0);
		machine().video().frame_update();
	}

	// if we're the empty driver, force the menus on
	if (ui_menu::stack_has_special_main_menu())
		set_handler(ui_menu::ui_handler, 0);
}


//-------------------------------------------------
//  set_startup_text - set the text to display
//  at startup
//-------------------------------------------------

void ui_manager::set_startup_text(const char *text, bool force)
{
	static osd_ticks_t lastupdatetime = 0;
	osd_ticks_t curtime = osd_ticks();

	// copy in the new text
	messagebox_text.assign(text);
	messagebox_backcolor = UI_BACKGROUND_COLOR;

	// don't update more than 4 times/second
	if (force || (curtime - lastupdatetime) > osd_ticks_per_second() / 4)
	{
		lastupdatetime = curtime;
		machine().video().frame_update();
	}
}


//-------------------------------------------------
//  update_and_render - update the UI and
//  render it; called by video.c
//-------------------------------------------------

void ui_manager::update_and_render(render_container *container)
{
#ifdef MAME_AVI
	extern void avi_info_view(running_machine &machine);
#endif /* MAME_AVI */

	// always start clean
	container->empty();

	// if we're paused, dim the whole screen
	if (machine().phase() >= MACHINE_PHASE_RESET && (single_step() || machine().paused()))
	{
		int alpha = (1.0f - machine().options().pause_brightness()) * 255.0f;
		if (ui_menu::stack_has_special_main_menu())
			alpha = 255;
		if (alpha > 255)
			alpha = 255;
		if (alpha >= 0)
			container->add_rect(0.0f, 0.0f, 1.0f, 1.0f, rgb_t(alpha,0x00,0x00,0x00), PRIMFLAG_BLENDMODE(BLENDMODE_ALPHA));
	}

	// render any cheat stuff at the bottom
	if (machine().phase() >= MACHINE_PHASE_RESET)
		machine().cheat().render_text(*container);

	// call the current UI handler
	assert(m_handler_callback != NULL);
	m_handler_param = (*m_handler_callback)(machine(), container, m_handler_param);

	// display any popup messages
	if (osd_ticks() < m_popup_text_end)
		draw_text_box(container, messagebox_poptext.c_str(), JUSTIFY_CENTER, 0.5f, 0.9f, messagebox_backcolor);
	else
		m_popup_text_end = 0;

	if (m_mouse_show || (is_menu_active() && machine().options().ui_mouse()))
	{
		INT32 mouse_target_x, mouse_target_y;
		bool mouse_button;
		render_target *mouse_target = ui_input_find_mouse(machine(), &mouse_target_x, &mouse_target_y, &mouse_button);

		if (mouse_target != NULL)
		{
			float mouse_y=-1,mouse_x=-1;
			if (mouse_target->map_point_container(mouse_target_x, mouse_target_y, *container, mouse_x, mouse_y)) {
				container->add_quad(mouse_x,mouse_y,mouse_x + 0.05f*container->manager().ui_aspect(container),mouse_y + 0.05f,UI_TEXT_COLOR,m_mouse_arrow_texture,PRIMFLAG_BLENDMODE(BLENDMODE_ALPHA));
			}
		}
	}

	// cancel takes us back to the ingame handler
	if (m_handler_param == UI_HANDLER_CANCEL)
		set_handler(handler_ingame, 0);

#ifdef MAME_AVI
    if (bAviRun) avi_info_view(machine());
#endif /* MAME_AVI */
}


//-------------------------------------------------
//  get_font - return the UI font
//-------------------------------------------------

render_font *ui_manager::get_font()
{
	// allocate the font and messagebox string
	if (m_font == NULL)
		m_font = machine().render().font_alloc(machine().options().ui_font());
	return m_font;
}


//-------------------------------------------------
//  get_line_height - return the current height
//  of a line
//-------------------------------------------------

float ui_manager::get_line_height()
{
	INT32 raw_font_pixel_height = get_font()->pixel_height();
	render_target &ui_target = machine().render().ui_target();
	INT32 target_pixel_height = ui_target.height();
	float one_to_one_line_height;
	float scale_factor;

	/* mamep: to avoid division by zero */
	if (target_pixel_height == 0)
		return 0.0f;

	// compute the font pixel height at the nominal size
	one_to_one_line_height = (float)raw_font_pixel_height / (float)target_pixel_height;

	// determine the scale factor
	scale_factor = UI_TARGET_FONT_HEIGHT / one_to_one_line_height;

	// if our font is small-ish, do integral scaling
	if (raw_font_pixel_height < 24)
	{
		// do we want to scale smaller? only do so if we exceed the threshhold
		if (scale_factor <= 1.0f)
		{
			if (one_to_one_line_height < UI_MAX_FONT_HEIGHT || raw_font_pixel_height < 12)
				scale_factor = 1.0f;
		}

		// otherwise, just ensure an integral scale factor
		else
			scale_factor = floor(scale_factor);
	}

	// otherwise, just make sure we hit an even number of pixels
	else
	{
		INT32 height = scale_factor * one_to_one_line_height * (float)target_pixel_height;
		scale_factor = (float)height / (one_to_one_line_height * (float)target_pixel_height);
	}

	return scale_factor * one_to_one_line_height;
}


//-------------------------------------------------
//  get_char_width - return the width of a
//  single character
//-------------------------------------------------

float ui_manager::get_char_width(unicode_char ch)
{
	return get_font()->char_width(get_line_height(), machine().render().ui_aspect(), ch);
}
//mamep: to render as fixed-width font
float ui_manager::get_char_width_no_margin(unicode_char ch)
{
	return get_font()->char_width_no_margin(get_line_height(), machine().render().ui_aspect(), ch);
}


float ui_manager::get_char_fixed_width(unicode_char uchar, double halfwidth, double fullwidth)
{
	float chwidth;

	switch (is_fullwidth_char(uchar))
	{
	case CHAR_WIDTH_HALFWIDTH:
		return halfwidth;

	case CHAR_WIDTH_UNKNOWN:
		chwidth = get_char_width_no_margin(uchar);
		if (chwidth <= halfwidth)
			return halfwidth;
	}

	return fullwidth;
}



//-------------------------------------------------
//  get_string_width - return the width of a
//  character string
//-------------------------------------------------

float ui_manager::get_string_width(const char *s)
{
	return get_font()->utf8string_width(get_line_height(), machine().render().ui_aspect(), s);
}


//-------------------------------------------------
//  ui_draw_box - add primitives to draw
//  a box with the given background color
//-------------------------------------------------

void ui_manager::draw_box(render_container *container, float x0, float y0, float x1, float y1, rgb_t backcolor)
{
#ifdef UI_COLOR_DISPLAY
	if (backcolor == UI_BACKGROUND_COLOR)
		container->add_quad(x0, y0, x1, y1, rgb_t(0xff, 0xff, 0xff, 0xff), bgtexture, PRIMFLAG_BLENDMODE(BLENDMODE_ALPHA));
	else
#endif /* UI_COLOR_DISPLAY */
		container->add_rect(x0, y0, x1, y1, backcolor, PRIMFLAG_BLENDMODE(BLENDMODE_ALPHA));
}


//-------------------------------------------------
//  draw_outlined_box - add primitives to draw
//  an outlined box with the given background
//  color
//-------------------------------------------------

void ui_manager::draw_outlined_box(render_container *container, float x0, float y0, float x1, float y1, rgb_t backcolor)
{
	draw_outlined_box(container, x0, y0, x1, y1, UI_BORDER_COLOR, backcolor);
}


//-------------------------------------------------
//  draw_outlined_box - add primitives to draw
//  an outlined box with the given background
//  color
//-------------------------------------------------

void ui_manager::draw_outlined_box(render_container *container, float x0, float y0, float x1, float y1, rgb_t fgcolor, rgb_t bgcolor)
{
	draw_box(container, x0, y0, x1, y1, bgcolor);
	container->add_line(x0, y0, x1, y0, UI_LINE_WIDTH, fgcolor, PRIMFLAG_BLENDMODE(BLENDMODE_ALPHA));
	container->add_line(x1, y0, x1, y1, UI_LINE_WIDTH, fgcolor, PRIMFLAG_BLENDMODE(BLENDMODE_ALPHA));
	container->add_line(x1, y1, x0, y1, UI_LINE_WIDTH, fgcolor, PRIMFLAG_BLENDMODE(BLENDMODE_ALPHA));
	container->add_line(x0, y1, x0, y0, UI_LINE_WIDTH, fgcolor, PRIMFLAG_BLENDMODE(BLENDMODE_ALPHA));
}


//-------------------------------------------------
//  draw_text - simple text renderer
//-------------------------------------------------

void ui_manager::draw_text(render_container *container, const char *buf, float x, float y)
{
	draw_text_full(container, buf, x, y, 1.0f - x, JUSTIFY_LEFT, WRAP_WORD, DRAW_NORMAL, UI_TEXT_COLOR, UI_TEXT_BG_COLOR, NULL, NULL);
}


#if defined(MAME_AVI) || defined(KAILLERA)
void ui_manager::draw_text2(render_container *container, const char *buf, float x, float y, int color)
{
	ui_manager::draw_text_full(container, buf, x, y, 1.0f - x, JUSTIFY_LEFT, WRAP_WORD, DRAW_OPAQUE, ARGB_BLACK, color, NULL, NULL);
}
#endif

#ifdef KAILLERA
void ui_manager::draw_colortext(render_container *container, const char *buf, float x, float y, int col)
{
	draw_text_full(container, buf, x, y, 1.0f - x, JUSTIFY_LEFT, WRAP_WORD, DRAW_OPAQUE, col, ui_bgcolor, NULL, NULL);
}

void ui_manager::draw_chattext(render_container *container, const char *buf, float x, float y, int mode, float *totalheight)
{
	const int posx[12] = { 0,-2, 2, 0, 0,-1, 1, 0,-1,-1, 1, 1};
	const int posy[12] = {-2, 0, 0, 2,-1, 0, 0, 1,-1, 1,-1, 1};

	#define ARGB_CHATEDGE ARGB_BLACK

	switch (mode) {
	case 1:
		draw_text_full(container, buf, x, y, 1.0f - x, JUSTIFY_LEFT, WRAP_WORD, DRAW_OPAQUE, ARGB_WHITE, ui_bgcolor, NULL, totalheight);
		break;
	case 2:
		draw_text_full(container, buf, x, y, 1.0f - x, JUSTIFY_LEFT, WRAP_WORD, DRAW_NORMAL, ARGB_WHITE, ui_bgcolor, NULL, totalheight);
		break;
	case 3:
		{
			int i=4,j=8;
			int x1, y1;

			for (; i<j; i++)
			{
				x1 = x + posx[i];
				y1 = y + posy[i];
				draw_text_full(container, buf, x1, y1, 1.0f - x1, JUSTIFY_LEFT, WRAP_WORD, DRAW_NORMAL, ARGB_CHATEDGE, ui_bgcolor, NULL, totalheight);
			}
		}
		draw_text_full(container, buf, x, y, 1.0f - x, JUSTIFY_LEFT, WRAP_WORD, DRAW_NORMAL, ARGB_WHITE, ui_bgcolor, NULL, totalheight);
		break;
	case 4:
		draw_text_full(container, buf, x, y, 1.0f - x, JUSTIFY_LEFT, WRAP_TRUNCATE, DRAW_OPAQUE, ARGB_WHITE, ui_bgcolor, NULL, totalheight);
		break;
	case 5:
		draw_text_full(container, buf, x, y, 1.0f - x, JUSTIFY_LEFT, WRAP_TRUNCATE, DRAW_NORMAL, ARGB_WHITE, ui_bgcolor, NULL, totalheight);
		break;
	default:
		{
			int i=4,j=8;
			float x1, y1;

			for (; i<j; i++)
			{
				x1 = x + (float)posx[i] * UI_LINE_WIDTH;
				y1 = y + (float)posy[i] * UI_LINE_WIDTH;
				draw_text_full(container, buf, x1, y1, 1.0f - x1, JUSTIFY_LEFT, WRAP_TRUNCATE, DRAW_NORMAL, ARGB_CHATEDGE, ui_bgcolor, NULL, totalheight);
			}
		}
		draw_text_full(container, buf, x, y, 1.0f - x, JUSTIFY_LEFT, WRAP_TRUNCATE, DRAW_NORMAL, ARGB_WHITE, ui_bgcolor, NULL, totalheight);
		break;
	}
}
#endif /* KAILLERA */


//-------------------------------------------------
//  draw_text_full - full featured text
//  renderer with word wrapping, justification,
//  and full size computation
//-------------------------------------------------

void ui_manager::draw_text_full(render_container *container, const char *origs, float x, float y, float origwrapwidth, int justify, int wrap, int draw, rgb_t fgcolor, rgb_t bgcolor, float *totalwidth, float *totalheight)
{
	float lineheight = get_line_height();
	const char *ends = origs + strlen(origs);
	float wrapwidth = origwrapwidth;
	const char *s = origs;
	const char *linestart;
	float cury = y;
	float maxwidth = 0;
	float aspect = machine().render().ui_aspect();
	const char *s_temp;
	const char *up_arrow = NULL;
	const char *down_arrow = _("(more)");

	//mamep: control scrolling text
	int curline = 0;

	//mamep: render as fixed-width font
	float fontwidth_halfwidth = 0.0f;
	float fontwidth_fullwidth = 0.0f;

	if (draw_text_fixed_mode)
	{
		int scharcount;
		int len = strlen(origs);
		int n;

		for (n = 0; len > 0; n += scharcount, len -= scharcount)
		{
			unicode_char schar;
			float scharwidth;

			scharcount = uchar_from_utf8(&schar, &origs[n], len);
			if (scharcount == -1)
				break;

			scharwidth = get_char_width_no_margin(schar);
			if (is_fullwidth_char(schar))
			{
				if (fontwidth_fullwidth < scharwidth)
					fontwidth_fullwidth = scharwidth;
			}
			else
			{
				if (fontwidth_halfwidth < scharwidth)
					fontwidth_halfwidth = scharwidth;
			}
		}

		if (fontwidth_fullwidth < fontwidth_halfwidth * 2.0f)
			fontwidth_fullwidth = fontwidth_halfwidth * 2.0f;
		if (fontwidth_halfwidth < fontwidth_fullwidth / 2.0f)
			fontwidth_halfwidth = fontwidth_fullwidth / 2.0f;
	}

	//mamep: check if we are scrolling
	if (draw_text_scroll_offset)
		up_arrow = _("(more)");
	if (draw_text_scroll_offset == multiline_text_box_target_lines - multiline_text_box_visible_lines)
		down_arrow = NULL;

	// if we don't want wrapping, guarantee a huge wrapwidth
	if (wrap == WRAP_NEVER)
		wrapwidth = 1000000.0f;
	if (wrapwidth <= 0)
		return;

	// loop over lines
	while (*s != 0)
	{
		const char *lastbreak = NULL;
		int line_justify = justify;
		unicode_char schar;
		int scharcount;
		float lastbreak_width = 0;
		float curwidth = 0;
		float curx = x;

		// get the current character
		scharcount = uchar_from_utf8(&schar, s, ends - s);
		if (scharcount == -1)
			break;

		// if the line starts with a tab character, center it regardless
		if (schar == '\t')
		{
			s += scharcount;
			line_justify = JUSTIFY_CENTER;
		}

		// remember the starting position of the line
		linestart = s;

		// loop while we have characters and are less than the wrapwidth
		while (*s != 0 && curwidth <= wrapwidth)
		{
			float chwidth;

			// get the current chcaracter
			scharcount = uchar_from_utf8(&schar, s, ends - s);
			if (scharcount == -1)
				break;

			// if we hit a newline, stop immediately
			if (schar == '\n')
				break;

			//mamep: render as fixed-width font
			if (draw_text_fixed_mode)
				chwidth = get_char_fixed_width(schar, fontwidth_halfwidth, fontwidth_fullwidth);
			else
				// get the width of this character
				chwidth = get_font()->char_width(lineheight, aspect, schar);


			// if we hit a space, remember the location and width *without* the space
			if (schar == ' ')
			{
				lastbreak = s;
				lastbreak_width = curwidth;
			}

			// add the width of this character and advance
			curwidth += chwidth;
			s += scharcount;

			// if we hit any non-space breakable character, remember the location and width
			// *with* the breakable character
			if (schar != ' ' && is_breakable_char(schar) && curwidth <= wrapwidth)
			{
				lastbreak = s;
				lastbreak_width = curwidth;
			}
		}

		// if we accumulated too much for the current width, we need to back off
		if (curwidth > wrapwidth)
		{
			// if we're word wrapping, back up to the last break if we can
			if (wrap == WRAP_WORD)
			{
				// if we hit a break, back up to there with the appropriate width
				if (lastbreak != NULL)
				{
					s = lastbreak;
					curwidth = lastbreak_width;
				}

				// if we didn't hit a break, back up one character
				else if (s > linestart)
				{
					// get the previous character
					s = (const char *)utf8_previous_char(s);
					scharcount = uchar_from_utf8(&schar, s, ends - s);
					if (scharcount == -1)
						break;

					//mamep: render as fixed-width font
					if (draw_text_fixed_mode)
						curwidth -= get_char_fixed_width(schar, fontwidth_halfwidth, fontwidth_fullwidth);
					else
						curwidth -= get_font()->char_width(lineheight, aspect, schar);
					// if back to 0, there is no space to draw even a single char
					if (curwidth <= 0)
						break;
				}
			}

			// if we're truncating, make sure we have enough space for the ...
			else if (wrap == WRAP_TRUNCATE)
			{
				// add in the width of the ...
				curwidth += 3.0f * get_font()->char_width(lineheight, aspect, '.');

				// while we are above the wrap width, back up one character
				while (curwidth > wrapwidth && s > linestart)
				{
					// get the previous character
					s = (const char *)utf8_previous_char(s);
					scharcount = uchar_from_utf8(&schar, s, ends - s);
					if (scharcount == -1)
						break;

					curwidth -= get_font()->char_width(lineheight, aspect, schar);
				}
			}
		}

		//mamep: add scrolling arrow
		if (draw != DRAW_NONE
		 && ((curline == 0 && up_arrow)
		 ||  (curline == multiline_text_box_visible_lines - 1 && down_arrow)))
		{
			if (curline == 0)
				linestart = up_arrow;
			else
				linestart = down_arrow;

			curwidth = get_string_width(linestart);
			ends = linestart + strlen(linestart);
			s_temp = ends;
			line_justify = JUSTIFY_CENTER;
		}
		else
			s_temp = s;

		// align according to the justfication
		if (line_justify == JUSTIFY_CENTER)
			curx += (origwrapwidth - curwidth) * 0.5f;
		else if (line_justify == JUSTIFY_RIGHT)
			curx += origwrapwidth - curwidth;

		// track the maximum width of any given line
		if (curwidth > maxwidth)
			maxwidth = curwidth;

		// if opaque, add a black box
		if (draw == DRAW_OPAQUE)
			draw_box(container, curx, cury, curx + curwidth, cury + lineheight, bgcolor);

		// loop from the line start and add the characters
		while (linestart < s_temp)
		{
			// get the current character
			unicode_char linechar;
			int linecharcount = uchar_from_utf8(&linechar, linestart, ends - linestart);
			if (linecharcount == -1)
				break;

			//mamep: consume the offset lines
			if (draw_text_scroll_offset == 0 && draw != DRAW_NONE)
			{
				//mamep: render as fixed-width font
				if (draw_text_fixed_mode)
				{
					float width = get_char_fixed_width(linechar, fontwidth_halfwidth, fontwidth_fullwidth);
					float xmargin = (width - get_char_width(linechar)) / 2.0f;

					container->add_char(curx + xmargin, cury, lineheight, machine().render().ui_aspect(), fgcolor, *get_font(), linechar);
					curx += width;
				}
				else
				{
					container->add_char(curx, cury, lineheight, aspect, fgcolor, *get_font(), linechar);
					curx += get_font()->char_width(lineheight, aspect, linechar);
				}
			}
			linestart += linecharcount;
		}

		// append ellipses if needed
		if (wrap == WRAP_TRUNCATE && *s != 0 && draw != DRAW_NONE)
		{
			container->add_char(curx, cury, lineheight, aspect, fgcolor, *get_font(), '.');
			curx += get_font()->char_width(lineheight, aspect, '.');
			container->add_char(curx, cury, lineheight, aspect, fgcolor, *get_font(), '.');
			curx += get_font()->char_width(lineheight, aspect, '.');
			container->add_char(curx, cury, lineheight, aspect, fgcolor, *get_font(), '.');
			curx += get_font()->char_width(lineheight, aspect, '.');
		}

		// if we're not word-wrapping, we're done
		if (wrap != WRAP_WORD)
			break;

		//mamep: text scrolling
		if (draw_text_scroll_offset > 0)
			draw_text_scroll_offset--;
		else
		// advance by a row
		{
			cury += lineheight;

			//mamep: skip overflow text
			//there's a bug when viewing the game information and bookkeeping,so we have to commet it
 			if (draw != DRAW_NONE && curline == multiline_text_box_visible_lines - 1 && down_arrow)
				break;

			//mamep: controll scrolling text
			if (draw_text_scroll_offset == 0)
				curline++;
		}

		// skip past any spaces at the beginning of the next line
		scharcount = uchar_from_utf8(&schar, s, ends - s);
		if (scharcount == -1)
			break;

		if (schar == '\n')
			s += scharcount;
		else
			while (*s && (schar < 0x80) && isspace(schar))
			{
				s += scharcount;
				scharcount = uchar_from_utf8(&schar, s, ends - s);
				if (scharcount == -1)
					break;
			}
	}

	// report the width and height of the resulting space
	if (totalwidth)
		*totalwidth = maxwidth;
	if (totalheight)
		*totalheight = cury - y;
}


int ui_manager::draw_text_set_fixed_width_mode(int mode)
{
	int mode_save = draw_text_fixed_mode;

	draw_text_fixed_mode = mode;

	return mode_save;
}


void ui_manager::draw_text_full_fixed_width(render_container *container, const char *origs, float x, float y, float wrapwidth, int justify, int wrap, int draw, rgb_t fgcolor, rgb_t bgcolor, float *totalwidth, float *totalheight)
{
	int mode_save = draw_text_set_fixed_width_mode(TRUE);

	draw_text_full(container, origs, x, y, wrapwidth, justify, wrap, draw, fgcolor, bgcolor, totalwidth, totalheight);
	draw_text_set_fixed_width_mode(mode_save);
}


void ui_manager::draw_text_full_scroll(render_container *container, const char *origs, float x, float y, float wrapwidth, int offset, int justify, int wrap, int draw, rgb_t fgcolor, rgb_t bgcolor, float *totalwidth, float *totalheight)
{
	int offset_save = draw_text_scroll_offset;

	draw_text_scroll_offset = offset;
	draw_text_full(container, origs, x, y, wrapwidth, justify, wrap, draw, fgcolor, bgcolor, totalwidth, totalheight);

	draw_text_scroll_offset = offset_save;
}


//-------------------------------------------------
//  draw_text_box - draw a multiline text
//  message with a box around it
//-------------------------------------------------
void ui_manager::draw_text_box_scroll(render_container *container, const char *text, int offset, int justify, float xpos, float ypos, rgb_t backcolor)
{
	float line_height = get_line_height();
	float max_width = 2.0f * ((xpos <= 0.5f) ? xpos : 1.0f - xpos) - 2.0f * UI_BOX_LR_BORDER;
	float target_width = max_width;
	float target_height = line_height;
	float target_x = 0, target_y = 0;
	float last_target_height = 0;

	// limit this iteration to a finite number of passes
	for (int pass = 0; pass < 5; pass++)
	{
		// determine the target location
		target_x = xpos - 0.5f * target_width;
		target_y = ypos - 0.5f * target_height;

		// make sure we stay on-screen
		if (target_x < UI_BOX_LR_BORDER)
			target_x = UI_BOX_LR_BORDER;
		if (target_x + target_width + UI_BOX_LR_BORDER > 1.0f)
			target_x = 1.0f - UI_BOX_LR_BORDER - target_width;
		if (target_y < UI_BOX_TB_BORDER)
			target_y = UI_BOX_TB_BORDER;
		if (target_y + target_height + UI_BOX_TB_BORDER > 1.0f)
			target_y = 1.0f - UI_BOX_TB_BORDER - target_height;

		// compute the multi-line target width/height
		draw_text_full(container, text, target_x, target_y, target_width + 0.00001f,
					justify, WRAP_WORD, DRAW_NONE, UI_TEXT_COLOR, UI_TEXT_BG_COLOR, &target_width, &target_height);

		multiline_text_box_target_lines = (int)(target_height / line_height + 0.5f);
		if (target_height > 1.0f - 2.0f * UI_BOX_TB_BORDER)
			target_height = floorf((1.0f - 2.0f * UI_BOX_TB_BORDER) / line_height) * line_height;
		multiline_text_box_visible_lines = (int)(target_height / line_height + 0.5f);

		// if we match our last value, we're done
		if (target_height == last_target_height)
			break;
		last_target_height = target_height;
	}

	// add a box around that
	draw_outlined_box(container, target_x - UI_BOX_LR_BORDER,
						target_y - UI_BOX_TB_BORDER,
						target_x + target_width + UI_BOX_LR_BORDER,
						target_y + target_height + UI_BOX_TB_BORDER, backcolor);
	draw_text_full_scroll(container, text, target_x, target_y, target_width + 0.00001f, offset,
				justify, WRAP_WORD, DRAW_NORMAL, UI_TEXT_COLOR, UI_TEXT_BG_COLOR, NULL, NULL);
}


void ui_manager::draw_text_box(render_container *container, const char *text, int justify, float xpos, float ypos, rgb_t backcolor)
{
	draw_text_box_scroll(container, text, message_window_scroll, justify, xpos, ypos, backcolor);
}


#if defined(CMD_LIST)
void ui_manager::draw_text_box_fixed_width(render_container *container, const char *text, int justify, float xpos, float ypos, rgb_t backcolor)
{
	int mode_save = draw_text_fixed_mode;

	draw_text_fixed_mode = 1;
	draw_text_box_scroll(container, text, message_window_scroll, justify, xpos, ypos, backcolor);

	draw_text_fixed_mode = mode_save;
}
#endif /* CMD_LIST */


int ui_manager::window_scroll_keys()
{
	static int counter = 0;
	static int fast = 6;
	int pan_lines;
	int max_scroll;
	int do_scroll = FALSE;

	max_scroll = multiline_text_box_target_lines - multiline_text_box_visible_lines;
	pan_lines = multiline_text_box_visible_lines - 2;

	if (scroll_reset)
	{
		message_window_scroll = 0;
		scroll_reset = 0;
	}

	/* up backs up by one item */
	if (ui_input_pressed_repeat(machine(), IPT_UI_UP, fast))
	{
		message_window_scroll--;
		do_scroll = TRUE;
	}

	/* down advances by one item */
	if (ui_input_pressed_repeat(machine(), IPT_UI_DOWN, fast))
	{
		message_window_scroll++;
		do_scroll = TRUE;
	}

	/* pan-up goes to previous page */
	if (ui_input_pressed_repeat(machine(), IPT_UI_PAGE_UP,8))
	{
		message_window_scroll -= pan_lines;
		do_scroll = TRUE;
	}

	/* pan-down goes to next page */
	if (ui_input_pressed_repeat(machine(), IPT_UI_PAGE_DOWN,8))
	{
		message_window_scroll += pan_lines;
		do_scroll = TRUE;
	}

	/* home goes to the start */
	if (ui_input_pressed(machine(), IPT_UI_HOME))
	{
		message_window_scroll = 0;
		do_scroll = TRUE;
	}

	/* end goes to the last */
	if (ui_input_pressed(machine(), IPT_UI_END))
	{
		message_window_scroll = max_scroll;
		do_scroll = TRUE;
	}

	if (message_window_scroll < 0)
		message_window_scroll = 0;
	if (message_window_scroll > max_scroll)
		message_window_scroll = max_scroll;

	if (machine().ioport().type_pressed(IPT_UI_UP,0) || machine().ioport().type_pressed(IPT_UI_DOWN,0))
	{
		if (++counter == 25)
		{
			fast--;
			if (fast < 1)
				fast = 0;

			counter = 0;
		}
	}
	else
	{
		fast = 6;
		counter = 0;
	}

	if (do_scroll)
		return -1;

	if (ui_input_pressed(machine(), IPT_UI_SELECT))
	{
		message_window_scroll = 0;
		return 1;
	}
	if (ui_input_pressed(machine(), IPT_UI_CANCEL))
	{
		message_window_scroll = 0;
		return 2;
	}

	return 0;
}

#ifdef KAILLERA
void ui_manager::displaychatlog(render_container *container, char *text)
{
	static char buf[65536];
	static int logsize = 0;
	//int selected = 0;
	int res;

	if (text)
	{
		strcpy(buf, text);
		logsize = strlen(text);
	}
	else
	{

		/* draw the text */
		machine().ui().draw_message_window(container, buf);

		res = machine().ui().window_scroll_keys();
		//if (res > 0)
			//return ui_menu_stack_pop();0

		if(ui_input_pressed(machine(), IPT_UI_KAILLERA_TEST1_9))
		{
			extern void KailleraChatLogClear(void);
			KailleraChatLogClear();
		}
	}

	//return selected;
}
#endif /* KAILLERA */


//-------------------------------------------------
//  popup_time - popup a message for a specific
//  amount of time
//-------------------------------------------------

void CLIB_DECL ui_manager::popup_time(int seconds, const char *text, ...)
{
	va_list arg;

	// extract the text
	va_start(arg,text);
	strvprintf(messagebox_poptext, text, arg);
	messagebox_backcolor = UI_BACKGROUND_COLOR;
	va_end(arg);

	// set a timer
	m_popup_text_end = osd_ticks() + osd_ticks_per_second() * seconds;
}


//-------------------------------------------------
//  show_fps_temp - show the FPS counter for
//  a specific period of time
//-------------------------------------------------

void ui_manager::show_fps_temp(double seconds)
{
	if (!m_showfps)
		m_showfps_end = osd_ticks() + seconds * osd_ticks_per_second();
}


//-------------------------------------------------
//  set_show_fps - show/hide the FPS counter
//-------------------------------------------------

void ui_manager::set_show_fps(bool show)
{
	m_showfps = show;
	if (!show)
	{
		m_showfps = 0;
		m_showfps_end = 0;
	}
}


//-------------------------------------------------
//  show_fps - return the current FPS
//  counter visibility state
//-------------------------------------------------

bool ui_manager::show_fps() const
{
	return m_showfps || (m_showfps_end != 0);
}


//-------------------------------------------------
//  show_fps_counter
//-------------------------------------------------

bool ui_manager::show_fps_counter()
{
	bool result = m_showfps || osd_ticks() < m_showfps_end;
	if (!result)
		m_showfps_end = 0;
	return result;
}


//-------------------------------------------------
//  set_show_profiler - show/hide the profiler
//-------------------------------------------------

void ui_manager::set_show_profiler(bool show)
{
	m_show_profiler = show;
	g_profiler.enable(show);
}


//-------------------------------------------------
//  show_profiler - return the current
//  profiler visibility state
//-------------------------------------------------

bool ui_manager::show_profiler() const
{
	return m_show_profiler;
}


//-------------------------------------------------
//  show_menu - show the menus
//-------------------------------------------------

void ui_manager::show_menu()
{
	set_handler(ui_menu::ui_handler, 0);
}


//-------------------------------------------------
//  show_mouse - change mouse status
//-------------------------------------------------

void ui_manager::show_mouse(bool status)
{
	m_mouse_show = status;
}


//-------------------------------------------------
//  is_menu_active - return true if the menu
//  UI handler is active
//-------------------------------------------------

bool ui_manager::is_menu_active(void)
{
	return (m_handler_callback == ui_menu::ui_handler);
}



/***************************************************************************
    TEXT GENERATORS
***************************************************************************/

//-------------------------------------------------
//  disclaimer_string - print the disclaimer
//  text to the given buffer
//-------------------------------------------------

std::string &ui_manager::disclaimer_string(std::string &str)
{
	str.assign(_("Usage of emulators in conjunction with ROMs you don't own is forbidden by copyright law.\n\n"));
	strcatprintf(str, _("IF YOU ARE NOT LEGALLY ENTITLED TO PLAY \"%s\" ON THIS EMULATOR, PRESS ESC.\n\n"), machine().system().description);
	str.append(_("Otherwise, type OK or move the joystick left then right to continue"));
	return str;
}


//-------------------------------------------------
//  warnings_string - print the warning flags
//  text to the given buffer
//-------------------------------------------------

std::string &ui_manager::warnings_string(std::string &str)
{
#define WARNING_FLAGS ( GAME_NOT_WORKING | \
						GAME_UNEMULATED_PROTECTION | \
						GAME_MECHANICAL | \
						GAME_WRONG_COLORS | \
						GAME_IMPERFECT_COLORS | \
						GAME_REQUIRES_ARTWORK | \
						GAME_NO_SOUND |  \
						GAME_IMPERFECT_SOUND |  \
						GAME_IMPERFECT_GRAPHICS | \
						GAME_IMPERFECT_KEYBOARD | \
						GAME_NO_COCKTAIL)

	str.clear();

	// if no warnings, nothing to return
	if (rom_load_warnings(machine()) == 0 && rom_load_knownbad(machine()) == 0 && !(machine().system().flags & WARNING_FLAGS) && software_load_warnings_message(machine()).length() == 0)
		return str;

	// add a warning if any ROMs were loaded with warnings
	if (rom_load_warnings(machine()) > 0)
	{
		str.append(_("One or more ROMs/CHDs for this "));
		str.append(emulator_info::get_gamenoun());
		str.append(_( " are incorrect. The "));
		str.append(emulator_info::get_gamenoun());
		str.append(_(" may not run correctly.\n"));
		if (machine().system().flags & WARNING_FLAGS)
			str.append("\n");
	}

	if (software_load_warnings_message(machine()).length()>0) {
		str.append(software_load_warnings_message(machine()));
		if (machine().system().flags & WARNING_FLAGS)
			str.append("\n");
	}
	// if we have at least one warning flag, print the general header
	if ((machine().system().flags & WARNING_FLAGS) || rom_load_knownbad(machine()) > 0)
	{
		str.append(_("There are known problems with this "));
		str.append(emulator_info::get_gamenoun());
		str.append("\n\n");

		// add a warning if any ROMs are flagged BAD_DUMP/NO_DUMP
		if (rom_load_knownbad(machine()) > 0) {
			str.append(_("One or more ROMs/CHDs for this "));
			str.append(emulator_info::get_gamenoun());
			str.append(_(" have not been correctly dumped.\n"));
		}
		// add one line per warning flag
		if (machine().system().flags & GAME_IMPERFECT_KEYBOARD)
			str.append(_("The keyboard emulation may not be 100% accurate.\n"));
		if (machine().system().flags & GAME_IMPERFECT_COLORS)
			str.append(_("The colors aren't 100% accurate.\n"));
		if (machine().system().flags & GAME_WRONG_COLORS)
			str.append(_("The colors are completely wrong.\n"));
		if (machine().system().flags & GAME_IMPERFECT_GRAPHICS)
			str.append(_("The video emulation isn't 100% accurate.\n"));
		if (machine().system().flags & GAME_IMPERFECT_SOUND)
			str.append(_("The sound emulation isn't 100% accurate.\n"));
		if (machine().system().flags & GAME_NO_SOUND) {
			str.append(_("The "));
			str.append(emulator_info::get_gamenoun());
			str.append(_(" lacks sound.\n"));
		}
		if (machine().system().flags & GAME_NO_COCKTAIL)
			str.append(_("Screen flipping in cocktail mode is not supported.\n"));

		// check if external artwork is present before displaying this warning?
		if (machine().system().flags & GAME_REQUIRES_ARTWORK) {
			str.append(_("The "));
			str.append(emulator_info::get_gamenoun());
			str.append(_(" requires external artwork files\n"));
		}

		// if there's a NOT WORKING, UNEMULATED PROTECTION or GAME MECHANICAL warning, make it stronger
		if (machine().system().flags & (GAME_NOT_WORKING | GAME_UNEMULATED_PROTECTION | GAME_MECHANICAL))
		{
			// add the strings for these warnings
			if (machine().system().flags & GAME_UNEMULATED_PROTECTION) {
				str.append(_("The "));
				str.append(emulator_info::get_gamenoun());
				str.append(_(" has protection which isn't fully emulated.\n"));
			}
			if (machine().system().flags & GAME_NOT_WORKING) {
				str.append(_("\nTHIS "));
				str.append(emulator_info::get_capgamenoun());
				str.append(_(" DOESN'T WORK. The emulation for this "));
				str.append(emulator_info::get_gamenoun());
				str.append(_(" is not yet complete. "
						"There is nothing you can do to fix this problem except wait for the developers to improve the emulation.\n"));
			}
			if (machine().system().flags & GAME_MECHANICAL) {
				str.append(_("\nCertain elements of this "));
				str.append(emulator_info::get_gamenoun());
				str.append(_(" cannot be emulated as it requires actual physical interaction or consists of mechanical devices. "
						"It is not possible to fully play this "));
				str.append(emulator_info::get_gamenoun());
				str.append(".\n");
			}

			// find the parent of this driver
			driver_enumerator drivlist(machine().options());
			int maindrv = drivlist.find(machine().system());
			int clone_of = drivlist.non_bios_clone(maindrv);
			if (clone_of != -1)
				maindrv = clone_of;

			// scan the driver list for any working clones and add them
			bool foundworking = false;
			while (drivlist.next())
				if (drivlist.current() == maindrv || drivlist.clone() == maindrv)
					if ((drivlist.driver().flags & (GAME_NOT_WORKING | GAME_UNEMULATED_PROTECTION | GAME_MECHANICAL)) == 0)
					{
						// this one works, add a header and display the name of the clone
						if (!foundworking) {
							str.append(_("\n\nThere are working clones of this "));
							str.append(emulator_info::get_gamenoun());
							str.append(": ");
						}
						else
							str.append(", ");
						str.append(drivlist.driver().name);
						foundworking = true;
					}

			if (foundworking)
				str.append("\n");
		}
	}

	// add the 'press OK' string
	str.append(_("\n\nType OK or move the joystick left then right to continue"));
	return str;
}


//-------------------------------------------------
//  game_info_std::string - populate an allocated
//  string with the game info text
//-------------------------------------------------

std::string &ui_manager::game_info_astring(std::string &str)
{
	// print description, manufacturer, and CPU:
	std::string tempstr;
	strprintf(str, _("%s\n%s %s\nDriver: %s\n\nCPU:\n"), _LST(machine().system().description), machine().system().year, _MANUFACT(machine().system().manufacturer), core_filename_extract_base(tempstr, machine().system().source_file).c_str());

	// loop over all CPUs
	execute_interface_iterator execiter(machine().root_device());
	tagmap_t<UINT8> exectags;
	for (device_execute_interface *exec = execiter.first(); exec != NULL; exec = execiter.next())
	{
		if (exectags.add(exec->device().tag(), 1, FALSE) == TMERR_DUPLICATE)
			continue;
		// get cpu specific clock that takes internal multiplier/dividers into account
		int clock = exec->device().clock();

		// count how many identical CPUs we have
		int count = 1;
		const char *name = exec->device().name();
		execute_interface_iterator execinneriter(machine().root_device());
		for (device_execute_interface *scan = execinneriter.first(); scan != NULL; scan = execinneriter.next())
		{
			if (exec->device().type() == scan->device().type() && strcmp(name, scan->device().name()) == 0 && exec->device().clock() == scan->device().clock())
				if (exectags.add(scan->device().tag(), 1, FALSE) != TMERR_DUPLICATE)
					count++;
		}

		// if more than one, prepend a #x in front of the CPU name
		if (count > 1)
			strcatprintf(str, "%d" UTF8_MULTIPLY, count);
		str.append(name);

		// display clock in kHz or MHz
		if (clock >= 1000000)
			strcatprintf(str, " %d.%06d" UTF8_NBSP "MHz\n", clock / 1000000, clock % 1000000);
		else
			strcatprintf(str, " %d.%03d" UTF8_NBSP "kHz\n", clock / 1000, clock % 1000);
	}

	// loop over all sound chips
	sound_interface_iterator snditer(machine().root_device());
	tagmap_t<UINT8> soundtags;
	bool found_sound = false;
	for (device_sound_interface *sound = snditer.first(); sound != NULL; sound = snditer.next())
	{
		if (soundtags.add(sound->device().tag(), 1, FALSE) == TMERR_DUPLICATE)
			continue;

		// append the Sound: string
		if (!found_sound)
			str.append(_("\nSound:\n"));
		found_sound = true;

		// count how many identical sound chips we have
		int count = 1;
		sound_interface_iterator sndinneriter(machine().root_device());
		for (device_sound_interface *scan = sndinneriter.first(); scan != NULL; scan = sndinneriter.next())
		{
			if (sound->device().type() == scan->device().type() && sound->device().clock() == scan->device().clock())
				if (soundtags.add(scan->device().tag(), 1, FALSE) != TMERR_DUPLICATE)
					count++;
		}
		// if more than one, prepend a #x in front of the CPU name
		if (count > 1)
			strcatprintf(str, "%d" UTF8_MULTIPLY, count);
		str.append(sound->device().name());

		// display clock in kHz or MHz
		int clock = sound->device().clock();
		if (clock >= 1000000)
			strcatprintf(str, " %d.%06d" UTF8_NBSP "MHz\n", clock / 1000000, clock % 1000000);
		else if (clock != 0)
			strcatprintf(str, " %d.%03d" UTF8_NBSP "kHz\n", clock / 1000, clock % 1000);
		else
			str.append("\n");
	}

	// display screen information
	str.append(_("\nVideo:\n"));
	screen_device_iterator scriter(machine().root_device());
	int scrcount = scriter.count();
	if (scrcount == 0)
		str.append(_("None\n"));
	else
	{
		for (screen_device *screen = scriter.first(); screen != NULL; screen = scriter.next())
		{
			if (scrcount > 1)
			{
				str.append(slider_get_screen_desc(*screen));
				str.append(": ");
			}

			if (screen->screen_type() == SCREEN_TYPE_VECTOR)
				str.append(_("Vector\n"));
			else
			{
				const rectangle &visarea = screen->visible_area();

				strcatprintf(str, "%d " UTF8_MULTIPLY " %d (%s) %f" UTF8_NBSP "Hz\n",
						visarea.width(), visarea.height(),
						(machine().system().flags & ORIENTATION_SWAP_XY) ? "V" : "H",
						ATTOSECONDS_TO_HZ(screen->frame_period().attoseconds));
			}
		}
	}

	return str;
}



/***************************************************************************
    UI HANDLERS
***************************************************************************/

//-------------------------------------------------
//  handler_messagebox - displays the current
//  messagebox_text string but handles no input
//-------------------------------------------------

UINT32 ui_manager::handler_messagebox(running_machine &machine, render_container *container, UINT32 state)
{
	machine.ui().draw_text_box(container, messagebox_text.c_str(), JUSTIFY_LEFT, 0.5f, 0.5f, messagebox_backcolor);
	return 0;
}


//-------------------------------------------------
//  handler_messagebox_ok - displays the current
//  messagebox_text string and waits for an OK
//-------------------------------------------------

UINT32 ui_manager::handler_messagebox_ok(running_machine &machine, render_container *container, UINT32 state)
{
	// draw a standard message window
	machine.ui().draw_text_box(container, messagebox_text.c_str(), JUSTIFY_LEFT, 0.5f, 0.5f, messagebox_backcolor);

	// an 'O' or left joystick kicks us to the next state
	if (state == 0 && (machine.input().code_pressed_once(KEYCODE_O) || ui_input_pressed(machine, IPT_UI_LEFT)))
		state++;

	// a 'K' or right joystick exits the state
	else if (state == 1 && (machine.input().code_pressed_once(KEYCODE_K) || ui_input_pressed(machine, IPT_UI_RIGHT)))
		state = UI_HANDLER_CANCEL;

	// if the user cancels, exit out completely
	else if (ui_input_pressed(machine, IPT_UI_CANCEL))
	{
		machine.schedule_exit();
		state = UI_HANDLER_CANCEL;
	}

	return state;
}


//-------------------------------------------------
//  handler_messagebox_anykey - displays the
//  current messagebox_text string and waits for
//  any keypress
//-------------------------------------------------

UINT32 ui_manager::handler_messagebox_anykey(running_machine &machine, render_container *container, UINT32 state)
{
	int res = machine.ui().window_scroll_keys();

	// draw a standard message window
	machine.ui().draw_text_box(container, messagebox_text.c_str(), JUSTIFY_LEFT, 0.5f, 0.5f, messagebox_backcolor);

	// if the user cancels, exit out completely
	if (res == 2)
	{
		machine.schedule_exit();
		state = UI_HANDLER_CANCEL;
	}

	// if select key is pressed, just exit
	if (res == 1)
	{
		// if any key is pressed, just exit
		if (machine.input().poll_switches() != INPUT_CODE_INVALID)
			state = UI_HANDLER_CANCEL;
	}

	return state;
}


//-------------------------------------------------
//  process_natural_keyboard - processes any
//  natural keyboard input
//-------------------------------------------------

void ui_manager::process_natural_keyboard()
{
	ui_event event;
	int i, pressed;
	input_item_id itemid;
	input_code code;
	UINT8 *key_down_ptr;
	UINT8 key_down_mask;

	// loop while we have interesting events
	while (ui_input_pop_event(machine(), &event))
	{
		// if this was a UI_EVENT_CHAR event, post it
		if (event.event_type == UI_EVENT_CHAR)
			machine().ioport().natkeyboard().post(event.ch);
	}

	// process natural keyboard keys that don't get UI_EVENT_CHARs
	for (i = 0; i < ARRAY_LENGTH(non_char_keys); i++)
	{
		// identify this keycode
		itemid = non_char_keys[i];
		code = machine().input().code_from_itemid(itemid);

		// ...and determine if it is pressed
		pressed = machine().input().code_pressed(code);

		// figure out whey we are in the key_down map
		key_down_ptr = &m_non_char_keys_down[i / 8];
		key_down_mask = 1 << (i % 8);

		if (pressed && !(*key_down_ptr & key_down_mask))
		{
			// this key is now down
			*key_down_ptr |= key_down_mask;

			// post the key
			machine().ioport().natkeyboard().post(UCHAR_MAMEKEY_BEGIN + code.item_id());
		}
		else if (!pressed && (*key_down_ptr & key_down_mask))
		{
			// this key is now up
			*key_down_ptr &= ~key_down_mask;
		}
	}
}


//-------------------------------------------------
//  increase_frameskip
//-------------------------------------------------

void ui_manager::increase_frameskip()
{
	// get the current value and increment it
	int newframeskip = machine().video().frameskip() + 1;
	if (newframeskip > MAX_FRAMESKIP)
		newframeskip = -1;
	machine().video().set_frameskip(newframeskip);

	// display the FPS counter for 2 seconds
	machine().ui().show_fps_temp(2.0);
}


//-------------------------------------------------
//  decrease_frameskip
//-------------------------------------------------

void ui_manager::decrease_frameskip()
{
	// get the current value and decrement it
	int newframeskip = machine().video().frameskip() - 1;
	if (newframeskip < -1)
		newframeskip = MAX_FRAMESKIP;
	machine().video().set_frameskip(newframeskip);

	// display the FPS counter for 2 seconds
	machine().ui().show_fps_temp(2.0);
}


//-------------------------------------------------
//  can_paste
//-------------------------------------------------

bool ui_manager::can_paste()
{
	// retrieve the clipboard text
	char *text = osd_get_clipboard_text();

	// free the string if allocated
	if (text != NULL)
		osd_free(text);

	// did we have text?
	return text != NULL;
}


//-------------------------------------------------
//  paste - does a paste from the keyboard
//-------------------------------------------------

void ui_manager::paste()
{
	// retrieve the clipboard text
	char *text = osd_get_clipboard_text();

	// was a result returned?
	if (text != NULL)
	{
		// post the text
		machine().ioport().natkeyboard().post_utf8(text);

		// free the string
		osd_free(text);
	}
}


//-------------------------------------------------
//  image_handler_ingame - execute display
//  callback function for each image device
//-------------------------------------------------

void ui_manager::image_handler_ingame()
{
	// run display routine for devices
	if (machine().phase() == MACHINE_PHASE_RUNNING)
	{
		image_interface_iterator iter(machine().root_device());
		for (device_image_interface *image = iter.first(); image != NULL; image = iter.next())
			image->call_display();
	}
}

#ifdef USE_SHOW_TIME

#define DISPLAY_AMPM 0

void ui_manager::display_time(render_container *container)
{
	char buf[20];
#if DISPLAY_AMPM
	char am_pm[] = "am";
#endif /* DISPLAY_AMPM */
	float width;
	time_t ltime;
	struct tm *today;
	float line_height = get_line_height();

	time(&ltime);
	today = localtime(&ltime);

#if DISPLAY_AMPM
	if( today->tm_hour > 12 )
	{
		strcpy( am_pm, "pm" );
		today->tm_hour -= 12;
	}
	if( today->tm_hour == 0 ) /* Adjust if midnight hour. */
		today->tm_hour = 12;
#endif /* DISPLAY_AMPM */

#if DISPLAY_AMPM
	sprintf(buf, "%02d:%02d:%02d %s", today->tm_hour, today->tm_min, today->tm_sec, am_pm);
#else
	sprintf(buf, "%02d:%02d:%02d", today->tm_hour, today->tm_min, today->tm_sec);
#endif /* DISPLAY_AMPM */
	width = get_string_width(buf) + UI_LINE_WIDTH * 2.0f;
	switch(Show_Time_Position)
	{
		case 0:
			draw_text_box_fixed_width(container, buf, JUSTIFY_LEFT, 1.0f - width, 1.0f - line_height, UI_BACKGROUND_COLOR);
			break;

		case 1:
			draw_text_box_fixed_width(container, buf, JUSTIFY_LEFT, 1.0f - width, 0.0f, UI_BACKGROUND_COLOR);
			break;

		case 2:
			draw_text_box_fixed_width(container, buf, JUSTIFY_LEFT, 0.0f + width, 0.0f, UI_BACKGROUND_COLOR);
			break;

		case 3:
			draw_text_box_fixed_width(container, buf, JUSTIFY_LEFT, 0.0f + width, 1.0f - line_height, UI_BACKGROUND_COLOR);
			break;
	}
}
#endif /* USE_SHOW_TIME */

#ifdef USE_SHOW_INPUT_LOG
/*-------------------------------------------------
    ui_display_input_log -
    show popup message if input exist any log
-------------------------------------------------*/

void ui_manager::display_input_log(render_container *container)
{
	double time_now = machine().time().as_double();
	double time_display = attotime::from_msec(1000).as_double();
	double time_fadeout = attotime::from_msec(1000).as_double();
	float curx;
	int i;
	struct ioport_manager::input_log *command_buffer = machine().ioport().command_buffer();

	if (!command_buffer[0].code)
		return;

	// adjust time for load state
	{
		double max = 0.0f;
		int i;

		for (i = 0; command_buffer[i].code; i++)
			if (max < command_buffer[i].time)
				max = command_buffer[i].time;

		if (max > time_now)
		{
			double adjust = max - time_now;

			for (i = 0; command_buffer[i].code; i++)
				command_buffer[i].time -= adjust;
		}
	}

	// find position to start display
	curx = 1.0f - UI_LINE_WIDTH;
	for (i = 0; command_buffer[i].code; i++)
		curx -= get_char_width(command_buffer[i].code);

	for (i = 0; command_buffer[i].code; i++)
	{
		if (curx >= UI_LINE_WIDTH)
			break;

		curx += get_char_width(command_buffer[i].code);
	}

	draw_box(container, 0.0f, 1.0f - get_line_height(), 1.0f, 1.0f, UI_BACKGROUND_COLOR);

	for (; command_buffer[i].code; i++)
	{
		double rate = time_now - command_buffer[i].time;

		if (rate < time_display + time_fadeout)
		{
			int level = 255 - ((rate - time_display) / time_fadeout) * 255;
			rgb_t fgcolor;

			if (level > 255)
				level = 255;

			fgcolor = rgb_t(255, level, level, level);

			container->add_char(curx, 1.0f - get_line_height(), get_line_height(), machine().render().ui_aspect(), fgcolor, *get_font(), command_buffer[i].code);
		}
		curx += get_char_width(command_buffer[i].code);
	}
}
#endif /* USE_SHOW_INPUT_LOG */


//-------------------------------------------------
//  handler_ingame - in-game handler takes care
//  of the standard keypresses
//-------------------------------------------------

UINT32 ui_manager::handler_ingame(running_machine &machine, render_container *container, UINT32 state)
{
	bool is_paused = machine.paused();

#ifdef KAILLERA
	//kt start
	if( kPlay && Kaillera_StateSave_SelectFile ) {
		int file = 0;
		input_code code;
		machine.ui().draw_message_window( container, _("Select position (0-9, A-Z) to save to") );

		if (ui_input_pressed(machine, IPT_UI_CANCEL)) {
			Kaillera_StateSave_SelectFile = 0;
			return 0;
		}
		/* check for A-Z or 0-9 */
		for (input_item_id id = ITEM_ID_A; id <= ITEM_ID_Z; id++)
			if (machine.input().code_pressed_once(input_code(DEVICE_CLASS_KEYBOARD, 0, ITEM_CLASS_SWITCH, ITEM_MODIFIER_NONE, id)))
				file = id - ITEM_ID_A + 'a';
		if (file == 0)
			for (input_item_id id = ITEM_ID_0; id <= ITEM_ID_9; id++)
				if (machine.input().code_pressed_once(input_code(DEVICE_CLASS_KEYBOARD, 0, ITEM_CLASS_SWITCH, ITEM_MODIFIER_NONE, id)))
					file = id - ITEM_ID_0 + '0';
		if (file == 0)
			for (input_item_id id = ITEM_ID_0_PAD; id <= ITEM_ID_9_PAD; id++)
				if (machine.input().code_pressed_once(input_code(DEVICE_CLASS_KEYBOARD, 0, ITEM_CLASS_SWITCH, ITEM_MODIFIER_NONE, id)))
					file = id - ITEM_ID_0_PAD + '0';
		if (file > 0) {
			long dat[64];
			
			KailleraChatdataPreparationcheck.nmb			= 2;
			KailleraChatdataPreparationcheck.str			= (char *)"Select Slot";
			KailleraChatdataPreparationcheck.count			= KailleraPlayerOption.max;
			KailleraChatdataPreparationcheck.timeremainder	= 256;
			KailleraChatdataPreparationcheck.addtime		= 256;
			KailleraChatdataPreparationcheck.maxtime		= 256;
			KailleraChatdataPreparationcheck.Callback	= PreparationcheckNull;

			dat[0] = KailleraChatdataPreparationcheck.nmb;
			dat[1] = file;
			kailleraChatSend(kChatData(&dat[0], 8));//�`���b�g�őS���ɓ`����B
			Kaillera_StateSave_SelectFile = 0;
			return 0;
		}
		return 0;
	}

	if( kPlay && Kaillera_Overclock_Flags ) {
		int rate = 0;
		input_code code;
		machine.ui().draw_message_window( container, _("Please push overclock rate (1-8) x 50%") );
		
		if (ui_input_pressed(machine, IPT_UI_CANCEL)) {
			Kaillera_Overclock_Flags = 0;
			return 0;
		}
		for (input_item_id id = ITEM_ID_1; id <= ITEM_ID_8; id++)
			if (machine.input().code_pressed_once(input_code(DEVICE_CLASS_KEYBOARD, 0, ITEM_CLASS_SWITCH, ITEM_MODIFIER_NONE, id)))
				rate = id - ITEM_ID_0 + '0';
		if (rate > 0) {
			long dat[64];
			
			KailleraChatdataPreparationcheck.nmb			= 7;
			KailleraChatdataPreparationcheck.str			= (char *)"Overclock";
			KailleraChatdataPreparationcheck.count			= KailleraPlayerOption.max;
			KailleraChatdataPreparationcheck.timeremainder	= 256;
			KailleraChatdataPreparationcheck.addtime		= 256;
			KailleraChatdataPreparationcheck.maxtime		= 256;
			KailleraChatdataPreparationcheck.Callback	= SendOverclockParam;

			dat[0] = KailleraChatdataPreparationcheck.nmb;
			dat[1] = rate;
			kailleraChatSend(kChatData(&dat[0], 8));//�`���b�g�őS���ɓ`����B
			Kaillera_Overclock_Flags = 0;
			return 0;
		}
		return 0;
	}

	if (kPlay && quiting) {
		machine.ui().draw_message_window( container, _("Please press the [Y] key, for ending") );
		if (ui_input_pressed(machine, IPT_UI_CANCEL)) {
			quiting = 0;
			return 0;
		}
		if (machine.input().code_pressed_once(input_code(DEVICE_CLASS_KEYBOARD, 0, ITEM_CLASS_SWITCH, ITEM_MODIFIER_NONE, ITEM_ID_Y))) {
			quiting = 0;
			//if(code_pressed( KEYCODE_LSHIFT ) &&
			if(KailleraStartOption.player == 1 &&
				KailleraPlayerOption.max > 1) {
				long dat[64];
				dat[0] = 12;
				dat[1] = 0xffffffff;	//�S���Q�[���I��
				kailleraChatSend(kChatData(&dat[0], 8));

				return 0;
			}
			machine.schedule_exit();
			return 0;
		}



		//if (osd_quit_window() || quiting == 2 ) {
		if ( quiting == 2 ) {
			quiting = 0;
			machine.schedule_exit();
			return 0;
		}
		return 0;
	}
	//kt end

	if (kPlay)
		KailleraChatUpdate(machine, container);

	if (KailleraChatIsActive())
	{
		/* This call is for the cheat, it must be called once a frame */
		//if (options.cheat) DoCheat(bitmap);
	}
	else
	{
#endif /* KAILLERA */

	// first draw the FPS counter
	if (machine.ui().show_fps_counter())
	{
		std::string tempstring;
		machine.ui().draw_text_full_fixed_width(container, machine.video().speed_text(tempstring).c_str(), 0.0f, 0.0f, 1.0f,
					JUSTIFY_RIGHT, WRAP_WORD, DRAW_OPAQUE, ARGB_WHITE, ARGB_BLACK, NULL, NULL);
	}

	// draw the profiler if visible
	if (machine.ui().show_profiler())
	{
		const char *text = g_profiler.text(machine);
		machine.ui().draw_text_full(container, text, 0.0f, 0.0f, 1.0f, JUSTIFY_LEFT, WRAP_WORD, DRAW_OPAQUE, ARGB_WHITE, ui_bgcolor, NULL, NULL);
	}

	// if we're single-stepping, pause now
	if (machine.ui().single_step())
	{
		machine.pause();
		machine.ui().set_single_step(false);
	}

	// determine if we should disable the rest of the UI
	bool ui_disabled = (machine.ioport().has_keyboard() && !machine.ui_active());

	// is ScrLk UI toggling applicable here?
	if (machine.ioport().has_keyboard())
	{
		// are we toggling the UI with ScrLk?
		if (ui_input_pressed(machine, IPT_UI_TOGGLE_UI))
		{
			// toggle the UI
			machine.set_ui_active(!machine.ui_active());

			// display a popup indicating the new status
			if (machine.ui_active())
			{
				machine.ui().popup_time(2, "%s\n%s\n%s\n%s\n%s\n%s\n",
					"Keyboard Emulation Status",
					"-------------------------",
					"Mode: PARTIAL Emulation",
					"UI:   Enabled",
					"-------------------------",
					"**Use ScrLock to toggle**");
			}
			else
			{
				machine.ui().popup_time(2, "%s\n%s\n%s\n%s\n%s\n%s\n",
					"Keyboard Emulation Status",
					"-------------------------",
					"Mode: FULL Emulation",
					"UI:   Disabled",
					"-------------------------",
					"**Use ScrLock to toggle**");
			}
		}
	}

	// is the natural keyboard enabled?
	if (machine.ui().use_natural_keyboard() && (machine.phase() == MACHINE_PHASE_RUNNING))
		machine.ui().process_natural_keyboard();

	if (!ui_disabled)
	{
		// paste command
		if (ui_input_pressed(machine, IPT_UI_PASTE))
			machine.ui().paste();
	}

	machine.ui().image_handler_ingame();

	if (ui_disabled) return ui_disabled;

	if (ui_input_pressed(machine, IPT_UI_CANCEL))
	{
		machine.ui().request_quit();
		return 0;
	}

	// turn on menus if requested
	if (ui_input_pressed(machine, IPT_UI_CONFIGURE))
		return machine.ui().set_handler(ui_menu::ui_handler, 0);

	// if the on-screen display isn't up and the user has toggled it, turn it on
	if ((machine.debug_flags & DEBUG_FLAG_ENABLED) == 0 && ui_input_pressed(machine, IPT_UI_ON_SCREEN_DISPLAY))
		return machine.ui().set_handler(ui_menu_sliders::ui_handler, 1);

#ifdef KAILLERA
	//input_ui_temp = 0;	//kt
	if (!kPlay)
	{
#endif /* KAILLERA */
	// handle a reset request
	if (ui_input_pressed(machine, IPT_UI_RESET_MACHINE))
		machine.schedule_hard_reset();
	if (ui_input_pressed(machine, IPT_UI_SOFT_RESET))
#ifdef KAILLERA
		input_ui_temp = 3;
#else
		machine.schedule_soft_reset();
#endif /* KAILLERA */

	// handle a request to display graphics/palette
	if (ui_input_pressed(machine, IPT_UI_SHOW_GFX))
	{
		if (!is_paused)
			machine.pause();
		return machine.ui().set_handler(ui_gfx_ui_handler, is_paused);
	}

	// handle a tape control key
	if (ui_input_pressed(machine, IPT_UI_TAPE_START))
	{
		cassette_device_iterator cassiter(machine.root_device());
		for (cassette_image_device *cass = cassiter.first(); cass != NULL; cass = cassiter.next())
		{
			cass->change_state(CASSETTE_PLAY, CASSETTE_MASK_UISTATE);
			return 0;
		}
	}
	if (ui_input_pressed(machine, IPT_UI_TAPE_STOP))
	{
		cassette_device_iterator cassiter(machine.root_device());
		for (cassette_image_device *cass = cassiter.first(); cass != NULL; cass = cassiter.next())
		{
			cass->change_state(CASSETTE_STOPPED, CASSETTE_MASK_UISTATE);
			return 0;
		}
	}

	// handle a save state request
	if (ui_input_pressed(machine, IPT_UI_SAVE_STATE))
	{
		machine.pause();
		return machine.ui().set_handler(handler_load_save, LOADSAVE_SAVE);
	}

	// handle a load state request
	if (ui_input_pressed(machine, IPT_UI_LOAD_STATE))
	{
		machine.pause();
		return machine.ui().set_handler(handler_load_save, LOADSAVE_LOAD);
	}
#ifdef KAILLERA
	}
#endif /* KAILLERA */

	// handle a save snapshot request
	if (ui_input_pressed(machine, IPT_UI_SNAPSHOT))
		machine.video().save_active_screen_snapshots();

#ifdef INP_CAPTION
	machine.ioport().draw_caption(container);
#endif /* INP_CAPTION */

#ifdef KAILLERA
	if (!kPlay)
#endif /* KAILLERA */
	// toggle pause
	if (ui_input_pressed(machine, IPT_UI_PAUSE))
	{
		// with a shift key, it is single step
		if (is_paused && (machine.input().code_pressed(KEYCODE_LSHIFT) || machine.input().code_pressed(KEYCODE_RSHIFT)))
		{
			machine.ui().set_single_step(true);
			machine.resume();
		}
		else
			machine.toggle_pause();
	}

#ifdef USE_SHOW_TIME
	if (ui_input_pressed(machine, IPT_UI_TIME))
	{
		if (show_time)
		{
			Show_Time_Position++;

			if (Show_Time_Position > 3)
			{
				Show_Time_Position = 0;
				show_time = 0;
			}
		}
		else
		{
			Show_Time_Position = 0;
			show_time = 1;
		}
	}

	if (show_time)
		machine.ui().display_time(container);
#endif /* USE_SHOW_TIME */

#ifdef USE_SHOW_INPUT_LOG
	if (ui_input_pressed(machine, IPT_UI_SHOW_INPUT_LOG))
	{
		machine.ioport().show_input_log() ^= 1;
		machine.ioport().command_buffer()[0].code = '\0';
	}

	/* show popup message if input exist any log */
	if (machine.ioport().show_input_log())
		machine.ui().display_input_log(container);
#endif /* USE_SHOW_INPUT_LOG */

#ifdef KAILLERA
	if (!kPlay)
	{
#endif /* KAILLERA */
	// handle a toggle cheats request
	if (ui_input_pressed(machine, IPT_UI_TOGGLE_CHEAT))
		machine.cheat().set_enable(!machine.cheat().enabled());

	// toggle movie recording
	if (ui_input_pressed(machine, IPT_UI_RECORD_MOVIE))
		machine.video().toggle_record_movie();

#ifdef MAME_AVI
	if (ui_input_pressed(machine, IPT_UI_RECORD_AVI))
		toggle_record_avi();
#endif /* MAME_AVI */

#ifdef KAILLERA
	}
	}
#endif /* KAILLERA */

	// toggle profiler display
	if (ui_input_pressed(machine, IPT_UI_SHOW_PROFILER))
		machine.ui().set_show_profiler(!machine.ui().show_profiler());

	// toggle FPS display
	if (ui_input_pressed(machine, IPT_UI_SHOW_FPS))
		machine.ui().set_show_fps(!machine.ui().show_fps());

#ifdef KAILLERA
	if (!kPlay)
	{
#endif /* KAILLERA */
	// increment frameskip?
	if (ui_input_pressed(machine, IPT_UI_FRAMESKIP_INC))
		machine.ui().increase_frameskip();

	// decrement frameskip?
	if (ui_input_pressed(machine, IPT_UI_FRAMESKIP_DEC))
		machine.ui().decrease_frameskip();

	// toggle throttle?
	if (ui_input_pressed(machine, IPT_UI_THROTTLE))
		machine.video().toggle_throttle();

	// check for fast forward
	if (machine.ioport().type_pressed(IPT_UI_FAST_FORWARD))
	{
		machine.video().set_fastforward(true);
		machine.ui().show_fps_temp(0.5);
	}
	else
		machine.video().set_fastforward(false);
#ifdef KAILLERA
	}
#endif /* KAILLERA */

	return 0;
}


//-------------------------------------------------
//  handler_load_save - leads the user through
//  specifying a game to save or load
//-------------------------------------------------

UINT32 ui_manager::handler_load_save(running_machine &machine, render_container *container, UINT32 state)
{
#ifndef KAILLERA
	char filename[20];
#endif /* !KAILLERA */
	input_code code;
	char file = 0;

	// if we're not in the middle of anything, skip
	if (state == LOADSAVE_NONE)
		return 0;

	// okay, we're waiting for a key to select a slot; display a message
	if (state == LOADSAVE_SAVE)
		machine.ui().draw_message_window(container, _("Select position to save to"));
	else
		machine.ui().draw_message_window(container, _("Select position to load from"));

	// check for cancel key
	if (ui_input_pressed(machine, IPT_UI_CANCEL))
	{
		// display a popup indicating things were cancelled
		if (state == LOADSAVE_SAVE)
			popmessage(_("Save cancelled"));
		else
			popmessage(_("Load cancelled"));

		// reset the state
		machine.resume();
		return UI_HANDLER_CANCEL;
	}

	// check for A-Z or 0-9
	for (input_item_id id = ITEM_ID_A; id <= ITEM_ID_Z; id++)
		if (machine.input().code_pressed_once(input_code(DEVICE_CLASS_KEYBOARD, 0, ITEM_CLASS_SWITCH, ITEM_MODIFIER_NONE, id)))
			file = id - ITEM_ID_A + 'a';
	if (file == 0)
		for (input_item_id id = ITEM_ID_0; id <= ITEM_ID_9; id++)
			if (machine.input().code_pressed_once(input_code(DEVICE_CLASS_KEYBOARD, 0, ITEM_CLASS_SWITCH, ITEM_MODIFIER_NONE, id)))
				file = id - ITEM_ID_0 + '0';
	if (file == 0)
		for (input_item_id id = ITEM_ID_0_PAD; id <= ITEM_ID_9_PAD; id++)
			if (machine.input().code_pressed_once(input_code(DEVICE_CLASS_KEYBOARD, 0, ITEM_CLASS_SWITCH, ITEM_MODIFIER_NONE, id)))
				file = id - ITEM_ID_0_PAD + '0';
	if (file == 0)
		return state;

#ifdef KAILLERA
	if (file > 0)
	{
		if (state == LOADSAVE_SAVE)
			input_ui_temp = 1;
		else
			input_ui_temp = 2;
		input_ui_temp_dat[0] = file;
	}
#else

	// display a popup indicating that the save will proceed
	sprintf(filename, "%c", file);
	if (state == LOADSAVE_SAVE)
	{
		popmessage(_("Save to position %c"), file);
		machine.schedule_save(filename);
	}
	else
	{
		popmessage(_("Load from position %c"), file);
		machine.schedule_load(filename);
	}
#endif /* KAILLERA */

	// remove the pause and reset the state
	machine.resume();
	return UI_HANDLER_CANCEL;
}


//-------------------------------------------------
//  request_quit
//-------------------------------------------------

void ui_manager::request_quit()
{
	if (!machine().options().confirm_quit())
		machine().schedule_exit();
	else
		set_handler(handler_confirm_quit, 0);
}


//-------------------------------------------------
//  handler_confirm_quit - leads the user through
//  confirming quit emulation
//-------------------------------------------------

UINT32 ui_manager::handler_confirm_quit(running_machine &machine, render_container *container, UINT32 state)
{
#ifdef KAILLERA
	if(kPlay) { quiting = 1; return UI_HANDLER_CANCEL; }
#endif /* KAILLERA */

	// get the text for 'UI Select'
	std::string ui_select_text;
	machine.input().seq_name(ui_select_text, machine.ioport().type_seq(IPT_UI_SELECT, 0, SEQ_TYPE_STANDARD));

	// get the text for 'UI Cancel'
	std::string ui_cancel_text;
	machine.input().seq_name(ui_cancel_text, machine.ioport().type_seq(IPT_UI_CANCEL, 0, SEQ_TYPE_STANDARD));

	// assemble the quit message
	std::string quit_message;
	strprintf(quit_message,
		_("Are you sure you want to quit?\n\n"
		"Press ''%s'' to quit,\n"
		"Press ''%s'' to return to emulation."),
		ui_select_text.c_str(),
		ui_cancel_text.c_str());

	machine.ui().draw_text_box(container, quit_message.c_str(), JUSTIFY_CENTER, 0.5f, 0.5f, UI_RED_COLOR);
	machine.pause();

	// if the user press ENTER, quit the game
	if (ui_input_pressed(machine, IPT_UI_SELECT))
		machine.schedule_exit();

	// if the user press ESC, just continue
	else if (ui_input_pressed(machine, IPT_UI_CANCEL))
	{
		machine.resume();
		state = UI_HANDLER_CANCEL;
	}

	return state;
}


/***************************************************************************
    SLIDER CONTROLS
***************************************************************************/

//-------------------------------------------------
//  ui_get_slider_list - get the list of sliders
//-------------------------------------------------

const slider_state *ui_manager::get_slider_list(void)
{
	return slider_list;
}


//-------------------------------------------------
//  slider_alloc - allocate a new slider entry
//-------------------------------------------------

static slider_state *slider_alloc(running_machine &machine, const char *title, INT32 minval, INT32 defval, INT32 maxval, INT32 incval, slider_update update, void *arg)
{
	int size = sizeof(slider_state) + strlen(title);
	slider_state *state = (slider_state *)auto_alloc_array_clear(machine, UINT8, size);

	state->minval = minval;
	state->defval = defval;
	state->maxval = maxval;
	state->incval = incval;
	state->update = update;
	state->arg = arg;
	strcpy(state->description, title);

	return state;
}


//-------------------------------------------------
//  slider_init - initialize the list of slider
//  controls
//-------------------------------------------------

static slider_state *slider_init(running_machine &machine)
{
	ioport_field *field;
	ioport_port *port;
	slider_state *listhead = NULL;
	slider_state **tailptr = &listhead;
	std::string str;
	int item;

	// add overall volume
	*tailptr = slider_alloc(machine, _("Master Volume"), -32, 0, 0, 1, slider_volume, NULL);
	tailptr = &(*tailptr)->next;

	// add per-channel volume
	mixer_input info;
	for (item = 0; machine.sound().indexed_mixer_input(item, info); item++)
	{
		INT32 maxval = 2000;
		INT32 defval = 1000;

		info.stream->input_name(info.inputnum, str);
		str.append(_(" Volume"));
		*tailptr = slider_alloc(machine, str.c_str(), 0, defval, maxval, 20, slider_mixervol, (void *)(FPTR)item);
		tailptr = &(*tailptr)->next;
	}

	// add analog adjusters
	for (port = machine.ioport().first_port(); port != NULL; port = port->next())
		for (field = port->first_field(); field != NULL; field = field->next())
			if (field->type() == IPT_ADJUSTER)
			{
				void *param = (void *)field;
				*tailptr = slider_alloc(machine, field->name(), field->minval(), field->defvalue(), field->maxval(), 1, slider_adjuster, param);
				tailptr = &(*tailptr)->next;
			}

#ifdef KAILLERA
	if (!kPlay)
#endif /* KAILLERA */
	// add CPU overclocking (cheat only)
	if (machine.options().cheat())
	{
		execute_interface_iterator iter(machine.root_device());
		for (device_execute_interface *exec = iter.first(); exec != NULL; exec = iter.next())
		{
			void *param = (void *)&exec->device();
			strprintf(str, _("Overclock CPU %s"), exec->device().tag());
			//mamep: 4x overclock
			*tailptr = slider_alloc(machine, str.c_str(), 10, 1000, 4000, 50, slider_overclock, param);
			tailptr = &(*tailptr)->next;
		}
	}

	// add screen parameters
	screen_device_iterator scriter(machine.root_device());
	for (screen_device *screen = scriter.first(); screen != NULL; screen = scriter.next())
	{
		int defxscale = floor(screen->xscale() * 1000.0f + 0.5f);
		int defyscale = floor(screen->yscale() * 1000.0f + 0.5f);
		int defxoffset = floor(screen->xoffset() * 1000.0f + 0.5f);
		int defyoffset = floor(screen->yoffset() * 1000.0f + 0.5f);
		void *param = (void *)screen;

		// add refresh rate tweaker
		if (machine.options().cheat())
		{
			strprintf(str, _("%s Refresh Rate"), slider_get_screen_desc(*screen));
			*tailptr = slider_alloc(machine, str.c_str(), -33000, 0, 33000, 1000, slider_refresh, param);
			tailptr = &(*tailptr)->next;
		}

		// add standard brightness/contrast/gamma controls per-screen
		strprintf(str, _("%s Brightness"), slider_get_screen_desc(*screen));
		*tailptr = slider_alloc(machine, str.c_str(), 100, 1000, 2000, 10, slider_brightness, param);
		tailptr = &(*tailptr)->next;
		strprintf(str, _("%s Contrast"), slider_get_screen_desc(*screen));
		*tailptr = slider_alloc(machine, str.c_str(), 100, 1000, 2000, 50, slider_contrast, param);
		tailptr = &(*tailptr)->next;
		strprintf(str, _("%s Gamma"), slider_get_screen_desc(*screen));
		*tailptr = slider_alloc(machine, str.c_str(), 100, 1000, 3000, 50, slider_gamma, param);
		tailptr = &(*tailptr)->next;

		// add scale and offset controls per-screen
		strprintf(str, _("%s Horiz Stretch"), slider_get_screen_desc(*screen));
		*tailptr = slider_alloc(machine, str.c_str(), 500, defxscale, 1500, 2, slider_xscale, param);
		tailptr = &(*tailptr)->next;
		strprintf(str, _("%s Horiz Position"), slider_get_screen_desc(*screen));
		*tailptr = slider_alloc(machine, str.c_str(), -500, defxoffset, 500, 2, slider_xoffset, param);
		tailptr = &(*tailptr)->next;
		strprintf(str, _("%s Vert Stretch"), slider_get_screen_desc(*screen));
		*tailptr = slider_alloc(machine, str.c_str(), 500, defyscale, 1500, 2, slider_yscale, param);
		tailptr = &(*tailptr)->next;
		strprintf(str, _("%s Vert Position"), slider_get_screen_desc(*screen));
		*tailptr = slider_alloc(machine, str.c_str(), -500, defyoffset, 500, 2, slider_yoffset, param);
		tailptr = &(*tailptr)->next;
	}

	laserdisc_device_iterator lditer(machine.root_device());
	for (laserdisc_device *laserdisc = lditer.first(); laserdisc != NULL; laserdisc = lditer.next())
		if (laserdisc->overlay_configured())
		{
			laserdisc_overlay_config config;
			laserdisc->get_overlay_config(config);
			int defxscale = floor(config.m_overscalex * 1000.0f + 0.5f);
			int defyscale = floor(config.m_overscaley * 1000.0f + 0.5f);
			int defxoffset = floor(config.m_overposx * 1000.0f + 0.5f);
			int defyoffset = floor(config.m_overposy * 1000.0f + 0.5f);
			void *param = (void *)laserdisc;

			// add scale and offset controls per-overlay
			strprintf(str, _("Laserdisc '%s' Horiz Stretch"), laserdisc->tag());
			*tailptr = slider_alloc(machine, str.c_str(), 500, (defxscale == 0) ? 1000 : defxscale, 1500, 2, slider_overxscale, param);
			tailptr = &(*tailptr)->next;
			strprintf(str, _("Laserdisc '%s' Horiz Position"), laserdisc->tag());
			*tailptr = slider_alloc(machine, str.c_str(), -500, defxoffset, 500, 2, slider_overxoffset, param);
			tailptr = &(*tailptr)->next;
			strprintf(str, _("Laserdisc '%s' Vert Stretch"), laserdisc->tag());
			*tailptr = slider_alloc(machine, str.c_str(), 500, (defyscale == 0) ? 1000 : defyscale, 1500, 2, slider_overyscale, param);
			tailptr = &(*tailptr)->next;
			strprintf(str, _("Laserdisc '%s' Vert Position"), laserdisc->tag());
			*tailptr = slider_alloc(machine, str.c_str(), -500, defyoffset, 500, 2, slider_overyoffset, param);
			tailptr = &(*tailptr)->next;
		}

	for (screen_device *screen = scriter.first(); screen != NULL; screen = scriter.next())
		if (screen->screen_type() == SCREEN_TYPE_VECTOR)
		{
			// add flicker control
			*tailptr = slider_alloc(machine, _("Vector Flicker"), 0, 0, 1000, 10, slider_flicker, NULL);
			tailptr = &(*tailptr)->next;
			*tailptr = slider_alloc(machine, _("Beam Width"), 10, 100, 1000, 10, slider_beam, NULL);
			tailptr = &(*tailptr)->next;
			break;
		}

#ifdef MAME_DEBUG
	// add crosshair adjusters
	for (port = machine.ioport().first_port(); port != NULL; port = port->next())
		for (field = port->first_field(); field != NULL; field = field->next())
			if (field->crosshair_axis() != CROSSHAIR_AXIS_NONE && field->player() == 0)
			{
				void *param = (void *)field;
				strprintf(str, _("Crosshair Scale %s"), (field->crosshair_axis() == CROSSHAIR_AXIS_X) ? "X" : "Y");
				*tailptr = slider_alloc(machine, str.c_str(), -3000, 1000, 3000, 100, slider_crossscale, param);
				tailptr = &(*tailptr)->next;
				strprintf(str, _("Crosshair Offset %s"), (field->crosshair_axis() == CROSSHAIR_AXIS_X) ? "X" : "Y");
				*tailptr = slider_alloc(machine, str.c_str(), -3000, 0, 3000, 100, slider_crossoffset, param);
				tailptr = &(*tailptr)->next;
			}
#endif

	return listhead;
}


//-------------------------------------------------
//  slider_volume - global volume slider callback
//-------------------------------------------------

static INT32 slider_volume(running_machine &machine, void *arg, std::string *str, INT32 newval)
{
	if (newval != SLIDER_NOCHANGE)
		machine.sound().set_attenuation(newval);
	if (str != NULL)
		strprintf(*str,"%3ddB", machine.sound().attenuation());
	return machine.sound().attenuation();
}


//-------------------------------------------------
//  slider_mixervol - single channel volume
//  slider callback
//-------------------------------------------------

static INT32 slider_mixervol(running_machine &machine, void *arg, std::string *str, INT32 newval)
{
	mixer_input info;
	if (!machine.sound().indexed_mixer_input((FPTR)arg, info))
		return 0;
	if (newval != SLIDER_NOCHANGE)
	{
		INT32 curval = floor(info.stream->user_gain(info.inputnum) * 1000.0f + 0.5f);
		if (newval > curval && (newval - curval) <= 4) newval += 4; // round up on increment
		info.stream->set_user_gain(info.inputnum, (float)newval * 0.001f);
	}
	if (str != NULL)
		strprintf(*str,"%4.2f", (double) info.stream->user_gain(info.inputnum));
	return floorf(info.stream->user_gain(info.inputnum) * 1000.0f + 0.5f);
}


//-------------------------------------------------
//  slider_adjuster - analog adjuster slider
//  callback
//-------------------------------------------------

static INT32 slider_adjuster(running_machine &machine, void *arg, std::string *str, INT32 newval)
{
	ioport_field *field = (ioport_field *)arg;
	ioport_field::user_settings settings;

	field->get_user_settings(settings);
	if (newval != SLIDER_NOCHANGE)
	{
		settings.value = newval;
		field->set_user_settings(settings);
	}
	if (str != NULL)
		strprintf(*str,"%d%%", settings.value);
	return settings.value;
}


//-------------------------------------------------
//  slider_overclock - CPU overclocker slider
//  callback
//-------------------------------------------------

static INT32 slider_overclock(running_machine &machine, void *arg, std::string *str, INT32 newval)
{
	device_t *cpu = (device_t *)arg;
	if (newval != SLIDER_NOCHANGE)
		cpu->set_clock_scale((float)newval * 0.001f);
	if (str != NULL)
		strprintf(*str,"%3.0f%%", floor(cpu->clock_scale() * 100.0 + 0.5));
	return floor(cpu->clock_scale() * 1000.0 + 0.5);
}


//-------------------------------------------------
//  slider_refresh - refresh rate slider callback
//-------------------------------------------------

static INT32 slider_refresh(running_machine &machine, void *arg, std::string *str, INT32 newval)
{
	screen_device *screen = reinterpret_cast<screen_device *>(arg);
	double defrefresh = ATTOSECONDS_TO_HZ(screen->refresh_attoseconds());
	double refresh;

	if (newval != SLIDER_NOCHANGE)
	{
		int width = screen->width();
		int height = screen->height();
		const rectangle &visarea = screen->visible_area();
		screen->configure(width, height, visarea, HZ_TO_ATTOSECONDS(defrefresh + (double)newval * 0.001));
	}
	if (str != NULL)
		strprintf(*str,"%.3ffps", ATTOSECONDS_TO_HZ(machine.first_screen()->frame_period().attoseconds));
	refresh = ATTOSECONDS_TO_HZ(machine.first_screen()->frame_period().attoseconds);
	return floor((refresh - defrefresh) * 1000.0 + 0.5);
}


//-------------------------------------------------
//  slider_brightness - screen brightness slider
//  callback
//-------------------------------------------------

static INT32 slider_brightness(running_machine &machine, void *arg, std::string *str, INT32 newval)
{
	screen_device *screen = reinterpret_cast<screen_device *>(arg);
	render_container::user_settings settings;

	screen->container().get_user_settings(settings);
	if (newval != SLIDER_NOCHANGE)
	{
		settings.m_brightness = (float)newval * 0.001f;
		screen->container().set_user_settings(settings);
	}
	if (str != NULL)
		strprintf(*str,"%.3f", (double) settings.m_brightness);
	return floor(settings.m_brightness * 1000.0f + 0.5f);
}


//-------------------------------------------------
//  slider_contrast - screen contrast slider
//  callback
//-------------------------------------------------

static INT32 slider_contrast(running_machine &machine, void *arg, std::string *str, INT32 newval)
{
	screen_device *screen = reinterpret_cast<screen_device *>(arg);
	render_container::user_settings settings;

	screen->container().get_user_settings(settings);
	if (newval != SLIDER_NOCHANGE)
	{
		settings.m_contrast = (float)newval * 0.001f;
		screen->container().set_user_settings(settings);
	}
	if (str != NULL)
		strprintf(*str,"%.3f", (double) settings.m_contrast);
	return floor(settings.m_contrast * 1000.0f + 0.5f);
}


//-------------------------------------------------
//  slider_gamma - screen gamma slider callback
//-------------------------------------------------

static INT32 slider_gamma(running_machine &machine, void *arg, std::string *str, INT32 newval)
{
	screen_device *screen = reinterpret_cast<screen_device *>(arg);
	render_container::user_settings settings;

	screen->container().get_user_settings(settings);
	if (newval != SLIDER_NOCHANGE)
	{
		settings.m_gamma = (float)newval * 0.001f;
		screen->container().set_user_settings(settings);
	}
	if (str != NULL)
		strprintf(*str,"%.3f", (double) settings.m_gamma);
	return floor(settings.m_gamma * 1000.0f + 0.5f);
}


//-------------------------------------------------
//  slider_xscale - screen horizontal scale slider
//  callback
//-------------------------------------------------

static INT32 slider_xscale(running_machine &machine, void *arg, std::string *str, INT32 newval)
{
	screen_device *screen = reinterpret_cast<screen_device *>(arg);
	render_container::user_settings settings;

	screen->container().get_user_settings(settings);
	if (newval != SLIDER_NOCHANGE)
	{
		settings.m_xscale = (float)newval * 0.001f;
		screen->container().set_user_settings(settings);
	}
	if (str != NULL)
		strprintf(*str,"%.3f", (double) settings.m_xscale);
	return floor(settings.m_xscale * 1000.0f + 0.5f);
}


//-------------------------------------------------
//  slider_yscale - screen vertical scale slider
//  callback
//-------------------------------------------------

static INT32 slider_yscale(running_machine &machine, void *arg, std::string *str, INT32 newval)
{
	screen_device *screen = reinterpret_cast<screen_device *>(arg);
	render_container::user_settings settings;

	screen->container().get_user_settings(settings);
	if (newval != SLIDER_NOCHANGE)
	{
		settings.m_yscale = (float)newval * 0.001f;
		screen->container().set_user_settings(settings);
	}
	if (str != NULL)
		strprintf(*str,"%.3f", (double) settings.m_yscale);
	return floor(settings.m_yscale * 1000.0f + 0.5f);
}


//-------------------------------------------------
//  slider_xoffset - screen horizontal position
//  slider callback
//-------------------------------------------------

static INT32 slider_xoffset(running_machine &machine, void *arg, std::string *str, INT32 newval)
{
	screen_device *screen = reinterpret_cast<screen_device *>(arg);
	render_container::user_settings settings;

	screen->container().get_user_settings(settings);
	if (newval != SLIDER_NOCHANGE)
	{
		settings.m_xoffset = (float)newval * 0.001f;
		screen->container().set_user_settings(settings);
	}
	if (str != NULL)
		strprintf(*str,"%.3f", (double) settings.m_xoffset);
	return floor(settings.m_xoffset * 1000.0f + 0.5f);
}


//-------------------------------------------------
//  slider_yoffset - screen vertical position
//  slider callback
//-------------------------------------------------

static INT32 slider_yoffset(running_machine &machine, void *arg, std::string *str, INT32 newval)
{
	screen_device *screen = reinterpret_cast<screen_device *>(arg);
	render_container::user_settings settings;

	screen->container().get_user_settings(settings);
	if (newval != SLIDER_NOCHANGE)
	{
		settings.m_yoffset = (float)newval * 0.001f;
		screen->container().set_user_settings(settings);
	}
	if (str != NULL)
		strprintf(*str,"%.3f",  (double) settings.m_yoffset);
	return floor(settings.m_yoffset * 1000.0f + 0.5f);
}


//-------------------------------------------------
//  slider_overxscale - screen horizontal scale slider
//  callback
//-------------------------------------------------

static INT32 slider_overxscale(running_machine &machine, void *arg, std::string *str, INT32 newval)
{
	laserdisc_device *laserdisc = (laserdisc_device *)arg;
	laserdisc_overlay_config settings;

	laserdisc->get_overlay_config(settings);
	if (newval != SLIDER_NOCHANGE)
	{
		settings.m_overscalex = (float)newval * 0.001f;
		laserdisc->set_overlay_config(settings);
	}
	if (str != NULL)
		strprintf(*str,"%.3f", (double) settings.m_overscalex);
	return floor(settings.m_overscalex * 1000.0f + 0.5f);
}


//-------------------------------------------------
//  slider_overyscale - screen vertical scale slider
//  callback
//-------------------------------------------------

static INT32 slider_overyscale(running_machine &machine, void *arg, std::string *str, INT32 newval)
{
	laserdisc_device *laserdisc = (laserdisc_device *)arg;
	laserdisc_overlay_config settings;

	laserdisc->get_overlay_config(settings);
	if (newval != SLIDER_NOCHANGE)
	{
		settings.m_overscaley = (float)newval * 0.001f;
		laserdisc->set_overlay_config(settings);
	}
	if (str != NULL)
		strprintf(*str,"%.3f", (double) settings.m_overscaley);
	return floor(settings.m_overscaley * 1000.0f + 0.5f);
}


//-------------------------------------------------
//  slider_overxoffset - screen horizontal position
//  slider callback
//-------------------------------------------------

static INT32 slider_overxoffset(running_machine &machine, void *arg, std::string *str, INT32 newval)
{
	laserdisc_device *laserdisc = (laserdisc_device *)arg;
	laserdisc_overlay_config settings;

	laserdisc->get_overlay_config(settings);
	if (newval != SLIDER_NOCHANGE)
	{
		settings.m_overposx = (float)newval * 0.001f;
		laserdisc->set_overlay_config(settings);
	}
	if (str != NULL)
		strprintf(*str,"%.3f", (double) settings.m_overposx);
	return floor(settings.m_overposx * 1000.0f + 0.5f);
}


//-------------------------------------------------
//  slider_overyoffset - screen vertical position
//  slider callback
//-------------------------------------------------

static INT32 slider_overyoffset(running_machine &machine, void *arg, std::string *str, INT32 newval)
{
	laserdisc_device *laserdisc = (laserdisc_device *)arg;
	laserdisc_overlay_config settings;

	laserdisc->get_overlay_config(settings);
	if (newval != SLIDER_NOCHANGE)
	{
		settings.m_overposy = (float)newval * 0.001f;
		laserdisc->set_overlay_config(settings);
	}
	if (str != NULL)
		strprintf(*str,"%.3f", (double) settings.m_overposy);
	return floor(settings.m_overposy * 1000.0f + 0.5f);
}


//-------------------------------------------------
//  slider_flicker - vector flicker slider
//  callback
//-------------------------------------------------

static INT32 slider_flicker(running_machine &machine, void *arg, std::string *str, INT32 newval)
{
	vector_device *vector = NULL;
	if (newval != SLIDER_NOCHANGE)
		vector->set_flicker((float)newval * 0.1f);
	if (str != NULL)
		strprintf(*str,"%1.2f", (double) vector->get_flicker());
	return floor(vector->get_flicker() * 10.0f + 0.5f);
}


//-------------------------------------------------
//  slider_beam - vector beam width slider
//  callback
//-------------------------------------------------

static INT32 slider_beam(running_machine &machine, void *arg, std::string *str, INT32 newval)
{
	vector_device *vector = NULL;
	if (newval != SLIDER_NOCHANGE)
		vector->set_beam((float)newval * 0.01f);
	if (str != NULL)
		strprintf(*str,"%1.2f", (double) vector->get_beam());
	return floor(vector->get_beam() * 100.0f + 0.5f);
}


//-------------------------------------------------
//  slider_get_screen_desc - returns the
//  description for a given screen
//-------------------------------------------------

static char *slider_get_screen_desc(screen_device &screen)
{
	screen_device_iterator iter(screen.machine().root_device());
	int scrcount = iter.count();
	static char descbuf[256];

	if (scrcount > 1)
		sprintf(descbuf, _("Screen '%s'"), screen.tag());
	else
		strcpy(descbuf, _("Screen"));

	return descbuf;
}

//-------------------------------------------------
//  slider_crossscale - crosshair scale slider
//  callback
//-------------------------------------------------

#ifdef MAME_DEBUG
static INT32 slider_crossscale(running_machine &machine, void *arg, std::string *str, INT32 newval)
{
	ioport_field *field = (ioport_field *)arg;

	if (newval != SLIDER_NOCHANGE)
		field->set_crosshair_scale(float(newval) * 0.001);
	if (str != NULL)
		strprintf(*str,"%s %s %1.3f", _("Crosshair Scale"), (field->crosshair_axis() == CROSSHAIR_AXIS_X) ? "X" : "Y", float(newval) * 0.001f);
	return floor(field->crosshair_scale() * 1000.0f + 0.5f);
}
#endif


//-------------------------------------------------
//  slider_crossoffset - crosshair scale slider
//  callback
//-------------------------------------------------

#ifdef MAME_DEBUG
static INT32 slider_crossoffset(running_machine &machine, void *arg, std::string *str, INT32 newval)
{
	ioport_field *field = (ioport_field *)arg;

	if (newval != SLIDER_NOCHANGE)
		field->set_crosshair_offset(float(newval) * 0.001f);
	if (str != NULL)
		strprintf(*str,"%s %s %1.3f", _("Crosshair Offset"), (field->crosshair_axis() == CROSSHAIR_AXIS_X) ? "X" : "Y", float(newval) * 0.001f);
	return field->crosshair_offset();
}
#endif


//-------------------------------------------------
//  use_natural_keyboard - returns
//  whether the natural keyboard is active
//-------------------------------------------------

bool ui_manager::use_natural_keyboard() const
{
	return m_use_natural_keyboard;
}


//-------------------------------------------------
//  set_use_natural_keyboard - specifies
//  whether the natural keyboard is active
//-------------------------------------------------

void ui_manager::set_use_natural_keyboard(bool use_natural_keyboard)
{
	m_use_natural_keyboard = use_natural_keyboard;
	std::string error;
	machine().options().set_value(OPTION_NATURAL_KEYBOARD, use_natural_keyboard, OPTION_PRIORITY_CMDLINE, error);
	assert(error.empty());
}


//-------------------------------------------------
//  draw_message_window - draw a multiline text
//  message with a box around it
//-------------------------------------------------

#undef draw_message_window
void ui_manager::draw_message_window(render_container *container, const char *text)
{
	draw_text_box(container, text, JUSTIFY_LEFT, 0.5f, 0.5f, UI_BACKGROUND_COLOR);
}


void ui_manager::build_bgtexture()
{
#ifdef UI_COLOR_DISPLAY
	float r = (float)uifont_colortable[UI_BACKGROUND_COLOR].r();
	float g = (float)uifont_colortable[UI_BACKGROUND_COLOR].g();
	float b = (float)uifont_colortable[UI_BACKGROUND_COLOR].b();
#else /* UI_COLOR_DISPLAY */
	UINT8 r = 0x10;
	UINT8 g = 0x10;
	UINT8 b = 0x30;
#endif /* UI_COLOR_DISPLAY */
	UINT8 a = 0xff;
	int i;

#ifdef TRANS_UI
	a = ui_transparency;
#endif /* TRANS_UI */

	bgbitmap = auto_alloc(machine(), bitmap_argb32(1, 1024));
	if (bgbitmap == NULL)
		fatalerror("build_bgtexture failed");

	for (i = 0; i < bgbitmap->height(); i++)
	{
		double gradual = (float)(1024 - i) / 1024.0f + 0.1f;

		if (gradual > 1.0f)
			gradual = 1.0f;
		else if (gradual < 0.1f)
			gradual = 0.1f;

		bgbitmap->pix32(i, 0) = rgb_t(a, (UINT8)(r * gradual), (UINT8)(g * gradual), (UINT8)(b * gradual));
	}

	bgtexture = machine().render().texture_alloc(render_texture::hq_scale);
	bgtexture->set_bitmap(*bgbitmap, bgbitmap->cliprect(), TEXFORMAT_ARGB32);
	machine().add_notifier(MACHINE_NOTIFY_EXIT, machine_notify_delegate(FUNC(ui_manager::free_bgtexture), this));
}


void ui_manager::free_bgtexture()
{
	machine().render().texture_free(bgtexture);
}

#ifdef MAME_AVI
int usrintrf_message_ok_cancel(running_machine &machine, const char *str)
{
	render_container *container = &machine.first_screen()->container();
	int ret = FALSE;
	machine.pause();

	while (1)
	{
		machine.ui().draw_message_window(container, str);

		//update_video_and_audio();

		if (ui_input_pressed(machine, IPT_UI_CANCEL))
			break;

		if (ui_input_pressed(machine, IPT_UI_SELECT))
		{
			ret = TRUE;
			break;
		}
	}

	machine.resume();

	return ret;
}
#endif /* MAME_AVI */
