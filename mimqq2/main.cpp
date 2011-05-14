/* MirandaQQ2 (libeva Version)
 * Copyright(C) 2005-2007 Studio KUMA. Written by Stark Wong.
 *
 * Distributed under terms and conditions of GNU GPLv2.
 *
 * Plugin framework based on BaseProtocol. Copyright (C) 2004 Daniel Savi (dss@brturbo.com)
 *
 * This plugin utilizes the libeva library. Copyright(C) yunfan.

Miranda ICQ: the free icq client for MS Windows 
Copyright (C) 2000-5  Richard Hughes, Roland Rabien & Tristan Van de Vreede
*/
#include "StdAfx.h"

// Enable the following line to use Visual Leak Detector
// Don't use VLD!!
//#include "vld/vld.h"

MM_INTERFACE   mmi;
UTF8_INTERFACE utfi;
MD5_INTERFACE md5i;

HINSTANCE hinstance;				// My hInstance
PLUGINLINK* pluginLink;				// Struct of functions pointers for service calls
char g_dllname[MAX_PATH];
LISTENINGTOINFO qqCurrentMedia={0};

HANDLE hNetlibUser=NULL;			// Handle of NetLib user
//static HANDLE hPrevInstanceService=NULL;
HANDLE CNetwork::m_folders[3];

#ifdef MIRANDAQQ_IPC
HANDLE hIPCEvent=NULL;
#endif

list<CNetwork*> g_networks;
bool g_enableBBCode=false;

PLUGININFOEX pluginInfo={
		sizeof(PLUGININFOEX),
#ifdef UNICODE
		"MirandaQQ2 (Unicode)",
#else
		"Project Nagato",
#endif
		PLUGINVERSION,
		"MirandaQQ Protocol 2006 Build (Beta Version - " __DATE__ " " __TIME__")",
		"Stark Wong",
		"starkwong@hotmail.com",
		"(C)2008 Stark Wong",
		"http://www.studiokuma.com/mimqq/",
		UNICODE_AWARE,
		0,
		{ 0x5fecee40, 0x7794, 0x4a2e, { 0xb9, 0xcf, 0x84, 0x47, 0xb8, 0xf, 0x80, 0x97 } } // {5FECEE40-7794-4a2e-B9CF-8447B80F8097}
};

// Imported Declarations
// All functions here are defined in services.cpp
#define DECL(a) extern int a(WPARAM wParam, LPARAM lParam)

// DllMain(): WINAPI DllMain
extern "C" {
	BOOL WINAPI DllMain(HINSTANCE hinst,DWORD fdwReason,LPVOID lpvReserved)
	{
		hinstance=hinst;
		DisableThreadLibraryCalls(hinst);
		return TRUE;
	}

	// MirandaPluginInfo(): Retrieve plugin information
	// mirandaVersion: Version of running Miranda IM
	// Return: Pointer to PLUGININFO, or NULL to disallow plugin load
	__declspec(dllexport) PLUGININFOEX* MirandaPluginInfoEx(DWORD mirandaVersion)
	{
		if ( mirandaVersion < PLUGIN_MAKE_VERSION( 0, 8, 0, 17 )) {
			MessageBox( NULL, _T("The QQ protocol plugin cannot be loaded. It requires Miranda IM 0.8.0.17 or later."), NULL, MB_OK|MB_ICONERROR|MB_SETFOREGROUND|MB_TOPMOST );
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
#ifdef MIRANDAQQ_IPC
		DestroyHookableEvent(hIPCEvent);
#endif

		return 0;
	}

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

	int OnModulesLoaded(WPARAM wParam, LPARAM lParam) {
		CHAR szTemp[MAX_PATH];
		NETLIBUSER nlu = {sizeof(nlu)};
		CLISTMENUITEM mi={sizeof(mi)};

		// Register NetLib User
		sprintf(szTemp,Translate("%s plugin connections"),"QQ");
		nlu.szDescriptiveName=szTemp;
		nlu.flags=NUF_INCOMING | NUF_OUTGOING | NUF_HTTPCONNS | NUF_NOHTTPSOPTION;

		nlu.szSettingsModule=g_dllname;
		hNetlibUser=(HANDLE)CallService(MS_NETLIB_REGISTERUSER, 0, ( LPARAM )&nlu);

		CallService("DBEditorpp/RegisterSingleModule",(WPARAM)nlu.szSettingsModule, 0);	

		WCHAR szTemp2[MAX_PATH];
		LPWSTR tmpPath = Utils_ReplaceVarsT(L"%miranda_avatarcache%");

		mir_sntprintf(szTemp2, MAX_PATH, L"%s\\%S\\QunImages", tmpPath,g_dllname);
		CNetwork::m_folders[0]=FoldersRegisterCustomPathW(g_dllname, "Qun Images", szTemp2);

		mir_sntprintf(szTemp2, MAX_PATH, L"%s\\%S\\WebServer", tmpPath,g_dllname);
		CNetwork::m_folders[1]=FoldersRegisterCustomPathW(g_dllname, "Web Server Document Root", szTemp2);

		mir_free(tmpPath);

		InitializeCriticalSection(&CQunImage::m_cs);
		CQunImage::m_imageServer=new CQunImageServer();

		if (ServiceExists(MS_ASSOCMGR_ADDNEWURLTYPE)) {
			extern int ParseTencentURI(WPARAM,LPARAM);
			CreateServiceFunction("MIRANDAQQ/ParseTencentURI", ParseTencentURI);
			AssocMgr_AddNewUrlTypeT("tencent:", TranslateT("MirandaQQ Link Protocol"), hinstance, IDI_TM, "MIRANDAQQ/ParseTencentURI", UTDF_DEFAULTDISABLED);
		}

		extern int init_stackwalk();

#ifdef STACKWALK
		init_stackwalk();
#endif

		if (ServiceExists(MS_IEVIEW_EVENT) && (DBGetContactSettingDword(NULL,"IEVIEW","GeneralFlags",0) & 1)) g_enableBBCode=true;

		return 0;
	}

	void CNetwork::InitFontService() {
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
		fid.deffontsettings.size = 16;
		fid.deffontsettings.style = 0;
		strncpy(fid.deffontsettings.szFace, "SimSun", LF_FACESIZE);
		CallService(MS_FONT_REGISTER, (WPARAM)&fid, 0);

		strcpy(fid.name,Translate("Qun Messaging Font"));
		strcpy(fid.prefix,"font2");
		CallService(MS_FONT_REGISTER, (WPARAM)&fid, 0);
	}

	void CNetwork::InitFoldersService() {
		if (!m_avatarFolder) {
			char AvatarsFolder[MAX_PATH];

			LPSTR tmpPath = Utils_ReplaceVars("%miranda_avatarcache%");

			mir_snprintf(AvatarsFolder, MAX_PATH, "%s\\%s", tmpPath, m_szModuleName);
			m_avatarFolder=FoldersRegisterCustomPath(m_szModuleName, "Avatars", AvatarsFolder);

			mir_free(tmpPath);
		}
	}

#define CSF(a,b) DECL(b); strcpy(pszTemp, a); this->QCreateService(szTemp,&CNetwork::b)

	// OnModulesLoaded(): Calls by Miranda when plugin loads
	int CNetwork::OnModulesLoadedEx( WPARAM wParam, LPARAM lParam )
	{
		NETLIBUSER nlu = {0};

		//add as a known module in DB Editor ++
		CallService("DBEditorpp/RegisterSingleModule",(WPARAM)m_szModuleName, 0);	

		// Setup custom hook
		QHookEvent(ME_USERINFO_INITIALISE, &CNetwork::OnDetailsInit);
		QHookEvent(ME_OPT_INITIALISE, &CNetwork::OnOptionsInit);

		if (CallService(MS_SYSTEM_GETVERSION,NULL,NULL)<0x00090000) OnEvent(EV_PROTO_ONMENU,0,0);

		InitUpdater();
		InitFontService();
		InitFoldersService();

		/*
		CHAR szTemp[MAX_PATH];
		LPSTR pszTemp;

		strcpy(szTemp, m_szProtoName);
		pszTemp=szTemp+strlen(szTemp);

		CSF(PS_GETAVATARCAPS,GetAvatarCaps);
		CSF(PS_GETMYAVATAR,GetMyAvatar);
		CSF(PS_SETMYAVATAR,SetMyAvatar);
		*/

		util_log(0,"Init Completed");

		return 0;
	}

	static int OnPreShutdown( WPARAM wParam, LPARAM lParam )
	{
		delete CQunImage::m_imageServer;
		if (DBGetContactSettingByte(NULL,g_dllname,QQ_NOREMOVEQUNIMAGE,0)==0) CNetwork::RemoveQunPics();
		DeleteCriticalSection(&CQunImage::m_cs);
		CEvaAccountSwitcher::Finalize();
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

		for (list<CNetwork*>::iterator iter=g_networks.begin(); iter!=g_networks.end(); iter++)
			if (*iter==this) {
				g_networks.erase(iter);
				break;
			}
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
		//mir_getSHA1I(&sha1i);

#if 0
		if (ServiceExists("MirandaQQ/HTTPDCommand")) {
			MessageBoxW(NULL,L"This version of MirandaQQ can only be loaded once. This copy will be deactivated.",NULL,MB_ICONERROR);
			return -1;
		} else {
			hPrevInstanceService=CreateServiceFunction("MirandaQQ/HTTPDCommand",NULL);
			//CSF("MirandaQQ/HTTPDCommand",QQHTTPDCommand);
		}
#endif

		// Register Protocol
		GetModuleFileNameA(hinstance,szTemp,MAX_PATH);
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

#ifdef MIRANDAQQ_IPC
		//CreateServiceFunction("MirandaQQ/IPCService",IPCService);
		hIPCEvent=CreateHookableEvent("MirandaQQ/IPCEvent");
#endif

		// Initialize libeva emot map
		EvaUtil::initMap();

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

	CSF(PS_GETNAME,GetName);
	CSF(PS_GETSTATUS,GetStatus);
	CSF(PS_SETMYNICKNAME,SetMyNickname);
	CSF(PS_GETAVATARINFO,GetAvatarInfo);

	CSF(PS_SET_LISTENINGTO,SetCurrentMedia);
	CSF(PS_GET_LISTENINGTO,GetCurrentMedia);
	CSF(PS_CREATEACCMGRUI,CreateAccMgrUI);

	CSF(PS_GETAVATARCAPS,GetAvatarCaps);
	CSF(PS_GETMYAVATAR,GetMyAvatar);
	CSF(PS_SETMYAVATAR,SetMyAvatar);

#ifdef MIRANDAQQ_IPC
	CSF("/IPCService",IPCService);
#endif

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
	this->QHookEvent(ME_DB_CONTACT_DELETED, &CNetwork::OnContactDeleted);
	this->QHookEvent(ME_CLIST_PREBUILDCONTACTMENU, &CNetwork::OnPrebuildContactMenu);

	SetContactsOffline();
	m_needAck=READ_B2(NULL,QQ_WAITACK); // WaitAck is now reversed

	g_networks.push_back(this);
	if (g_networks.size()==2) CEvaAccountSwitcher::Initialize();
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
HANDLE CNetwork::QCreateService(LPCSTR pszService, ServiceFunc pFunc) {
	HANDLE hRet=NULL;
	if (hRet=CreateServiceFunctionObj(pszService,(MIRANDASERVICEOBJ)*(void**)&pFunc,this))
		m_serviceList.push_back(hRet);
	
	return hRet;
}

HANDLE CNetwork::QHookEvent(LPCSTR pszEvent, EventFunc pFunc) {
	HANDLE hRet=NULL;
	if (hRet=HookEventObj(pszEvent,(MIRANDAHOOKOBJ)*(void**)&pFunc,this))
		m_hookList.push_back(hRet);

	return hRet;
}
