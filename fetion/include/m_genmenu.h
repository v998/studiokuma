#ifndef M_GENMENU_H
#define M_GENMENU_H

#ifndef M_CLIST_H__
   #include <m_clist.h>
#endif

/*
  Main features:
  1) Independet from clist,may be used in any module.
  2) Module defined Exec and Check services.
  3) Menu with any level of popups,icons for root of popup.
  4) You may use measure/draw/processcommand even if menuobject is unknown.

  Idea of GenMenu module consists of that,
  it must be independet and offers only general menu purpose services:
  MO_CREATENEWMENUOBJECT
  MO_REMOVEMENUOBJECT
  MO_ADDNEWMENUITEM
  MO_REMOVEMENUITEM
  ...etc

  And then each module that want use and offer to others menu handling
  must create own services.For example i rewrited mainmenu and
  contactmenu code in clistmenus.c.If you look at code all functions
  are very identical, and vary only in check/exec services.

  So template set of function will like this:
  Remove<NameMenu>Item
  Add<NameMenu>Item
  Build<NameMenu>
  <NameMenu>ExecService
  <NameMenu>CheckService

  ExecService and CheckService used as callbacks when GenMenu must
  processcommand for menu item or decide to show or not item.This make
  GenMenu independet of which params must passed to service when user
  click on menu,this decide each module.
						28-04-2003 Bethoven

*/



/*
Analog to CLISTMENUITEM,but invented two params root and ownerdata.
root is used for creating any level popup menus,set to -1 to build
at first level and root=MenuItemHandle to place items in submenu
of this item.Must be used two new flags CMIF_ROOTPOPUP and CMIF_CHILDPOPUP
(defined in m_clist.h)

ownerdata is passed to callback services(ExecService and CheckService)
when building menu or processed command.
*/

/*GENMENU_MODULE*/
/*
Changes:

28-04-2003
Moved all general stuff to genmenu.c(m_genmenu.h,genmenu.h),
so removed all frames stuff.


Changes:

28-12-2002

Contact menu item service called with wparam=hcontact,lparam=popupPosition -
plugin may add different menu items with some service.
(old behavior wparam=hcontact lparam=0)



25-11-2002		Full support of runtime build of all menus.
				Contact		MS_CLIST_ADDCONTACTMENUITEM
								MS_CLIST_REMOVECONTACTMENUITEM
								MS_CLIST_MENUBUILDCONTACT
								ME_CLIST_PREBUILDCONTACTMENU

				MainMenu		MS_CLIST_ADDMAINMENUITEM
								MS_CLIST_REMOVEMAINMENUITEM
								MS_CLIST_MENUBUILDMAIN
								ME_CLIST_PREBUILDMAINMENU

				FrameMenu	MS_CLIST_ADDCONTEXTFRAMEMENUITEM
								MS_CLIST_REMOVECONTEXTFRAMEMENUITEM
								MS_CLIST_MENUBUILDFRAMECONTEXT
								ME_CLIST_PREBUILDFRAMEMENU

				For All menus may be used
								MS_CLIST_MODIFYMENUITEM

				All menus supported any level of popups
				(pszPopupName=(char *)hMenuItem - for make child of popup)
*/

// SubGroup MENU
//remove a item from SubGroup menu
//wParam=hMenuItem returned by MS_CLIST_ADDSubGroupMENUITEM
//lParam=0
//returns 0 on success, nonzero on failure
#define MS_CLIST_REMOVESUBGROUPMENUITEM					"CList/RemoveSubGroupMenuItem"

//builds the SubGroup menu
//wParam=lParam=0
//returns a HMENU identifying the menu.
#define MS_CLIST_MENUBUILDSUBGROUP							"CList/MenuBuildSubGroup"

//add a new item to the SubGroup menus
//wParam=lpGroupMenuParam, params to call when exec menuitem
//lParam=(LPARAM)(CLISTMENUITEM*)&mi
#define MS_CLIST_ADDSUBGROUPMENUITEM						"CList/AddSubGroupMenuItem"

//the SubGroup menu is about to be built
//wParam=lParam=0
#define ME_CLIST_PREBUILDSUBGROUPMENU						"CList/PreBuildSubGroupMenu"

// SubGroup MENU

// Group MENU
typedef struct{
int wParam;
int lParam;
}GroupMenuParam,*lpGroupMenuParam;

//remove a item from Group menu
//wParam=hMenuItem returned by MS_CLIST_ADDGroupMENUITEM
//lParam=0
//returns 0 on success, nonzero on failure
#define MS_CLIST_REMOVEGROUPMENUITEM					"CList/RemoveGroupMenuItem"

//builds the Group menu
//wParam=lParam=0
//returns a HMENU identifying the menu.
#define MS_CLIST_MENUBUILDGROUP							"CList/MenuBuildGroup"

//add a new item to the Group menus
//wParam=lpGroupMenuParam, params to call when exec menuitem
//lParam=(LPARAM)(CLISTMENUITEM*)&mi
#define MS_CLIST_ADDGROUPMENUITEM						"CList/AddGroupMenuItem"

//the Group menu is about to be built
//wParam=lParam=0
#define ME_CLIST_PREBUILDGROUPMENU						"CList/PreBuildGroupMenu"

// Group MENU


// TRAY MENU
//remove a item from tray menu
//wParam=hMenuItem returned by MS_CLIST_ADDTRAYMENUITEM
//lParam=0
//returns 0 on success, nonzero on failure
#define MS_CLIST_REMOVETRAYMENUITEM					"CList/RemoveTrayMenuItem"

//builds the tray menu
//wParam=lParam=0
//returns a HMENU identifying the menu.
#define MS_CLIST_MENUBUILDTRAY						"CList/MenuBuildTray"

//add a new item to the tray menus
//wParam=0
//lParam=(LPARAM)(CLISTMENUITEM*)&mi
#define MS_CLIST_ADDTRAYMENUITEM					"CList/AddTrayMenuItem"

//the tray menu is about to be built
//wParam=lParam=0
#define ME_CLIST_PREBUILDTRAYMENU					"CList/PreBuildTrayMenu"

// STATUS MENU

//the status menu is about to be built
//wParam=lParam=0
#define ME_CLIST_PREBUILDSTATUSMENU "CList/PreBuildStatusMenu"

//add a new item to the status menu
//wParam=0
//lParam=(LPARAM)(CLISTMENUITEM*)&mi
#define MS_CLIST_ADDSTATUSMENUITEM "CList/AddStatusMenuItem"

//remove a item from main menu
//wParam=hMenuItem returned by MS_CLIST_ADDMAINMENUITEM
//lParam=0
//returns 0 on success, nonzero on failure
#define MS_CLIST_REMOVEMAINMENUITEM					"CList/RemoveMainMenuItem"

//builds the main menu
//wParam=lParam=0
//returns a HMENU identifying the menu.
#define MS_CLIST_MENUBUILDMAIN						"CList/MenuBuildMain"



//the main menu is about to be built
//wParam=lParam=0
#define ME_CLIST_PREBUILDMAINMENU					"CList/PreBuildMainMenu"




//remove a item from contact menu
//wParam=hMenuItem returned by MS_CLIST_ADDCONTACTMENUITEM
//lParam=0
//returns 0 on success, nonzero on failure
#define MS_CLIST_REMOVECONTACTMENUITEM			"CList/RemoveContactMenuItem"
/*GENMENU_MODULE*/

#define SETTING_NOOFFLINEBOTTOM_DEFAULT 0

typedef struct
{
	int cbSize;
	union
	{
		char *pszName;
		TCHAR *ptszName;
	};
	int position;
	int root;
	int flags;
	union {
		HICON hIcon;
		HANDLE hIcolibItem;
	};
	DWORD hotKey;
	void *ownerdata;
}
	TMO_MenuItem,*PMO_MenuItem;

/*
This structure passed to CheckService.
*/
typedef struct
{
	void *MenuItemOwnerData;
	int MenuItemHandle;
	WPARAM wParam;//from  ListParam.wParam when building menu
	LPARAM lParam;//from  ListParam.lParam when building menu
}
	TCheckProcParam,*PCheckProcParam;

typedef struct
{
	int cbSize;
	char *name;

	/*
	This service called when module build menu(MO_BUILDMENU).
	Service called with params

	wparam=PCheckProcParam
	lparam=0
	if return==FALSE item is skiped.
	*/
	char *CheckService;

	/*
	This service called when user select menu item.
	Service called with params
	wparam=ownerdata
	lparam=lParam from MO_PROCESSCOMMAND
	*/
	char *ExecService;//called when processmenuitem called
}
	TMenuParam,*PMenuParam;

//used in MO_BUILDMENU
typedef struct tagListParam
{
	int rootlevel;
	int MenuObjectHandle;
	int wParam,lParam;
}
	ListParam,*lpListParam;

typedef struct
{
	HMENU menu;
	int ident;
	LPARAM lParam;
}
	ProcessCommandParam,*lpProcessCommandParam;

//wparam started hMenu
//lparam ListParam*
//result hMenu
#define MO_BUILDMENU						"MO/BuildMenu"

//wparam=MenuItemHandle
//lparam userdefined
//returns TRUE if it processed the command, FALSE otherwise
#define MO_PROCESSCOMMAND					"MO/ProcessCommand"

//if menu not known call this
//LOWORD(wparam) menuident (from WM_COMMAND message)
//returns TRUE if it processed the command, FALSE otherwise
//Service automatically find right menuobject and menuitem
//and call MO_PROCESSCOMMAND
#define MO_PROCESSCOMMANDBYMENUIDENT		"MO/ProcessCommandByMenuIdent"


//wparam=0;
//lparam=PMenuParam;
//returns=MenuObjectHandle on success,-1 on failure
#define MO_CREATENEWMENUOBJECT				"MO/CreateNewMenuObject"

//wparam=MenuObjectHandle
//lparam=0
//returns 0 on success,-1 on failure
//Note: you must free all ownerdata structures, before you
//call this service.MO_REMOVEMENUOBJECT NOT free it.
#define MO_REMOVEMENUOBJECT					"MO/RemoveMenuObject"


//wparam=MenuItemHandle
//lparam=0
//returns 0 on success,-1 on failure.
//You must free ownerdata before this call.
//If MenuItemHandle is root all child will be removed too.
#define MO_REMOVEMENUITEM					"MO/RemoveMenuItem"

//wparam=MenuObjectHandle
//lparam=PMO_MenuItem
//return MenuItemHandle on success,-1 on failure
//Service supports old menu items (without CMIF_ROOTPOPUP or
//CMIF_CHILDPOPUP flag).For old menu items needed root will be created
//automatically.
#define MO_ADDNEWMENUITEM					"MO/AddNewMenuItem"

//wparam MenuItemHandle
//returns ownerdata on success,NULL on failure
//Useful to get and free ownerdata before delete menu item.
#define MO_MENUITEMGETOWNERDATA				"MO/MenuItemGetOwnerData"

//wparam MenuItemHandle
//lparam PMO_MenuItem
//returns 0 on success,-1 on failure
#define MO_MODIFYMENUITEM					"MO/ModifyMenuItem"

//wparam=MenuItemHandle
//lparam=PMO_MenuItem
//returns 0 and filled PMO_MenuItem structure on success and
//-1 on failure
#define MO_GETMENUITEM						"MO/GetMenuItem"

//wparam=MenuObjectHandle
//lparam=vKey
//returns TRUE if it processed the command, FALSE otherwise
//this should be called in WM_KEYDOWN
#define	MO_PROCESSHOTKEYS					"MO/ProcessHotKeys"

//set uniq name to menuitem(used to store it in database when enabled OPT_USERDEFINEDITEMS)
#define OPT_MENUITEMSETUNIQNAME								1

//Set FreeService for menuobject. When freeing menuitem it will be called with
//wParam=MenuItemHandle
//lParam=mi.ownerdata
#define OPT_MENUOBJECT_SET_FREE_SERVICE						2

//Set onAddService for menuobject.
#define OPT_MENUOBJECT_SET_ONADD_SERVICE					3

//Set menu check service
#define OPT_MENUOBJECT_SET_CHECK_SERVICE          4

//enable ability user to edit menuitems via options page.
#define OPT_USERDEFINEDITEMS 1


typedef struct tagOptParam
{
	int Handle;
	int Setting;
	int Value;
}
	OptParam,*lpOptParam;

//wparam=0
//lparam=*lpOptParam
//returns TRUE if it processed the command, FALSE otherwise
#define MO_SETOPTIONSMENUOBJECT					"MO/SetOptionsMenuObject"


//wparam=0
//lparam=*lpOptParam
//returns TRUE if it processed the command, FALSE otherwise
#define MO_SETOPTIONSMENUITEM					"MO/SetOptionsMenuItem"

#endif
