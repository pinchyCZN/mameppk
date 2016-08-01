/***************************************************************************

  M.A.M.E.UI  -  Multiple Arcade Machine Emulator with User Interface
  Win32 Portions Copyright (C) 1997-2003 Michael Soderstrom and Chris Kirmse,
  Copyright (C) 2003-2007 Chris Kirmse and the MAME32/MAMEUI team.

  This file is part of MAMEUI, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.

 ***************************************************************************/

/***************************************************************************

  treeview.c

  TreeView support routines - MSH 11/19/1998

***************************************************************************/

// standard windows headers
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <commctrl.h>

// standard C headers
#include <stdio.h>  // for sprintf
#include <stdlib.h> // For malloc and free
#include <ctype.h> // For tolower
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef _MSC_VER
#include <direct.h>
#endif
#include <tchar.h>
#include <io.h>

// MAME/MAMEUI headers
#include "emu.h"
#include "hash.h"
#include "mui_util.h"
#include "bitmask.h"
#include "winui.h"
#include "treeview.h"
#include "resource.h"
#include "mui_opts.h"
#include "dialogs.h"
#include "winutf8.h"
#include "strconv.h"
#include "translate.h"

#ifdef _MSC_VER
#define snprintf _snprintf
#define snwprintf _snwprintf
#endif

#define MAX_EXTRA_FOLDERS 256

/***************************************************************************
    public structures
 ***************************************************************************/

#define ICON_MAX ARRAY_LENGTH(treeIconNames)

/* Name used for user-defined custom icons */
/* external *.ico file to look for. */

typedef struct
{
	int		nResourceID;
	LPCSTR	lpName;
} TREEICON;

static TREEICON treeIconNames[] =
{
#ifdef MAMEUIPLUSPLUS
	{ IDI_FOLDER_OPEN,         "hfoldopen" },
	{ IDI_FOLDER,              "hfolder" },
	{ IDI_FOLDER_AVAILABLE,    "hfoldavail" },
	{ IDI_FOLDER_MANUFACTURER, "hfoldmanu" },
	{ IDI_FOLDER_UNAVAILABLE,  "hfoldunav" },
	{ IDI_FOLDER_YEAR,         "hfoldyear" },
	{ IDI_FOLDER_SOURCE,       "hfoldsrc" },
	{ IDI_FOLDER_HORIZONTAL,   "horz" },
	{ IDI_FOLDER_VERTICAL,     "vert" },
	{ IDI_MANUFACTURER,        "hmanufact" },
	{ IDI_WORKING,             "hworking" },
	{ IDI_NONWORKING,          "hnonwork" },
	{ IDI_YEAR,                "hyear" },
	{ IDI_SOUND,               "hstereo" },
	{ IDI_CPU,                 "hcpu" },
	{ IDI_HARDDISK,            "hharddisk" },
	{ IDI_SOURCE,              "hsource" },
	{ IDI_SND,                 "hsnd" },
	{ IDI_BIOS,                "hbios" },
	{ IDI_FOLDER_ORIENTATION,  "orientation" },
	{ IDI_FOLDER_IMPERFECT,    "foldimp" },
	{ IDI_FOLDER_DUMPING,      "dumping" },
	{ IDI_FOLDER_ORIGINAL,     "foldoriginal" },
	{ IDI_FOLDER_CLONES,       "foldclones" },
//	{ IDI_FOLDER_RASTER,       "raster" },
//	{ IDI_FOLDER_VECTOR,       "vector" },
	{ IDI_FOLDER_RESOLUTION,   "resolution" },
	{ IDI_FOLDER_FPS,          "foldfps" },
	{ IDI_FOLDER_SAVESTATE,    "savestate" },
	{ IDI_FOLDER_CONTROL,      "control" },
	{ IDI_FOLDER_SAMPLES,      "samples" },
	{ IDI_FOLDER_NEOGEO,       "neogeo" },
	{ IDI_FOLDER_NAMCO,        "namco" },
	{ IDI_FOLDER_TAITO,        "taito" },
	{ IDI_FOLDER_80S,          "80s" },
	{ IDI_FOLDER_OLDS,         "olds" },
	{ IDI_FOLDER_SEGA,         "sega" },
	{ IDI_FOLDER_TOA,          "toa" },
	{ IDI_FOLDER_CAVE,         "cave" },
	{ IDI_FOLDER_KONAMI,       "konami" },
	{ IDI_FOLDER_CAPCOM,       "capcom" },
	{ IDI_FOLDER_MAHJONG,      "mahjong" },
	{ IDI_FOLDER_GOLDEN,       "golden" }
#else
	{ IDI_FOLDER_OPEN,         "foldopen" },
	{ IDI_FOLDER,              "folder" },
	{ IDI_FOLDER_AVAILABLE,    "foldavail" },
	{ IDI_FOLDER_MANUFACTURER, "foldmanu" },
	{ IDI_FOLDER_UNAVAILABLE,  "foldunav" },
	{ IDI_FOLDER_YEAR,         "foldyear" },
	{ IDI_FOLDER_SOURCE,       "foldsrc" },
	{ IDI_FOLDER_HORIZONTAL,   "horz" },
	{ IDI_FOLDER_VERTICAL,     "vert" },
	{ IDI_MANUFACTURER,        "manufact" },
	{ IDI_WORKING,             "working" },
	{ IDI_NONWORKING,          "nonwork" },
	{ IDI_YEAR,                "year" },
	{ IDI_SOUND,               "sound" },
	{ IDI_CPU,                 "cpu" },
	{ IDI_HARDDISK,            "harddisk" },
	{ IDI_SOURCE,              "source" },
	{ IDI_SND,                 "snd" },
	{ IDI_BIOS,                "bios" }
#endif /* MAMEUIPLUSPLUS */
};

/***************************************************************************
    private variables
 ***************************************************************************/

/* this has an entry for every folder eventually in the UI, including subfolders */
static TREEFOLDER **treeFolders = 0;
static UINT         numFolders  = 0;        /* Number of folder in the folder array */
static UINT         next_folder_id = MAX_FOLDERS;
static UINT         folderArrayLength = 0;  /* Size of the folder array */
static LPTREEFOLDER lpCurrentFolder = 0;    /* Currently selected folder */
static UINT         nCurrentFolder = 0;     /* Current folder ID */
static WNDPROC      g_lpTreeWndProc = 0;    /* for subclassing the TreeView */
static HIMAGELIST   hTreeSmall = 0;         /* TreeView Image list of icons */

/* this only has an entry for each TOP LEVEL extra folder + SubFolders*/
static LPEXFOLDERDATA		ExtraFolderData[MAX_EXTRA_FOLDERS];
static int			        numExtraFolders = 0;
static int          numExtraIcons = 0;
static char         *ExtraFolderIcons[MAX_EXTRA_FOLDERS];

// built in folders and filters
static LPCFOLDERDATA  g_lpFolderData;
static LPCFILTER_ITEM g_lpFilterList;

/***************************************************************************
    private function prototypes
 ***************************************************************************/

extern BOOL			InitFolders(void);
static BOOL         CreateTreeIcons(void);
static void         TreeCtrlOnPaint(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static const TCHAR *ParseManufacturer(const TCHAR *s, int *pParsedChars );
static const TCHAR *TrimManufacturer(const TCHAR *s);
static void         CreateAllChildFolders(void);
static BOOL         AddFolder(LPTREEFOLDER lpFolder);
static LPTREEFOLDER NewFolder(const TCHAR *lpTitle,
                              UINT nCategoryID, BOOL bTranslate, UINT nFolderId, int nParent, UINT nIconId);
static void         DeleteFolder(LPTREEFOLDER lpFolder);
static const TCHAR *GetFolderOrigName(LPTREEFOLDER lpFolder);

static LRESULT CALLBACK TreeWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

static int InitExtraFolders(void);
static void FreeExtraFolders(void);
static BOOL RegistExtraFolder(const TCHAR *name, LPEXFOLDERDATA *fExData, int msgcat, int icon, int subicon);
static void SetExtraIcons(char *name, int *id);
static BOOL TryAddExtraFolderAndChildren(int parent_index);

static BOOL TrySaveExtraFolder(LPTREEFOLDER lpFolder);

/***************************************************************************
    public functions
 ***************************************************************************/

static struct FOLDER_DATA {
	TCHAR*	name;
	int		count;
	int		malloc_count;
	const char	**buf;
} *extra_folders = 0;
static int extra_folder_count = 0;

static BOOL NeedToUpdateIni(void)
{
	return GetRequiredDriverCacheStatus();
}

static int CompareFunc( const void* a , const void* b )
{
	return wcsicmp(*(LPWSTR*)a, *(LPWSTR*)b);
}

static int dataCompareFunc( const void* a , const void* b )
{
	return wcsicmp(((struct FOLDER_DATA *)a)->name, ((struct FOLDER_DATA *)b)->name);
}

static void dataAddGame(struct FOLDER_DATA *x, int n)
{
	if (++x->count > x->malloc_count)
	{
		const char **p = x->buf;
		x->malloc_count += 500;
		p = (const char**)malloc(x->malloc_count * sizeof(char*));
		assert(p);
		if (x->buf)
		{
			memcpy(p, x->buf, (x->malloc_count - 500) * sizeof(char*));
			free(x->buf);
		}
		x->buf = p;
	}
	x->buf[x->count - 1] = driver_list::driver(n).name;
}

static void InitExtraFolderData(int n)
{
	extra_folder_count = 0;
	extra_folders = (struct FOLDER_DATA *)malloc(n * sizeof(struct FOLDER_DATA));
	assert(extra_folders);
	memset(extra_folders, 0, n * sizeof(struct FOLDER_DATA));
}
static void FreeExtraFolderData(void)
{
	int i;

	for (i = 0; i < extra_folder_count; i++)
	{
		osd_free(extra_folders[i].name);
		if (extra_folders[i].buf)
			free(extra_folders[i].buf);
	}
	free(extra_folders);
	extra_folders = NULL;
	extra_folder_count = 0;
}

static void OutputIniAndFreeExtraFolderData(const TCHAR *fname)
{
	int i, j;
	struct stat st;
	if (stat(_String(GetUIDir()), &st) != 0 && _wmkdir(GetUIDir()) != 0)
	{
		FreeExtraFolderData();
		return;
	}

	FILE *fp;
	TCHAR filename[MAX_PATH];

	wcscpy(filename, GetUIDir());
	wcscat(filename, fname);
	fp = _wfopen(filename, TEXT("w"));
	fprintf(fp, "[ROOT_FOLDER]\n");

	if (extra_folder_count != 0)
	{
		qsort(extra_folders, extra_folder_count, sizeof(extra_folders[0]), dataCompareFunc);
		for (i = 0; i < extra_folder_count; i++)
		{
			// folder
			fwprintf(fp, TEXT("\n[%s]\n"), extra_folders[i].name);
			if (extra_folders[i].count > 0)
			{
				// game
				qsort(extra_folders[i].buf, extra_folders[i].count, sizeof(extra_folders[i].buf[0]), CompareFunc);
				for (j = 0; j < extra_folders[i].count; j++)
					fprintf(fp, "%s\n", extra_folders[i].buf[j]);
			}
		}

		FreeExtraFolderData();
	}
	fclose(fp);
}

static void LoadExternalFolders(int parent_index, const TCHAR *fname, int id)
{
    FILE*   fp = NULL;
    char    readbuf[256];
    char*   p;
    char*   name;
    int     current_id;
    LPTREEFOLDER lpTemp = NULL;
	LPTREEFOLDER lpFolder = treeFolders[parent_index];

	// no games in top level folder
	SetAllBits(lpFolder->m_lpGameBits,FALSE);

	current_id = lpFolder->m_nFolderId;

    fp = wfopen(fname, TEXT("r"));
 
	if (fp == NULL)
		return;

	while (fgets(readbuf, 256, fp))
	{
		/* do we have [...] ? */

		if (readbuf[0] == '[')
		{
			p = strchr(readbuf, ']');
			if (p == NULL)
			{
				continue;
			}

			*p = '\0';
			name = &readbuf[1];

			/* is it [FOLDER_SETTINGS]? */

			if (strcmp(name, "FOLDER_SETTINGS") == 0)
			{
				current_id = -1;
				continue;
			}
			else
			{
				/* drop DBCS folder */
				for (p = name; *p; p++)
					if (*p & 0x80)
						break;

				if (*p)
				{
					current_id = -1;
					continue;
				}

				/* it it [ROOT_FOLDER]? */

				if (!strcmp(name, "ROOT_FOLDER"))
				{
					current_id = lpFolder->m_nFolderId;
					lpTemp = lpFolder;

				}
				else
				{
					/* must be [folder name] */
					TCHAR *foldername = _Unicode(name);

					current_id = next_folder_id++;
					/* create a new folder with this name,
					   and the flags for this folder as read from the registry */
					lpTemp = NewFolder(foldername, 0, TRUE, current_id, parent_index, id);

					//lpTemp->m_dwFlags |= F_CUSTOM;

					AddFolder(lpTemp);
				}
			}
		}
		else if (current_id != -1)
		{
			/* string on a line by itself -- game name */

			name = strtok(readbuf, " \t\r\n");
			if (name == NULL)
			{
				current_id = -1;
				continue;
			}

			/* IMPORTANT: This assumes that all driver names are lowercase! */
			strlwr(name);

			if (lpTemp == NULL)
			{
				ErrorMsg("Error parsing %s: missing [folder name] or [ROOT_FOLDER]",
						 fname);
				current_id = lpFolder->m_nFolderId;
				lpTemp = lpFolder;
			}
			AddGame(lpTemp, GetGameNameIndex(name));
        }
    }

    if ( fp )
    {
        fclose( fp );
    }

    return;
}

#ifdef UNUSED_FUNCTION
/**************************************************************************
 *      ci_strncmp - case insensitive character array compare
 *
 *      Returns zero if the first n characters of s1 and s2 are equal,
 *      ignoring case.
 *      stolen from datafile.c
 **************************************************************************/
static int ci_strncmp (const char *s1, const char *s2, int n)
{
        int c1, c2;

        while (n)
        {
                if ((c1 = tolower (*s1)) != (c2 = tolower (*s2)))
                        return (c1 - c2);
                else if (!c1)
                        break;
                --n;

                s1++;
                s2++;
        }
        return 0;
}
#endif



/* De-allocate all folder memory */
void FreeFolders(void)
{
	int i = 0;

	if (treeFolders != NULL)
	{
		if (numExtraFolders)
		{
			FreeExtraFolders();
			numFolders -= numExtraFolders;
		}

		for (i = numFolders - 1; i >= 0; i--)
		{
			DeleteFolder(treeFolders[i]);
			treeFolders[i] = NULL;
			numFolders--;
		}
		free(treeFolders);
		treeFolders = NULL;
	}
	numFolders = 0;
}

/* Reset folder filters */
void ResetFilters(void)
{
	int i = 0;

	if (treeFolders != 0)
	{
		for (i = 0; i < numFolders; i++)
		{
			treeFolders[i]->m_dwFlags &= ~F_MASK;

			/* Save the filters to the ini file */
			SaveFolderFlags(treeFolders[i]->m_lpPath, 0);
		}
	}
}

void InitTree(LPCFOLDERDATA lpFolderData, LPCFILTER_ITEM lpFilterList)
{
	LONG_PTR l;

	g_lpFolderData = lpFolderData;
	g_lpFilterList = lpFilterList;

	InitFolders();

	/* this will subclass the treeview (where WM_DRAWITEM gets sent for
	   the header control) */
	l = GetWindowLongPtr(GetTreeView(), GWLP_WNDPROC);
	g_lpTreeWndProc = (WNDPROC)l;
	SetWindowLongPtr(GetTreeView(), GWLP_WNDPROC, (LONG_PTR)TreeWndProc);
}

#ifdef UNUSED_FUNCTION
void DestroyTree(HWND hWnd)
{
    if ( hTreeSmall )
    {
        ImageList_Destroy( hTreeSmall );
        hTreeSmall = NULL;
    }
}
#endif

void SetCurrentFolder(LPTREEFOLDER lpFolder)
{
	lpCurrentFolder = (lpFolder == 0) ? treeFolders[0] : lpFolder;
	nCurrentFolder = (lpCurrentFolder) ? lpCurrentFolder->m_nFolderId : 0;
}

LPTREEFOLDER GetCurrentFolder(void)
{
	return lpCurrentFolder;
}

UINT GetCurrentFolderID(void)
{
	return nCurrentFolder;
}

LPTREEFOLDER GetFolder(UINT nFolder)
{
	return (nFolder < numFolders) ? treeFolders[nFolder] : NULL;
}

LPTREEFOLDER GetFolderByID(UINT nID)
{
	UINT i;

	for (i = 0; i < numFolders; i++)
	{
		if (treeFolders[i]->m_nFolderId == nID)
		{
			return treeFolders[i];
		}
	}

	return (LPTREEFOLDER)0;
}

void AddGame(LPTREEFOLDER lpFolder, UINT nGame)
{
	if (lpFolder)
		SetBit(lpFolder->m_lpGameBits, nGame);
}

void RemoveGame(LPTREEFOLDER lpFolder, UINT nGame)
{
	ClearBit(lpFolder->m_lpGameBits, nGame);
}

int FindGame(LPTREEFOLDER lpFolder, int nGame)
{
	return FindBit(lpFolder->m_lpGameBits, nGame, TRUE);
}

// Called to re-associate games with folders
void ResetWhichGamesInFolders(void)
{
	UINT	i, jj, k;
	BOOL b;
	int nGames = GetNumGames();

	for (i = 0; i < numFolders; i++)
	{
		LPTREEFOLDER lpFolder = treeFolders[i];
		// setup the games in our built-in folders
		for (k = 0; g_lpFolderData[k].m_lpTitle; k++)
		{
			if (lpFolder->m_nFolderId == g_lpFolderData[k].m_nFolderId)
			{
				if (g_lpFolderData[k].m_pfnQuery || g_lpFolderData[k].m_bExpectedResult)
				{
					SetAllBits(lpFolder->m_lpGameBits, FALSE);
					for (jj = 0; jj < nGames; jj++)
					{
						// invoke the query function
						b = g_lpFolderData[k].m_pfnQuery ? g_lpFolderData[k].m_pfnQuery(jj) : TRUE;

						// if we expect FALSE, flip the result
						if (!g_lpFolderData[k].m_bExpectedResult)
							b = !b;

						// if we like what we hear, add the game
						if (b)
							AddGame(lpFolder, jj);
					}
				}
				break;
			}
		}
	}
}


/* Used to build the GameList */
BOOL GameFiltered(int nGame, DWORD dwMask)
{
	int i;
	LPTREEFOLDER lpFolder = GetCurrentFolder();
	LPTREEFOLDER lpParent = NULL;
	LPCWSTR filter_text = GetFilterText();
	LPCWSTR search_text = GetSearchText();

	//Filter out the Bioses on all Folders, except for the Bios Folder
	if( lpFolder->m_nFolderId != FOLDER_BIOS )
	{
		if( !( (driver_list::driver(nGame).flags & GAME_IS_BIOS_ROOT ) == 0) )
			return TRUE;
		if( driver_list::driver(nGame).name[0] == '_' )
			return TRUE;
	}

	//mamep: filter for search box control, place it here for better performance
	if (wcslen(search_text) && _wcsicmp(search_text, _UIW(TEXT(SEARCH_PROMPT))))
	{
		if (MyStrStrI(driversw[nGame]->description, search_text) == NULL &&
			MyStrStrI(_LSTW(driversw[nGame]->description), search_text) == NULL &&
		    MyStrStrI(driversw[nGame]->name, search_text) == NULL)
			return TRUE;
	}

 	// Filter games--return TRUE if the game should be HIDDEN in this view
	if( GetFilterInherit() )
	{
		if( lpFolder )
		{
			lpParent = GetFolder( lpFolder->m_nParent );
			if( lpParent )
			{
				/* Check the Parent Filters and inherit them on child,
				 * The inherited filters don't display on the custom Filter Dialog for the Child folder
				 * No need to promote all games to parent folder, works as is */
				dwMask |= lpParent->m_dwFlags;
			}
		}
	}

	if (wcslen(filter_text))
	{
		if (MyStrStrI(UseLangList() ? _LSTW(driversw[nGame]->description) : driversw[nGame]->description, filter_text) == NULL &&
		    MyStrStrI(driversw[nGame]->name, filter_text) == NULL &&
		    MyStrStrI(driversw[nGame]->source_file, filter_text) == NULL &&
		    MyStrStrI(UseLangList()? _MANUFACTW(driversw[nGame]->manufacturer) : driversw[nGame]->manufacturer, filter_text) == NULL)
			return TRUE;
	}
	// Are there filters set on this folder?
	if ((dwMask & F_MASK) == 0)
		return FALSE;

	// Filter out clones?
	if (dwMask & F_CLONES && DriverIsClone(nGame))
		return TRUE;

	for (i = 0; g_lpFilterList[i].m_dwFilterType; i++)
	{
		if (dwMask & g_lpFilterList[i].m_dwFilterType)
		{
			if (g_lpFilterList[i].m_pfnQuery(nGame) == g_lpFilterList[i].m_bExpectedResult)
				return TRUE;
		}
	}
	return FALSE;
}

/* Get the parent of game in this view */
BOOL GetParentFound(int nGame)
{
	int nParentIndex = -1;
	LPTREEFOLDER lpFolder = GetCurrentFolder();

	if( lpFolder )
	{
		nParentIndex = GetParentIndex(&driver_list::driver(nGame));

		/* return FALSE if no parent is there in this view */
		if( nParentIndex == -1)
			return FALSE;

		/* return FALSE if the folder should be HIDDEN in this view */
		if (TestBit(lpFolder->m_lpGameBits, nParentIndex) == 0)
			return FALSE;

		/* return FALSE if the game should be HIDDEN in this view */
		if (GameFiltered(nParentIndex, lpFolder->m_dwFlags))
			return FALSE;

		return TRUE;
	}

	return FALSE;
}

LPCFILTER_ITEM GetFilterList(void)
{
	return g_lpFilterList;
}

/***************************************************************************
	private functions
 ***************************************************************************/

void CreateNEOGEOFolders(int parent_index)
{
	int jj;
	int nGames = GetNumGames();
	LPTREEFOLDER lpFolder = treeFolders[parent_index];

	// no games in top level folder
	SetAllBits(lpFolder->m_lpGameBits,FALSE);

	for (jj = 0; jj < nGames; jj++)
	{
		const WCHAR *s = GetDriverFilename(jj);

		if (s == NULL || s[0] == '\0')
			continue;

		if (!wcscmp(TEXT("neogeo.c"), s) ||
			!wcscmp(TEXT("neogeo_noslot.c"), s))
			AddGame(lpFolder, jj);
	}
}

void CreateCPSFolders(int parent_index)
{
	int jj;
	int nGames = GetNumGames();
	LPTREEFOLDER lpFolder = treeFolders[parent_index];

	// no games in top level folder
	SetAllBits(lpFolder->m_lpGameBits,FALSE);

	for (jj = 0; jj < nGames; jj++)
	{
		const WCHAR *s = GetDriverFilename(jj);

		if (s == NULL || s[0] == '\0')
			continue;

		if (!wcscmp(TEXT("cps1.c"), s) ||
			!wcscmp(TEXT("cps2.c"), s) ||
			!wcscmp(TEXT("cps3.c"), s))
			AddGame(lpFolder, jj);
	}
}

void CreateNAMCOFolders(int parent_index)
{
	int jj;
	int nGames = GetNumGames();
	LPTREEFOLDER lpFolder = treeFolders[parent_index];

	// no games in top level folder
	SetAllBits(lpFolder->m_lpGameBits,FALSE);

	for (jj = 0; jj < nGames; jj++)
	{
		const WCHAR *s = GetDriverFilename(jj);

		if (s == NULL || s[0] == '\0')
			continue;

		if (!wcscmp(TEXT("namcos1.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("namcos2.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("namcos86.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("baraduke.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("galaga.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("digdug.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("gaplus.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("mappy.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("xevious.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("toypop.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("skykid.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("pacland.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("phozon.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("grobda.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("namcona1.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("namconb1.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("namcond1.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("tceptor.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("bosco.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("polepos.c"), s))
			AddGame(lpFolder, jj);
	}
}

void CreateTAITOFolders(int parent_index)
{
	int jj;
	int nGames = GetNumGames();
	LPTREEFOLDER lpFolder = treeFolders[parent_index];

	// no games in top level folder
	SetAllBits(lpFolder->m_lpGameBits,FALSE);

	for (jj = 0; jj < nGames; jj++)
	{
		const WCHAR *s = GetDriverFilename(jj);

		if (s == NULL || s[0] == '\0')
			continue;

		if (!wcscmp(TEXT("arkanoid.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("taito_f2.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("taito_f3.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("taito_z.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("taito_b.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("taito_l.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("taito_h.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("warriorb.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("ninjaw.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("darius.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("bublbobl.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("rainbow.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("tnzs.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("asuka.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("rastan.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("flstory.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("chaknpop.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("lkage.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("halleys.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("ashnojoe.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("slapshot.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("topspeed.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("opwolf.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("othunder.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("mexico86.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("volfied.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("lsasquad.c"), s))
			AddGame(lpFolder, jj);
	}
}

void CreateKONAMIFolders(int parent_index)
{
	int jj;
	int nGames = GetNumGames();
	LPTREEFOLDER lpFolder = treeFolders[parent_index];

	// no games in top level folder
	SetAllBits(lpFolder->m_lpGameBits,FALSE);

	for (jj = 0; jj < nGames; jj++)
	{
		const WCHAR *s = GetDriverFilename(jj);

		if (s == NULL || s[0] == '\0')
			continue;

		if (!wcscmp(TEXT("konamigx.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("mystwarr.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("xexex.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("tmnt.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("twin16.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("nemesis.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("gradius3.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("thunderx.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("ajax.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("flkatck.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("contra.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("parodius.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("hcastle.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("battlnts.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("combatsc.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("labyrunr.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("moo.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("vendetta.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("yiear.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("gbusters.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("xmen.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("chqflag.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("blockhl.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("gberet.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("gijoe.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("scotrsht.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("simpsons.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("ironhors.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("rockrage.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("mainevt.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("aliens.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("wecleman.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("jackal.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("hornet.c"), s))
			AddGame(lpFolder, jj);
	}
}

void CreateSEGAFolders(int parent_index)
{
	int jj;
	int nGames = GetNumGames();
	LPTREEFOLDER lpFolder = treeFolders[parent_index];

	// no games in top level folder
	SetAllBits(lpFolder->m_lpGameBits,FALSE);

	for (jj = 0; jj < nGames; jj++)
	{
		const WCHAR *s = GetDriverFilename(jj);

		if (s == NULL || s[0] == '\0')
			continue;

		if (!wcscmp(TEXT("system1.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("system16.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("segas16a.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("segas16b.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("segaorun.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("segahang.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("segas18.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("system24.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("segas32.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("segaxbd.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("segaybd.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("segac2.c"), s))
			AddGame(lpFolder, jj);
	}
}

void CreateTOAFolders(int parent_index)
{
	int jj;
	int nGames = GetNumGames();
	LPTREEFOLDER lpFolder = treeFolders[parent_index];

	// no games in top level folder
	SetAllBits(lpFolder->m_lpGameBits,FALSE);

	for (jj = 0; jj < nGames; jj++)
	{
		const WCHAR *s = GetDriverFilename(jj);

		if (s == NULL || s[0] == '\0')
			continue;

		if (!wcscmp(TEXT("toaplan1.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("toaplan2.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("twincobr.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("slapfght.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("snowbros.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("cave.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("macrossp.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("psikyo.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("psikyo4.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("seta.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("seta2.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("ssv.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("srmp2.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("raiden.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("suprnova.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("kaneko16.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("taito_x.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("inufuku.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("dcon.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("mjsister.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("ms32.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("fuukifg3.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("aerofgt.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("rabbit.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("powerins.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("psikyosh.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("tetrisp2.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("deco_mlc.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("deco32.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("seibuspi.c"), s))
			AddGame(lpFolder, jj);
	}
}

void CreateOTHERS1Folders(int parent_index)
{
	int jj;
	int nGames = GetNumGames();
	LPTREEFOLDER lpFolder = treeFolders[parent_index];

	// no games in top level folder
	SetAllBits(lpFolder->m_lpGameBits,FALSE);

	for (jj = 0; jj < nGames; jj++)
	{
		const WCHAR *s = GetDriverFilename(jj);

		if (s == NULL || s[0] == '\0')
			continue;

		if (!wcscmp(TEXT("airbustr.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("snk.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("hal21.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("1942.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("1943.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("bionicc.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("blktiger.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("tigeroad.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("lwings.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("gng.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("sonson.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("exedexes.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("commando.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("gunsmoke.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("sidearms.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("srumbler.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("bwing.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("dec8.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("kalnov.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("vaportra.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("darkseal.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("rohga.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("cninja.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("actfancr.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("gaiden.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("megasys1.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("psychic5.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("ginganin.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("terracre.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("armedf.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("galivan.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("argus.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("tecmo.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("bombjack.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("senjyo.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("omegaf.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("ninjakid.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("ninjakd2.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("m72.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("m92.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("equites.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("vigilant.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("nmk16.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("dassault.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("fcombat.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("vball.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("dec0.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("boogwing.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("brkthru.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("sf1.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("karnov.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("dynduke.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("dietgo.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("simpl156.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("mcatadv.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("m107.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("magmax.c"), s))
			AddGame(lpFolder, jj);
	}
}

void CreateOTHERS2Folders(int parent_index)
{
	int jj;
	int nGames = GetNumGames();
	LPTREEFOLDER lpFolder = treeFolders[parent_index];

	// no games in top level folder
	SetAllBits(lpFolder->m_lpGameBits,FALSE);

	for (jj = 0; jj < nGames; jj++)
	{
		const WCHAR *s = GetDriverFilename(jj);

		if (s == NULL || s[0] == '\0')
			continue;

		if (!wcscmp(TEXT("8080bw.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("n8080.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("galaxian.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("warpwarp.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("cclimber.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("geebee.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("rallyx.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("missile.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("nyny.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("phoenix.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("pacman.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("dkong.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("mrdo.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("docastle.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("scramble.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("spacefb.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("vicdual.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("rockola.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("crbaloon.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("naughtyb.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("taitosj.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("frogger.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("pooyan.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("timeplt.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("scobra.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("trackfld.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("hyperspt.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("rocnrope.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("pengo.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("segar.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("zaxxon.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("astrocde.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("ladybug.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("superpac.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("mario.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("decocass.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("mystston.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("champbas.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("seicross.c"), s))
			AddGame(lpFolder, jj);
	}
}

void CreateOTHERS3Folders(int parent_index)
{
	int jj;
	int nGames = GetNumGames();
	LPTREEFOLDER lpFolder = treeFolders[parent_index];

	// no games in top level folder
	SetAllBits(lpFolder->m_lpGameBits,FALSE);

	for (jj = 0; jj < nGames; jj++)
	{
		const WCHAR *s = GetDriverFilename(jj);

		if (s == NULL || s[0] == '\0')
			continue;

		if (!wcscmp(TEXT("gauntlet.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("atarisy1.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("atarisy2.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("artmagic.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("blockout.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("gaelco2.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("harddriv.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("pgm.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("atarigx2.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("rampart.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("midvunit.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("midwunit.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("midtunit.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("midyunit.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("skullxbo.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("afega.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("atarig1.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("klax.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("nmg5.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("1945kiii.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("dooyong.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("itech8.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("itech32.c"), s))
			AddGame(lpFolder, jj);
	}
}

void CreateOTHERS4Folders(int parent_index)
{
	int jj;
	int nGames = GetNumGames();
	LPTREEFOLDER lpFolder = treeFolders[parent_index];

	// no games in top level folder
	SetAllBits(lpFolder->m_lpGameBits,FALSE);

	for (jj = 0; jj < nGames; jj++)
	{
		const WCHAR *s = GetDriverFilename(jj);

		if (s == NULL || s[0] == '\0')
			continue;

		if (!wcscmp(TEXT("fromanc2.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("fromance.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("homedata.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("metro.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("mitchell.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("nbmj8688.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("nbmj8891.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("nbmj8991.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("nbmj9195.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("niyanpai.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("hanaroku.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("deniam.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("galpanic.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("gomoku.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("ojankohs.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("shangha3.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("shanghai.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("shisen.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("pipedrm.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("dynax.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("ddenlovr.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("funkyjet.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("realbrk.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("crospang.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("funybubl.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("st0016.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("paradise.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("m62.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("momoko.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("solomon.c"), s))
			AddGame(lpFolder, jj);
	}
}

void CreateOTHERS5Folders(int parent_index)
{
	int jj;
	int nGames = GetNumGames();
	LPTREEFOLDER lpFolder = treeFolders[parent_index];

	// no games in top level folder
	SetAllBits(lpFolder->m_lpGameBits,FALSE);

	for (jj = 0; jj < nGames; jj++)
	{
		const WCHAR *s = GetDriverFilename(jj);

		if (s == NULL || s[0] == '\0')
			continue;

		if (!wcscmp(TEXT("namcos11.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("namcos12.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("namcos21.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("namcos22.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("model1.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("stv.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("zn.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("konamigv.c"), s))
			AddGame(lpFolder, jj);
	}
}

void CreateOTHERS6Folders(int parent_index)
{
	int jj;
	int nGames = GetNumGames();
	LPTREEFOLDER lpFolder = treeFolders[parent_index];

	// no games in top level folder
	SetAllBits(lpFolder->m_lpGameBits,FALSE);

	for (jj = 0; jj < nGames; jj++)
	{
		const WCHAR *s = GetDriverFilename(jj);

		if (s == NULL || s[0] == '\0')
			continue;

		if (!wcscmp(TEXT("playch10.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("segasyse.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("megaplay.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("megatech.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("vsnes.c"), s))
			AddGame(lpFolder, jj);
		if (!wcscmp(TEXT("arcadia.c"), s))
			AddGame(lpFolder, jj);
	}
}

void CreateOTHERS7Folders(int parent_index)
{
	int jj;
	int nGames = GetNumGames();
	LPTREEFOLDER lpFolder = treeFolders[parent_index];

	// no games in top level folder
	SetAllBits(lpFolder->m_lpGameBits,FALSE);

	for (jj = 0; jj < nGames; jj++)
	{
		const WCHAR *s = GetDriverFilename(jj);

		if (s == NULL || s[0] == '\0')
			continue;

		if (!wcscmp(TEXT("unmamed.c"), s))
			AddGame(lpFolder, jj);
	}
}

void CreateSourceFolders(int parent_index)
{
	int i,jj;
	int nGames = GetNumGames();
	int start_folder = numFolders;
	LPTREEFOLDER lpFolder = treeFolders[parent_index];
	LPTREEFOLDER lpTemp;

	// no games in top level folder
	SetAllBits(lpFolder->m_lpGameBits,FALSE);
	for (jj = 0; jj < nGames; jj++)
	{
		const TCHAR *s = GetDriverFilename(jj);

		if (s == NULL || s[0] == '\0')
			continue;

		// look for an existant source treefolder for this game
		// (likely to be the previous one, so start at the end)
		for (i=numFolders-1;i>=start_folder;i--)
		{
			if (wcscmp(treeFolders[i]->m_lpTitle, s) == 0)
			{
				AddGame(treeFolders[i],jj);
				break;
			}
		}
		if (i == start_folder-1)
		{
			// nope, it's a source file we haven't seen before, make it.
			lpTemp = NewFolder(s, 0, FALSE, next_folder_id, parent_index, IDI_SOURCE);

			// Increment next_folder_id here in case code is added above
			next_folder_id++;

			AddFolder(lpTemp);
			AddGame(lpTemp,jj);
		}
	}
}

void GenerateScreenFoldersIni(const TCHAR *ini_name)
{
	int i, j;
	struct FOLDER_DATA *folder;

	InitExtraFolderData(16);

	for (i = 0; i < driver_list::total(); i++)
	{
		int screens = DriverNumScreens(i);
		TCHAR s[2];
		swprintf(s, TEXT("%d"), screens);

		// look for an existant screens treefolder for this game
		// (likely to be the previous one, so start at the end)
		folder = NULL;
		for (j = 0; j < extra_folder_count; j++)
		{
			if (!wcscmp(s, extra_folders[j].name))
			{
				folder = &extra_folders[j];
				break;
			}
		}

		if (folder == NULL)
		{
			// record that we found this folder
			extra_folders[extra_folder_count].name = win_tstring_strdup(s);

			folder = &extra_folders[extra_folder_count];

			extra_folder_count++;
		}
		dataAddGame(folder, i);
	}

	OutputIniAndFreeExtraFolderData(ini_name);
}

void CreateScreenFolders(int parent_index)
{
	const TCHAR *ini_name = TEXT("\\screen.ini");

	if (NeedToUpdateIni())
		GenerateScreenFoldersIni(ini_name);

	TCHAR filename[MAX_PATH];
	wcscpy(filename, GetUIDir());
	wcscat(filename, ini_name);
	LoadExternalFolders(parent_index, filename, IDI_FOLDER);
}

void CreateManufacturerFolders(int parent_index)
{
	int i,jj;
	int nGames = GetNumGames();
	int start_folder = numFolders;
	LPTREEFOLDER lpFolder = treeFolders[parent_index];
	LPTREEFOLDER lpTemp;

	// no games in top level folder
	SetAllBits(lpFolder->m_lpGameBits,FALSE);

	for (jj = 0; jj < nGames; jj++)
	{
		const TCHAR *manufacturer = driversw[jj]->manufacturer;
		int iChars = 0;
		while (manufacturer != NULL && manufacturer[0] != '\0')
		{
			const TCHAR *s = ParseManufacturer(manufacturer, &iChars);
			manufacturer += iChars;
			//shift to next start char
			if( s != NULL && *s != 0 )
 			{
				const TCHAR *t = TrimManufacturer(s);
				for (i = numFolders-1; i >= start_folder; i--)
				{
					//RS Made it case insensitive
					if (_tcsnicmp(GetFolderOrigName(treeFolders[i]), t, 20) == 0)
					{
						AddGame(treeFolders[i],jj);
						break;
					}
				}
				if (i == start_folder-1)
				{
					// nope, it's a manufacturer we haven't seen before, make it.
					lpTemp = NewFolder(t, wcscmp(s, TEXT("<unknown>")) ? UI_MSG_MANUFACTURE : 0, TRUE, next_folder_id, parent_index, IDI_MANUFACTURER);

					// Increment next_folder_id here in case code is added above
					next_folder_id++;

					AddFolder(lpTemp);
					AddGame(lpTemp,jj);
				}
			}
		}
	}
}

/* Make a reasonable name out of the one found in the driver array */
static const TCHAR *ParseManufacturer(const TCHAR *s, int *pParsedChars )
{
	static TCHAR tmp[256];
	TCHAR *ptmp;
	TCHAR *t;
	*pParsedChars= 0;

	if (*s == '?' || *s == '<' || s[3] == '?')
	{
		(*pParsedChars) = wcslen(s);
		return TEXT("<unknown>");
	}

	 ptmp = tmp;
	/*if first char is a space, skip it*/
	if (*s == ' ')
	{
		(*pParsedChars)++;
		++s;
	}
	while (*s)
	{
		/* combinations where to end string */

		if (
		    ((*s == ' ') && (s[1] == '(' || s[1] == '/' || s[1] == '+')) ||
		    (*s == ']') ||
		    (*s == '/') ||
		    (*s == '?')
		)
		{
			(*pParsedChars)++;
			if( s[1] == '/' || s[1] == '+' )
				(*pParsedChars)++;
			break;
		}
		if (s[0] == ' ' && s[1] == '?')
		{
			(*pParsedChars) += 2;
			s+=2;
		}

		/* skip over opening braces*/

		if (*s != '[')
        {
			*ptmp++ = *s;
	    }
		(*pParsedChars)++;
		/*for "distributed by" and "supported by" handling*/
		if (((s[1] == ',') && (s[2] == ' ') && ( (s[3] == 's') || (s[3] == 'd'))))
		{
			//*ptmp++ = *s;
			++s;
			break;
		}
	        ++s;
	}
	*ptmp = '\0';
	t = tmp;
	if (tmp[0] == '(' || tmp[wcslen(tmp)-1] == ')' || tmp[0] == ',')
	{
		ptmp = wcschr(tmp,'(');
		if (ptmp == NULL)
		{
			ptmp = wcschr(tmp,',');
			if (ptmp != NULL)
			{
				//parse the new "supported by" and "distributed by"
				ptmp++;

				if (_tcsnicmp(ptmp, TEXT(" supported by"), 13) == 0)
				{
					ptmp += 13;
				}
				else if (_tcsnicmp(ptmp, TEXT(" distributed by"), 15) == 0)
				{
					ptmp += 15;
				}
				else
				{
					return NULL;
				}
			}
			else
			{
				ptmp = tmp;
				if (ptmp == NULL)
				{
					return NULL;
				}
			}
		}
		if (tmp[0] == '(' || tmp[0] == ',')
		{
			ptmp++;
		}
		if (_tcsnicmp(ptmp, TEXT("licensed from "), 14) == 0)
		{
			ptmp += 14;
		}
		// for the licenced from case
		if (_tcsnicmp(ptmp, TEXT("licenced from "), 14) == 0)
		{
			ptmp += 14;
		}

		while ((*ptmp != ')' ) && (*ptmp != '/' ) && *ptmp != '\0')
		{
			if (*ptmp == ' ' && _tcsnicmp(ptmp, TEXT(" license"), 8) == 0)
			{
				break;
			}
			if (*ptmp == ' ' && _tcsnicmp(ptmp, TEXT(" licence"), 8) == 0)
			{
				break;
			}
			*t++ = *ptmp++;
		}

		*t = '\0';
	}

	*ptmp = '\0';
	return tmp;
}

/* Analyze Manufacturer Names for typical patterns, that don't distinguish between companies (e.g. Co., Ltd., Inc., etc. */
static const TCHAR *TrimManufacturer(const TCHAR *s)
{
	//Also remove Country specific suffixes (e.g. Japan, Italy, America, USA, ...)
	int i = 0;
	TCHAR strTemp[256];
	static TCHAR strTemp2[256];
	int j = 0;
	int k = 0;
	int l = 0;
	memset(strTemp, '\0', 256 );
	memset(strTemp2, '\0', 256 );
	//start analyzing from the back, as these are usually suffixes
	for (i = wcslen(s)-1; i >= 0; i--)
	{

		l = wcslen(strTemp);
		for (k=l; k>=0; k--)
			strTemp[k+1] = strTemp[k];
		strTemp[0] = s[i];
		strTemp[++l] = '\0';
		switch (l)
		{
			case 2:
				if (_tcsnicmp(strTemp, TEXT("co"), 2) == 0)
				{
					j = l;
					while (s[wcslen(s)-j-1] == ' ' || s[wcslen(s)-j-1] == ',')
					{
						j++;
					}
					if (j != l)
					{
						memset(strTemp2, '\0', sizeof strTemp2);
						wcsncpy(strTemp2, s, wcslen(s) - j);
					}
				}
				break;
			case 3:
				if (_tcsnicmp(strTemp, TEXT("co."), 3) == 0 || _tcsnicmp(strTemp, TEXT("ltd"), 3) == 0 || _tcsnicmp(strTemp, TEXT("inc"), 3) == 0 || _tcsnicmp(strTemp, TEXT("SRL"), 3) == 0 || _tcsnicmp(strTemp, TEXT("USA"), 3) == 0)
				{
					j = l;
					while (s[wcslen(s)-j-1] == ' ' || s[wcslen(s)-j-1] == ',')
					{
						j++;
					}
					if (j != l)
					{
						memset(strTemp2, '\0', sizeof strTemp2);
						wcsncpy(strTemp2, s, wcslen(s) - j);
					}
				}
				break;
			case 4:
				if (_tcsnicmp(strTemp, TEXT("inc."), 4) == 0 || _tcsnicmp(strTemp, TEXT("ltd."), 4) == 0 || _tcsnicmp(strTemp, TEXT("corp"), 4) == 0 || _tcsnicmp(strTemp, TEXT("game"), 4) == 0)
				{
					j = l;
					while (s[wcslen(s)-j-1] == ' ' || s[wcslen(s)-j-1] == ',')
					{
						j++;
					}
					if (j != l)
					{
						memset(strTemp2, '\0', sizeof strTemp2);
						wcsncpy(strTemp2, s, wcslen(s) - j);
					}
				}
				break;
			case 5:
				if (_tcsnicmp(strTemp, TEXT("corp."), 5) == 0 || _tcsnicmp(strTemp, TEXT("Games"), 5) == 0 || _tcsnicmp(strTemp, TEXT("Italy"), 5) == 0 || _tcsnicmp(strTemp, TEXT("Japan"), 5) == 0)
				{
					j = l;
					while (s[wcslen(s)-j-1] == ' ' || s[wcslen(s)-j-1] == ',')
					{
						j++;
					}
					if (j != l)
					{
						memset(strTemp2, '\0', sizeof strTemp2);
						wcsncpy(strTemp2, s, wcslen(s) - j);
					}
				}
				break;
			case 6:
				if (_tcsnicmp(strTemp, TEXT("co-ltd"), 6) == 0 || _tcsnicmp(strTemp, TEXT("S.R.L."), 6) == 0)
				{
					j = l;
					while (s[wcslen(s)-j-1] == ' ' || s[wcslen(s)-j-1] == ',')
					{
						j++;
					}
					if (j != l)
					{
						memset(strTemp2, '\0', sizeof strTemp2);
						wcsncpy(strTemp2, s, wcslen(s) - j);
					}
				}
				break;
			case 7:
				if (_tcsnicmp(strTemp, TEXT("co. ltd"), 7) == 0 || _tcsnicmp(strTemp, TEXT("America"), 7) == 0)
				{
					j = l;
					while (s[wcslen(s)-j-1] == ' ' || s[wcslen(s)-j-1] == ',')
					{
						j++;
					}
					if (j != l)
					{
						memset(strTemp2, '\0', sizeof strTemp2);
						wcsncpy(strTemp2, s, wcslen(s) - j);
					}
				}
				break;
			case 8:
				if (_tcsnicmp(strTemp, TEXT("co. ltd."), 8) == 0)
				{
					j = l;
					while (s[wcslen(s)-j-1] == ' ' || s[wcslen(s)-j-1] == ',')
					{
						j++;
					}
					if (j != l)
					{
						memset(strTemp2, '\0', sizeof strTemp2);
						wcsncpy(strTemp2, s, wcslen(s) - j);
					}
				}
				break;
			case 9:
				if (_tcsnicmp(strTemp, TEXT("co., ltd."), 9) == 0 || _tcsnicmp(strTemp, TEXT("gmbh & co"), 9) == 0)
				{
					j = l;
					while (s[wcslen(s)-j-1] == ' ' || s[wcslen(s)-j-1] == ',')
					{
						j++;
					}
					if (j != l)
					{
						memset(strTemp2, '\0', sizeof strTemp2);
						wcsncpy(strTemp2, s, wcslen(s) - j);
					}
				}
				break;
			case 10:
				if (_tcsnicmp(strTemp, TEXT("corp, ltd."), 10) == 0 || _tcsnicmp(strTemp, TEXT("industries"), 10) == 0 || _tcsnicmp(strTemp, TEXT("of America"), 10) == 0)
				{
					j = l;
					while (s[wcslen(s)-j-1] == ' ' || s[wcslen(s)-j-1] == ',')
					{
						j++;
					}
					if (j != l)
					{
						memset(strTemp2, '\0', sizeof strTemp2);
						wcsncpy(strTemp2, s, wcslen(s) - j);
					}
				}
				break;
			case 11:
				if (_tcsnicmp(strTemp, TEXT("corporation"), 11) == 0 || _tcsnicmp(strTemp, TEXT("enterprises"), 11) == 0)
				{
					j = l;
					while (s[wcslen(s)-j-1] == ' ' || s[wcslen(s)-j-1] == ',')
					{
						j++;
					}
					if (j != l)
					{
						memset(strTemp2, '\0', sizeof strTemp2);
						wcsncpy(strTemp2, s, wcslen(s) - j);
					}
				}
				break;
			case 16:
				if (_tcsnicmp(strTemp, TEXT("industries japan"), 16) == 0)
				{
					j = l;
					while (s[wcslen(s)-j-1] == ' ' || s[wcslen(s)-j-1] == ',')
					{
						j++;
					}
					if (j != l)
					{
						memset(strTemp2, '\0', sizeof strTemp2);
						wcsncpy(strTemp2, s, wcslen(s) - j);
					}
				}
				break;
			default:
				break;
		}
	}
	if( *strTemp2 == 0 )
		return s;
	return strTemp2;
}

static void GenerateCPUFoldersIni(const TCHAR *ini_name)
{
	int i, j;
	struct FOLDER_DATA *folder;

	InitExtraFolderData(512);

	for (i = 0; i < driver_list::total(); i++)
	{
		machine_config config(driver_list::driver(i), MameUIGlobal());

		// enumerate through all devices
		execute_interface_iterator iter(config.root_device());
		for (device_execute_interface *device = iter.first(); device != NULL; device = iter.next())
		{
			// get the name
			const TCHAR *dev_name = _Unicode(device->device().name());

			// do we have a folder for this device?
			folder = NULL;
			for (j = 0; j < extra_folder_count; j++)
			{
				if (!wcscmp(dev_name, extra_folders[j].name))
				{
					folder = &extra_folders[j];
					break;
				}
			}

			// are we forced to create a folder?
			if (folder == NULL)
			{
				// record that we found this folder
				extra_folders[extra_folder_count].name = win_tstring_strdup(_Unicode(device->device().name()));

				folder = &extra_folders[extra_folder_count];

				extra_folder_count++;
			}

			// cpu type #'s are one-based
			dataAddGame(folder, i);
		}
	}

	OutputIniAndFreeExtraFolderData(ini_name);
}

void CreateCPUFolders(int parent_index)
{
	const TCHAR *ini_name = TEXT("\\cpu.ini");

	if (NeedToUpdateIni())
		GenerateCPUFoldersIni(ini_name);

	TCHAR filename[MAX_PATH];
	wcscpy(filename, GetUIDir());
	wcscat(filename, ini_name);
	LoadExternalFolders(parent_index, filename, IDI_CPU);
}

void GenerateSoundFoldersIni(const TCHAR *ini_name)
{
	int i, j;
	struct FOLDER_DATA *folder;

	InitExtraFolderData(512);

	for (i = 0; i < driver_list::total(); i++)
	{
		machine_config config(driver_list::driver(i), MameUIGlobal());

		// enumerate through all devices

		sound_interface_iterator iter(config.root_device());
		for (device_sound_interface *device = iter.first(); device != NULL; device = iter.next())
		{
			// get the name
			const TCHAR *dev_name = _Unicode(device->device().name());

			// do we have a folder for this device?
			folder = NULL;
			for (j = 0; j < extra_folder_count; j++)
			{
				if (!wcscmp(dev_name, extra_folders[j].name))
				{
					folder = &extra_folders[j];
					break;
				}
			}

			// are we forced to create a folder?
			if (folder == NULL)
			{
				// record that we found this folder
				extra_folders[extra_folder_count].name = win_tstring_strdup(_Unicode(device->device().name()));

				folder = &extra_folders[extra_folder_count];

				extra_folder_count++;
			}

			// cpu type #'s are one-based
			dataAddGame(folder, i);
		}
	}

	OutputIniAndFreeExtraFolderData(ini_name);
}

void CreateSoundFolders(int parent_index)
{
	const TCHAR *ini_name = TEXT("\\sound.ini");

	if (NeedToUpdateIni())
		GenerateSoundFoldersIni(ini_name);

	TCHAR filename[MAX_PATH];
	wcscpy(filename, GetUIDir());
	wcscat(filename, ini_name);
	LoadExternalFolders(parent_index, filename, IDI_SND);

}

// mamep: updated mameui's horrible version
void CreateDeficiencyFolders(int parent_index)
{
	static UINT32 deficiency_flags[] =
	{
		GAME_UNEMULATED_PROTECTION,
		GAME_WRONG_COLORS,
		GAME_IMPERFECT_COLORS,
		GAME_IMPERFECT_GRAPHICS,
		GAME_NO_SOUND,
		GAME_IMPERFECT_SOUND,
		GAME_NO_COCKTAIL,
		GAME_REQUIRES_ARTWORK,
	};

#define NUM_FLAGS	ARRAY_LENGTH(deficiency_flags)

	static const TCHAR *deficiency_names[NUM_FLAGS] =
	{
		TEXT("Unemulated Protection"),
		TEXT("Wrong Colors"),
		TEXT("Imperfect Colors"),
		TEXT("Imperfect Graphics"),
		TEXT("Missing Sound"),
		TEXT("Imperfect Sound"),
		TEXT("No Cocktail"),
		TEXT("Requires Artwork"),
	};

	int i, j;
	int nGames = GetNumGames();
	int nFolder = numFolders;
	LPTREEFOLDER lpFolder = treeFolders[parent_index];
	LPTREEFOLDER map[NUM_FLAGS];
	LPTREEFOLDER lpTemp;

	// no games in top level folder
	SetAllBits(lpFolder->m_lpGameBits,FALSE);

	for (i = 0; i < NUM_FLAGS; i++)
	{
		lpTemp = NewFolder(deficiency_names[i], 0, TRUE, next_folder_id++, parent_index, IDI_FOLDER);
		AddFolder(lpTemp);
		map[i] = treeFolders[nFolder++];
	}

	for (i = 0; i < nGames; i++)
	{
		UINT32 flag = driver_list::driver(i).flags;

		for (j = 0; j < NUM_FLAGS; j++)
			if (flag & deficiency_flags[j])
				AddGame(map[j],i);
	}
}

void GenerateDumpingFoldersIni(const TCHAR *ini_name)
{
	int jj;
	BOOL bBadDump  = FALSE;
	BOOL bNoDump = FALSE;
	const rom_entry *region, *rom;
	const game_driver *gamedrv;
	struct FOLDER_DATA *folder_bad, *folder_no;

	InitExtraFolderData(2);

	// create our two subfolders
	extra_folders[extra_folder_count].name = win_tstring_strdup(TEXT("Bad Dump"));
	folder_bad = &extra_folders[extra_folder_count++];
	extra_folders[extra_folder_count].name = win_tstring_strdup(TEXT("No Dump"));
	folder_no = &extra_folders[extra_folder_count++];

	for (jj = 0; jj < driver_list::total(); jj++)
	{
		gamedrv = &driver_list::driver(jj);

		if (!gamedrv->rom)
			continue;
		bBadDump = FALSE;
		bNoDump = FALSE;
		/* Allocate machine config */
		machine_config config(*gamedrv, MameUIGlobal());

		device_iterator deviter(config.root_device());
		for (device_t *device = deviter.first(); device != NULL; device = deviter.next())
		{
			for (region = rom_first_region(*device); region; region = rom_next_region(region))
			{
				for (rom = rom_first_file(region); rom; rom = rom_next_file(rom))
				{
					if (ROMREGION_ISROMDATA(region) || ROMREGION_ISDISKDATA(region) )
					{
						//name = ROM_GETNAME(rom);
						hash_collection hashes(ROM_GETHASHDATA(rom));
						if (hashes.flag(hash_collection::FLAG_BAD_DUMP))
							bBadDump = TRUE;
						if (hashes.flag(hash_collection::FLAG_NO_DUMP))
							bNoDump = TRUE;
					}
				}
			}
		}
		if (bBadDump)
		{
			dataAddGame(folder_bad, jj);
		}
		if (bNoDump)
		{
			dataAddGame(folder_no, jj);
		}
	}

	OutputIniAndFreeExtraFolderData(ini_name);
}

void CreateDumpingFolders(int parent_index)
{
	const TCHAR *ini_name = TEXT("\\dumping.ini");

	if (NeedToUpdateIni())
		GenerateDumpingFoldersIni(ini_name);

	TCHAR filename[MAX_PATH];
	wcscpy(filename, GetUIDir());
	wcscat(filename, ini_name);
	LoadExternalFolders(parent_index, filename, IDI_FOLDER);
}

void CreateYearFolders(int parent_index)
{
	int i,jj;
	int nGames = GetNumGames();
	int start_folder = numFolders;
	LPTREEFOLDER lpFolder = treeFolders[parent_index];
	LPTREEFOLDER lpTemp;

	// no games in top level folder
	SetAllBits(lpFolder->m_lpGameBits,FALSE);

	for (jj = 0; jj < nGames; jj++)
	{
		TCHAR s[100];
		_tcscpy(s,driversw[jj]->year);

		if (s[0] == '\0')
			continue;

		// look for an extant year treefolder for this game
		// (likely to be the previous one, so start at the end)
		for (i=numFolders-1;i>=start_folder;i--)
		{
			if (_tcsncmp(treeFolders[i]->m_lpTitle,s,5) == 0)
			{
				AddGame(treeFolders[i],jj);
				break;
			}
		}
		if (i == start_folder-1)
		{
			// nope, it's a year we haven't seen before, make it.
			lpTemp = NewFolder(s, 0, !wcscmp(s, TEXT("<unknown>")), next_folder_id, parent_index, IDI_YEAR);

			// Increment next_folder_id here in case code is added above
			next_folder_id++;

			AddFolder(lpTemp);
			AddGame(lpTemp,jj);
		}
	}
}

void GenerateBIOSFoldersIni(const TCHAR *ini_name)
{
	int i,jj;
	int nGames = GetNumGames();
	const game_driver *drv;
	int nParentIndex = -1;

	InitExtraFolderData(512);

	for (jj = 0; jj < nGames; jj++)
	{
		if ( DriverIsClone(jj) )
		{
			nParentIndex = GetParentIndex(&driver_list::driver(jj));
			if (nParentIndex < 0) return;
			drv = &driver_list::driver(nParentIndex);
		}
		else
			drv = &driver_list::driver(jj);
		nParentIndex = GetParentIndex(drv);

		if (nParentIndex < 0 || !driver_list::driver(nParentIndex).description)
			continue;

		for (i = 0; i < extra_folder_count; i++)
		{
			if (wcscmp(extra_folders[i].name, driversw[nParentIndex]->description) == 0)
			{
				dataAddGame(&extra_folders[i], jj);
				break;
			}
		}

		if (i >= extra_folder_count)
		{
			// record that we found this folder
			extra_folders[extra_folder_count].name = win_tstring_strdup(driversw[nParentIndex]->description);
			dataAddGame(&extra_folders[extra_folder_count], jj);

			extra_folder_count++;
		}
	}

	OutputIniAndFreeExtraFolderData(ini_name);
}

void CreateBIOSFolders(int parent_index)
{
	const TCHAR *ini_name = TEXT("\\bios.ini");

	if (NeedToUpdateIni())
		GenerateBIOSFoldersIni(ini_name);

	TCHAR filename[MAX_PATH];
	wcscpy(filename, GetUIDir());
	wcscat(filename, ini_name);
	LoadExternalFolders(parent_index, filename, IDI_BIOS);
}

void GenerateResolutionFoldersIni(const TCHAR *ini_name)
{
	int i,jj;
	TCHAR Resolution[20];
	struct FOLDER_DATA *folder, *folder_V, *folder_H;

	InitExtraFolderData(512);

	// create our two subfolders
	extra_folders[extra_folder_count].name = win_tstring_strdup(TEXT("Vector (V)"));
	folder_V = &extra_folders[extra_folder_count++];
	extra_folders[extra_folder_count].name = win_tstring_strdup(TEXT("Vector (H)"));
	folder_H = &extra_folders[extra_folder_count++];

	for (jj = 0; jj < driver_list::total(); jj++)
	{
		machine_config config(driver_list::driver(jj), MameUIGlobal());
		const screen_device *screen;
		screen = config.first_screen();
		if (screen != NULL)
		{
			const rectangle &visarea = screen->visible_area();

			if (DriverIsVector(jj))
			{
				if (driver_list::driver(jj).flags & ORIENTATION_SWAP_XY)
				{
					dataAddGame(folder_V, jj);
				}
				else
				{
					dataAddGame(folder_H, jj);
				}
			}
			else
			if (driver_list::driver(jj).flags & ORIENTATION_SWAP_XY)
			{
				swprintf(Resolution, TEXT("%dx%d (V)"),
					visarea.max_y - visarea.min_y + 1,
					visarea.max_x - visarea.min_x + 1);
			}
			else
			{
				swprintf(Resolution, TEXT("%dx%d (H)"),
					visarea.max_x - visarea.min_x + 1,
					visarea.max_y - visarea.min_y + 1);
			}

			folder = NULL;
			for (i = 0; i < extra_folder_count; i++)
			{
				if (wcscmp(extra_folders[i].name, Resolution) == 0)
				{
					folder = &extra_folders[i];
					break;
				}
			}
			if (folder == NULL)
			{
				extra_folders[extra_folder_count].name = win_tstring_strdup(Resolution);
				folder = &extra_folders[extra_folder_count];
				extra_folder_count++;
			}
			dataAddGame(folder, jj);
		}
	}

	OutputIniAndFreeExtraFolderData(ini_name);
}

void CreateResolutionFolders(int parent_index)
{
	const TCHAR *ini_name = TEXT("\\resolution.ini");

	if (NeedToUpdateIni())
		GenerateResolutionFoldersIni(ini_name);

	TCHAR filename[MAX_PATH];
	wcscpy(filename, GetUIDir());
	wcscat(filename, ini_name);
	LoadExternalFolders(parent_index, filename, IDI_FOLDER);
}

void GenerateFPSFoldersIni(const TCHAR *ini_name)
{
	int i,jj;
	float fps[256];

	InitExtraFolderData(256);

	for (i = 0; i < driver_list::total(); i++)
	{
		float f;
		machine_config config(driver_list::driver(i), MameUIGlobal());
		const screen_device *screen;
		screen = config.first_screen();
		if (screen != NULL)
		{
			f = ATTOSECONDS_TO_HZ(screen->refresh_attoseconds());

			for (jj = 0; jj < extra_folder_count; jj++)
				if (fps[jj] == f)
					break;

			if (extra_folder_count == jj)
			{
				TCHAR buf[50];

				assert(extra_folder_count + 1 < ARRAY_LENGTH(fps));

				swprintf(buf, TEXT("%f Hz"), f);

				// record that we found this folder
				extra_folders[extra_folder_count].name = win_tstring_strdup(buf);
				fps[extra_folder_count] = f;

				extra_folder_count++;
			}

			dataAddGame(&extra_folders[jj], i);
		}
	}

	OutputIniAndFreeExtraFolderData(ini_name);
}

void CreateFPSFolders(int parent_index)
{
	const TCHAR *ini_name = TEXT("\\fps.ini");

	if (NeedToUpdateIni())
		GenerateFPSFoldersIni(ini_name);

	TCHAR filename[MAX_PATH];
	wcscpy(filename, GetUIDir());
	wcscat(filename, ini_name);
	LoadExternalFolders(parent_index, filename, IDI_FOLDER);
}

#ifdef USE_MORE_FOLDER_INFO
void CreateSaveStateFolders(int parent_index)
{
	int jj;
	int nGames = GetNumGames();
	LPTREEFOLDER lpFolder = treeFolders[parent_index];
	LPTREEFOLDER lpSupported, lpUnsupported;

	// create our two subfolders
	lpSupported = NewFolder(TEXT("Supported"), 0, TRUE, next_folder_id++, parent_index, IDI_FOLDER);
	lpUnsupported = NewFolder(TEXT("Unsupported"), 0, TRUE, next_folder_id++, parent_index, IDI_FOLDER);
	AddFolder(lpSupported);
	AddFolder(lpUnsupported);

	// no games in top level folder
	SetAllBits(lpFolder->m_lpGameBits,FALSE);

	for (jj = 0; jj < nGames; jj++)
	{
		if (driver_list::driver(jj).flags & GAME_SUPPORTS_SAVE)
		{
			AddGame(lpSupported,jj);
		}
		else
		{
			AddGame(lpUnsupported,jj);
		}
	}
}

void GenerateControlFoldersIni(const TCHAR *ini_name)
{
	enum {
		FOLDER_JOY2WAY, FOLDER_JOY4WAY, FOLDER_JOY8WAY, FOLDER_JOY16WAY,
		FOLDER_VJOY2WAY,
		FOLDER_DOUBLEJOY2WAY, FOLDER_DOUBLEJOY4WAY, FOLDER_DOUBLEJOY8WAY, FOLDER_DOUBLEJOY16WAY,
		FOLDER_VDOUBLEJOY2WAY,
		FOLDER_ADSTICK, FOLDER_PADDLE, FOLDER_DIAL, FOLDER_TRACKBALL, FOLDER_LIGHTGUN, FOLDER_PEDAL,
		FOLDER_PLAYER1, FOLDER_PLAYER2, FOLDER_PLAYER3, FOLDER_PLAYER4,
		FOLDER_PLAYER5, FOLDER_PLAYER6, FOLDER_PLAYER7, FOLDER_PLAYER8,
		FOLDER_BUTTON1, FOLDER_BUTTON2, FOLDER_BUTTON3, FOLDER_BUTTON4, FOLDER_BUTTON5,
		FOLDER_BUTTON6, FOLDER_BUTTON7, FOLDER_BUTTON8, FOLDER_BUTTON9, FOLDER_BUTTON10,
		FOLDER_MAX
	};

	static const TCHAR *ctrl_names[FOLDER_MAX] = {
		TEXT("Joy 2-Way"),
		TEXT("Joy 4-Way"),
		TEXT("Joy 8-Way"),
		TEXT("Joy 16-Way"),
		TEXT("Joy 2-Way (V)"),
		TEXT("Double Joy 2-Way"),
		TEXT("Double Joy 4-Way"),
		TEXT("Double Joy 8-Way"),
		TEXT("Double Joy 16-Way"),
		TEXT("Double Joy 2-Way (V)"),
		TEXT("AD Stick"),
		TEXT("Paddle"),
		TEXT("Dial"),
		TEXT("Trackball"),
		TEXT("Lightgun"),
		TEXT("Pedal"),
		TEXT("Player 1"),
		TEXT("Players 2"),
		TEXT("Players 3"),
		TEXT("Players 4"),
		TEXT("Players 5"),
		TEXT("Players 6"),
		TEXT("Players 7"),
		TEXT("Players 8"),
		TEXT("Button 1"),
		TEXT("Buttons 2"),
		TEXT("Buttons 3"),
		TEXT("Buttons 4"),
		TEXT("Buttons 5"),
		TEXT("Buttons 6"),
		TEXT("Buttons 7"),
		TEXT("Buttons 8"),
		TEXT("Buttons 9"),
		TEXT("Buttons 10")
	};

	InitExtraFolderData(FOLDER_MAX);

	int i;

	for (i = 0; i < FOLDER_MAX; i++)
		extra_folders[extra_folder_count++].name = win_tstring_strdup(ctrl_names[i]);

	for (i = 0; i < driver_list::total(); i++)
	{
		int p = DriverNumPlayers(i);
		int b = DriverNumButtons(i);
		int j;

		for (j = 0; j < CONTROLLER_MAX; j++)
			if (DriverUsesController(i, j))
				dataAddGame(&extra_folders[j], i);

		if (p)
			dataAddGame(&extra_folders[FOLDER_PLAYER1 + p - 1], i);
		if (b)
			dataAddGame(&extra_folders[FOLDER_BUTTON1 + b - 1], i);
	}

	OutputIniAndFreeExtraFolderData(ini_name);
}

void CreateControlFolders(int parent_index)
{
	const TCHAR *ini_name = TEXT("\\control.ini");

	if (NeedToUpdateIni())
		GenerateControlFoldersIni(ini_name);

	TCHAR filename[MAX_PATH];
	wcscpy(filename, GetUIDir());
	wcscat(filename, ini_name);
	LoadExternalFolders(parent_index, filename, IDI_FOLDER);
}
#endif /* USE_MORE_FOLDER_INFO */

static int compare_folder(const void *vp1, const void *vp2)
{
	const LPTREEFOLDER p1 = *(const LPTREEFOLDER *)vp1;
	const LPTREEFOLDER p2 = *(const LPTREEFOLDER *)vp2;
	const TCHAR *s1 = p1->m_lpTitle;
	const TCHAR *s2 = p2->m_lpTitle;

	while (*s1 && *s2)
	{
		if (iswdigit(*s1) && iswdigit(*s2))
		{
			int i1 = 0;
			int i2 = 0;

			while (iswdigit(*s1))
			{
				i1 *= 10;
				i1 += *s1++ - '0';
			}

			while (iswdigit(*s2))
			{
				i2 *= 10;
				i2 += *s2++ - '0';
			}

			if (i1 == i2)
				continue;

			return i1 - i2;
		}
		else if (*s1 != *s2)
			break;

		s1++;
		s2++;
	}

	return *s1 - *s2;
}

// creates child folders of all the top level folders, including custom ones
void CreateAllChildFolders(void)
{
	int num_top_level_folders = numFolders;
	int i, j;

	for (i = 0; i < num_top_level_folders; i++)
	{
		LPTREEFOLDER lpFolder = treeFolders[i];
		LPCFOLDERDATA lpFolderData = NULL;
		UINT start_folder = numFolders;

		for (j = 0; g_lpFolderData[j].m_lpTitle; j++)
		{
			if (g_lpFolderData[j].m_nFolderId == lpFolder->m_nFolderId)
			{
				lpFolderData = &g_lpFolderData[j];
				break;
			}
		}

		if (lpFolderData != NULL)
		{
			//dprintf("Found built-in-folder id %i %i\n",i,lpFolder->m_nFolderId);
			if (lpFolderData->m_pfnCreateFolders != NULL)
			{
				clock_t start = clock();
				lpFolderData->m_pfnCreateFolders(i);
				dwprintf(TEXT("[%s] : %.3f sec\n"), lpFolderData->m_lpTitle, (float)(clock() - start) / (float)CLOCKS_PER_SEC);
			}
		}
		else
		{
			if ((lpFolder->m_dwFlags & F_CUSTOM) == 0)
			{
				dprintf("Internal inconsistency with non-built-in folder, but not custom\n");
				continue;
			}

			//dprintf("Loading custom folder %i %i\n",i,lpFolder->m_nFolderId);

			// load the extra folder files, which also adds children
			if (TryAddExtraFolderAndChildren(i) == FALSE)
			{
				lpFolder->m_nFolderId = FOLDER_NONE;
			}
		}

		qsort(&treeFolders[start_folder], numFolders - start_folder, sizeof (treeFolders[0]), compare_folder);
	}
}

// adds these folders to the treeview
void ResetTreeViewFolders(void)
{
	HWND hTreeView = GetTreeView();
	int i;
	TVITEM tvi;
	TVINSERTSTRUCT	tvs;
	BOOL res;

	HTREEITEM shti; // for current child branches

	// currently "cached" parent
	HTREEITEM hti_parent = NULL;
	int index_parent = -1;

	res = TreeView_DeleteAllItems(hTreeView);

	//dprintf("Adding folders to tree ui indices %i to %i\n",start_index,end_index);

	tvs.hInsertAfter = TVI_LAST;

	for (i=0;i<numFolders;i++)
	{
		LPTREEFOLDER lpFolder = treeFolders[i];

		if (lpFolder->m_nParent == -1)
		{
			if (lpFolder->m_nFolderId < MAX_FOLDERS)
			{
				// it's a built in folder, let's see if we should show it
				if (GetShowFolder(lpFolder->m_nFolderId) == FALSE)
				{
					continue;
				}
			}

			tvi.mask	= TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
			tvs.hParent = TVI_ROOT;
			tvi.pszText = lpFolder->m_lpTitle;
			tvi.lParam	= (LPARAM)lpFolder;
			tvi.iImage	= GetTreeViewIconIndex(lpFolder->m_nIconId);
			tvi.iSelectedImage = 0;

#if !defined(NONAMELESSUNION)
			tvs.item = tvi;
#else
			tvs.DUMMYUNIONNAME.item = tvi;
#endif

			// Add root branch
			hti_parent = TreeView_InsertItem(hTreeView, &tvs);

			continue;
		}

		// not a top level branch, so look for parent
		if (treeFolders[i]->m_nParent != index_parent)
		{

			hti_parent = TreeView_GetRoot(hTreeView);
			while (1)
			{
				if (hti_parent == NULL)
				{
					// couldn't find parent folder, so it's a built-in but
					// not shown folder
					break;
				}

				tvi.hItem = hti_parent;
				tvi.mask = TVIF_PARAM;
				res= TreeView_GetItem(hTreeView,&tvi);
				if (((LPTREEFOLDER)tvi.lParam) == treeFolders[treeFolders[i]->m_nParent])
					break;

				hti_parent = TreeView_GetNextSibling(hTreeView,hti_parent);
			}

			// if parent is not shown, then don't show the child either obviously!
			if (hti_parent == NULL)
				continue;

			index_parent = treeFolders[i]->m_nParent;
		}

		tvi.mask	= TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
		tvs.hParent = hti_parent;
		tvi.iImage	= GetTreeViewIconIndex(treeFolders[i]->m_nIconId);
		tvi.iSelectedImage = 0;
		tvi.pszText = treeFolders[i]->m_lpTitle;
		tvi.lParam	= (LPARAM)treeFolders[i];

#if !defined(NONAMELESSUNION) /* bug in commctrl.h */
		tvs.item = tvi;
#else
		tvs.DUMMYUNIONNAME.item = tvi;
#endif
		// Add it to this tree branch
		shti = TreeView_InsertItem(hTreeView, &tvs);

	}
}

void SelectTreeViewFolder(int folder_id)
{
	HWND hTreeView = GetTreeView();
	HTREEITEM hti;
	TVITEM tvi;
	BOOL res;

	memset(&tvi,0,sizeof(tvi));

	hti = TreeView_GetRoot(hTreeView);

	while (hti != NULL)
	{
		HTREEITEM hti_next;

		tvi.hItem = hti;
		tvi.mask = TVIF_PARAM;
		res = TreeView_GetItem(hTreeView,&tvi);

		if (((LPTREEFOLDER)tvi.lParam)->m_nFolderId == folder_id)
		{
			res = TreeView_SelectItem(hTreeView,tvi.hItem);
			SetCurrentFolder((LPTREEFOLDER)tvi.lParam);
			return;
		}

		hti_next = TreeView_GetChild(hTreeView,hti);
		if (hti_next == NULL)
		{
			hti_next = TreeView_GetNextSibling(hTreeView,hti);
			if (hti_next == NULL)
			{
				hti_next = TreeView_GetParent(hTreeView,hti);
				if (hti_next != NULL)
					hti_next = TreeView_GetNextSibling(hTreeView,hti_next);
			}
		}
		hti = hti_next;
	}

	// could not find folder to select
	// make sure we select something
	tvi.hItem = TreeView_GetRoot(hTreeView);
	tvi.mask = TVIF_PARAM;
	res = TreeView_GetItem(hTreeView,&tvi);

	res = TreeView_SelectItem(hTreeView,tvi.hItem);
	SetCurrentFolder((LPTREEFOLDER)tvi.lParam);

}

/*
 * Does this folder have an INI associated with it?
 * Currently only TRUE for FOLDER_VECTOR and children
 * of FOLDER_SOURCE.
 */
static BOOL FolderHasIni(LPTREEFOLDER lpFolder) {
	if (FOLDER_VECTOR == lpFolder->m_nFolderId ||
		FOLDER_VERTICAL == lpFolder->m_nFolderId ||
		FOLDER_HORIZONTAL == lpFolder->m_nFolderId) {
		return TRUE;
	}
	if (lpFolder->m_nParent != -1
		&& FOLDER_SOURCE == treeFolders[lpFolder->m_nParent]->m_nFolderId) {
		return TRUE;
	}
	return FALSE;
}

/* Add a folder to the list.  Does not allocate */
static BOOL AddFolder(LPTREEFOLDER lpFolder)
{
	TREEFOLDER **tmpTree = NULL;
	UINT oldFolderArrayLength = folderArrayLength;
	if (numFolders + 1 >= folderArrayLength)
	{
		folderArrayLength += 500;
		tmpTree = (TREEFOLDER **)malloc(sizeof(TREEFOLDER **) * folderArrayLength);
		memcpy(tmpTree,treeFolders,sizeof(TREEFOLDER **) * oldFolderArrayLength);
		if (treeFolders) free(treeFolders);
		treeFolders = tmpTree;
	}

	/* Is there an folder.ini that can be edited? */
	if (FolderHasIni(lpFolder)) {
		lpFolder->m_dwFlags |= F_INIEDIT;
	}

	treeFolders[numFolders] = lpFolder;
	numFolders++;
	return TRUE;
}

/* Allocate and initialize a NEW TREEFOLDER */
static LPTREEFOLDER NewFolder(const TCHAR *lpTitle, UINT nCategoryID, BOOL bTranslate,
					   UINT nFolderId, int nParent, UINT nIconId)
{
	LPTREEFOLDER lpFolder = (LPTREEFOLDER)malloc(sizeof(TREEFOLDER));
	const char *title = _String(lpTitle);

	memset(lpFolder, '\0', sizeof (TREEFOLDER));

	if (nParent == -1)
	{
		int len = 1 + strlen(title) + 1;

		lpFolder->m_lpPath = (char *)osd_malloc(len * sizeof (*lpFolder->m_lpPath));
		snprintf(lpFolder->m_lpPath, len, "/%s", title);
	}
	else
	{
		int len = strlen(treeFolders[nParent]->m_lpPath) + 1 + strlen(title) + 1;

		lpFolder->m_lpPath = (char *)osd_malloc(len * sizeof (*lpFolder->m_lpPath));
		snprintf(lpFolder->m_lpPath, len, "%s/%s", treeFolders[nParent]->m_lpPath, title);
	}

	if (bTranslate)
	{
		if (!nCategoryID)
			nCategoryID = UI_MSG_UI;
		lpFolder->m_nCategoryID = nCategoryID;

		lpFolder->m_lpOriginalTitle = win_tstring_strdup(lpTitle);
		lpTitle = w_lang_message(nCategoryID, lpTitle);
	}
	lpFolder->m_lpTitle = win_tstring_strdup(lpTitle);
	lpFolder->m_lpGameBits = NewBits(GetNumGames());
	lpFolder->m_nFolderId = nFolderId;
	lpFolder->m_nParent = nParent;
	lpFolder->m_nIconId = nIconId;
	lpFolder->m_dwFlags = LoadFolderFlags(lpFolder->m_lpPath);

	return lpFolder;
}

/* Deallocate the passed in LPTREEFOLDER */
static void DeleteFolder(LPTREEFOLDER lpFolder)
{
	if (lpFolder)
	{
		if (lpFolder->m_lpGameBits)
		{
			DeleteBits(lpFolder->m_lpGameBits);
			lpFolder->m_lpGameBits = 0;
		}

		FreeIfAllocatedW(&lpFolder->m_lpTitle);
		FreeIfAllocatedW(&lpFolder->m_lpOriginalTitle);
		FreeIfAllocated(&lpFolder->m_lpPath);

		free(lpFolder);
		lpFolder = 0;
	}
}

/* Can be called to re-initialize the array of treeFolders */
BOOL InitFolders(void)
{
	int 			i = 0;
	BOOL            doCreateFavorite;
	LPCFOLDERDATA	fData = 0;
	if (treeFolders != NULL)
	{
		for (i = numFolders - 1; i >= 0; i--)
		{
			DeleteFolder(treeFolders[i]);
			treeFolders[i] = 0;
			numFolders--;
		}
	}
	numFolders = 0;
	if (folderArrayLength == 0)
	{
		folderArrayLength = 200;
		treeFolders = (TREEFOLDER **)malloc(sizeof(TREEFOLDER **) * folderArrayLength);
		if (!treeFolders)
		{
			folderArrayLength = 0;
			return 0;
		}
		else
		{
			memset(treeFolders,'\0', sizeof(TREEFOLDER **) * folderArrayLength);
		}
	}
	// built-in top level folders
	for (i = 0; g_lpFolderData[i].m_lpTitle; i++)
	{
		fData = &g_lpFolderData[i];

		/* create the folder */
		AddFolder(NewFolder(fData->m_lpTitle, 0, TRUE, fData->m_nFolderId, -1, fData->m_nIconId));
	}

	numExtraFolders = InitExtraFolders();
	extern const EXTFOLDER_TEMPLATE extFavorite;
	doCreateFavorite = TRUE;

	for (i = 0; i < numExtraFolders; i++)
	{
		if (_wcsicmp(ExtraFolderData[i]->m_szTitle, extFavorite.title) == 0)
			doCreateFavorite = FALSE;
	}

	dwprintf(TEXT("I %shave %s"), doCreateFavorite ? TEXT("don't ") : TEXT(""), extFavorite.title);
	if (doCreateFavorite)
	{
		int rooticon = 0;
		int subicon = 0;
		const TCHAR *title = extFavorite.title;
		TCHAR *filename;
		char *rootname = core_strdup(extFavorite.root_icon);
		char *subname = core_strdup(extFavorite.sub_icon);

		filename = (TCHAR *)malloc(wcslen(title) * sizeof (*filename) + sizeof (TEXT(".ini")));
		_tcscpy(filename, title);
		wcscat(filename, TEXT(".ini"));

		SetExtraIcons(rootname, &rooticon);
		SetExtraIcons(subname, &subicon);

		if (RegistExtraFolder(filename, &ExtraFolderData[numExtraFolders], UI_MSG_EXTRA + numExtraFolders, rooticon, subicon))
			numExtraFolders++;
		else
			doCreateFavorite = FALSE;

		osd_free(filename);
		osd_free(rootname);
		osd_free(subname);
	}

	for (i = 0; i < numExtraFolders; i++)
	{
		LPEXFOLDERDATA fExData = ExtraFolderData[i];
		LPTREEFOLDER   lpFolder;

		// create the folder
		//dprintf("creating top level custom folder with icon %i\n",fExData->m_nIconId);
		lpFolder = NewFolder(fExData->m_szTitle,UI_MSG_EXTRA + (fExData->m_nFolderId - MAX_FOLDERS),TRUE,fExData->m_nFolderId,fExData->m_nParent,
		                    fExData->m_nIconId);

		// OR in the saved folder flags
		lpFolder->m_dwFlags |= fExData->m_dwFlags;

		AddFolder(lpFolder);

		if (doCreateFavorite && i == numExtraFolders - 1)
		{
			if (TrySaveExtraFolder(lpFolder))
				dwprintf(TEXT("created: %s.ini"), fExData->m_szTitle);
		}
	}

	CreateAllChildFolders();
	CreateTreeIcons();
	ResetWhichGamesInFolders();
	ResetTreeViewFolders();
	SelectTreeViewFolder(GetSavedFolderID());
	return TRUE;
}

// create iconlist and Treeview control
static BOOL CreateTreeIcons()
{
	HICON	hIcon;
	INT 	i;
	HINSTANCE hInst = GetModuleHandle(0);

	int numIcons = ICON_MAX + numExtraIcons;
	hTreeSmall = ImageList_Create (16, 16, ILC_COLORDDB | ILC_MASK, numIcons, numIcons);

	//dprintf("Trying to load %i normal icons\n",ICON_MAX);
	for (i = 0; i < ICON_MAX; i++)
	{
		hIcon = LoadIconFromFile(treeIconNames[i].lpName);
		if (!hIcon)
			hIcon = LoadIcon(hInst, MAKEINTRESOURCE(treeIconNames[i].nResourceID));

		if (ImageList_AddIcon (hTreeSmall, hIcon) == -1)
		{
			ErrorMsg("Error creating icon on regular folder, %i %i",i,hIcon != NULL);
			return FALSE;
		}
	}

	//dprintf("Trying to load %i extra custom-folder icons\n",numExtraIcons);
	for (i = 0; i < numExtraIcons; i++)
	{
		if ((hIcon = LoadIconFromFile(ExtraFolderIcons[i])) == 0)
			hIcon = LoadIcon (hInst, MAKEINTRESOURCE(IDI_FOLDER));

		if (ImageList_AddIcon(hTreeSmall, hIcon) == -1)
		{
			ErrorMsg("Error creating icon on extra folder, %i %i",i,hIcon != NULL);
			return FALSE;
		}
	}

	// Be sure that all the small icons were added.
	if (ImageList_GetImageCount(hTreeSmall) < numIcons)
	{
		ErrorMsg("Error with icon list--too few images.  %i %i",
				ImageList_GetImageCount(hTreeSmall),numIcons);
		return FALSE;
	}

	// Be sure that all the small icons were added.

	if (ImageList_GetImageCount (hTreeSmall) < ICON_MAX)
	{
		ErrorMsg("Error with icon list--too few images.  %i < %i",
				 ImageList_GetImageCount(hTreeSmall),(INT)ICON_MAX);
		return FALSE;
	}

	// Associate the image lists with the list view control.
	(void)TreeView_SetImageList(GetTreeView(), hTreeSmall, TVSIL_NORMAL);

	return TRUE;
}


static void TreeCtrlOnPaint(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC 		hDC;
	RECT		rcClip, rcClient;
	HDC 		memDC;
	HBITMAP 	bitmap;
	HBITMAP 	hOldBitmap;

	HBITMAP hBackground = GetBackgroundBitmap();
	MYBITMAPINFO *bmDesc = GetBackgroundInfo();

	hDC = BeginPaint(hWnd, &ps);

	GetClipBox(hDC, &rcClip);
	GetClientRect(hWnd, &rcClient);

	// Create a compatible memory DC
	memDC = CreateCompatibleDC(hDC);

	// Select a compatible bitmap into the memory DC
	bitmap = CreateCompatibleBitmap(hDC, rcClient.right - rcClient.left,
									rcClient.bottom - rcClient.top);
	hOldBitmap = (HBITMAP)SelectObject(memDC, bitmap);

	// First let the control do its default drawing.
	CallWindowProc(g_lpTreeWndProc, hWnd, uMsg, (WPARAM)memDC, 0);

	// Draw bitmap in the background

	{
		HPALETTE hPAL;
		HDC maskDC;
		HBITMAP maskBitmap;
		HDC tempDC;
		HDC imageDC;
		HBITMAP bmpImage;
		HBITMAP hOldBmpImage;
		HBITMAP hOldMaskBitmap;
		HBITMAP hOldHBitmap;
		int i, j;
		RECT rcRoot;

		// Now create a mask
		maskDC = CreateCompatibleDC(hDC);

		// Create monochrome bitmap for the mask
		maskBitmap = CreateBitmap(rcClient.right - rcClient.left,
								  rcClient.bottom - rcClient.top,
								  1, 1, NULL);

		hOldMaskBitmap = (HBITMAP)SelectObject(maskDC, maskBitmap);
		SetBkColor(memDC, GetSysColor(COLOR_WINDOW));

		// Create the mask from the memory DC
		BitBlt(maskDC, 0, 0, rcClient.right - rcClient.left,
			   rcClient.bottom - rcClient.top, memDC,
			   rcClient.left, rcClient.top, SRCCOPY);

		tempDC = CreateCompatibleDC(hDC);
		hOldHBitmap = (HBITMAP)SelectObject(tempDC, hBackground);

		imageDC = CreateCompatibleDC(hDC);
		bmpImage = CreateCompatibleBitmap(hDC,
										  rcClient.right - rcClient.left,
										  rcClient.bottom - rcClient.top);
		hOldBmpImage = (HBITMAP)SelectObject(imageDC, bmpImage);

		hPAL = GetBackgroundPalette();
		if (hPAL == NULL)
			hPAL = CreateHalftonePalette(hDC);

		if (GetDeviceCaps(hDC, RASTERCAPS) & RC_PALETTE && hPAL != NULL)
		{
			SelectPalette(hDC, hPAL, FALSE);
			RealizePalette(hDC);
			SelectPalette(imageDC, hPAL, FALSE);
		}

		// Get x and y offset
		TreeView_GetItemRect(hWnd, TreeView_GetRoot(hWnd), &rcRoot, FALSE);
		rcRoot.left = -GetScrollPos(hWnd, SB_HORZ);

		// Draw bitmap in tiled manner to imageDC
		for (i = rcRoot.left; i < rcClient.right; i += bmDesc->bmWidth)
			for (j = rcRoot.top; j < rcClient.bottom; j += bmDesc->bmHeight)
				BitBlt(imageDC,  i, j, bmDesc->bmWidth, bmDesc->bmHeight,
					   tempDC, 0, 0, SRCCOPY);

		// Set the background in memDC to black. Using SRCPAINT with black and any other
		// color results in the other color, thus making black the transparent color
		SetBkColor(memDC, RGB(0,0,0));
		SetTextColor(memDC, RGB(255,255,255));
		BitBlt(memDC, rcClip.left, rcClip.top, rcClip.right - rcClip.left,
			   rcClip.bottom - rcClip.top,
			   maskDC, rcClip.left, rcClip.top, SRCAND);

		// Set the foreground to black. See comment above.
		SetBkColor(imageDC, RGB(255,255,255));
		SetTextColor(imageDC, RGB(0,0,0));
		BitBlt(imageDC, rcClip.left, rcClip.top, rcClip.right - rcClip.left,
			   rcClip.bottom - rcClip.top,
			   maskDC, rcClip.left, rcClip.top, SRCAND);

		// Combine the foreground with the background
		BitBlt(imageDC, rcClip.left, rcClip.top, rcClip.right - rcClip.left,
			   rcClip.bottom - rcClip.top,
			   memDC, rcClip.left, rcClip.top, SRCPAINT);

		// Draw the final image to the screen
		BitBlt(hDC, rcClip.left, rcClip.top, rcClip.right - rcClip.left,
			   rcClip.bottom - rcClip.top,
			   imageDC, rcClip.left, rcClip.top, SRCCOPY);

		SelectObject(maskDC, hOldMaskBitmap);
		SelectObject(tempDC, hOldHBitmap);
		SelectObject(imageDC, hOldBmpImage);

		DeleteDC(maskDC);
		DeleteDC(imageDC);
		DeleteDC(tempDC);
		DeleteBitmap(bmpImage);
		DeleteBitmap(maskBitmap);

		if (GetBackgroundPalette() == NULL)
		{
			DeletePalette(hPAL);
			hPAL = NULL;
		}
	}

	SelectObject(memDC, hOldBitmap);
	DeleteBitmap(bitmap);
	DeleteDC(memDC);
	EndPaint(hWnd, &ps);
	ReleaseDC(hWnd, hDC);
}

/* Header code - Directional Arrows */
static LRESULT CALLBACK TreeWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (GetBackgroundBitmap() != NULL)
	{
		switch (uMsg)
		{
	    case WM_MOUSEMOVE:
		{
			if (MouseHasBeenMoved())
				ShowCursor(TRUE);
			break;
		}

		case WM_KEYDOWN :
			if (wParam == VK_F2)
			{
				if (lpCurrentFolder->m_dwFlags & F_CUSTOM)
				{
					TreeView_EditLabel(hWnd,TreeView_GetSelection(hWnd));
					return TRUE;
				}
			}
			break;

		case WM_ERASEBKGND:
			return TRUE;

		case WM_PAINT:
			TreeCtrlOnPaint(hWnd, uMsg, wParam, lParam);
			UpdateWindow(hWnd);
			break;
		}
	}

	/* message not handled */
	return CallWindowProc(g_lpTreeWndProc, hWnd, uMsg, wParam, lParam);
}

/*
 * Filter code - should be moved to filter.c/filter.h
 * Added 01/09/99 - MSH <mhaaland@hypertech.com>
 */

/* find a FOLDERDATA by folderID */
LPCFOLDERDATA FindFilter(DWORD folderID)
{
	int i;

	for (i = 0; g_lpFolderData[i].m_lpTitle; i++)
	{
		if (g_lpFolderData[i].m_nFolderId == folderID)
			return &g_lpFolderData[i];
	}
	return (LPFOLDERDATA) 0;
}

/***************************************************************************
    private structures
 ***************************************************************************/

/***************************************************************************
    private functions prototypes
 ***************************************************************************/

/***************************************************************************
    private functions
 ***************************************************************************/

/**************************************************************************/

static const TCHAR *GetFolderOrigName(LPTREEFOLDER lpFolder)
{
	if (lpFolder->m_lpOriginalTitle)
		return lpFolder->m_lpOriginalTitle;

	return lpFolder->m_lpTitle;
}

LPTREEFOLDER GetFolderByName(int nParentId, const TCHAR *pszFolderName)
{
	int i = 0, nParent;

	//First Get the Parent TreeviewItem
	//Enumerate Children
	for(i = 0; i < numFolders/* ||treeFolders[i] != NULL*/; i++)
	{
		if (!wcscmp(GetFolderOrigName(treeFolders[i]), pszFolderName))
		{
			nParent = treeFolders[i]->m_nParent;
			if ((nParent >= 0) && treeFolders[nParent]->m_nFolderId == nParentId)
				return treeFolders[i];
		}
	}
	return NULL;
}

#ifdef KAILLERA
LPTREEFOLDER GetFavoritesFolderID(void)
{
	int i = 0;

	//First Get the Parent TreeviewItem
	//Enumerate Children
	for(i = 0; i < numFolders/* ||treeFolders[i] != NULL*/; i++)
	{
		if (!wcscmp(GetFolderOrigName(treeFolders[i]), TEXT("Favorites")))
		{
			return treeFolders[i];
		}
	}
	return NULL;
}
#endif /* KAILLERA */

static BOOL RegistExtraFolder(const TCHAR *name, LPEXFOLDERDATA *fExData, int msgcat, int icon, int subicon)
{
	TCHAR *ext = (TCHAR*) wcsrchr(name, '.');

	if (ext && !wcsicmp(ext, TEXT(".ini")))
	{
		*fExData = (EXFOLDERDATA *)malloc(sizeof(EXFOLDERDATA));
		if (*fExData)
		{
			char *utf8title;
			int i;

			*ext = '\0';

			utf8title = utf8_from_wstring(name);

			// drop DBCS filename
			for (i = 0; utf8title[i]; i++)
				if (utf8title[i] & 0x80)
				{
					free(*fExData);
					osd_free(utf8title);
					*fExData = NULL;
					return FALSE;
				}

			assign_msg_catategory(msgcat, utf8title);
			osd_free(utf8title);

			memset(*fExData, 0, sizeof(EXFOLDERDATA));

			wcsncpy((*fExData)->m_szTitle, name, 63);

			(*fExData)->m_nFolderId   = next_folder_id++;
			(*fExData)->m_nParent	  = -1;
			(*fExData)->m_dwFlags	  = F_CUSTOM;
			(*fExData)->m_nIconId	  = icon ? -icon : IDI_FOLDER;
			(*fExData)->m_nSubIconId  = subicon ? -subicon : IDI_FOLDER;

			return TRUE;
		}
	}

	return FALSE;
}

static int InitExtraFolders(void)
{
	WIN32_FIND_DATAW    ffd;
	HANDLE              hFile;
	int                 count = 0;
	TCHAR               path[MAX_PATH];
	const TCHAR        *dir = GetFolderDir();
	int                 done = FALSE;

	memset(ExtraFolderData, 0, MAX_EXTRA_FOLDERS * sizeof(LPEXFOLDERDATA));

	CreateDirectoryW(dir, NULL);

	_tcscpy(path, dir);
	wcscat(path, TEXT("\\*"));
	hFile = FindFirstFileW(path, &ffd);
	if (hFile == INVALID_HANDLE_VALUE)
		done = TRUE;

	memset(ExtraFolderIcons, 0, sizeof ExtraFolderIcons);
	numExtraIcons = 0;

	while (!done)
	{
		int rooticon, subicon;
		FILE *fp;
		char buf[256];

		if ((ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
			goto skip_it;

		_tcscpy(path, dir);
		wcscat(path, TEXT("\\"));
		wcscat(path, ffd.cFileName);
		if ((fp = wfopen(path, TEXT("r"))) == NULL)
			goto skip_it;

		rooticon = 0;
		subicon = 0;

		while (fgets(buf, 256, fp))
		{
			char *p;

			if (buf[0] != '[')
				continue;

			p = strchr(buf, ']');
			if (p == NULL)
				continue;

			*p = '\0';
			if (!strcmp(&buf[1], "FOLDER_SETTINGS"))
			{
				while (fgets(buf, 256, fp))
				{
					char *name = strtok(buf, " =\r\n");
					if (name == NULL)
						break;

					if (!strcmp(name, "RootFolderIcon"))
					{
						name = strtok(NULL, " =\r\n");
						if (name != NULL)
							SetExtraIcons(name, &rooticon);
					}
					if (!strcmp(name, "SubFolderIcon"))
					{
						name = strtok(NULL, " =\r\n");
						if (name != NULL)
							SetExtraIcons(name, &subicon);
					}
				}
				break;
			}
		}
		fclose(fp);

		if (RegistExtraFolder(ffd.cFileName, &ExtraFolderData[count], UI_MSG_EXTRA + count, rooticon, subicon))
			count++;

skip_it:
		done = !FindNextFileW(hFile, &ffd);
	}

	if (hFile != INVALID_HANDLE_VALUE)
		FindClose(hFile);

	return count;
}

void FreeExtraFolders(void)
{
	int i;

	for (i = 0; i < numExtraFolders; i++)
	{
		if (ExtraFolderData[i])
		{
			free(ExtraFolderData[i]);
			ExtraFolderData[i] = NULL;
		}
	}

	for (i = 0; i < numExtraIcons; i++)
    {
		free(ExtraFolderIcons[i]);
    }

	numExtraIcons = 0;

}


static void SetExtraIcons(char *name, int *id)
{
	char *p = strchr(name, '.');

	if (p != NULL)
		*p = '\0';

	ExtraFolderIcons[numExtraIcons] = (char*)malloc(strlen(name) + 1);
	if (ExtraFolderIcons[numExtraIcons])
	{
		*id = ICON_MAX + numExtraIcons;
		strcpy(ExtraFolderIcons[numExtraIcons], name);
		numExtraIcons++;
	}
}


// Called to add child folders of the top level extra folders already created
BOOL TryAddExtraFolderAndChildren(int parent_index)
{
	FILE*   fp = NULL;
	TCHAR   fname[MAX_PATH];
	char    readbuf[256];
	char*   p;
	char*   name;
	int     id, current_id;
	LPTREEFOLDER lpTemp = NULL;
	LPTREEFOLDER lpFolder = treeFolders[parent_index];

	current_id = lpFolder->m_nFolderId;

	id = lpFolder->m_nFolderId - MAX_FOLDERS;

	/* "folder\title.ini" */

	swprintf(fname, TEXT("%s\\%s.ini"), GetFolderDir(), ExtraFolderData[id]->m_szTitle);

	fp = wfopen(fname, TEXT("r"));
	if (fp == NULL)
		return FALSE;

	while (fgets(readbuf, 256, fp))
	{
		/* do we have [...] ? */

		if (readbuf[0] == '[')
		{
			p = strchr(readbuf, ']');
			if (p == NULL)
			{
				continue;
			}

			*p = '\0';
			name = &readbuf[1];

			/* is it [FOLDER_SETTINGS]? */

			if (strcmp(name, "FOLDER_SETTINGS") == 0)
			{
				current_id = -1;
				continue;
			}
			else
			{
				/* drop DBCS folder */
				for (p = name; *p; p++)
					if (*p & 0x80)
						break;

				if (*p)
				{
					current_id = -1;
					continue;
				}

				/* it it [ROOT_FOLDER]? */

				if (!strcmp(name, "ROOT_FOLDER"))
				{
					current_id = lpFolder->m_nFolderId;
					lpTemp = lpFolder;

				}
				else
				{
					/* must be [folder name] */
					TCHAR *foldername = _Unicode(name);

					current_id = next_folder_id++;
					/* create a new folder with this name,
					   and the flags for this folder as read from the registry */
					lpTemp = NewFolder(foldername, UI_MSG_EXTRA + (ExtraFolderData[id]->m_nFolderId - MAX_FOLDERS),
					                   TRUE, current_id, parent_index, ExtraFolderData[id]->m_nSubIconId);

					lpTemp->m_dwFlags |= F_CUSTOM;

					AddFolder(lpTemp);
				}
			}
		}
		else if (current_id != -1)
		{
			/* string on a line by itself -- game name */

			name = strtok(readbuf, " \t\r\n");
			if (name == NULL)
			{
				current_id = -1;
				continue;
			}

			/* IMPORTANT: This assumes that all driver names are lowercase! */
			strlwr(name);

			if (lpTemp == NULL)
			{
				ErrorMsg("Error parsing %s: missing [folder name] or [ROOT_FOLDER]",
						 fname);
				current_id = lpFolder->m_nFolderId;
				lpTemp = lpFolder;
			}
			AddGame(lpTemp, GetGameNameIndex(name));
        }
    }

    if ( fp )
    {
        fclose( fp );
    }

    return TRUE;
}


void GetFolders(TREEFOLDER ***folders,int *num_folders)
{
	*folders = treeFolders;
	*num_folders = numFolders;
}

static BOOL TryRenameCustomFolderIni(LPTREEFOLDER lpFolder,const TCHAR *old_name,const TCHAR *new_name)
{
	TCHAR filename[MAX_PATH];
	TCHAR new_filename[MAX_PATH];
	LPTREEFOLDER lpParent = NULL;
	const TCHAR *ini_dirw = GetIniDir();

	if (lpFolder->m_nParent >= 0)
	{
		//it is a custom SubFolder
		lpParent = GetFolder( lpFolder->m_nParent );
		if( lpParent )
		{
			snwprintf(filename, ARRAY_LENGTH(filename),
				TEXT("%s\\%s\\%s.ini"), ini_dirw, lpParent->m_lpTitle, old_name);
			snwprintf(new_filename, ARRAY_LENGTH(new_filename),
				TEXT("%s\\%s\\%s.ini"), ini_dirw, lpParent->m_lpTitle, new_name);
			MoveFile(filename, new_filename);
		}
	}
	else
	{
		//Rename the File, if it exists
		snwprintf(filename, ARRAY_LENGTH(filename),
			TEXT("%s\\%s.ini"), ini_dirw, old_name);
		snwprintf(new_filename, ARRAY_LENGTH(new_filename),
			TEXT("%s\\%s.ini"), ini_dirw, new_name);
		MoveFile(filename, new_filename);

		//Rename the Directory, if it exists
		snwprintf(filename, ARRAY_LENGTH(filename),
			TEXT("%s\\%s"), ini_dirw, old_name);
		snwprintf(new_filename, ARRAY_LENGTH(new_filename),
			TEXT("%s\\%s"), ini_dirw, new_name);
		MoveFile(filename, new_filename);
	}
	return TRUE;
}

BOOL TryRenameCustomFolder(LPTREEFOLDER lpFolder,const TCHAR *new_name)
{
	BOOL retval;
	TCHAR filename[MAX_PATH];
	TCHAR new_filename[MAX_PATH];
	const TCHAR *folder_dirw = GetFolderDir();

	if (lpFolder->m_nParent >= 0)
	{
		// a child extra folder was renamed, so do the rename and save the parent

		// save old title
		TCHAR *old_title = lpFolder->m_lpTitle;

		// set new title
		lpFolder->m_lpTitle = win_tstring_strdup(new_name);

		// try to save
		if (TrySaveExtraFolder(lpFolder) == FALSE)
		{
			// failed, so free newly allocated title and restore old
			FreeIfAllocatedW(&lpFolder->m_lpTitle);
			lpFolder->m_lpTitle = old_title;
			return FALSE;
		}
		TryRenameCustomFolderIni(lpFolder, old_title, new_name);
		// successful, so free old title
		FreeIfAllocatedW(&old_title);
		return TRUE;
	}

	// a parent extra folder was renamed, so rename the file

	snwprintf(new_filename, ARRAY_LENGTH(new_filename),
		TEXT("%s\\%s.ini"), folder_dirw, new_name);
	snwprintf(filename, ARRAY_LENGTH(filename),
		TEXT("%s\\%s.ini"), folder_dirw, lpFolder->m_lpTitle);

	retval = MoveFile(filename, new_filename);

	if (retval)
	{
		TryRenameCustomFolderIni(lpFolder, lpFolder->m_lpTitle, new_name);
		FreeIfAllocatedW(&lpFolder->m_lpTitle);
		lpFolder->m_lpTitle = win_tstring_strdup(new_name);
	}
	else
	{
		TCHAR buf[500];

		snwprintf(buf, ARRAY_LENGTH(buf), _UIW(TEXT("Error while renaming custom file %s to %s")),
				 filename, new_filename);
		MessageBox(GetMainWindow(), buf, TEXT(MAMEUINAME), MB_OK | MB_ICONERROR);
	}
	return retval;
}

void AddToCustomFolder(LPTREEFOLDER lpFolder,int driver_index)
{
	if ((lpFolder->m_dwFlags & F_CUSTOM) == 0)
	{
	    MessageBox(GetMainWindow(),_UIW(TEXT("Unable to add game to non-custom folder")),
				   TEXT(MAMEUINAME),MB_OK | MB_ICONERROR);
		return;
	}

	if (TestBit(lpFolder->m_lpGameBits,driver_index) == 0)
	{
		AddGame(lpFolder,driver_index);
		if (TrySaveExtraFolder(lpFolder) == FALSE)
			RemoveGame(lpFolder,driver_index); // undo on error
	}
}

void RemoveFromCustomFolder(LPTREEFOLDER lpFolder,int driver_index)
{
    if ((lpFolder->m_dwFlags & F_CUSTOM) == 0)
	{
	    MessageBox(GetMainWindow(),_UIW(TEXT("Unable to remove game from non-custom folder")),
				   TEXT(MAMEUINAME),MB_OK | MB_ICONERROR);
		return;
	}

	if (TestBit(lpFolder->m_lpGameBits,driver_index) != 0)
	{
		RemoveGame(lpFolder,driver_index);
		if (TrySaveExtraFolder(lpFolder) == FALSE)
			AddGame(lpFolder,driver_index); // undo on error
	}
}

BOOL TrySaveExtraFolder(LPTREEFOLDER lpFolder)
{
	TCHAR fname[MAX_PATH];
	FILE *fp;
	BOOL error = FALSE;
    int i,j;

	LPTREEFOLDER root_folder = NULL;
	LPEXFOLDERDATA extra_folder = NULL;
	const TCHAR *folder_dirw = GetFolderDir();

	for (i=0;i<numExtraFolders;i++)
	{
	    if (ExtraFolderData[i]->m_nFolderId == lpFolder->m_nFolderId)
		{
		    root_folder = lpFolder;
		    extra_folder = ExtraFolderData[i];
			break;
		}

		if (lpFolder->m_nParent >= 0 &&
			ExtraFolderData[i]->m_nFolderId == treeFolders[lpFolder->m_nParent]->m_nFolderId)
		{
			root_folder = treeFolders[lpFolder->m_nParent];
			extra_folder = ExtraFolderData[i];
			break;
		}
	}

	if (extra_folder == NULL || root_folder == NULL)
	{
	   MessageBox(GetMainWindow(), _UIW(TEXT("Error finding custom file name to save")),	TEXT(MAMEUINAME), MB_OK | MB_ICONERROR);
	   return FALSE;
	}
	/* "folder\title.ini" */

	snwprintf(fname, ARRAY_LENGTH(fname),
		TEXT("%s\\%s.ini"), folder_dirw, extra_folder->m_szTitle);

    fp = wfopen(fname, TEXT("wt"));
    if (fp == NULL)
	   error = TRUE;
	else
	{
	   TREEFOLDER *folder_data;


	   fprintf(fp,"[FOLDER_SETTINGS]\n");
	   // negative values for icons means it's custom, so save 'em
	   if (extra_folder->m_nIconId < 0)
	   {
		   fprintf(fp, "RootFolderIcon %s\n",
				   ExtraFolderIcons[(-extra_folder->m_nIconId) - ICON_MAX]);
	   }
	   if (extra_folder->m_nSubIconId < 0)
	   {
		   fprintf(fp,"SubFolderIcon %s\n",
				   ExtraFolderIcons[(-extra_folder->m_nSubIconId) - ICON_MAX]);
	   }

	   /* need to loop over all our TREEFOLDERs--first the root one, then each child.
		  start with the root */

	   folder_data = root_folder;

	   fprintf(fp,"\n[ROOT_FOLDER]\n");

	   for (i=0;i<GetNumGames();i++)
	   {
		   int driver_index = GetIndexFromSortedIndex(i);
		   if (TestBit(folder_data->m_lpGameBits,driver_index))
		   {
			   fprintf(fp, "%s\n", driver_list::driver(driver_index).name);
		   }
	   }

	   /* look through the custom folders for ones with our root as parent */
	   for (j=0;j<numFolders;j++)
	   {
		   folder_data = treeFolders[j];

		   if (folder_data->m_nParent >= 0 &&
			   treeFolders[folder_data->m_nParent] == root_folder)
		   {
			   fprintf(fp,"\n[%s]\n", _String(folder_data->m_lpTitle));

			   for (i=0;i<GetNumGames();i++)
			   {
				   int driver_index = GetIndexFromSortedIndex(i);
				   if (TestBit(folder_data->m_lpGameBits,driver_index))
				   {
					   fprintf(fp, "%s\n", driver_list::driver(driver_index).name);
				   }
			   }
		   }
	   }
	   if (fclose(fp) != 0)
		   error = TRUE;
	}

	if (error)
	{
		TCHAR buf[500];
		snwprintf(buf, ARRAY_LENGTH(buf), _UIW(TEXT("Error while saving custom file %s")), fname);
		MessageBox(GetMainWindow(), buf, TEXT(MAMEUINAME), MB_OK | MB_ICONERROR);
	}
	return !error;
}

HIMAGELIST GetTreeViewIconList(void)
{
    return hTreeSmall;
}

int GetTreeViewIconIndex(int icon_id)
{
	int i;

	if (icon_id < 0)
		return -icon_id;

	for (i = 0; i < ARRAY_LENGTH(treeIconNames); i++)
	{
		if (icon_id == treeIconNames[i].nResourceID)
			return i;
	}

	return -1;
}

/* End of source file */
