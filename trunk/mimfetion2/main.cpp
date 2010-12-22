#include "StdAfx.h"

MM_INTERFACE   mmi;
UTF8_INTERFACE utfi;
//MD5_INTERFACE md5i;

PLUGINLINK* pluginLink;				// Struct of functions pointers for service calls
char g_dllname[MAX_PATH];
// LISTENINGTOINFO qqCurrentMedia={0};
static const MUUID interfaces[] = {MIID_PROTOCOL, MIID_LAST};
static list<tagPROTO_INTERFACE*> protocols;

bool g_enableBBCode=false;

PLUGININFOEX pluginInfo={
	sizeof(PLUGININFOEX),
	"MIMFetion2 (Unicode)",
	PLUGINVERSION,
	"MIMFetion WebFetion Build (Preview Version - " __DATE__ " " __TIME__")",
	"Stark Wong",
	"starkwong@hotmail.com",
	"(C)2010 Stark Wong",
	"http://www.studiokuma.com/mimfetion/",
	UNICODE_AWARE,
	0,
	{ /* d349f62a-dca5-49dd-9cf9-0d71d8965239 */ 0xd349f62a, 0xdca5, 0x49dd, {0x9c, 0xf9, 0x0d, 0x71, 0xd8, 0x96, 0x52, 0x39}}
};

extern "C" {
	BOOL WINAPI DllMain(HINSTANCE hinst,DWORD fdwReason,LPVOID lpvReserved) {
		if (fdwReason==DLL_PROCESS_ATTACH) {
			CProtocol::g_hInstance=hinst;
			DisableThreadLibraryCalls(hinst);
		}
		return TRUE;
	}

	// MirandaPluginInfo(): Retrieve plugin information
	// mirandaVersion: Version of running Miranda IM
	// Return: Pointer to PLUGININFO, or NULL to disallow plugin load
	__declspec(dllexport) PLUGININFOEX* MirandaPluginInfoEx(DWORD mirandaVersion) {
		if (mirandaVersion<PLUGIN_MAKE_VERSION( 0, 9, 0, 0 )) {
			MessageBoxA(NULL, "MIMFetion2 plugin can only be loaded on Miranda IM 0.9.0.0 or later.", NULL, MB_OK|MB_ICONERROR|MB_SETFOREGROUND|MB_TOPMOST);
			return NULL;
		}

		return &pluginInfo;
	}

	__declspec(dllexport) const MUUID* MirandaPluginInterfaces(void) {
		return interfaces;
	}

	// Unload(): Called then the plugin is being unloaded (Miranda IM exiting)
	__declspec(dllexport)int Unload(void) {
		return 0;
	}

	////////////////////////////////////////////////////

	// Load(): Called when plugin is loaded into Miranda
	int __declspec(dllexport)Load(PLUGINLINK *link)
	{
		PROTOCOLDESCRIPTOR pd={sizeof(PROTOCOLDESCRIPTOR)};
		CHAR szTemp[MAX_PATH];
		pluginLink=link;
		mir_getMMI(&mmi);
		//mir_getLI(&li);
		mir_getUTFI(&utfi);
		//mir_getMD5I(&md5i);
		//mir_getSHA1I(&sha1i);

		if (ServiceExists("MIMFetion2/PrevInstance")) {
			MessageBoxA(NULL,"This version of MIMFetion2 can only be loaded once. This copy will be deactivated.",NULL,MB_ICONERROR);
			return -1;
		}
		
		CreateServiceFunction("MIMFetion2/PrevInstance",NULL);

		// Register Protocol
		GetModuleFileNameA(CProtocol::g_hInstance,szTemp,MAX_PATH);
		*strrchr(szTemp,'.')=0;
		strcpy(g_dllname,CharUpperA(strrchr(szTemp,'\\')+1));
		
		pd.szName=g_dllname;
		pd.type=PROTOTYPE_PROTOCOL;
		pd.fnInit=&CProtocol::InitAccount;
		pd.fnUninit=&CProtocol::UninitAccount;
		//pd.fnDestroy=&CNetwork::DestroyAccount;

		CallService(MS_PROTO_REGISTERMODULE,0,(LPARAM)&pd);

		return 0;
	}
} // extern "C"

/////////////////////////////////

tagPROTO_INTERFACE* CProtocol::InitAccount(LPCSTR szModuleName, LPCTSTR szUserName) {
	CProtocol* protocol=new CProtocol(szModuleName,szUserName);
	protocols.push_back(protocol);
	return protocol;
}

int CProtocol::UninitAccount(struct tagPROTO_INTERFACE* pInterface) {
	delete (CProtocol*)pInterface;
	protocols.remove(pInterface);
	return 0;
}

