/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2008 Miranda ICQ/IM project,
all portions of this codebase are copyrighted to the people
listed in contributors.txt.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
#ifndef M_SKIN_H__
#define M_SKIN_H__ 1

//loads an icon from the user's custom skin library, or from the exe if there
//isn't one of them
//wParam=id of icon to load - see below
//lParam=0
//returns an hIcon for the new icon. Do *not* DestroyIcon() the return value
//returns NULL if id is invalid, but will always succeed for a valid id
#define MS_SKIN_LOADICON     "Skin/Icons/Load"
//nice function to wrap this:
__inline static HICON LoadSkinnedIcon(int id) {return (HICON)CallService(MS_SKIN_LOADICON,id,0);}
__inline static HANDLE LoadSkinnedIconHandle(int id) {return (HANDLE)CallService(MS_SKIN_LOADICON,id,1);}

//event icons
#define SKINICON_EVENT_MESSAGE      100
#define SKINICON_EVENT_URL          101
#define SKINICON_EVENT_FILE         102
//other icons
#define SKINICON_OTHER_MIRANDA      200
#define SKINICON_OTHER_EXIT         201
#define SKINICON_OTHER_SHOWHIDE     202
#define SKINICON_OTHER_GROUPOPEN    203     //v0.1.1.0+
#define SKINICON_OTHER_USERONLINE   204     //v0.1.0.1+
#define SKINICON_OTHER_GROUPSHUT    205     //v0.1.1.0+
#define SKINICON_OTHER_CONNECTING   206     //v0.1.0.1+
#define SKINICON_OTHER_ADDCONTACT   207     //v0.7.0.2+
#define SKINICON_OTHER_USERDETAILS  208     //v0.7.0.2+
#define SKINICON_OTHER_HISTORY      209     //v0.7.0.2+
#define SKINICON_OTHER_DOWNARROW    210     //v0.7.0.2+
#define SKINICON_OTHER_FINDUSER     211     //v0.7.0.2+
#define SKINICON_OTHER_OPTIONS      212     //v0.7.0.2+
#define SKINICON_OTHER_SENDEMAIL    213     //v0.7.0.2+
#define SKINICON_OTHER_DELETE       214     //v0.7.0.2+
#define SKINICON_OTHER_RENAME       215     //v0.7.0.2+
#define SKINICON_OTHER_SMS          216     //v0.7.0.2+
#define SKINICON_OTHER_SEARCHALL    217     //v0.7.0.2+
#define SKINICON_OTHER_TICK         218     //v0.7.0.2+
#define SKINICON_OTHER_NOTICK       219     //v0.7.0.2+
#define SKINICON_OTHER_HELP         220     //v0.7.0.2+
#define SKINICON_OTHER_MIRANDAWEB   221     //v0.7.0.2+
#define SKINICON_OTHER_TYPING       222     //v0.7.0.2+
#define SKINICON_OTHER_SMALLDOT     223     //v0.7.0.2+
#define SKINICON_OTHER_FILLEDBLOB   224     //v0.7.0.2+
#define SKINICON_OTHER_EMPTYBLOB    225     //v0.7.0.2+
#define SKINICON_OTHER_UNICODE      226     //v0.7.0.19+
#define SKINICON_OTHER_ANSI         227     //v0.7.0.19+
#define SKINICON_OTHER_LOADED       228     //v0.7.0.19+
#define SKINICON_OTHER_NOTLOADED    229     //v0.7.0.19+
#define SKINICON_OTHER_UNDO         230     //v0.8.0.4+
#define SKINICON_OTHER_WINDOW       231     //v0.8.0.4+
#define SKINICON_OTHER_WINDOWS      232     //v0.8.0.4+
#define SKINICON_OTHER_ACCMGR       233     //v0.8.0.4+
#define SKINICON_OTHER_MAINMENU     234     //v0.8.0.12+
#define SKINICON_OTHER_STATUS       235     //v0.8.0.12+

//menu icons are owned by the module that uses them so are not and should not
//be skinnable. Except exit and show/hide

//status mode icons. NOTE: These are deprecated in favour of LoadSkinnedProtoIcon()
#define SKINICON_STATUS_OFFLINE     0
#define SKINICON_STATUS_ONLINE      1
#define SKINICON_STATUS_AWAY        2
#define SKINICON_STATUS_NA          3
#define SKINICON_STATUS_OCCUPIED    4
#define SKINICON_STATUS_DND         5
#define SKINICON_STATUS_FREE4CHAT   6
#define SKINICON_STATUS_INVISIBLE   7
#define SKINICON_STATUS_ONTHEPHONE  8
#define SKINICON_STATUS_OUTTOLUNCH  9

//Loads an icon representing the status mode for a particular protocol.
//wParam=(WPARAM)(const char*)szProto
//lParam=status
//returns an hIcon for the new icon. Do *not* DestroyIcon() the return value
//returns NULL on failure
//if szProto is NULL the function will load the user's selected 'all protocols'
//status icon.
#define MS_SKIN_LOADPROTOICON     "Skin/Icons/LoadProto"
//nice function to wrap this:
__inline static HICON LoadSkinnedProtoIcon(const char *szProto,int status) {return (HICON)CallService(MS_SKIN_LOADPROTOICON,(WPARAM)szProto,status);}

//add a new sound so it has a default and can be changed in the options dialog
//wParam=0
//lParam=(LPARAM)(SKINSOUNDDESC*)ssd;
//returns 0 on success, nonzero otherwise
typedef struct {
	int cbSize;
	const char *pszName;		   //name to refer to sound when playing and in db
	const char *pszDescription;	   //description for options dialog
    const char *pszDefaultFile;    //default sound file to use
    const char *pszSection;        //section name used to group sounds (NULL is acceptable) (added during 0.3.4+ (2004/10/*))
} SKINSOUNDDESCEX;
// Old struct pre 0.3.4
typedef struct {
	int cbSize;
	const char *pszName;		   //name to refer to sound when playing and in db
	const char *pszDescription;	   //description for options dialog
	const char *pszDefaultFile;	   //default sound file to use
} SKINSOUNDDESC;
#define MS_SKIN_ADDNEWSOUND      "Skin/Sounds/AddNew"

// inline only works after 0.3.4+ (2004/10/*)
__inline static int SkinAddNewSoundEx(const char *name,const char *section,const char *description)
{
	SKINSOUNDDESCEX ssd;
	ZeroMemory(&ssd,sizeof(ssd));
	ssd.cbSize=sizeof(ssd);
	ssd.pszName=name;
	ssd.pszSection=section;
	ssd.pszDescription=description;
	return CallService(MS_SKIN_ADDNEWSOUND, 0, (LPARAM)&ssd);
}

__inline static int SkinAddNewSound(const char *name,const char *description,const char *defaultFile)
{
	SKINSOUNDDESC ssd;
	ZeroMemory(&ssd,sizeof(ssd));
	ssd.cbSize=sizeof(ssd);
	ssd.pszName=name;
	ssd.pszDescription=description;
	ssd.pszDefaultFile=defaultFile;
	return CallService(MS_SKIN_ADDNEWSOUND, 0, (LPARAM)&ssd);
}

//play a named sound event
//wParam=0
//lParam=(LPARAM)(const char*)pszName
//pszName should have been added with Skin/Sounds/AddNew, but if not the
//function will not fail, it will play the Windows default sound instead.
#define MS_SKIN_PLAYSOUND        "Skin/Sounds/Play"
__inline static int SkinPlaySound(const char *name) {return CallService(MS_SKIN_PLAYSOUND,0,(LPARAM)name);}

//sent when the icons DLL has been changed in the options dialog, and everyone
//should re-make their image lists
//wParam=lParam=0
#define ME_SKIN_ICONSCHANGED     "Skin/IconsChanged"


/*
	wParam: 0 when playing sound (1 when sound is being previewed)
	lParam: (char*) pszSoundFile
	Affect: This hook is fired when the sound module needs to play a sound
	Note  : This event has default processing, if no one HookEvent()'s this event then it will
			use the default hook code, which uses PlaySound()
	Version: 0.3.4a (2004/09/15)
*/
#define ME_SKIN_PLAYINGSOUND  "Skin/Sounds/Playing"

//random ideas for the future:
// Skin/LoadNetworkAnim - get some silly spinner thing when we want to be busy

#endif //M_SKIN_H__
