// ARM.h : PROJECT_NAME ���ε{�����D�n���Y��
//

#pragma once

/*#ifndef __AFXWIN_H__
	#error �b�� PCH �]�t���ɮ׫e���]�t 'stdafx.h'
#endif*/

#include "resource.h"		// �D�n�Ÿ�
#include "XUnzip.h"
#include "plugins_private.h"

// CARMApp:
// �аѾ\��@�����O�� ARM.cpp
//

class CARMApp : public CWinApp
{
public:
	CARMApp();

// �мg
	public:
	virtual BOOL InitInstance();
	bool m_exiting;

// �{���X��@

	DECLARE_MESSAGE_MAP()

};

extern CARMApp theApp;

extern TCHAR szFileINI[MAX_PATH];
extern TCHAR szSettingINI[MAX_PATH];

typedef struct _arfile_t {
	LPTSTR	fileName;
	LPTSTR	description;
	LPTSTR	previewFile;
	UINT	features;
	ULONG	size;
	bool	enabled;
	_arfile_t* next;
	_arfile_t* prev;
} arfile;

extern arfile* arFiles;

#define CARINFO	"Car Info"
#define ARCHAR_GEOMETRY	1<<0
#define ARCHAR_TUNINGS	1<<1
#define ARCHAR_CITY		1<<2
#define ARCHAR_TUNINGS2	1<<3
#define ARCHAR_TEXTURE	1<<4
#define ARCHAR_AUDIO	1<<5
#define ARCHAR_TRACKS	1<<6
#define ARCHAR_VEHICLE	1<<7

#define SECTION_GENERAL	"General"
#define SECTION_PLUGINS	"Plugins"

#define SETTING_MM2		"MM2Path"
#define SETTING_BACKUP	"BackupPath"
#define SETTING_ARGS	"MM2Arguments"
#define SETTING_YIKES	"RemoveYikes"
#define SETTING_MOVETOTRASH	"MoveToTrash"
#define SETTING_RUNMINIMIZE	"RunMinimize"
#define SETTING_NOTIFYCRASH	"NotifyCrash"

#define WM_CHANGESTATUS	WM_USER+1
