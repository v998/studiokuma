// fetion.cpp : DLL アプリケーションのエントリ ポイントを定義します。
//

#include "stdafx.h"

// Enable the following line to use Visual Leak Detector
// Don't use VLD!!
//#include "vld/vld.h"

MM_INTERFACE   mmi;
UTF8_INTERFACE utfi;
MD5_INTERFACE md5i;
SHA1_INTERFACE sha1i;
//XML_API xi;

HINSTANCE g_hInstance;				// My hInstance
PLUGINLINK* pluginLink;				// Struct of functions pointers for service calls
static char g_dllname[MAX_PATH];
//LISTENINGTOINFO qqCurrentMedia={0};

HANDLE g_hNetlibUser=NULL;			// Handle of NetLib user
static HANDLE hPrevInstanceService=NULL;

list<CNetwork*> g_networks;
bool g_enableBBCode=false;

PLUGININFOEX pluginInfo={
	sizeof(PLUGININFOEX),
	"MIMFetion (Unicode)",
	PLUGINVERSION,
	"Miranda Fetion Plugin (Beta Version - " __DATE__ " " __TIME__")",
	"Stark Wong",
	"starkwong@hotmail.com",
	"(C)2008 Stark Wong",
	"http://www.studiokuma.com/mimfetion/",
	UNICODE_AWARE,
	0,
	{ 0x5fecee40, 0x7794, 0x4a2e, { 0xb9, 0xcf, 0x84, 0x47, 0xb8, 0xf, 0x80, 0x99 } } // {5FECEE40-7794-4a2e-B9CF-8447B80F8097}
};

// Imported Declarations
// All functions here are defined in services.cpp
#define DECL(a) extern int a(WPARAM wParam, LPARAM lParam)

// DllMain(): WINAPI DllMain
extern "C" {
	BOOL WINAPI DllMain(HINSTANCE hinst,DWORD fdwReason,LPVOID lpvReserved)
	{
		g_hInstance=hinst;
		DisableThreadLibraryCalls(hinst);
		return TRUE;
	}

	// MirandaPluginInfo(): Retrieve plugin information
	// mirandaVersion: Version of running Miranda IM
	// Return: Pointer to PLUGININFO, or NULL to disallow plugin load
	__declspec(dllexport) PLUGININFOEX* MirandaPluginInfoEx(DWORD mirandaVersion)
	{
		if ( mirandaVersion < PLUGIN_MAKE_VERSION( 0, 8, 0, 19 )) {
			MessageBox( NULL, _T("The Fetion protocol plugin cannot be loaded. It requires Miranda IM 0.8.0.19 or later."), NULL, MB_OK|MB_ICONERROR|MB_SETFOREGROUND|MB_TOPMOST );
			return NULL;
		}

		return &pluginInfo;
	}

	static const MUUID interfaces[] = {MIID_PROTOCOL, MIID_LAST};
	__declspec(dllexport) const MUUID* MirandaPluginInterfaces(void)
	{
		return interfaces;
	}

	// Unload(): Called then the plugin is being unloaded (Miranda IM exiting)
	__declspec(dllexport)int Unload(void)
	{
		return 0;
	}

#if 0
	// InitUpdater(): Registration for Updater Plugin
	static void InitUpdater() {
		Update update = {0};
		char szVersion[16];

		update.szComponentName = pluginInfo.shortName;
		update.pbVersion = (BYTE *)CreateVersionStringPlugin((PLUGININFO*)&pluginInfo, szVersion);
		update.cpbVersion = strlen((char *)update.pbVersion);

		update.szUpdateURL = "http://miranda-im.org/download/feed.php?dlfile=2107";
		update.szVersionURL = "http://miranda-im.org/download/details.php?action=viewfile&id=2107";
		update.pbVersionPrefix = (BYTE *)"<span class=\"fileNameHeader\">MirandaQQ (libeva Version) ";

		/*
		update.szBetaUpdateURL = "http://starkwong.ivehost.net/mimqq/mimqq-libeva.zip";
		update.szBetaVersionURL = "http://starkwong.ivehost.net/mimqq/updater.php";
		update.pbBetaVersionPrefix = (BYTE *)"Version ";
		*/

		update.cpbVersionPrefix = strlen((char *)update.pbVersionPrefix);
		//update.cpbBetaVersionPrefix = strlen((char *)update.pbBetaVersionPrefix);

		CallService(MS_UPDATE_REGISTER, 0, (WPARAM)&update);
	}
#endif

	int OnModulesLoaded(WPARAM wParam, LPARAM lParam) {
		CHAR szTemp[MAX_PATH];
		NETLIBUSER nlu = {sizeof(nlu)};
		CLISTMENUITEM mi={sizeof(mi)};

		// Register NetLib User
		sprintf(szTemp,Translate("%s plugin connections"),Translate("Fetion"));
		nlu.szDescriptiveName=szTemp;
		nlu.flags=NUF_INCOMING | NUF_OUTGOING | NUF_HTTPCONNS | NUF_NOHTTPSOPTION;

		nlu.szSettingsModule=g_dllname;
		g_hNetlibUser=(HANDLE)CallService(MS_NETLIB_REGISTERUSER, 0, ( LPARAM )&nlu);

		CallService("DBEditorpp/RegisterSingleModule",(WPARAM)nlu.szSettingsModule, 0);	

		extern int init_stackwalk();

#ifdef STACKWALK
		init_stackwalk();
#endif

		return 0;
	}

	// OnModulesLoaded(): Calls by Miranda when plugin loads
	int CNetwork::OnModulesLoadedEx( WPARAM wParam, LPARAM lParam )
	{
		CHAR szTemp[MAX_PATH]={0};
		//TCHAR szPopupName[MAX_PATH];
		LPSTR pszTemp;
		NETLIBUSER nlu = {0};
		CLISTMENUITEM mi={sizeof(mi)};
		int c=0, c2=0;

		//add as a known module in DB Editor ++
		CallService("DBEditorpp/RegisterSingleModule",(WPARAM)m_szModuleName, 0);	

		// Setup custom hook
		//QHookEvent(ME_USERINFO_INITIALISE, &CNetwork::OnDetailsInit);
		//QHookEvent(ME_OPT_INITIALISE, &CNetwork::OnOptionsInit);

		mi.popupPosition=500090000;
		mi.pszService=szTemp;
		mi.ptszName=m_tszUserName;
		mi.position=-1999901009;
		mi.ptszPopupName=(LPWSTR)-1;
		//mi.flags=CMIF_ROOTPOPUP|CMIF_UNICODE;
		mi.hIcon=LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_ICON1));
		//m_hMenuRoot=(HANDLE)CallService(MS_CLIST_ADDMAINMENUITEM, (WPARAM)0, (LPARAM)&mi);

		//mi.flags=CMIF_CHILDPOPUP|CMIF_UNICODE;
		//mi.ptszPopupName=(LPWSTR)m_hMenuRoot;
		//mi.position=500090000;

		strcpy(szTemp,m_szModuleName);
		pszTemp=szTemp+strlen(szTemp);

#define _CRMI(a,b,d,e,f) DECL(b); strcpy(pszTemp, a);m_serviceList.push_back(FCreateService(szTemp, &CNetwork::b));mi.ptszName=d;e.push_back((HANDLE)CallService(f, 0, (LPARAM)&mi));mi.position+=5;
#if 0
#define CRMI(a,b,d) _CRMI(a,b,d,m_menuItemList,MS_CLIST_ADDMAINMENUITEM)
		CRMI(QQ_MENU_CHANGENICKNAME,ChangeNickname,TranslateT("Change &Nickname"));
		CRMI(QQ_MENU_MODIFYSIGNATURE,ModifySignature,TranslateT("&Modify Personal Signature"));
		//CRMI(QQ_MENU_CHANGEHEADIMAGE,ChangeHeadImage,Translate("Change &Head Image"));
		CRMI(QQ_MENU_QQMAIL,QQMail,TranslateT("&QQ Mail"));
		CRMI(QQ_MENU_COPYIP,CopyMyIP,TranslateT("Show and copy my Public &IP"));
		CRMI(QQ_MENU_DOWNLOADGROUP,DownloadGroup,TranslateT("&Download Group"));
		CRMI(QQ_MENU_UPLOADGROUP,UploadGroup,TranslateT("&Upload Group"));
		CRMI(QQ_MENU_REMOVENONSERVERCONTACTS,RemoveNonServerContacts,TranslateT("&Remove contacts not in Server"));
		CRMI(QQ_MENU_SUPPRESSADDREQUESTS,SuppressAddRequests,TranslateT("&Ignore any Add Requests"));
		CRMI(QQ_MENU_DOWNLOADUSERHEAD,DownloadUserHead,TranslateT("Re&download User Head"));
		CRMI(QQ_MENU_GETWEATHER,GetWeather,TranslateT("Get &Weather Information"));
		//CRMI(QQ_MENU_SUPPRESSQUN,SuppressQunMessages,Translate("&Suppress Qun Message Receive"));
		//CRMI(QQ_MENU_TOGGLEQUNLIST,ToggleQunList,Translate("&Toggle Qun List"));
		//CRMI("/TestService",TestService,Translate("Test Service"));
		NotifyEventHooks(hIPCEvent,QQIPCEVT_CREATE_MAIN_MENU,(LPARAM)&mi);
#endif
		// Context Menus
		mi.flags=CMIF_HIDDEN|CMIF_UNICODE; //CMIF_NOTOFFLINE;
		mi.position=-500050000;

		c=0;

#define CRMI2(a,b,d) _CRMI(a,b,d,m_contextMenuItemList,MS_CLIST_ADDCONTACTMENUITEM)
		CRMI2(CTXMENU_SMS,ForceSMS,LPGENT("Switch Send Mode to SMS"));
#if 0
		InitUpdater();

		// FontService
		FontID fid = {sizeof(fid)};
		LPSTR pszPopupName=mir_u2a(m_tszUserName);
		strcpy(fid.name,Translate("Contact Messaging Font"));
		strcpy(fid.group,pszPopupName);
		strcpy(fid.dbSettingsGroup,m_szModuleName);
		strcpy(fid.prefix,"font1");
		mir_free(pszPopupName);
		fid.flags=FIDF_NOAS|FIDF_SAVEPOINTSIZE|FIDF_DEFAULTVALID|FIDF_ALLOWEFFECTS;

		// you could register the font at this point - getting it will get either the global default or what the user has set it 
		// to - but we'll set a default font:

		fid.deffontsettings.charset = GB2312_CHARSET;
		fid.deffontsettings.colour = RGB(0, 0, 0);
		fid.deffontsettings.size = 12;
		fid.deffontsettings.style = 0;
		strncpy(fid.deffontsettings.szFace, "SimSun", LF_FACESIZE);
		c=CallService(MS_FONT_REGISTER, (WPARAM)&fid, 0);

		strcpy(fid.name,Translate("Qun Messaging Font"));
		strcpy(fid.prefix,"font2");
		c=CallService(MS_FONT_REGISTER, (WPARAM)&fid, 0);
#endif

		util_log("Init Completed");

		return 0;
	}

	static int OnPreShutdown( WPARAM wParam, LPARAM lParam )
	{
		return 0;
	}

	void CNetwork::UnloadAccount() {
		while (m_hookList.size()) {
			UnhookEvent(m_hookList.front());
			m_hookList.pop_front();
		}

		while (m_serviceList.size()) {
			DestroyServiceFunction(m_serviceList.front());
			m_serviceList.pop_front();
		}

#if 0
		for (list<CNetwork*>::iterator iter=g_networks.begin(); iter!=g_networks.end(); iter++)
			if (*iter==this) {
				g_networks.erase(iter);
				break;
			}
#endif
	}

	// Load(): Called when plugin is loaded into Miranda
	int __declspec(dllexport)Load(PLUGINLINK *link)
	{
		PROTOCOLDESCRIPTOR pd={sizeof(PROTOCOLDESCRIPTOR)};
		CHAR szTemp[MAX_PATH];
		pluginLink=link;
		mir_getMMI(&mmi);
		//mir_getLI(&li );
		mir_getUTFI(&utfi);
		mir_getMD5I(&md5i);
		mir_getSHA1I(&sha1i);
		//mir_getXI(&xi);

		if (ServiceExists("Fetion/PrevInstance")) {
			MessageBoxW(NULL,L"The Fetion Plugin can only be loaded once. This copy will be deactivated.",NULL,MB_ICONERROR);
			return -1;
		} else {
			hPrevInstanceService=CreateServiceFunction("Fetion/PrevInstance",NULL);
		}

		// Register Protocol
		GetModuleFileNameA(g_hInstance,szTemp,MAX_PATH);
		*strrchr(szTemp,'.')=0;
		strcpy(g_dllname,CharUpperA(strrchr(szTemp,'\\')+1));

		pd.szName=g_dllname;
		pd.type=PROTOTYPE_PROTOCOL;
		pd.fnInit=&CNetwork::InitAccount;
		pd.fnUninit=&CNetwork::UninitAccount;
		//pd.fnDestroy=&CNetwork::DestroyAccount;

		CallService(MS_PROTO_REGISTERMODULE,0,(LPARAM)&pd);

		HookEvent(ME_SYSTEM_MODULESLOADED, OnModulesLoaded);
		HookEvent(ME_SYSTEM_PRESHUTDOWN, OnPreShutdown);

		return 0;
	}
}

void CNetwork::LoadAccount() {
	CHAR szTemp[MAX_PATH];
	LPSTR pszTemp;
	int c=0;

	// Register Service Functions
	strcpy(szTemp, m_szProtoName);
	pszTemp=szTemp+strlen(szTemp);

#define CSF(a,b) DECL(b); strcpy(pszTemp, a); this->FCreateService(szTemp,&CNetwork::b)

	CSF(PS_GETNAME,GetName);
	CSF(PS_GETSTATUS,GetStatus);

	CSF(PS_CREATEACCMGRUI,CreateAccMgrUI);


#if 0
	CSF(PS_SETMYNICKNAME,SetMyNickname);
	CSF(PS_GETAVATARINFO,GetAvatarInfo);

	CSF(PS_SET_LISTENINGTO,SetCurrentMedia);
	CSF(PS_GET_LISTENINGTO,GetCurrentMedia);

	// Auth
	CSF(PSR_AUTH,RecvAuth);
#if 0
	CSF(PSS_ADDED,Added);

	// Aux

	// File
	CSF(PSS_FILE,SendFile);
	CSF(PSS_FILECANCEL,FileCancel);
	CSF(PS_FILERESUME,FileResume);

#endif

	// Hook modules loaded
	c=0;

	//this->QHookEvent(ME_SYSTEM_MODULESLOADED, &CNetwork::OnModulesLoadedEx);
#endif
	this->FHookEvent(ME_CLIST_PREBUILDCONTACTMENU, &CNetwork::OnPrebuildContactMenu);
	this->FHookEvent(ME_DB_CONTACT_DELETED, &CNetwork::OnContactDeleted);

	SetContactsOffline();

	//g_networks.push_back(this);
}

// initializes an empty account
tagPROTO_INTERFACE* CNetwork::InitAccount(LPCSTR szModuleName, LPCTSTR szUserName) {
	CNetwork* network = new CNetwork(szModuleName,szUserName);

	return (tagPROTO_INTERFACE*)network;
}

// deallocates an account instance
int CNetwork::UninitAccount(struct tagPROTO_INTERFACE* pInterface) {
	delete (CNetwork*)pInterface;
	return 0;
}
/*
// removes an account from the database
int CNetwork::DestroyAccount(struct tagPROTO_INTERFACE*) {
return 0;
}
*/
HANDLE CNetwork::FCreateService(LPCSTR pszService, ServiceFunc pFunc) {
	HANDLE hRet=NULL;
	if (hRet=CreateServiceFunctionObj(pszService,(MIRANDASERVICEOBJ)*(void**)&pFunc,this))
		m_serviceList.push_back(hRet);

	return hRet;
}

HANDLE CNetwork::FHookEvent(LPCSTR pszEvent, EventFunc pFunc) {
	HANDLE hRet=NULL;
	if (hRet=HookEventObj(pszEvent,(MIRANDAHOOKOBJ)*(void**)&pFunc,this))
		m_hookList.push_back(hRet);

	return hRet;
}
