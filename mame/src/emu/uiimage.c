/*********************************************************************

    filemngr.c

    MESS's clunky built-in file manager

    TODO
        - Support image creation arguments
        - Restrict directory listing by file extension
        - Support file manager invocation from the main menu for
          required images
        - Restrict empty slot if image required

*********************************************************************/

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

#include "emu.h"
#include "image.h"
#include "ui.h"
#include "uimenu.h"
#include "uiswlist.h"
#include "uiimage.h"
#include "zippath.h"
#include "unicode.h"
#include "imagedev/cassette.h"
#include "imagedev/bitbngr.h"



/***************************************************************************
    CONSTANTS
***************************************************************************/

/* conditional compilation to enable chosing of image formats - this is not
 * yet fully implemented */
#define ENABLE_FORMATS			0

/* time (in seconds) to display errors */
#define ERROR_MESSAGE_TIME		5

/* itemrefs for key menu items */
#define ITEMREF_NEW_IMAGE_NAME	((void *) 0x0001)
#define ITEMREF_CREATE			((void *) 0x0002)
#define ITEMREF_FORMAT			((void *) 0x0003)
#define ITEMREF_NO				((void *) 0x0004)
#define ITEMREF_YES				((void *) 0x0005)

/***************************************************************************
    MENU HELPERS
***************************************************************************/

/*-------------------------------------------------
    input_character - inputs a typed character
    into a buffer
-------------------------------------------------*/

static void input_character(char *buffer, size_t buffer_length, unicode_char unichar, int (*filter)(unicode_char))
{
	size_t buflen = strlen(buffer);

	if ((unichar == 8) && (buflen > 0))
	{
		*(char *)utf8_previous_char(&buffer[buflen]) = 0;
	}
	else if ((unichar > ' ') && ((filter == NULL) || (*filter)(unichar)))
	{
		buflen += utf8_from_uchar(&buffer[buflen], buffer_length - buflen, unichar);
		buffer[buflen] = 0;
	}
}



/*-------------------------------------------------
    extra_text_draw_box - generically adds header
    or footer text
-------------------------------------------------*/

static void extra_text_draw_box(render_container *container, float origx1, float origx2, float origy, float yspan, const char *text, int direction)
{
	float text_width, text_height;
	float width, maxwidth;
	float x1, y1, x2, y2, temp;

	/* get the size of the text */
	ui_draw_text_full(container,text, 0.0f, 0.0f, 1.0f, JUSTIFY_LEFT, WRAP_WORD,
		DRAW_NONE, ARGB_WHITE, ARGB_BLACK, &text_width, &text_height);
	width = text_width + (2 * UI_BOX_LR_BORDER);
	maxwidth = MAX(width, origx2 - origx1);

	/* compute our bounds */
	x1 = 0.5f - 0.5f * maxwidth;
	x2 = x1 + maxwidth;
	y1 = origy + (yspan * direction);
	y2 = origy + (UI_BOX_TB_BORDER * direction);

	if (y1 > y2)
	{
		temp = y1;
		y1 = y2;
		y2 = temp;
	}

	/* draw a box */
	ui_draw_outlined_box(container,x1, y1, x2, y2, UI_BACKGROUND_COLOR);

	/* take off the borders */
	x1 += UI_BOX_LR_BORDER;
	x2 -= UI_BOX_LR_BORDER;
	y1 += UI_BOX_TB_BORDER;
	y2 -= UI_BOX_TB_BORDER;

	/* draw the text within it */
	ui_draw_text_full(container,text, x1, y1, text_width, JUSTIFY_LEFT, WRAP_WORD,
					  DRAW_NORMAL, ARGB_WHITE, ARGB_BLACK, NULL, NULL);
}



/*-------------------------------------------------
    extra_text_render - generically adds header
    and footer text
-------------------------------------------------*/

static void extra_text_render(render_container *container, float top, float bottom,
	float origx1, float origy1, float origx2, float origy2,
	const char *header, const char *footer)
{
	header = ((header != NULL) && (header[0] != '\0')) ? header : NULL;
	footer = ((footer != NULL) && (footer[0] != '\0')) ? footer : NULL;

	if (header != NULL)
		extra_text_draw_box(container, origx1, origx2, origy1, top, header, -1);
	if (footer != NULL)
		extra_text_draw_box(container, origx1, origx2, origy2, bottom, footer, +1);
}



/***************************************************************************
    CONFIRM SAVE AS MENU
***************************************************************************/

/*-------------------------------------------------
    menu_confirm_save_as_populate - populates the
    confirm save as menu
-------------------------------------------------*/

ui_menu_confirm_save_as::ui_menu_confirm_save_as(running_machine &machine, render_container *container, int *_yes) : ui_menu(machine, container)
{
	yes = _yes;
}

ui_menu_confirm_save_as::~ui_menu_confirm_save_as()
{
}

void ui_menu_confirm_save_as::populate()
{
	item_append("File Already Exists - Overide?", NULL, MENU_FLAG_DISABLE, NULL);
	item_append(MENU_SEPARATOR_ITEM, NULL, MENU_FLAG_DISABLE, NULL);
	item_append("No", NULL, 0, ITEMREF_NO);
	item_append("Yes", NULL, 0, ITEMREF_YES);
}

/*-------------------------------------------------
    menu_confirm_save_as - confirm save as menu
-------------------------------------------------*/

void ui_menu_confirm_save_as::handle()
{
	/* process the menu */
	const ui_menu_event *event = process(0);

	/* process the event */
	if ((event != NULL) && (event->iptkey == IPT_UI_SELECT))
	{
		if (event->itemref == ITEMREF_YES)
			*yes = TRUE;

		/* no matter what, pop out */
		ui_menu::stack_pop(machine());
	}
}



/***************************************************************************
    FILE CREATE MENU
***************************************************************************/

/*-------------------------------------------------
    is_valid_filename_char - tests to see if a
    character is valid in a filename
-------------------------------------------------*/

static int is_valid_filename_char(unicode_char unichar)
{
	/* this should really be in the OSD layer */
	static const char valid_filename_char[] =
	{
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 	/* 00-0f */
		0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 	/* 10-1f */
		1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 0, 	/*  !"#$%&'()*+,-./ */
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 	/* 0123456789:;<=>? */
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 	/* @ABCDEFGHIJKLMNO */
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 	/* PQRSTUVWXYZ[\]^_ */
		0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 	/* `abcdefghijklmno */
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 0, 	/* pqrstuvwxyz{|}~  */
	};
	return (unichar < ARRAY_LENGTH(valid_filename_char)) && valid_filename_char[unichar];
}



/*-------------------------------------------------
    file_create_render_extra - perform our
    special rendering
-------------------------------------------------*/

void ui_menu_file_create::custom_render(void *selectedref, float top, float bottom, float origx1, float origy1, float origx2, float origy2)
{
	extra_text_render(container, top, bottom, origx1, origy1, origx2, origy2,
		astring_c(manager->current_directory),
		NULL);
}



/*-------------------------------------------------
    menu_file_create_populate - populates the file
    creator menu
-------------------------------------------------*/

ui_menu_file_create::ui_menu_file_create(running_machine &machine, render_container *container, ui_menu_file_manager *parent) : ui_menu(machine, container)
{
	manager = parent;
}

ui_menu_file_create::~ui_menu_file_create()
{
}

void ui_menu_file_create::populate()
{
	astring buffer;
	device_image_interface *device = manager->selected_device;
	const image_device_format *format;
	const char *new_image_name;

	/* append the "New Image Name" item */
	if (get_selection() == ITEMREF_NEW_IMAGE_NAME)
	{
		astring_assemble_2(&buffer, filename_buffer, "_");
		new_image_name = astring_c(&buffer);
	}
	else
	{
		new_image_name = filename_buffer;
	}
	item_append("New Image Name:", new_image_name, 0, ITEMREF_NEW_IMAGE_NAME);

	/* do we support multiple formats? */
	format = device->device_get_creatable_formats();
	if (ENABLE_FORMATS && (format != NULL))
	{
		item_append("Image Format:", current_format->m_description, 0, ITEMREF_FORMAT);
		current_format = format;
	}

	/* finish up the menu */
	item_append(MENU_SEPARATOR_ITEM, NULL, 0, NULL);
	item_append("Create", NULL, 0, ITEMREF_CREATE);

	customtop = ui_get_line_height(machine()) + 3.0f * UI_BOX_TB_BORDER;
}



/*-------------------------------------------------
    create_new_image - creates a new disk image
-------------------------------------------------*/

int ui_menu_file_create::create_new_image(device_image_interface *image, const char *directory, const char *filename, int *yes)
{
	astring *path;
	osd_directory_entry *entry;
	osd_dir_entry_type file_type;
	int do_create, err;
	int result = FALSE;

	/* assemble the full path */
	path = zippath_combine(astring_alloc(), directory, filename);

	/* does a file or a directory exist at the path */
	entry = osd_stat(astring_c(path));
	file_type = (entry != NULL) ? entry->type : ENTTYPE_NONE;
	if (entry != NULL)
		free(entry);

	/* special case */
	if ((file_type == ENTTYPE_FILE) && *yes)
		file_type = ENTTYPE_NONE;

	switch(file_type)
	{
		case ENTTYPE_NONE:
			/* no file/dir here - always create */
			do_create = TRUE;
			break;

		case ENTTYPE_FILE:
			/* a file exists here - ask for permission from the user */
			ui_menu::stack_push(auto_alloc_clear(machine(), ui_menu_confirm_save_as(machine(), container, yes)));
			do_create = FALSE;
			break;

		case ENTTYPE_DIR:
			/* a directory exists here - we can't save over it */
			ui_popup_time(ERROR_MESSAGE_TIME, "Cannot save over directory");
			do_create = FALSE;
			break;

		default:
			fatalerror("Unexpected");
			do_create = FALSE;
			break;
	}

	/* create the image, if appropriate */
	if (do_create)
	{
		err = image->create(astring_c(path), 0, NULL);
		if (err != 0)
			popmessage("Error: %s", image->error());
		else
			result = TRUE;
	}

	/* free the path */
	astring_free(path);

	return result;
}



/*-------------------------------------------------
    menu_file_create - file creator menu
-------------------------------------------------*/

void ui_menu_file_create::handle()
{
	const ui_menu_event *event;
	ui_menu_event fake_event;

	if (confirm_save_as_yes)
	{
		/* we just returned from a "confirm save as" dialog and the user said "yes" - fake an event */
		memset(&fake_event, 0, sizeof(fake_event));
		fake_event.iptkey = IPT_UI_SELECT;
		fake_event.itemref = ITEMREF_CREATE;
		event = &fake_event;
	}
	else
	{
		/* process the menu */
		event = process(0);
	}

	/* process the event */
	if (event != NULL)
	{
		/* handle selections */
		switch(event->iptkey)
		{
			case IPT_UI_SELECT:
				if ((event->itemref == ITEMREF_CREATE) || (event->itemref == ITEMREF_NEW_IMAGE_NAME))
				{
					if (create_new_image(
						manager->selected_device,
						astring_c(manager->current_directory),
						filename_buffer,
						&confirm_save_as_yes))
					{
						/* success - pop out twice to device view */
						ui_menu::stack_pop(machine());
						ui_menu::stack_pop(machine());
					}
				}
				break;

			case IPT_SPECIAL:
				if (get_selection() == ITEMREF_NEW_IMAGE_NAME)
				{
					input_character(
						filename_buffer,
						ARRAY_LENGTH(filename_buffer),
						event->unichar,
						is_valid_filename_char);
				}
				break;
		}
	}
}



/***************************************************************************
    FILE SELECTOR MENU
***************************************************************************/

/*-------------------------------------------------
    file_selector_render_extra - perform our
    special rendering
-------------------------------------------------*/

void ui_menu_file_selector::custom_render(void *selectedref, float top, float bottom, float origx1, float origy1, float origx2, float origy2)
{
	extra_text_render(container, top, bottom,
		origx1, origy1, origx2, origy2,
		astring_c(manager->current_directory),
		NULL);
}



/*-------------------------------------------------
    compare_file_selector_entries - sorting proc
    for file selector entries
-------------------------------------------------*/

int ui_menu_file_selector::compare_entries(const file_selector_entry *e1, const file_selector_entry *e2)
{
	int result;
	const char *e1_basename = (e1->basename != NULL) ? e1->basename : "";
	const char *e2_basename = (e2->basename != NULL) ? e2->basename : "";

	if (e1->type < e2->type)
	{
		result = -1;
	}
	else if (e1->type > e2->type)
	{
		result = 1;
	}
	else
	{
		result = mame_stricmp(e1_basename, e2_basename);
		if (result == 0)
		{
			result = strcmp(e1_basename, e2_basename);
			if (result == 0)
			{
				if (e1 < e2)
					result = -1;
				else if (e1 > e2)
					result = 1;
			}
		}
	}

	return result;
}



/*-------------------------------------------------
    append_entry - appends a new
    file selector entry to an entry list
-------------------------------------------------*/

ui_menu_file_selector::file_selector_entry *ui_menu_file_selector::append_entry(
	file_selector_entry_type entry_type, const char *entry_basename, const char *entry_fullpath)
{
	file_selector_entry *entry;
	file_selector_entry **entryptr;

	/* allocate a new entry */
	entry = (file_selector_entry *) m_pool_alloc(sizeof(*entry));
	memset(entry, 0, sizeof(*entry));
	entry->type = entry_type;
	entry->basename = (entry_basename != NULL) ? pool_strdup(entry_basename) : entry_basename;
	entry->fullpath = (entry_fullpath != NULL) ? pool_strdup(entry_fullpath) : entry_fullpath;

	/* find the end of the list */
	entryptr = &entrylist;
	while ((*entryptr != NULL) && (compare_entries(entry, *entryptr) >= 0))
		entryptr = &(*entryptr)->next;

	/* insert the entry */
	entry->next = *entryptr;
	*entryptr = entry;
	return entry;
}



/*-------------------------------------------------
    append_entry_menu_item - appends
    a menu item for a file selector entry
-------------------------------------------------*/

ui_menu_file_selector::file_selector_entry *ui_menu_file_selector::append_dirent_entry(const osd_directory_entry *dirent)
{
	astring *buffer;
	file_selector_entry_type entry_type;
	file_selector_entry *entry;

	switch(dirent->type)
	{
		case ENTTYPE_FILE:
			entry_type = SELECTOR_ENTRY_TYPE_FILE;
			break;

		case ENTTYPE_DIR:
			entry_type = SELECTOR_ENTRY_TYPE_DIRECTORY;
			break;

		default:
			/* exceptional case; do not add a menu item */
			return NULL;
	}

	/* determine the full path */
	buffer = zippath_combine(
		astring_alloc(),
		astring_c(manager->current_directory),
		dirent->name);

	/* create the file selector entry */
	entry = append_entry(
		entry_type,
		dirent->name,
		astring_c(buffer));

	astring_free(buffer);
	return entry;
}



/*-------------------------------------------------
    append_entry_menu_item - appends
    a menu item for a file selector entry
-------------------------------------------------*/

void ui_menu_file_selector::append_entry_menu_item(const file_selector_entry *entry)
{
	const char *text = NULL;
	const char *subtext = NULL;

	switch(entry->type)
	{
		case SELECTOR_ENTRY_TYPE_EMPTY:
			text = "[empty slot]";
			break;

		case SELECTOR_ENTRY_TYPE_CREATE:
			text = "[create]";
			break;

		case SELECTOR_ENTRY_TYPE_SOFTWARE_LIST:
			text = "[software list]";
			break;

		case SELECTOR_ENTRY_TYPE_DRIVE:
			text = entry->basename;
			subtext = "[DRIVE]";
			break;

		case SELECTOR_ENTRY_TYPE_DIRECTORY:
			text = entry->basename;
			subtext = "[DIR]";
			break;

		case SELECTOR_ENTRY_TYPE_FILE:
			text = entry->basename;
			subtext = "[FILE]";
			break;
	}
	item_append(text, subtext, 0, (void *) entry);
}


/*-------------------------------------------------
    menu_file_selector_populate - creates and
    allocates all menu items for a directory
-------------------------------------------------*/

ui_menu_file_selector::ui_menu_file_selector(running_machine &machine, render_container *container, ui_menu_file_manager *parent) : ui_menu(machine, container)
{
	manager = parent;
}

ui_menu_file_selector::~ui_menu_file_selector()
{
}

void ui_menu_file_selector::populate()
{
	zippath_directory *directory = NULL;
	file_error err = FILERR_NONE;
	const osd_directory_entry *dirent;
	const file_selector_entry *entry;
	const file_selector_entry *selected_entry = NULL;
	int i;
	const char *volume_name;
	device_image_interface *device = manager->selected_device;
	const char *path = astring_c(manager->current_directory);

	/* open the directory */
	err = zippath_opendir(path, &directory);
	if (err != FILERR_NONE)
		goto done;

	/* clear out the menu entries */
	entrylist = NULL;

	/* add the "[empty slot]" entry */
	append_entry(SELECTOR_ENTRY_TYPE_EMPTY, NULL, NULL);

	if (device->is_creatable() && !zippath_is_zip(directory))
	{
		/* add the "[create]" entry */
		append_entry(SELECTOR_ENTRY_TYPE_CREATE, NULL, NULL);
	}

	/* add the "[software list]" entry */
	append_entry(SELECTOR_ENTRY_TYPE_SOFTWARE_LIST, NULL, NULL);

	/* add the drives */
	i = 0;
	while((volume_name = osd_get_volume_name(i))!=NULL)
	{
		append_entry(SELECTOR_ENTRY_TYPE_DRIVE,
			volume_name, volume_name);
		i++;
	}

	/* build the menu for each item */
	while((dirent = zippath_readdir(directory)) != NULL)
	{
		/* append a dirent entry */
		entry = append_dirent_entry(dirent);

		if (entry != NULL)
		{
			/* set the selected item to be the first non-parent directory or file */
			if ((selected_entry == NULL) && strcmp(dirent->name, ".."))
				selected_entry = entry;

			/* do we have to select this file? */
			if (!mame_stricmp(astring_c(manager->current_file), dirent->name))
				selected_entry = entry;
		}
	}

	/* append all of the menu entries */
	for (entry = entrylist; entry != NULL; entry = entry->next)
		append_entry_menu_item(entry);

	/* set the selection (if we have one) */
	if (selected_entry != NULL)
		set_selection((void *) selected_entry);

	/* set up custom render proc */
	customtop = ui_get_line_height(machine()) + 3.0f * UI_BOX_TB_BORDER;

done:
	if (directory != NULL)
		zippath_closedir(directory);
}

/*-------------------------------------------------
    check_path - performs a quick check to see if
    a path exists
-------------------------------------------------*/

static file_error check_path(const char *path)
{
	return zippath_opendir(path, NULL);
}



/*-------------------------------------------------
    menu_file_selector - file selector menu
-------------------------------------------------*/

void ui_menu_file_selector::handle()
{
	file_error err;
	const file_selector_entry *entry;
	const file_selector_entry *selected_entry = NULL;
	int bestmatch = 0;

	/* process the menu */
	const ui_menu_event *event = process(0);
	if (event != NULL && event->itemref != NULL)
	{
		/* handle selections */
		if (event->iptkey == IPT_UI_SELECT)
		{
			entry = (const file_selector_entry *) event->itemref;
			switch(entry->type)
			{
				case SELECTOR_ENTRY_TYPE_EMPTY:
					/* empty slot - unload */
					manager->selected_device->unload();
					ui_menu::stack_pop(machine());
					break;

				case SELECTOR_ENTRY_TYPE_CREATE:
					/* create */
					ui_menu::stack_push(auto_alloc_clear(machine(), ui_menu_file_create(machine(), container, manager)));
					break;
				case SELECTOR_ENTRY_TYPE_SOFTWARE_LIST:
					ui_menu::stack_push(auto_alloc_clear(machine(), ui_menu_software(machine(), container, manager->selected_device)));
					break;
				case SELECTOR_ENTRY_TYPE_DRIVE:
				case SELECTOR_ENTRY_TYPE_DIRECTORY:
					/* drive/directory - first check the path */
					err = check_path(entry->fullpath);
					if (err != FILERR_NONE)
					{
						/* this path is problematic; present the user with an error and bail */
						ui_popup_time(1, "Error accessing %s", entry->fullpath);
						break;
					}
					astring_cpyc(manager->current_directory, entry->fullpath);
					reset((ui_menu_reset_options)0);
					break;

				case SELECTOR_ENTRY_TYPE_FILE:
					/* file */
					manager->selected_device->load(entry->fullpath);
					ui_menu::stack_pop(machine());
					break;
			}

			// reset the char buffer when pressing IPT_UI_SELECT
			if (filename_buffer[0] != '\0')
				memset(filename_buffer, '\0', ARRAY_LENGTH(filename_buffer));
		}
		else if (event->iptkey == IPT_SPECIAL)
		{
			int buflen = strlen(filename_buffer);
			bool update_selected = FALSE;

			/* if it's a backspace and we can handle it, do so */
			if ((event->unichar == 8 || event->unichar == 0x7f) && buflen > 0)
			{
				*(char *)utf8_previous_char(&filename_buffer[buflen]) = 0;
				update_selected = TRUE;

				if (ARRAY_LENGTH(filename_buffer) > 0)
					ui_popup_time(ERROR_MESSAGE_TIME, "%s", filename_buffer);
			}
			/* if it's any other key and we're not maxed out, update */
			else if (event->unichar >= ' ' && event->unichar < 0x7f)
			{
				buflen += utf8_from_uchar(&filename_buffer[buflen], ARRAY_LENGTH(filename_buffer) - buflen, event->unichar);
				filename_buffer[buflen] = 0;
				update_selected = TRUE;

				if (ARRAY_LENGTH(filename_buffer) > 0)
					ui_popup_time(ERROR_MESSAGE_TIME, "%s", filename_buffer);
			}

			if (update_selected)
			{
				const file_selector_entry *cur_selected = (const file_selector_entry *)get_selection();

				// check for entries which matches our filename_buffer:
				// from current entry to the end
				for (entry = cur_selected; entry != NULL; entry = entry->next)
				{
					if (entry->basename != NULL && filename_buffer != NULL)
					{
						int match = 0;
						for (int i = 0; i < ARRAY_LENGTH(filename_buffer); i++)
						{
							if (mame_strnicmp(entry->basename, filename_buffer, i) == 0)
								match = i;
						}

						if (match > bestmatch)
						{
							bestmatch = match;
							selected_entry = entry;
						}
					}
				}
				// and from the first entry to current one
				for (entry = entrylist; entry != cur_selected; entry = entry->next)
				{
					if (entry->basename != NULL && filename_buffer != NULL)
					{
						int match = 0;
						for (int i = 0; i < ARRAY_LENGTH(filename_buffer); i++)
						{
							if (mame_strnicmp(entry->basename, filename_buffer, i) == 0)
								match = i;
						}

						if (match > bestmatch)
						{
							bestmatch = match;
							selected_entry = entry;
						}
					}
				}

				if (selected_entry != NULL && selected_entry != cur_selected)
					set_selection((void *) selected_entry);
			}
		}
		else if (event->iptkey == IPT_UI_CANCEL)
		{
			// reset the char buffer also in this case
			if (filename_buffer[0] != '\0')
				memset(filename_buffer, '\0', ARRAY_LENGTH(filename_buffer));
		}
	}
}



/***************************************************************************
    FILE MANAGER
***************************************************************************/

/*-------------------------------------------------
    fix_working_directory - checks the working
    directory for this device to ensure that it
    "makes sense"
-------------------------------------------------*/

void ui_menu_file_manager::fix_working_directory(device_image_interface *image)
{
	/* if the image exists, set the working directory to the parent directory */
	if (image->exists())
	{
		astring *astr = astring_alloc();
		zippath_parent(astr, image->filename());
		image->set_working_directory(astring_c(astr));
		astring_free(astr);
	}

	/* check to see if the path exists; if not clear it */
	if (check_path(image->working_directory()) != FILERR_NONE)
		image->set_working_directory("");
}



/*-------------------------------------------------
    file_manager_render_extra - perform our
    special rendering
-------------------------------------------------*/

void ui_menu_file_manager::custom_render(void *selectedref, float top, float bottom, float origx1, float origy1, float origx2, float origy2)
{
	const char *path;

	/* access the path */
	path = selected_device ? selected_device->filename() : NULL;
	extra_text_render(container, top, bottom,
					  origx1, origy1, origx2, origy2, NULL, path);
}



/*-------------------------------------------------
    menu_file_manager_populate - populates the main
    file manager menu
-------------------------------------------------*/

ui_menu_file_manager::ui_menu_file_manager(running_machine &machine, render_container *container) : ui_menu(machine, container)
{
	current_directory = astring_alloc();
	current_file = astring_alloc();
}

void ui_menu_file_manager::populate()
{
	char buffer[2048];
	device_image_interface *image = NULL;
	astring tmp_name;

	/* cycle through all devices for this system */
	for (bool gotone = machine().devicelist().first(image); gotone; gotone = image->next(image))
	{
		/* get the image type/id */
		snprintf(buffer, ARRAY_LENGTH(buffer),
			"%s (%s)",
			image->device().name(), image->brief_instance_name());

		/* get the base name */
		if (image->basename() != NULL)
		{
			tmp_name.cpy(image->basename());

			/* if the image has been loaded through softlist, also show the loaded part */
			if (image->part_entry() != NULL)
			{
				const software_part *tmp = image->part_entry();
				if (tmp->name != NULL)
				{
					tmp_name.cat(" (");
					tmp_name.cat(tmp->name);
					/* also check if this part has a specific part_id (e.g. "Map Disc", "Bonus Disc", etc.), and in case display it */
					if (image->get_feature("part_id") != NULL)
					{
						tmp_name.cat(": ");
						tmp_name.cat(image->get_feature("part_id"));
					}
					tmp_name.cat(")");
				}
			}
		}
		else
			tmp_name.cpy("---");

		/* record the menu item */
		item_append(buffer, tmp_name.cstr(), 0, (void *) image);
	}

	custombottom = ui_get_line_height(machine()) + 3.0f * UI_BOX_TB_BORDER;
}



/*-------------------------------------------------
    file_manager_destroy_state - state destructor
-------------------------------------------------*/

ui_menu_file_manager::~ui_menu_file_manager()
{
	astring_free(current_directory);
	astring_free(current_file);
}



/*-------------------------------------------------
    menu_file_manager - main file manager menu
-------------------------------------------------*/

void ui_menu_file_manager::handle()
{
	/* update the selected device */
	selected_device = (device_image_interface *) get_selection();

	/* process the menu */
	const ui_menu_event *event = process(0);
	if (event != NULL && event->iptkey == IPT_UI_SELECT)
	{
		selected_device = (device_image_interface *) event->itemref;
		if (selected_device != NULL)
		{
			/* ensure that the working directory for this device exists */
			fix_working_directory(selected_device);

			/* set up current_directory and current_file - depends on whether we have an image */
			astring_cpyc(current_directory, selected_device->working_directory());
			astring_cpyc(current_file, selected_device->exists() ? selected_device->basename() : "");

			/* reset the existing menu */
			reset(UI_MENU_RESET_REMEMBER_POSITION);

			/* push the menu */
			ui_menu::stack_push(auto_alloc_clear(machine(), ui_menu_file_selector(machine(), container, this)));
		}
	}
}

/*-------------------------------------------------
    ui_menu_image_info - menu that shows info
    on all loaded images
-------------------------------------------------*/

ui_menu_image_info::ui_menu_image_info(running_machine &machine, render_container *container) : ui_menu(machine, container)
{
}

void ui_menu_image_info::populate()
{
	astring *tempstring = image_info_astring(machine(), astring_alloc());
	item_append(astring_c(tempstring), NULL, MENU_FLAG_MULTILINE, NULL);
	astring_free(tempstring);
}

ui_menu_image_info::~ui_menu_image_info()
{
}

void ui_menu_image_info::handle()
{
	/* process the menu */
	process(0);
}

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/
#define TAPECMD_NULL			((void *) 0x0000)
#define TAPECMD_STOP			((void *) 0x0001)
#define TAPECMD_PLAY			((void *) 0x0002)
#define TAPECMD_RECORD			((void *) 0x0003)
#define TAPECMD_REWIND			((void *) 0x0004)
#define TAPECMD_FAST_FORWARD		((void *) 0x0005)
#define TAPECMD_SLIDER			((void *) 0x0006)
#define TAPECMD_SELECT			((void *) 0x0007)

#define BITBANGERCMD_SELECT			((void *) 0x0000)
#define BITBANGERCMD_MODE			((void *) 0x0001)
#define BITBANGERCMD_BAUD			((void *) 0x0002)
#define BITBANGERCMD_TUNE			((void *) 0x0003)


/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

ui_menu_mess_tape_control::ui_menu_mess_tape_control(running_machine &machine, render_container *container) : ui_menu(machine, container)
{
}

ui_menu_mess_tape_control::~ui_menu_mess_tape_control()
{
}

ui_menu_mess_bitbanger_control::ui_menu_mess_bitbanger_control(running_machine &machine, render_container *container) : ui_menu(machine, container)
{
}

ui_menu_mess_bitbanger_control::~ui_menu_mess_bitbanger_control()
{
}

/*-------------------------------------------------
    cassette_count - returns the number of cassette
    devices in the machine
-------------------------------------------------*/

int ui_menu_mess_tape_control::cassette_count()
{
	int count = 0;
	device_t *device = machine().devicelist().first(CASSETTE);

	while ( device )
	{
		count++;
		device = device->typenext();
	}
	return count;
}

/*-------------------------------------------------
    bitbanger_count - returns the number of bitbanger
    devices in the machine
-------------------------------------------------*/

int ui_menu_mess_bitbanger_control::bitbanger_count()
{
	int count = 0;
	device_t *device = machine().devicelist().first(BITBANGER);

	while ( device )
	{
		count++;
		device = device->typenext();
	}
	return count;
}

/*-------------------------------------------------
    tapecontrol_gettime - returns a textual
    representation of the time
-------------------------------------------------*/

astring *tapecontrol_gettime(astring *dest, cassette_image_device *cassette, int *curpos, int *endpos)
{
	double t0, t1;

	t0 = cassette->get_position();
	t1 = cassette->get_length();

	if (t1)
		astring_printf(dest, "%04d/%04d", (int) t0, (int) t1);
	else
		astring_printf(dest, "%04d/%04d", 0, (int) t1);

	if (curpos != NULL)
		*curpos = t0;
	if (endpos != NULL)
		*endpos = t1;

	return dest;
}



/*-------------------------------------------------
    menu_tape_control_populate - populates the
    main tape control menu
-------------------------------------------------*/

void ui_menu_mess_tape_control::populate()
{
	astring timepos;
	cassette_state state;
	int count = cassette_count();
	UINT32 flags = 0;

	if( count > 0 )
	{
		if( index == (count-1) )
			flags |= MENU_FLAG_LEFT_ARROW;
		else
			flags |= MENU_FLAG_RIGHT_ARROW;
	}

	if ((device != NULL) && (device->exists()))
	{
		double t0, t1;
		UINT32 tapeflags = 0;
		cassette_image_device* cassette = dynamic_cast<cassette_image_device*>(&device->device());

		t0 = cassette->get_position();
		t1 = cassette->get_length();

		if (t1 > 0)
		{
			if (t0 > 0)
				tapeflags |= MENU_FLAG_LEFT_ARROW;
			if (t0 < t1)
				tapeflags |= MENU_FLAG_RIGHT_ARROW;
		}

		/* name of tape */
		item_append(device->device().name(), device->filename(), flags, TAPECMD_SELECT);

		/* state */
		tapecontrol_gettime(&timepos, cassette, NULL, NULL);
		state = cassette->get_state();
		item_append(
			(state & CASSETTE_MASK_UISTATE) == CASSETTE_STOPPED
				?	"stopped"
				:	((state & CASSETTE_MASK_UISTATE) == CASSETTE_PLAY
					? ((state & CASSETTE_MASK_MOTOR) == CASSETTE_MOTOR_ENABLED ? "playing" : "(playing)")
					: ((state & CASSETTE_MASK_MOTOR) == CASSETTE_MOTOR_ENABLED ? "recording" : "(recording)")
					),
			astring_c(&timepos),
			tapeflags,
			TAPECMD_SLIDER);

		/* pause or stop */
		item_append("Pause/Stop", NULL, 0, TAPECMD_STOP);

		/* play */
		item_append("Play", NULL, 0, TAPECMD_PLAY);

		/* record */
		item_append("Record", NULL, 0, TAPECMD_RECORD);

		/* rewind */
		item_append("Rewind", NULL, 0, TAPECMD_REWIND);

		/* fast forward */
		item_append("Fast Forward", NULL, 0, TAPECMD_FAST_FORWARD);
	}
	else
	{
		/* no tape loaded */
		item_append("No Tape Image loaded", NULL, flags, NULL);
	}
}


/*-------------------------------------------------
    menu_bitbanger_control_populate - populates the
    main bitbanger control menu
-------------------------------------------------*/

void ui_menu_mess_bitbanger_control::populate()
{
	int count = bitbanger_count();
	UINT32 flags = 0, mode_flags = 0, baud_flags = 0, tune_flags = 0;

	if( count > 0 )
	{
		if( index == (count-1) )
			flags |= MENU_FLAG_LEFT_ARROW;
		else
			flags |= MENU_FLAG_RIGHT_ARROW;
	}

	if ((device != NULL) && (device->exists()))
	{
		bitbanger_device *bitbanger = downcast<bitbanger_device *>(&device->device());

		if (bitbanger->inc_mode(TRUE))
		  mode_flags |= MENU_FLAG_RIGHT_ARROW;

		if (bitbanger->dec_mode(TRUE))
		  mode_flags |= MENU_FLAG_LEFT_ARROW;

		if (bitbanger->inc_baud(TRUE))
		  baud_flags |= MENU_FLAG_RIGHT_ARROW;

		if (bitbanger->dec_baud(TRUE))
		  baud_flags |= MENU_FLAG_LEFT_ARROW;

		if (bitbanger->inc_tune(TRUE))
		  tune_flags |= MENU_FLAG_RIGHT_ARROW;

		if (bitbanger->dec_tune(TRUE))
		  tune_flags |= MENU_FLAG_LEFT_ARROW;

		/* name of bitbanger file */
		item_append(device->device().name(), device->filename(), flags, BITBANGERCMD_SELECT);
		item_append("Device Mode:", bitbanger->mode_string(), mode_flags, BITBANGERCMD_MODE);
		item_append("Baud:", bitbanger->baud_string(), baud_flags, BITBANGERCMD_BAUD);
		item_append("Baud Tune:", bitbanger->tune_string(), tune_flags, BITBANGERCMD_TUNE);
		item_append("Protocol:", "8-1-N", 0, NULL);
	}
	else
	{
		/* no tape loaded */
		item_append("No Bitbanger Image loaded", NULL, flags, NULL);
	}
}


/*-------------------------------------------------
    menu_tape_control - main tape control menu
-------------------------------------------------*/

void ui_menu_mess_tape_control::handle()
{
	/* do we have to load the device? */
	if (device == NULL)
	{
		int cindex = index;
		for (bool gotone = machine().devicelist().first(device); gotone; gotone = device->next(device))
		{
			if(device->device().type() == CASSETTE) {
				if (cindex==0) break;
				cindex--;
			}
		}
		reset((ui_menu_reset_options)0);
	}

	/* rebuild the menu - we have to do this so that the counter updates */
	reset(UI_MENU_RESET_REMEMBER_POSITION);
	populate();

	cassette_image_device* cassette = dynamic_cast<cassette_image_device*>(&device->device());

	/* process the menu */
	const ui_menu_event *event = process(UI_MENU_PROCESS_LR_REPEAT);
	if (event != NULL)
	{
		switch(event->iptkey)
		{
			case IPT_UI_LEFT:
				if (event->itemref==TAPECMD_SLIDER)
					cassette->seek(-1, SEEK_CUR);
				else
				if (event->itemref==TAPECMD_SELECT)
				{
					/* left arrow - rotate left through cassette devices */
					if (index > 0)
						index--;
					else
						index = cassette_count() - 1;
					device = NULL;
				}
				break;

			case IPT_UI_RIGHT:
				if (event->itemref==TAPECMD_SLIDER)
					cassette->seek(+1, SEEK_CUR);
				else
				if (event->itemref==TAPECMD_SELECT)
				{
					/* right arrow - rotate right through cassette devices */
					if (index < cassette_count() - 1)
						index++;
					else
						index = 0;
					device = NULL;
				}
				break;

			case IPT_UI_SELECT:
				{
					if (event->itemref==TAPECMD_STOP)
						cassette->change_state(CASSETTE_STOPPED, CASSETTE_MASK_UISTATE);
					else
					if (event->itemref==TAPECMD_PLAY)
						cassette->change_state(CASSETTE_PLAY, CASSETTE_MASK_UISTATE);
					else
					if (event->itemref==TAPECMD_RECORD)
						cassette->change_state(CASSETTE_RECORD, CASSETTE_MASK_UISTATE);
					else
					if (event->itemref==TAPECMD_REWIND)
						cassette->seek(-30, SEEK_CUR);
					else
					if (event->itemref==TAPECMD_FAST_FORWARD)
						cassette->seek(30, SEEK_CUR);
					else
					if (event->itemref==TAPECMD_SLIDER)
						cassette->seek(0, SEEK_SET);
				}
				break;
		}
	}
}


/*-------------------------------------------------
    menu_bitbanger_control - main bitbanger
    control menu
-------------------------------------------------*/

void ui_menu_mess_bitbanger_control::handle()
{
	/* do we have to load the device? */
	if (device == NULL)
	{
		int cindex = index;
		for (bool gotone = machine().devicelist().first(device); gotone; gotone = device->next(device))
		{
			if(device->device().type() == BITBANGER) {
				if (cindex==0) break;
				cindex--;
			}
		}
		reset((ui_menu_reset_options)0);
	}

	/* get the bitbanger */
	bitbanger_device *bitbanger = downcast<bitbanger_device *>(device);

	/* rebuild the menu */
	reset(UI_MENU_RESET_REMEMBER_POSITION);
	populate();

	/* process the menu */
	const ui_menu_event *event = process(UI_MENU_PROCESS_LR_REPEAT);
	if (event != NULL)
	{
		switch(event->iptkey)
		{
			case IPT_UI_LEFT:
				if (event->itemref==BITBANGERCMD_SELECT)
				{
					/* left arrow - rotate left through cassette devices */
					if (index > 0)
						index--;
					else
						index = bitbanger_count() - 1;
					device = NULL;
				}
				else if (event->itemref==BITBANGERCMD_MODE)
				{
				   bitbanger->dec_mode(FALSE);
				}
				else if (event->itemref==BITBANGERCMD_BAUD)
				{
				   bitbanger->dec_baud(FALSE);
				}
				else if (event->itemref==BITBANGERCMD_TUNE)
				{
				   bitbanger->dec_tune(FALSE);
				}
				break;

			case IPT_UI_RIGHT:
				if (event->itemref==BITBANGERCMD_SELECT)
				{
					/* right arrow - rotate right through cassette devices */
					if (index < bitbanger_count() - 1)
						index++;
					else
						index = 0;
					device = NULL;
				}
				else if (event->itemref==BITBANGERCMD_MODE)
				{
				   bitbanger->inc_mode(FALSE);
				}
				else if (event->itemref==BITBANGERCMD_BAUD)
				{
				   bitbanger->inc_baud(FALSE);
				}
				else if (event->itemref==BITBANGERCMD_TUNE)
				{
				   bitbanger->inc_tune(FALSE);
				}
				break;
		}
	}
}

