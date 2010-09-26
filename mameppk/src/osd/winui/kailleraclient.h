
#ifndef KAILLERA_CLIENT_H
#define KAILLERA_CLIENT_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>


#define KAILLERA_CLIENT_API_VERSION "0.8"


  typedef struct {
    char *appName;
    char *gameList;

    int (WINAPI *gameCallback)(char *game, int player, int numplayers);

    void (WINAPI *chatReceivedCallback)(char *nick, char *text);
    void (WINAPI *clientDroppedCallback)(char *nick, int playernb);

    void (WINAPI *moreInfosCallback)(char *gamename);
  } kailleraInfos;

struct KAILLERA_IPC_GAMELIST
{
	char *filename;
	char *appname;
	char *gamelist;
	char *processdis;
	BOOL alignment;
	BOOL standby;
};

extern int (WINAPI *kailleraGetVersion) (char *);

extern int (WINAPI *kailleraSetInfos) (kailleraInfos*);
extern int (WINAPI *kailleraSelectServerDialog)(HWND);
extern int (WINAPI *kailleraModifyPlayValues)(void *, int);
extern int (WINAPI *kailleraChatSend)(char *);
extern int kailleraChatSendUTF8(char *);

extern int (WINAPI *kailleraInit)(void);
extern int (WINAPI *kailleraShutdown)(void);
extern int (WINAPI *kailleraEndGame)(void);

extern int (WINAPI *kailleraChatSend_cb)(char *);
extern int (WINAPI *kailleraChatSend_cb_update)(void);


BOOL LoadLibrary_KailleraClient_DLL(const TCHAR *fname);
void FreeLibrary_KailleraClient_DLL(void);
void KailleraClient_DLL_SaveFunc(void);
void KailleraClient_DLL_LoadFunc(void);

BOOL Kaillera_GetVersion(const TCHAR *fname, char *version);

#endif
