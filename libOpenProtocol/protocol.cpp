#include "StdAfx.h"
#include "Protocol.h"
#include <fcntl.h>
#include <string>
#include <time.h>

typedef struct _LINKEDLIST {
	DWORD key;
	LPVOID values[5];
	_LINKEDLIST* next;
} LINKEDLIST, *PLINKEDLIST, *LPLINKEDLIST;

MM_INTERFACE   mmi;
UTF8_INTERFACE utfi;
MD5_INTERFACE md5i;
int hLangpack;

PLUGINLINK* pluginLink;				// Struct of functions pointers for service calls
char g_dllname[MAX_PATH];
// LISTENINGTOINFO qqCurrentMedia={0};
static const MUUID interfaces[] = {MIID_PROTOCOL, MIID_LAST};
static list<tagPROTO_INTERFACE*> protocols;
HINSTANCE g_hInst;

PLUGININFOEX pluginInfo={
	sizeof(PLUGININFOEX),
	"OpenProtocol (Unicode)",
	PLUGINVERSION,
	"OpenProtocol Build (Preview Version - " __DATE__ " " __TIME__")",
	"Stark Wong",
	"starkwong@hotmail.com",
	"(C)2012 Stark Wong",
	"http://www.studiokuma.com/libop/",
	UNICODE_AWARE,
	0,
	
	{ /* {C559F033-00A9-4277-995E-2A4836E04A3F} */ 0xc559f033, 0xa9, 0x4277, { 0x99, 0x5e, 0x2a, 0x48, 0x36, 0xe0, 0x4a, 0x3f } }
};

class CProtocol: public PROTO_INTERFACE, COpenProtocolHandler {
private:
	COpenProtocol* m_op;
	HANDLE m_nlu;
	LPSTR m_debugMsgBuffer;
	typedef int (__cdecl CProtocol::*ServiceFunc)(WPARAM, LPARAM);
	typedef void (__cdecl CProtocol::*ThreadFunc)(LPVOID);
	typedef int (__cdecl CProtocol::*EventFunc)(WPARAM, LPARAM);
	stack<HANDLE> m_services;
	stack<HANDLE> m_hooks;
	HANDLE m_hMenuRoot;
	map<int,int> m_localgroups;
	map<DWORD,DWORD> m_uins;
	LPSTR m_uin;
	map<DWORD,LPLINKEDLIST> m_qunmembers;
	stack<HANDLE> m_menuItems;

public:
	void Log(LPCSTR pcszFormat,...) {
		if (!m_debugMsgBuffer) {
			m_debugMsgBuffer=(LPSTR)mir_alloc(1024);
		}

		va_list vl;
		va_start(vl,pcszFormat);
		strcpy(m_debugMsgBuffer+vsprintf(m_debugMsgBuffer,pcszFormat,vl),"\n");
		va_end(vl);

		oph_printdebug(m_debugMsgBuffer);
	}

	HANDLE QCreateService(LPCSTR pszService, ServiceFunc pFunc) {
		HANDLE hRet=NULL;
		if (hRet=CreateServiceFunctionObj(pszService,(MIRANDASERVICEOBJ)*(void**)&pFunc,this))
			m_services.push(hRet);
		
		return hRet;
	}

	HANDLE QHookEvent(LPCSTR pszEvent, EventFunc pFunc) {
		HANDLE hRet=NULL;
		if (hRet=HookEventObj(pszEvent,(MIRANDAHOOKOBJ)*(void**)&pFunc,this))
			m_hooks.push(hRet);
	
		return hRet;
	}

	void CreateThreadObj(ThreadFunc func, void* arg) {
		unsigned int threadid;
		mir_forkthreadowner((pThreadFuncOwner) *(void**)&func,this,arg,&threadid);
	}

	void BroadcastStatus(int newStatus) {
		int oldStatus=m_iStatus;
		m_iStatus=newStatus;
		ProtoBroadcastAck(m_szModuleName,NULL,ACKTYPE_STATUS,ACKRESULT_SUCCESS,(HANDLE)oldStatus,newStatus);

		if (newStatus==ID_STATUS_OFFLINE) {
			SetContactsOffline();
			m_op=NULL;
			// libOP cleans itself when offline
			/*
			if (m_op!=NULL) {
				delete m_op;
				m_op=NULL;
			}*/
		}
	}

	int ShowNotification(LPCWSTR info, DWORD flags) {
		POPUPDATAW ppd={0};
		_tcscpy(ppd.lpwzContactName,m_tszUserName);
		_tcscpy(ppd.lpwzText,info);

		if (ServiceExists(MS_POPUP_ADDPOPUPW)) {
			ppd.lchIcon=(HICON)LoadImage(g_hInst, MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0);
			CallService(MS_POPUP_ADDPOPUPW,(WPARAM)&ppd,0);
		} else if (ServiceExists(MS_CLIST_SYSTRAY_NOTIFY)) {
			/*
			LPSTR szMsg=(LPSTR)_alloca(_tcslen(info)*2);
			WideCharToMultiByte(CP_ACP,NULL,info,-1,szMsg,_tcslen(info)*2,NULL,NULL);
			*/
			// Guest OS supporting tray notification
			MIRANDASYSTRAYNOTIFY err;
			err.szProto = m_szModuleName;
			err.cbSize = sizeof(err);
			err.tszInfoTitle = ppd.lpwzContactName;
			err.tszInfo = ppd.lpwzText;
			err.dwInfoFlags = flags|NIIF_INTERN_UNICODE;
			err.uTimeout = 1000 * 3;
			CallService(MS_CLIST_SYSTRAY_NOTIFY, 0, (LPARAM) & err);
			return 1;
		} else if (flags != NIIF_INFO) {
			// Gust OS does not support tray notification
			MessageBoxW(NULL,ppd.lpwzText,ppd.lpwzContactName,flags==NIIF_ERROR?MB_ICONERROR:MB_ICONWARNING);
		}
		return 0;
	}

	void SetContactsOffline() {	
		HANDLE hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);

		while (hContact) {
			if (!lstrcmpA(m_szModuleName, (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0))) {
				WRITEC_W("Status",ID_STATUS_OFFLINE);
			}
			hContact = ( HANDLE )CallService( MS_DB_CONTACT_FINDNEXT, ( WPARAM )hContact, 0 );
		}
	}

	CProtocol(LPCSTR szModuleName, LPCTSTR szUserName):
	m_op(NULL),m_debugMsgBuffer(NULL),m_uin(NULL) {
		WCHAR wszTemp[MAX_PATH];
		NETLIBUSER nlu = {sizeof(nlu)};

		m_szModuleName=m_szProtoName=mir_strdup(szModuleName);
		m_tszUserName=mir_wstrdup(szUserName);

		// Register NetLib User
		swprintf(wszTemp,TranslateT("%S plugin connections"),m_szModuleName);
		nlu.ptszDescriptiveName=wszTemp;
		nlu.flags=NUF_UNICODE/*|NUF_INCOMING*/|NUF_OUTGOING|NUF_NOHTTPSOPTION|NUF_HTTPCONNS; // NUF_INCOMING for HttpServer

		nlu.szSettingsModule=m_szModuleName;
		m_nlu=(HANDLE)CallService(MS_NETLIB_REGISTERUSER, 0, ( LPARAM )&nlu);
	}

	~CProtocol() {
		mir_free(m_szModuleName);
		mir_free(m_tszUserName);
		if (m_uin) mir_free(m_uin);
		if (m_debugMsgBuffer) mir_free(m_debugMsgBuffer);
	}

	// PROTO_INTERFACE
	static int DestroyAccount(struct tagPROTO_INTERFACE*) {
	}

	int __cdecl CreateAccMgrUI(WPARAM wParam, LPARAM lParam) {
		HWND CreateAccountManagerUI(PROTO_INTERFACE* protocol, HWND hWndAccMgr);
		HWND hWnd=CreateAccountManagerUI(this,(HWND)lParam);
		ShowWindow(hWnd,SW_SHOWNORMAL);

		return (intptr_t)hWnd;
	}

	void ResetContacts() {
		// Remove all old contacts without real UID for now until identification is done
		HANDLE hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, (WPARAM)NULL, (LPARAM)NULL);
		HANDLE hContact2;
		int c=0;
		int c2=0;

		while (hContact) {
			hContact2=NULL;

			if (!lstrcmpA(m_szModuleName,(char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact,(LPARAM)NULL))) {
				c++;
				if (READC_D2(CKEY_REALUIN)==0) {
					hContact2=(HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, (LPARAM)NULL);
					CallService(MS_DB_CONTACT_DELETE,(WPARAM)hContact,0);
					c2++;
				}
			}

			if (hContact2)
				hContact=hContact2;
			else
				hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, (LPARAM)NULL);
		}

		Log("%s(): %d/%d protocol contacts removed",__FUNCTION__);
	}

	HANDLE FindOrAddContact(DWORD dwUIN, DWORD dwTUIN, BOOL isAdd=FALSE, BOOL isHidden=FALSE, BOOL isTemp=FALSE) {
		HANDLE hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, (WPARAM)NULL, (LPARAM)NULL);
		int c=0;
		LPCSTR pszKey=dwUIN?KEY_UIN:KEY_TUIN;
		DWORD dwSearch=dwUIN?dwUIN:dwTUIN;

		while (hContact) {
			if (!lstrcmpA(m_szModuleName,(char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact,(LPARAM)NULL))) {
				if (READC_D2(pszKey)==dwSearch) {
					// If dwTUIN exists and dwSearch==dwUIN, update TUIN
					if (dwTUIN!=0 && dwSearch==dwUIN && m_uins[dwTUIN]!=dwUIN) {
						m_uins[dwTUIN]=dwUIN;
						WRITEC_D(KEY_TUIN,dwTUIN);
						Log("%s(): Updated contact uin=%u tuin=%u",__FUNCTION__,dwUIN,dwTUIN);
					}
					return hContact;
				}
			}
			hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, (LPARAM)NULL);
		}

		if (!hContact && isAdd) {
			if (dwUIN==0) {
				char szTUIN[16];
				_ultoa(dwTUIN,szTUIN,10);
				m_protocol->callFunction(m_op->m_threads[THREAD_INNER], "OP_GetRealUIN",szTUIN);
				lua_getglobal(m_protocol->m_L,"ruin_result");
				if (lua_isnumber(m_protocol->m_L,-1)) {
					dwUIN=lua_tounsigned(m_protocol->m_L,-1);
					if (hContact=FindOrAddContact(dwUIN,dwTUIN)) {
						Log("%s(): Second-chance matching successful uin=%u tuin=%u",__FUNCTION__,dwUIN,dwTUIN);
						lua_pop(m_protocol->m_L,1);
						return hContact;
					} else {
						m_uins[dwTUIN]=dwUIN;
						Log("%s(): Retrieved contact uin=%u tuin=%u",__FUNCTION__,dwUIN,dwTUIN);
					}
				} else {
					Log("%s(): Failed retrieving contact uin for tuin=%u",__FUNCTION__,dwTUIN);
				}
				lua_pop(m_protocol->m_L,1);
			} else {
				m_uins[dwTUIN]=dwUIN;
			}
			if (hContact=(HANDLE)CallService(MS_DB_CONTACT_ADD, (WPARAM)NULL, (LPARAM)NULL)) {
				// Creation successful, associate protocol
				CallService(MS_PROTO_ADDTOCONTACT,(WPARAM)hContact,(LPARAM)m_szModuleName);
				WRITEC_D(KEY_UIN,dwUIN);
				WRITEC_D(KEY_TUIN,dwTUIN);
				DBWriteContactSettingByte(hContact,"CList","NotOnList",isTemp?1:0);
				DBWriteContactSettingByte(hContact,"CList","Hidden",isHidden?1:0);

				Log("%s(): Added contact uin=%u tuin=%u",__FUNCTION__,dwUIN,dwTUIN);
			}
		}

		return hContact;
	}

	HANDLE FindSessionContact(LPCSTR pszID) {
		HANDLE hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, (WPARAM)NULL, (LPARAM)NULL);
		DBVARIANT dbv;

		while (hContact) {
			if (!lstrcmpA(m_szModuleName,(char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact,(LPARAM)NULL))) {
				if (!READC_U8S2("sm_id",&dbv)) {
					if (!strcmp(dbv.pszVal,pszID)) {
						DBFreeVariant(&dbv);
						return hContact;
					}
					DBFreeVariant(&dbv);
				}
			}
			hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, (LPARAM)NULL);
		}

		return NULL;
	}

	int __cdecl GetAvatarInfo(WPARAM wParam, LPARAM lParam) {
		if (m_iStatus>ID_STATUS_OFFLINE) {
			PROTO_AVATAR_INFORMATION* lpPAI = (PROTO_AVATAR_INFORMATION*)lParam;
			HANDLE hContact=lpPAI->hContact;
			DWORD uin=READC_D2(KEY_UIN);
			DWORD tuin=READC_D2(KEY_TUIN);
			lua_State* L=m_op->m_threads[THREAD_AVATAR];

			lua_getglobal(L,"OP_avatardir");
			sprintf(lpPAI->filename,"%s\\%u.jpg",lua_tostring(L,-1),uin);
			lua_pop(L,1);

			Log("%s(): %u -> %s",__FUNCTION__,uin,lpPAI->filename);

			lpPAI->format=PA_FORMAT_JPEG;

			char szParam[MAX_PATH];
			// Code is for QQ Qun
			sprintf(szParam,"%d\t%u\t%u\t%u",READC_D2("IsQun"),tuin,uin,READC_D2("code"));
			Log("%s(): param=%s",__FUNCTION__,szParam);
			m_protocol->callFunction(L,"OP_GetAvatar",szParam);

			//return GetFileAttributesA(lpPAI->filename)==INVALID_FILE_ATTRIBUTES?GAIR_NOAVATAR:GAIR_SUCCESS;
			DWORD ret=GetFileAttributesA(lpPAI->filename);
			return ret==INVALID_FILE_ATTRIBUTES?GAIR_NOAVATAR:GAIR_SUCCESS;
		}
		return GAIR_NOAVATAR;
	}

	int __cdecl GetAvatarCaps(WPARAM wParam, LPARAM lParam) {
		switch (wParam) {
			case AF_MAXSIZE:
				((LPPOINT)lParam)->x=40;
				((LPPOINT)lParam)->y=40;
				return 0;
			case AF_PROPORTION:
				return PIP_SQUARE;
			case AF_FORMATSUPPORTED:
				return lParam==PA_FORMAT_JPEG?1:0;
			case AF_ENABLED:
				return 1;
			case AF_DONTNEEDDELAYS:
				return 1;
			case AF_MAXFILESIZE:
				return 0;
			case AF_DELAYAFTERFAIL:
				return 0;
			case AF_FETCHALWAYS:
				return 0;
		}
		return 0;
	}

	int __cdecl SetMyAvatar(WPARAM wParam, LPARAM lParam) {
		/*
		wParam=0
		lParam=(const char *)Avatar file name or NULL to remove the avatar
		return=0 for sucess
		*/
		return 1; // Not supported yet
	}

	int __cdecl GetMyAvatar(WPARAM wParam, LPARAM lParam) {
		if (m_iStatus>ID_STATUS_OFFLINE && m_protocol!=NULL) {
			HANDLE hContact=NULL;

			lua_getglobal(m_protocol->m_L,"OP_avatardir");
			mir_snprintf((LPSTR)wParam,lParam,"%s\\%s.jpg",lua_tostring(m_protocol->m_L,-1),m_uin);
			lua_pop(m_protocol->m_L,1);

			return 0;
		} else
			return 1;
	}

	// GetName(): Get Protocol Name to be displayed in tray icon
	// wParam: Max length of lParam
	// lParam: String to receive protocol name
	int __cdecl GetName(WPARAM wParam, LPARAM lParam) {
		char* pszResult=(char*)lParam;
		char* pszANSI=mir_u2a(m_tszUserName);
		mir_snprintf(pszResult,wParam,"%s",pszANSI);
		mir_free(pszANSI);
		//lstrcpynA((LPSTR)lParam, m_szModuleName, wParam);

		return 0;
	}

	// GetStatus(): Retrieve Prototol Status
	// wParam: N/A
	// lParam: N/A
	// Return: Current Status
	int __cdecl GetStatus(WPARAM wParam, LPARAM lParam) {
		return m_iStatus;
	}

	int __cdecl TestService(WPARAM wParam, LPARAM lParam) {
		m_protocol->callFunction(m_op->m_L,NULL);
		return 0;
	}

	int __cdecl ShowLogWindow(WPARAM wParam, LPARAM lParam) {
		if(::AllocConsole())
		{
			int hCrt = ::_open_osfhandle((intptr_t) ::GetStdHandle(STD_OUTPUT_HANDLE), _O_TEXT);
			FILE *hf = ::_fdopen( hCrt, "w" );
			*stdout = *hf;
			::setvbuf(stdout, NULL, _IONBF, 0 );

			hCrt = ::_open_osfhandle((intptr_t) ::GetStdHandle(STD_ERROR_HANDLE), _O_TEXT);
			hf = ::_fdopen( hCrt, "w" );
			*stderr = *hf;
			::setvbuf(stderr, NULL, _IONBF, 0 );
		}
		return 0;
	}

	void InitFontService() {
		// FontService
		FontIDW fid = {sizeof(fid)};
		wcscpy(fid.name,TranslateT("Messaging Font"));
		wcscpy(fid.group,m_tszUserName);
		strcpy(fid.dbSettingsGroup,m_szModuleName);
		strcpy(fid.prefix,"font1");
		fid.flags=FIDF_NOAS|FIDF_SAVEPOINTSIZE|FIDF_DEFAULTVALID|FIDF_ALLOWEFFECTS;

		// you could register the font at this point - getting it will get either the global default or what the user has set it 
		// to - but we'll set a default font:

		fid.deffontsettings.charset = GB2312_CHARSET;
		fid.deffontsettings.colour = RGB(0, 0, 0);
		fid.deffontsettings.size = 12;
		fid.deffontsettings.style = 0;
		wcsncpy(fid.deffontsettings.szFace, L"SimSun", LF_FACESIZE);
		CallService(MS_FONT_REGISTERW, (WPARAM)&fid, 0);

		/*
		wcscpy(fid.name,TranslateT("Qun Messaging Font"));
		strcpy(fid.prefix,"font2");
		CallService(MS_FONT_REGISTERW, (WPARAM)&fid, 0);
		*/
	}

	void SetFont() {
		if (m_protocol) {
			FontIDW fid = {sizeof(fid)};
			LOGFONTW font;
			wcscpy(fid.name,TranslateT("Messaging Font"));
			// wcscpy(fid.prefix,"font1");
			wcscpy(fid.group,m_tszUserName);
			CallService(MS_FONT_GETW,(WPARAM)&fid,(LPARAM)&font);
			COLORREF color=DBGetContactSettingDword(NULL,m_szModuleName,"font1Col",0);
			LPSTR pszFont=*font.lfFaceName?/*mir_utf8encodecp(font.lfFaceName,GetACP())*/mir_utf8encodeW(font.lfFaceName):mir_utf8encodeW(L"NSimSun"/*L"‘v‘Ì"*/);

			if (color!=0) color=(color&0xff)<<16|(color&0xff00)|(color>>16);
			
			int fontsize=(int)DBGetContactSettingByte(NULL,m_szModuleName,"font1Size",12);
			if (fontsize<9) fontsize=9;

			lua_State* L=m_protocol->m_L;
			lua_pushstring(L,pszFont);
			lua_setglobal(L,"OP_fontname");
			mir_free(pszFont);

			lua_pushnumber(L,fontsize);
			lua_setglobal(L,"OP_fontsize");

			char szTemp[16];
			sprintf(szTemp,"%08X",color);
			lua_pushstring(L,szTemp);
			lua_setglobal(L,"OP_fontcolor");

			lua_pushnumber(L,font.lfWeight>FW_NORMAL?1:0);
			lua_setglobal(L,"OP_fontbold");

			lua_pushnumber(L,font.lfItalic?1:0);
			lua_setglobal(L,"OP_fontitalic");

			lua_pushnumber(L,font.lfUnderline?1:0);
			lua_setglobal(L,"OP_fontunderline");
		}
	}

	int __cdecl OnContactDeleted(WPARAM wParam, LPARAM lParam) {
		if (m_iStatus>ID_STATUS_OFFLINE) {
			char* szProto;
			unsigned int uid;
			char is_qun;
			HANDLE hContact=(HANDLE)wParam;
			
			szProto=(char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, wParam, 0);
			if (!szProto || strcmp(szProto, m_szModuleName)) return 0;
		
			if (DBGetContactSettingByte(hContact,"CList","NotOnList",0)==1) return 0;
		
			uid=READC_D2(KEY_TUIN);
			is_qun=READC_D2("IsQun");
			
			char szUID[16];
			_ultoa(uid,szUID,10);
			
			if (uid && !is_qun) { // Remove general contact
				m_op->callFunction(m_op->m_threads[THREAD_GENERIC],"OP_DeleteUser",szUID);
			} else if (is_qun) { // Remove qun contact
				// TODO
			}
		}
	
	
		return 0;
	}
	
	void OnProtoLoad() {
		CHAR szTemp[MAX_PATH]={0};
		LPSTR pszTemp;
		CLISTMENUITEM mi={sizeof(mi)};
		int c2=0;
		HANDLE hContact=NULL;

		//add as a known module in DB Editor ++
		CallService("DBEditorpp/RegisterSingleModule",(WPARAM)m_szModuleName, 0);	

		if(READC_B2(KEY_SHOWCONSOLE)==1 && ::AllocConsole())
		{
			int hCrt = ::_open_osfhandle((intptr_t) ::GetStdHandle(STD_OUTPUT_HANDLE), _O_TEXT);
			FILE *hf = ::_fdopen( hCrt, "w" );
			*stdout = *hf;
			::setvbuf(stdout, NULL, _IONBF, 0 );

			hCrt = ::_open_osfhandle((intptr_t) ::GetStdHandle(STD_ERROR_HANDLE), _O_TEXT);
			hf = ::_fdopen( hCrt, "w" );
			*stderr = *hf;
			::setvbuf(stderr, NULL, _IONBF, 0 );
		}


		// m_enableBBCode=(ServiceExists(MS_IEVIEW_EVENT) && (DBGetContactSettingDword(NULL,"IEVIEW","GeneralFlags",0) & 1)); // TODO

		// Setup custom hook
		/*
		QHookEvent(ME_USERINFO_INITIALISE, &CNetwork::OnDetailsInit);
		QHookEvent(ME_OPT_INITIALISE, &CNetwork::OnOptionsInit);
		*/

		mi.popupPosition=500090000;
		mi.pszService=szTemp;
		mi.ptszName=m_tszUserName;
		mi.position=-1999901009;
		mi.ptszPopupName=(LPWSTR)-1;
		mi.flags=CMIF_ROOTPOPUP|CMIF_UNICODE;
		mi.hIcon=LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_ICON1));
		m_hMenuRoot=(HANDLE)CallService(MS_CLIST_ADDMAINMENUITEM, (WPARAM)0, (LPARAM)&mi);

		mi.flags=CMIF_CHILDPOPUP|CMIF_UNICODE;
		mi.ptszPopupName=(LPWSTR)m_hMenuRoot;
		mi.position=500090000;

		strcpy(szTemp,m_szModuleName);
		pszTemp=szTemp+strlen(szTemp);

	#define _CRMI(a,b,d,e,f) strcpy(pszTemp, a);m_services.push(QCreateService(szTemp, &CProtocol::b));mi.ptszName=d;e.push((HANDLE)CallService(f, 0, (LPARAM)&mi));mi.position+=5;
	#define CRMI(a,b,d) _CRMI(a,b,d,m_menuItems,MS_CLIST_ADDMAINMENUITEM)
			/*
			CRMI(QQ_MENU_CHANGENICKNAME,ChangeNickname,TranslateT("Change &Nickname"));
			CRMI(QQ_MENU_MODIFYSIGNATURE,ModifySignature,TranslateT("&Modify Personal Signature"));
			//CRMI(QQ_MENU_CHANGEHEADIMAGE,ChangeHeadImage,Translate("Change &Head Image"));
			CRMI(QQ_MENU_QQMAIL,QQMail,TranslateT("&QQ Mail"));
			CRMI(QQ_MENU_COPYIP,CopyMyIP,TranslateT("Show and copy my Public &IP"));
			*/
			// CRMI(QQ_MENU_DOWNLOADGROUP,DownloadGroup,TranslateT("&Download Group"));
			/*
			CRMI(QQ_MENU_UPLOADGROUP,UploadGroup,TranslateT("&Upload Group"));
			CRMI(QQ_MENU_REMOVENONSERVERCONTACTS,RemoveNonServerContacts,TranslateT("&Remove contacts not in Server"));
			CRMI(QQ_MENU_SUPPRESSADDREQUESTS,SuppressAddRequests,TranslateT("&Ignore any Add Requests"));
			CRMI(QQ_MENU_DOWNLOADUSERHEAD,DownloadUserHead,TranslateT("Re&download User Head"));
			CRMI(QQ_MENU_GETWEATHER,GetWeather,TranslateT("Get &Weather Information"));
			//CRMI(QQ_MENU_SUPPRESSQUN,SuppressQunMessages,Translate("&Suppress Qun Message Receive"));
			//CRMI(QQ_MENU_TOGGLEQUNLIST,ToggleQunList,Translate("&Toggle Qun List"));
			*/
	#ifdef TESTSERVICE
			CRMI("/TestService",TestService,TranslateT("Test Service"));
			CRMI("/ShowLogWindow",ShowLogWindow,TranslateT("Show Log Window"));
	#endif

			/*
	#ifdef MIRANDAQQ_IPC
			NotifyEventHooks(hIPCEvent,QQIPCEVT_CREATE_MAIN_MENU,(LPARAM)&mi);
	#endif
	*/
			// Context Menus
			mi.flags=CMIF_HIDDEN|CMIF_UNICODE;
			mi.position=-500050000;

	#define _CRMI2(a,b,d,e,f) strcpy(pszTemp, a);m_services.push(QCreateService(szTemp, &CProtocol::b));mi.ptszName=d;e.push_back((HANDLE)CallService(f, 0, (LPARAM)&mi));mi.position+=5;
	#define CRMI2(a,b,d) _CRMI2(a,b,d,m_contextMenuItems,MS_CLIST_ADDCONTACTMENUITEM)
			/*
			CRMI2(QQ_CNXTMENU_REMOVEME,RemoveMe,TranslateT("&Remove me from his/her list"));
			CRMI2(QQ_CNXTMENU_ADDQUNMEMBER,AddQunMember,TranslateT("&Add a member to Qun"));
			CRMI2(QQ_CNXTMENU_REAUTHORIZE,Reauthorize,TranslateT("Resend &authorization request"));
			CRMI2(QQ_CNXTMENU_CHANGECARDNAME,ChangeCardName,TranslateT("Change my Qun &Card Name"));
			*/
			// CRMI2(QQ_CNXTMENU_SILENTQUN,SilentQun,TranslateT("Toggle &Silent"));
			// CRMI2(QQ_CNXTMENU_POSTIMAGE,PostImage,TranslateT("Post &Image"));
			/*
			CRMI2(QQ_CNXTMENU_FORCEREFRESH,QunSpace,TranslateT("Qun &Space"));

			InitUpdater();
			*/
			/* TODO
		InitFoldersService();
		InitAssocManager();
		*/
	InitFontService();

	#define DECL(a) extern int a(WPARAM wParam, LPARAM lParam)
	// #define CSF(a,b) strcpy(pszTemp, a); this->QCreateService(szTemp,&CProtocol::b)
	#define CSF(a,b) strcpy(pszTemp, a); this->QCreateService(szTemp,&CProtocol::b)
	CSF(PS_GETAVATARINFO,GetAvatarInfo);
	CSF(PS_GETAVATARCAPS,GetAvatarCaps);
	CSF(PS_GETMYAVATAR,GetMyAvatar);
	CSF(PS_SETMYAVATAR,SetMyAvatar);
	CSF(PS_GETNAME,GetName);
	CSF(PS_GETSTATUS,GetStatus);
	/*
		CSF(PS_SETMYNICKNAME,SetMyNickname);
		/*

		CSF(PS_SET_LISTENINGTO,SetCurrentMedia);
		CSF(PS_GET_LISTENINGTO,GetCurrentMedia);
		*/
		CSF(PS_CREATEACCMGRUI,CreateAccMgrUI);

		SetContactsOffline();
		/* TODO

		*/
		// ResetContacts();
		

		//this->QHookEvent(ME_SYSTEM_MODULESLOADED, &CNetwork::OnModulesLoadedEx);
		this->QHookEvent(ME_DB_CONTACT_DELETED, &CProtocol::OnContactDeleted);
		// this->QHookEvent(ME_CLIST_PREBUILDCONTACTMENU, &CProtocol::OnPrebuildContactMenu);
		// this->QHookEvent(ME_USERINFO_INITIALISE, &CProtocol::OnDetailsInit);

	}



	//====================================================================================
	// PROTO_INTERFACE
	//====================================================================================
	HANDLE   __cdecl AddToList( int flags, PROTOSEARCHRESULT* psr ) {
		
		if (DWORD uid=wcstoul(psr->id,NULL,10)) {
			HANDLE hContact=FindOrAddContact(uid,uid,TRUE,flags&PALF_TEMPORARY,flags&PALF_TEMPORARY);
	
			WRITEC_TS("Find_ID",psr->id);
			WRITEC_U8S("Find_Nick",(char*)psr->nick);
			WRITEC_U8S("Find_FirstName",(char*)psr->firstName);
			WRITEC_U8S("Find_LastName",(char*)psr->lastName);
			WRITEC_U8S("Find_EMail",(char*)psr->email);
	
			return hContact;
		}
	
		return 0; 
	}

	HANDLE   __cdecl AddToListByEvent( int flags, int iContact, HANDLE hDbEvent ) {
		return NULL;
	}

	int      __cdecl Authorize( HANDLE hDbEvent ) {
		DBEVENTINFO dbei={sizeof(dbei)};
		unsigned int* uid;
		HANDLE hContact;
		LPSTR pszBlob;
	
		if ((dbei.cbBlob=CallService(MS_DB_EVENT_GETBLOBSIZE, (WPARAM)hDbEvent, 0))==-1) return 1;
	
		pszBlob=(LPSTR)(dbei.pBlob=(PBYTE)mir_alloc(dbei.cbBlob));
		if (CallService(MS_DB_EVENT_GET, (WPARAM)hDbEvent, (LPARAM)&dbei)) return 1;
		if (dbei.eventType!=EVENTTYPE_AUTHREQUEST) return 1;
		if (strcmp(dbei.szModule, m_szModuleName)) return 1;
	
		uid=(unsigned int*) dbei.pBlob;
	
		uid=(unsigned int*)dbei.pBlob;pszBlob+=sizeof(DWORD);
		hContact=*(HANDLE*)(dbei.pBlob+sizeof(DWORD));pszBlob+=sizeof(HANDLE);
	
	// pre.lParam=sizeof(DWORD)+sizeof(HANDLE)+strlen(pcszNick)+strlen(pcszFN)+strlen(pcszLN)+strlen(pcszEMail)+strlen(pcszMsg)+5;
		string str;
		char szUID[16];
		_ultoa(*uid,szUID,10);
		str.append(szUID);
		str.append("\t");
		str.append(READC_D2("IsQun")?"1":"0");
		str.append("\t");
		
		LPCSTR pszField=(LPCSTR)(&hContact+1);
		DBVARIANT dbv;
		if (!DBGetContactSettingUTF8String(hContact,"CList","MyHandle",&dbv)) {
			str.append(dbv.pszVal);
			DBFreeVariant(&dbv);
		} else {
			str.append(pszField); // Nick
		}
		str.append("\t");

		pszField+=strlen(pszField)+1;
		str.append(pszField); // FN
		str.append("\t");
		
		pszField+=strlen(pszField)+1;
		str.append(pszField); // LN
		str.append("\t");

		pszField+=strlen(pszField)+1;
		str.append(pszField); // EM
		
		m_op->callFunction(m_op->m_threads[THREAD_GENERIC],"OP_Authorize",str.c_str());
		mir_free(pszBlob);
		
		return 1;
	}

	int      __cdecl AuthDeny( HANDLE hDbEvent, const PROTOCHAR* szReason ) {
		DBEVENTINFO dbei={sizeof(dbei)};
		unsigned int* uid;
		HANDLE hContact;
		LPSTR pszBlob;
	
		if ((dbei.cbBlob=CallService(MS_DB_EVENT_GETBLOBSIZE, (WPARAM)hDbEvent, 0))==-1) return 1;
	
		pszBlob=(LPSTR)(dbei.pBlob=(PBYTE)mir_alloc(dbei.cbBlob));
		if (CallService(MS_DB_EVENT_GET, (WPARAM)hDbEvent, (LPARAM)&dbei)) return 1;
		if (dbei.eventType!=EVENTTYPE_AUTHREQUEST) return 1;
		if (strcmp(dbei.szModule, m_szModuleName)) return 1;
	
		uid=(unsigned int*) dbei.pBlob;
	
		uid=(unsigned int*)dbei.pBlob;pszBlob+=sizeof(DWORD);
		hContact=*(HANDLE*)(dbei.pBlob+sizeof(DWORD));pszBlob+=sizeof(HANDLE);
	
		string str;
		char szUID[16];
		_ultoa(*uid,szUID,10);
		str.append(szUID);
		str.append("\t");
		str.append(READC_D2("IsQun")?"1":"0");
		str.append("\t");
		
		LPCSTR pszField=(LPCSTR)(&hContact+1);
		DBVARIANT dbv;
		if (!DBGetContactSettingUTF8String(hContact,"CList","MyHandle",&dbv)) {
			str.append(dbv.pszVal);
			DBFreeVariant(&dbv);
		} else {
			str.append(pszField); // Nick
		}
		str.append("\t");

		pszField+=strlen(pszField)+1;
		str.append(pszField); // FN
		str.append("\t");
		
		pszField+=strlen(pszField)+1;
		str.append(pszField); // LN
		str.append("\t");

		pszField+=strlen(pszField)+1;
		str.append(pszField); // EM
		str.append("\t");
		
		LPSTR pszMsg=mir_utf8encode((LPSTR)szReason);
		str.append(pszMsg);
		mir_free(pszMsg);
		
		m_op->callFunction(m_op->m_threads[THREAD_GENERIC],"OP_Deny",str.c_str());
		mir_free(pszBlob);
		
		return 1;
	}

	int      __cdecl AuthRecv( HANDLE hContact, PROTORECVEVENT* pre) {
		if (pre) {
			DBEVENTINFO dbei;
	
			Log("%s(): Received authorization request",__FUNCTION__);
			// Show that guy
			DBDeleteContactSetting(hContact,"CList","Hidden");
	
			ZeroMemory(&dbei,sizeof(dbei));
			dbei.cbSize=sizeof(dbei);
			dbei.szModule=m_szModuleName;
			dbei.timestamp=pre->timestamp;
			dbei.flags=pre->flags & (PREF_CREATEREAD?DBEF_READ:0);
			if (pre->flags & PREF_UTF) dbei.flags|=DBEF_UTF;
			dbei.eventType=EVENTTYPE_AUTHREQUEST;
			dbei.cbBlob=pre->lParam;
			dbei.pBlob=(PBYTE)pre->szMessage;
			CallService(MS_DB_EVENT_ADD,(WPARAM)NULL,(LPARAM)&dbei);
		}
		return 0;
	}

	int      __cdecl AuthRequest( HANDLE hContact, const PROTOCHAR* szMessage ) {
		char* keys[]={
			"Find_ID",
			"Find_Nick",
			"Find_FirstName",
			"Find_LastName",
			"Find_EMail",
			NULL
		};
		string str;
		DBVARIANT dbv;
		
		for (char** pKey=keys; *pKey; pKey++) {
			READC_U8S2(*pKey,&dbv);
			if (str.length()>0) str.append("\t"); // str+="\t";
			str.append(dbv.pszVal); //+=dbv.pszVal;
			DBFreeVariant(&dbv);
		}

		char* pszMessage=mir_utf8encode((LPCSTR)szMessage); //mir_utf8encodeW(szMessage);
		
		str.append("\t"); //+="\t";
		str.append(pszMessage);// +=pszMessage;
		mir_free(pszMessage);

		m_op->callFunction(m_op->m_threads[THREAD_GENERIC],"OP_AddUser",str.c_str());
		
		return 0;
	}

	HANDLE   __cdecl ChangeInfo( int iInfoType, void* pInfoData ) {
		return NULL;
	}

	HANDLE   __cdecl FileAllow( HANDLE hContact, HANDLE hTransfer, const PROTOCHAR* szPath ) {
		return NULL;
	}

	int      __cdecl FileCancel( HANDLE hContact, HANDLE hTransfer ) {
		return 0;
	}

	int      __cdecl FileDeny( HANDLE hContact, HANDLE hTransfer, const PROTOCHAR* szReason ) {
		return 0;
	}

	int      __cdecl FileResume( HANDLE hTransfer, int* action, const PROTOCHAR** szFilename ) {
		return 0;
	}

	DWORD_PTR __cdecl GetCaps( int type, HANDLE hContact = NULL ) {
		switch (type) {
			case PFLAGNUM_1:
				return PF1_IM | PF1_SERVERCLIST | PF1_CHAT | PF1_BASICSEARCH | PF1_ADDSEARCHRES/* | PF1_ADDED | PF1_BASICSEARCH | PF1_SEARCHBYEMAIL | PF1_SEARCHBYNAME | PF1_NUMERICUSERID | PF1_ADDSEARCHRES | PF1_AUTHREQ*/ | PF1_MODEMSG | PF1_FILE /*| PF1_BASICSEARCH*/;
			case PFLAGNUM_2: // Possible Status
				return PF2_ONLINE | PF2_INVISIBLE | PF2_SHORTAWAY | PF2_LONGAWAY | PF2_LIGHTDND | PF2_HEAVYDND | PF2_FREECHAT; // | PF2_LONGAWAY | PF2_LIGHTDND; // PF2_SHORTAWAY=Away
				break;
			case PFLAGNUM_3:
				return PF2_ONLINE | PF2_INVISIBLE | PF2_SHORTAWAY | PF2_LONGAWAY | PF2_LIGHTDND | PF2_HEAVYDND | PF2_FREECHAT; // | PF2_LONGAWAY | PF2_LIGHTDND; // Status that supports mode message
				break;
			case PFLAGNUM_4: // Additional Capabilities
				return PF4_FORCEAUTH | PF4_FORCEADDED/* | PF4_NOCUSTOMAUTH*/ | PF4_AVATARS | PF4_IMSENDUTF | PF4_OFFLINEFILES | PF4_IMSENDOFFLINE | PF4_SUPPORTTYPING; // PF4_FORCEADDED="Send you were added" checkbox becomes uncheckable
				break;
			case PFLAG_UNIQUEIDTEXT: // Description for unique ID (For search use)
				return (intptr_t)Translate("UIN");
			case PFLAG_UNIQUEIDSETTING: // Where is my Unique ID stored in?
				return (intptr_t)KEY_UIN;
			case PFLAG_MAXLENOFMESSAGE: // Maximum message length
				return 0;
			default:
				return 0;
		}
	}

	HICON     __cdecl GetIcon( int iconIndex ) {
		return (iconIndex & 0xFFFF)==PLI_PROTOCOL?(HICON)LoadImage(g_hInst, MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, GetSystemMetrics(iconIndex&PLIF_SMALL?SM_CXSMICON:SM_CXICON), GetSystemMetrics(iconIndex&PLIF_SMALL?SM_CYSMICON:SM_CYICON), 0):0;
	}

	int       __cdecl GetInfo( HANDLE hContact, int infoType ) {
		return 0;
	}

	typedef struct {
		COpenProtocol* op;
		LPSTR pszUIN;
	} SEARCHBASIC, *PSEARCHBASIC, *LPSEARCHBASIC;

	void __cdecl SearchBasicThread(LPVOID lpParameter) {
		LPSEARCHBASIC lpSB=(LPSEARCHBASIC)lpParameter;
		
		lpSB->op->callFunction(lpSB->op->m_threads[THREAD_GENERIC],"OP_SearchBasic",lpSB->pszUIN);
		mir_free(lpSB->pszUIN);
		mir_free(lpSB);
	}

	HANDLE    __cdecl SearchBasic( const PROTOCHAR* id ) {
		// Probe for type
		LPSTR pszStr=NULL;
		LPCSTR pcszStr=(LPCSTR)id;
		if (pcszStr[1]==0) {
			// Second byte is null, either the string is 1 byte or wide character
			pszStr=mir_u2a((LPWSTR)id);
		} else {
			pszStr=mir_strdup((LPSTR)id);
		}
		
		LPSEARCHBASIC lpSB=(LPSEARCHBASIC)mir_alloc(sizeof(SEARCHBASIC));
		lpSB->op=m_op;
		lpSB->pszUIN=pszStr;
		CreateThreadObj(&CProtocol::SearchBasicThread,lpSB);
		
		return (HANDLE)1;
	}

	HANDLE    __cdecl SearchByEmail( const PROTOCHAR* email ) {
		return 0;
	}

	HANDLE    __cdecl SearchByName( const PROTOCHAR* nick, const PROTOCHAR* firstName, const PROTOCHAR* lastName ) {
		return 0;
	}

	HWND      __cdecl SearchAdvanced( HWND owner ) {
		return 0;
	}

	HWND      __cdecl CreateExtendedSearchUI( HWND owner ) {
		return 0;
	}


	int       __cdecl RecvContacts( HANDLE hContact, PROTORECVEVENT* ) {
		return 0;
	}

	int       __cdecl RecvFile( HANDLE hContact, PROTOFILEEVENT* ) {
		return 0;
	}

	int       __cdecl RecvMsg( HANDLE hContact, PROTORECVEVENT* evt) {
		CCSDATA ccs={hContact, PSR_MESSAGE, 0, ( LPARAM )evt};
		return CallService(MS_PROTO_RECVMSG, 0, ( LPARAM )&ccs);
	}

	int       __cdecl RecvUrl( HANDLE hContact, PROTORECVEVENT* ) {
		return 0;
	}


	int       __cdecl SendContacts( HANDLE hContact, int flags, int nContacts, HANDLE* hContactsList ) {
		return 0;
	}

	HANDLE    __cdecl SendFile( HANDLE hContact, const PROTOCHAR* szDescription, PROTOCHAR** ppszFiles ) {
		char* file=*(char**)ppszFiles;
		// char* afile=NULL;
		LPWSTR pwszFile=NULL;

		if (ppszFiles[1]!=NULL) {
			MessageBoxW(NULL,TranslateT("Only 1 file is allowed"),NULL,MB_ICONERROR);
			return 0;
		}

		if (true || READC_D2("IsQun")==1) {
			// Qun
			HWND hWndFT=NULL;
			if (CallService(MS_SYSTEM_GETVERSION,0,0)<0x00090000)
				SendMessage(GetForegroundWindow(),WM_CLOSE,0,0);
			else
				hWndFT=GetForegroundWindow();

			if (file[1]==0) {
				// Miranda IM 0.9: ppszFiles maybe in Unicode
				//afile=mir_u2a_cp((LPWSTR)file,GetACP());
				//file=afile;
				pwszFile=mir_wstrdup((LPWSTR)file);
			} else {
				pwszFile=mir_a2u(file);
			}

			// Test file first
			LPWSTR pszExt=wcsrchr(pwszFile,'.')+1;
			if (wcsicmp(pszExt,L"bmp") && wcsicmp(pszExt,L"jpg") && wcsicmp(pszExt,L"gif") && wcsicmp(pszExt,L"png")) {
				MessageBox(NULL,TranslateT("Warning! Sending non-GIF nor JPG files to qun can only be received by clients using MirandaQQ."),NULL,MB_ICONEXCLAMATION);
				//ForkThread((ThreadFunc)&CNetwork::ThreadMsgBox,mir_tstrdup(TranslateT("Warning! Sending non-GIF nor JPG files to qun can only be received by clients using MirandaQQ.")));
				//return;
			} 
			/*
			HANDLE hFile=CreateFileW(pwszFile,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);
			//if (GetFileAttributesA(file)==INVALID_FILE_ATTRIBUTES) {
			if (hFile==INVALID_HANDLE_VALUE) {
				ForkThread((ThreadFunc)&CNetwork::ThreadMsgBox,mir_tstrdup(TranslateT("Failed sending qun image because the file is inaccessible")));
				return 0;
			} else*/ {
				// CloseHandle(hFile);
				/*
				wstring str=L"[img]";
				str.append(pwszFile);
				str.append(L"[/img]");
				*/
				string str="[img]";
				char* pszFile=mir_utf8encodeW(pwszFile);
				str.append(pszFile);
				str.append("[/img]");
				mir_free(pszFile);
				
				LPWSTR pszStr=mir_utf8decodeW(str.c_str());
				
				CallService(MS_MSG_SENDMESSAGE"W",(WPARAM)hContact,(LPARAM)pszStr);
				mir_free(pszStr);
			}

			if (pwszFile) mir_free(pwszFile);
			if (hWndFT) PostMessage(hWndFT,WM_CLOSE,0,0);
		} else {
			MessageBoxW(NULL,TranslateT("This MirandaQQ2 build does not support file transfer."),NULL,MB_ICONERROR);
		}
		return 0;
	}

	typedef struct {
		HANDLE hContact;
		lua_State* L;
		int flags;
		DWORD seq;
		LPSTR msg;
	} SENDMSG, *PSENDMSG, *LPSENDMSG;

	void __cdecl SendMsgThread(LPVOID lpParameter) {
		LPSENDMSG lpSM=(LPSENDMSG)lpParameter;
		HANDLE hContact=lpSM->hContact;
		LPSTR msg=lpSM->msg;
		int flags=lpSM->flags;

		if (flags&PREF_UTF) {
			bool isQun=READC_D2("IsQun")!=0;
			bool isDiscu=READC_D2("IsDiscu")!=0;
			bool isSession=READC_D2("IsSession")!=0;
			DWORD dwTUIN=READC_D2(isSession?"sm_tuin":KEY_TUIN);
			LPSTR pszQunImageDir=NULL;

			lua_getglobal(lpSM->L,"OP_qunimagedir");
			if (lua_isstring(lpSM->L,-1)) {
				pszQunImageDir=mir_strdup(lua_tostring(lpSM->L,-1));
			}
			lua_pop(lpSM->L,1);

			string str;

			LPSTR pszMarker;
			LPSTR psz2;
			LPSTR psz3;
			stack<string> tempimagelist;
			char szTempImageFile[MAX_PATH];
			int tempImageFileIndex=1;

			char msg2[64];
			if (isQun)
				sprintf(msg2,"%u\t1\t%u\t",dwTUIN,READC_D2("code"));
			else if (isSession)
				sprintf(msg2,"%u\t3\t%u_%u\t",dwTUIN,READC_D2("sm_gtuin"),dwTUIN);
			else
				sprintf(msg2,"%u\t%d\t0\t",dwTUIN,isDiscu?2:0);

			str.append(msg2);

			while (pszMarker=strchr(msg,'[')) {
				psz2=strchr(pszMarker,']');
				psz3=strchr(pszMarker+1,'[');

				if (psz2==NULL) {
					// No close tag
					break;
				} else if (psz3!=NULL && psz3<psz2) {
					// Reopen tag
					*psz3=0;
					str.append(msg);
					str.append("[");
					msg=psz3+1;
					continue;
				}

				*pszMarker=0;
				str.append(msg);

				if (!strncmp(pszMarker+1,"face:",5)) {
					// *pszMarker=0;
					// str.append(msg);
					msg=pszMarker+1;
					*psz2=0;
					str.append("\t[");
					str.append(msg);
					// str.append("]");
					str.append("\t");
					msg=psz2+1;
				} else if (!strncmp(pszMarker+1,"img]",4)) {
					// *pszMarker=0;
					// str.append(msg);

					if (psz2=strstr(pszMarker+1,"[/img]")) {
						// Valid tags
						str.append("\t[img]");
						msg=pszMarker+5;
						*psz2=0;
						if (GetFileAttributesA(msg)==INVALID_FILE_ATTRIBUTES) {
							WCHAR wszFileName[MAX_PATH];
							WCHAR wszTempImageFile[MAX_PATH];
							MultiByteToWideChar(CP_UTF8,0,msg,-1,wszFileName,MAX_PATH);
							if (GetFileAttributesW(wszFileName)==INVALID_FILE_ATTRIBUTES) {
								ProtoBroadcastAck(m_szModuleName, hContact, ACKTYPE_MESSAGE, ACKRESULT_FAILED, (HANDLE)lpSM->seq, (LPARAM)"Image file is not accessible");
								return;
							}

							sprintf(szTempImageFile,"%s\\tmp_%d%s",pszQunImageDir,tempImageFileIndex++,strrchr(msg,'.'));
							swprintf(wszTempImageFile,L"%S",szTempImageFile);
							CopyFile(wszFileName,wszTempImageFile,FALSE);
							tempimagelist.push(string(szTempImageFile));
							str.append(szTempImageFile);
						} else
							str.append(msg);
						str.append("\t");
						msg=psz2+6;
					} else {
						// Invalid tags
						str.append("[img]");
						msg+=5;
					}
				} else {
					// Unmatched tags
					// *pszMarker=0;
					// str.append(msg);
					str.append("[");
					msg=pszMarker+1;
				}
			}

			if (msg && *msg) {
				str.append(msg);
			}
			str.append(" ");

			m_op->callFunction(lpSM->L,"OP_SendMessage",str.c_str());

			while (tempimagelist.size()) {
				DeleteFileA(tempimagelist.top().c_str());
				tempimagelist.pop();
			}
			/*
			LPSTR msg2=(LPSTR)mir_alloc(strlen(msg)+20);
			sprintf(msg2,"%u\t%d\t%s",dwTUIN,isQun?1:isDiscu?2:0,msg);

			m_op->callFunction("OP_SendMessage",msg2,TRUE);
			mir_free(msg2);
			*/
		}

		ProtoBroadcastAck(m_szModuleName, hContact, ACKTYPE_MESSAGE, ACKRESULT_SUCCESS, (HANDLE)lpSM->seq, 0);
		mir_free(lpSM->msg);
		mir_free(lpSM);
	}

	int       __cdecl SendMsg( HANDLE hContact, int flags, const char* msg ) {
		if (m_iStatus>ID_STATUS_OFFLINE && m_protocol!=NULL) {
			LPSENDMSG lpSM=(LPSENDMSG)mir_alloc(sizeof(SENDMSG));
			DWORD seq=GetTickCount();

			lpSM->hContact=hContact;
			lpSM->L=m_protocol->m_threads[THREAD_SENDMESSAGE];
			lpSM->flags=flags;
			lpSM->msg=mir_strdup(msg);
			lpSM->seq=seq;
			CreateThreadObj(&CProtocol::SendMsgThread,lpSM);

			return seq;
		} else
			return 0;
	}

	int       __cdecl SendUrl( HANDLE hContact, int flags, const char* url ) {
		return 0;
	}


	int       __cdecl SetApparentMode( HANDLE hContact, int mode ) {
		return 0;
	}

	int       __cdecl SetStatus( int iNewStatus ) {
		m_iDesiredStatus=iNewStatus;

		if (m_iStatus==ID_STATUS_OFFLINE && iNewStatus!=ID_STATUS_OFFLINE) {
			char szTemp[MAX_PATH];
			char szUIN[MAX_PATH];
			char szPW[MAX_PATH];
			DBVARIANT dbv;
			HANDLE hContact=NULL;

			if (!READC_S2(KEY_UIN,&dbv)) {
				strcpy(szUIN,dbv.pszVal);
				DBFreeVariant(&dbv);
			} else {
				ShowNotification(TranslateT("UIN not entered!"),NIIF_ERROR);
				return 1;
			}

			if (!READC_S2(KEY_PW,&dbv)) {
				strcpy(szPW,dbv.pszVal);
				CallService(MS_DB_CRYPT_DECODESTRING,sizeof(szPW),(LPARAM)szPW);
				DBFreeVariant(&dbv);
			} else {
				ShowNotification(TranslateT("Password not entered!"),NIIF_ERROR);
				return 1;
			}

			if (!READC_S2(KEY_BOOTSTRAP,&dbv)) {
				strcpy(szTemp,dbv.pszVal);
				DBFreeVariant(&dbv);
			} else {
				strcpy(szTemp,VAL_BOOTSTRAP);
			}

			if (_access(szTemp,0)!=0) {
				ShowNotification(TranslateT("Bootstrap script inaccessable!"),NIIF_ERROR);
				return 1;
			}

			BroadcastStatus(ID_STATUS_CONNECTING);

			m_op=new COpenProtocol(szTemp,this);
			if (m_uin) mir_free(m_uin);
			m_uin=mir_strdup(szUIN);
			m_op->setLogin(szUIN,szPW);
			m_op->setInitStatus(iNewStatus);

			GetModuleFileNameA(NULL,szTemp,MAX_PATH);
			strcpy(strrchr(szTemp,'\\')+1,"OpenProtocol");
			CreateDirectoryA(szTemp,NULL);
			strcat(szTemp,"\\Avatars");
			CreateDirectoryA(szTemp,NULL);
			m_op->setAvatarPath(szTemp);

			if (READC_B2(KEY_RECEIVEGCIMAGES)==1) {
				int port=READC_W2(KEY_SERVERPORT);
				strcpy(strrchr(szTemp,'\\')+1,"GroupImages");
				CreateDirectoryA(szTemp,NULL);
				m_op->setQunImagePath(szTemp,port==0?VAL_SERVERPORT:port);
			}

			m_op->setBBCode(ServiceExists(MS_IEVIEW_EVENT) && (DBGetContactSettingDword(NULL,"IEVIEW","GeneralFlags",0) & 1));
			SetFont();

			if (READC_B2("NLUseProxy")==1) {
				int type;
				switch (READC_B2("NLProxyType")) {
#ifdef LIBCURL
					case PROXYTYPE_HTTP: type=CURLPROXY_HTTP; break;
					case PROXYTYPE_SOCKS4: type=CURLPROXY_SOCKS4; break;
					case PROXYTYPE_SOCKS5: type=CURLPROXY_SOCKS5; break;
#else
					case PROXYTYPE_HTTP: type=1; break;
					case PROXYTYPE_SOCKS4: type=2; break;
					case PROXYTYPE_SOCKS5: type=3; break;
#endif
				}
				
				char szProxyUrl[MAX_PATH]={0};
				char szProxyUserPwd[MAX_PATH]={0};

				if (!READC_S2("NLProxyServer",&dbv)) {
					sprintf(szProxyUrl,"%s:%d",dbv.pszVal,READC_W2("NLProxyPort"));
					DBFreeVariant(&dbv);

					if (READC_B2("NLUseProxyAuth")==1) {
						if (!READC_S2("NLProxyAuthUser",&dbv)) {
							strcpy(szProxyUserPwd,dbv.pszVal);
							DBFreeVariant(&dbv);
							strcat(szProxyUserPwd,":");

							READC_S2("NLProxyAuthUser",&dbv);
							strcat(szProxyUserPwd,dbv.pszVal);
							DBFreeVariant(&dbv);
						}
					}

					m_op->setProxy(type,szProxyUrl,szProxyUserPwd);
				}

			}

			m_op->start();
		} else if (m_iStatus>ID_STATUS_OFFLINE) {
			char szStatus[16];
			m_iDesiredStatus=iNewStatus;
			itoa(iNewStatus,szStatus,10);
			if (iNewStatus==ID_STATUS_OFFLINE) m_op->signalInterrupt();
			m_op->callFunction(m_op->m_threads[THREAD_GENERIC],"OP_ChangeStatus",szStatus);
			BroadcastStatus(iNewStatus);

			/*
			if (iNewStatus==ID_STATUS_OFFLINE) {
				delete m_op;
				m_op=NULL;
			}
			*/
		}
		return 0;
	}


	HANDLE    __cdecl GetAwayMsg( HANDLE hContact ) {
		return 0;
	}

	int       __cdecl RecvAwayMsg( HANDLE hContact, int mode, PROTORECVEVENT* evt ) {
		return 0;
	}

	int       __cdecl SendAwayMsg( HANDLE hContact, HANDLE hProcess, const char* msg ) {
		return 0;
	}

	int       __cdecl SetAwayMsg( int iStatus, const PROTOCHAR* msg ) {
		return 0;
	}


	int       __cdecl UserIsTyping( HANDLE hContact, int type ) {
		return 0;
	}


	int       __cdecl OnEvent( PROTOEVENTTYPE iEventType, WPARAM wParam, LPARAM lParam ) {
		Log("%s(iEventType=%d, wParam=%p, lParam=%p)",__FUNCTION__,iEventType,wParam,lParam);
		switch (iEventType) {
			case EV_PROTO_ONLOAD: OnProtoLoad(); break;
			case EV_PROTO_ONOPTIONS: Log("%s(EV_PROTO_ONOPTIONS) Stub!",__FUNCTION__); break;
			case EV_PROTO_ONMENU: Log("%s(EV_PROTO_ONMENU) Stub!",__FUNCTION__); break;
			case EV_PROTO_ONRENAME:
				/* TODO
				if (m_hMenuRoot) {
					CLISTMENUITEM clmi={sizeof(clmi)};
					clmi.flags=CMIM_NAME|CMIF_UNICODE;
					clmi.ptszName=m_tszUserName;
					CallService(MS_CLIST_MODIFYMENUITEM, (WPARAM)m_hMenuRoot, (LPARAM)&clmi);
				}
				*/
				break;
			case EV_PROTO_ONEXIT:
				/*
				if (Miranda_Terminated()) {
					if (g_httpServer) {
						delete g_httpServer;
						g_httpServer=NULL;
					}
				}
				*/
			// Stub
			case EV_PROTO_ONREADYTOEXIT:
			case EV_PROTO_ONERASE:
				break;
		}
		return 1; 
	}


	// COpenProtocolHandler
	LPSTR oph_strdup(LPCSTR pcszStr) {
		return mir_strdup(pcszStr);
	}
	LPVOID oph_malloc(int cbAllocation) {
		return mir_alloc(cbAllocation);
	}
	void oph_free(LPVOID pData) {
		mir_free(pData);
	}
	void oph_printdebug(LPCSTR pcszStr) {
		printf(pcszStr);
		// fprintf(stdout,"%s",pcszStr);
	}
	void oph_pthread_create(void(*start_routine)(LPVOID), LPVOID arg) {
		_beginthread(start_routine,0,arg);
	}

	// copied from groups.c - horrible, but only possible as this is not available as service
	int FindGroupByName(LPCSTR name)
	{
	  char idstr[16];
	  DBVARIANT dbv;

	  for(int i=0;;i++)
	  {
		itoa(i,idstr,10);
		if(DBGetContactSettingUTF8String(NULL,"CListGroups",idstr,&dbv)) return -1;
		if(!strcmp(dbv.pszVal+1,name)) 
		{
		  DBFreeVariant(&dbv);
		  return i;
		}
		DBFreeVariant(&dbv);
	  }
	  return -1;
	}

	void freeQunMembers(DWORD dwTUIN) {
		LPLINKEDLIST lpLL=m_qunmembers[dwTUIN];
		if (lpLL) {
			LPLINKEDLIST lpLL2;
			m_qunmembers.erase(dwTUIN);

			while (lpLL) {
				lpLL2=lpLL->next;
				for (int c=0; c<5; c++) {
					if (lpLL->values[c]) free(lpLL->values[c]);
				}
				free(lpLL);
				lpLL=lpLL2;
			}
		}
	}

	LPCSTR findQunMember(DWORD dwTUIN, DWORD dwTUIN2) {
		LPLINKEDLIST lpLL=m_qunmembers[dwTUIN];
		while (lpLL) {
			if (lpLL->key==dwTUIN2)
				return (LPCSTR)lpLL->values[0];
			else
				lpLL=lpLL->next;
		}

		return NULL;
	}

	int handler(int nEvent, LPCSTR pcszStatus, LPCVOID pAux) {
		switch (nEvent) {
			case OPEVENT_LOGINSUCCESS:
				BroadcastStatus(m_iDesiredStatus);
				m_iDesiredStatus=0;
				break;
			case OPEVENT_ERROR:
				if (strstr(pcszStatus,": ERR") || !strncmp(pcszStatus,"ERR",3)) {
					LPCSTR pcszCode=strstr(pcszStatus,"ERR")+3;

					if (!strncmp(pcszCode,"RESP",4)||!strncmp(pcszCode,"BRPY",4)) {
						ShowNotification(TranslateT("Server responded with invalid response"),NIIF_ERROR);
						MessageBoxA(NULL,pcszCode+4,NULL,MB_ICONERROR);
						// if (pAux==NULL) BroadcastStatus(ID_STATUS_OFFLINE);
					} else if (!strncmp(pcszCode,"NOVC",4)) {
						ShowNotification(TranslateT("Verification Code cancelled by user"),NIIF_ERROR);
						ProtoBroadcastAck(m_szModuleName,NULL,ACKTYPE_LOGIN,ACKRESULT_FAILED,NULL,LOGINERR_WRONGPASSWORD);
					} else if (!strncmp(pcszCode,"LOGN",4)) {
						LPWSTR pszMsg=(LPWSTR)oph_malloc((strlen(pcszStatus)+1)*2);
						MultiByteToWideChar(CP_UTF8,0,pcszCode+4,-1,pszMsg,(int)strlen(pcszStatus)+1);
						ShowNotification(pszMsg,NIIF_ERROR);
						oph_free(pszMsg);
						ProtoBroadcastAck(m_szModuleName,NULL,ACKTYPE_LOGIN,ACKRESULT_FAILED,NULL,LOGINERR_WRONGPASSWORD);
					} else if (!strncmp(pcszCode,"MESG",4)) {
						LPWSTR pszMsg=(LPWSTR)oph_malloc((strlen(pcszStatus)+1)*2);
						MultiByteToWideChar(CP_UTF8,0,pcszCode+4,-1,pszMsg,(int)strlen(pcszStatus)+1);
						ShowNotification(pszMsg,NIIF_ERROR);
						oph_free(pszMsg);
						return 1; // 1=Resume
					} else if (!strncmp(pcszCode,"LOG2",4)) {
						ShowNotification(TranslateT("Channel Login (Part 2) failed"),NIIF_ERROR);
						ProtoBroadcastAck(m_szModuleName,NULL,ACKTYPE_LOGIN,ACKRESULT_FAILED,NULL,LOGINERR_WRONGPROTOCOL);
					} else if (!strncmp(pcszCode,"POLL",4)) {
						LPWSTR pszMsg=(LPWSTR)oph_malloc((strlen(pcszStatus)+1)*2);
						MultiByteToWideChar(CP_UTF8,0,pcszCode+4,-1,pszMsg,(int)strlen(pcszStatus)+1);
						ShowNotification(pszMsg,NIIF_ERROR);
						oph_free(pszMsg);
						BroadcastStatus(ID_STATUS_OFFLINE);
					} else if (!strncmp(pcszCode,"DUPL",4)) {
						ShowNotification(TranslateT("You have logged on from another location"),NIIF_ERROR);
						ProtoBroadcastAck(m_szModuleName,NULL,ACKTYPE_LOGIN,ACKRESULT_FAILED,NULL,LOGINERR_OTHERLOCATION);
					} else if (!strncmp(pcszCode,"LOFF",4)) {
						// Regular logoff
						// BroadcastStatus(ID_STATUS_OFFLINE);
						Log("Log off");
					}
					BroadcastStatus(ID_STATUS_OFFLINE);
				} else {
					// Script error, maybe possible to continue
					LPWSTR pszMsg=(LPWSTR)oph_malloc((strlen(pcszStatus)+1)*2);
					MultiByteToWideChar(CP_UTF8,0,pcszStatus,-1,pszMsg,(int)strlen(pcszStatus)+1);
					MessageBox(NULL,pszMsg,NULL,MB_ICONERROR);
					oph_free(pszMsg);

					if (pAux==NULL) BroadcastStatus(ID_STATUS_OFFLINE);
					// return 1;
				}
				break;
			case OPEVENT_LOCALGROUP:
				{
					int index=*(int*)pAux;
					int id;//=FindGroupByName(pcszStatus);
					/*if (id==-1)*/ {
						LPWSTR pwszGroup=mir_utf8decodeW(pcszStatus);
						id=CallService(MS_CLIST_GROUPCREATE,0,(LPARAM)pwszGroup);
						mir_free(pwszGroup);
					}
					m_localgroups[index]=id;
				}
				break;
			case OPEVENT_CONTACTINFO:
				{
					DWORD tuin=*(LPDWORD)pcszStatus;
					BOOL isMe=(tuin==strtoul(m_uin,NULL,10));

					for (int cc=0; (cc==0 || (cc==1&&isMe)); cc++) {
						int face=-1;
						int flag=-1;
						LPSTR pszNick=NULL;
						LPCSTR pszKey;
						LPCSTR pszValue;
						HANDLE hContact=NULL;
						lua_State* L=(lua_State*) pAux;
						BOOL isQun=FALSE;
						BOOL isSession=FALSE;
						int nNext;
						int infolevel=0;

						// { face, flag, nick, groupflag, group, MyHandle, is_vip, vip_level }
						if (cc==0) {
							lua_pushnil(L); // For first key

							while ((nNext=lua_next(L,2))!=0) {
								pszKey=lua_tostring(L,-2);
								pszValue=lua_tostring(L,-1);
								printf("%s(): tuin=%u %s=%s\n",__FUNCTION__,tuin,pszKey,pszValue);

								if (!strcmp(pszKey,"face")) {
									face=atoi(pszValue);
									infolevel++;
								} else if (!strcmp(pszKey,"IsQun")) {
									isQun=TRUE;
								} else if (!strcmp(pszKey,"IsSession")) {
									isSession=TRUE;
								} else if (!strcmp(pszKey,"flag")) {
									flag=atoi(pszValue);
									infolevel++;
								} else if (!strcmp(pszKey,"Nick")) {
									pszNick=strdup(pszValue);
									infolevel++;
								}

								lua_pop(L,1); // Remove value and keep key

								if (infolevel==3) break;
							}
						}

						if (cc==0 && !isSession && pszNick) {
							// Match
							if ((face!=-1||isQun) && flag!=-1) {
								DBVARIANT dbv;
								hContact=FindOrAddContact(0,tuin);
								if (hContact && (isQun || READC_D2("face")==face) && READC_D2("flag")==flag) {
									if (!READC_U8S2("Nick",&dbv)) {
										if (!strcmp(dbv.pszVal,pszNick)) {
											DBFreeVariant(&dbv);
											Log("%s(): tuin identical for uin=%u!",__FUNCTION__,READC_D2(KEY_UIN));
											m_uins[tuin]=READC_D2(KEY_UIN);
										} else
											hContact=NULL;
										DBFreeVariant(&dbv);
									} else
										hContact=NULL;
								} else
									hContact=NULL;

								if (!hContact) {
									hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, (WPARAM)NULL, (LPARAM)NULL);

									while (hContact) {
										if (!lstrcmpA(m_szModuleName,(char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact,(LPARAM)NULL))) {
											if ((isQun || READC_D2("face")==face) && READC_D2("flag")==flag) {
												if (!READC_U8S2("Nick",&dbv)) {
													if (!strcmp(dbv.pszVal,pszNick)) {
														DBFreeVariant(&dbv);
														Log("%s(): Found matching contact for uin=%u! tuin %u->%u",__FUNCTION__,READC_D2(KEY_UIN),READC_D2(KEY_TUIN),tuin);
														WRITEC_D(KEY_TUIN,tuin);
														break; // Found!
													}
													DBFreeVariant(&dbv);
												}
											}
										}
										hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, (LPARAM)NULL);
									}
								}
							}

							if (!hContact) hContact=FindOrAddContact(0,tuin,TRUE,FALSE,FALSE);
						} else if (cc==1) {
							hContact=NULL;
						} else {
							hContact=FindOrAddContact(0,tuin);
						}

						if (cc==1 || hContact) {
							// Force rewind
							// if (nNext!=0) lua_pop(L,1);
							lua_pushnil(L); // For first key

							int type;
							DWORD dwValue;

							while (lua_next(L,2)!=0) {
								// groupflag, group, MyHandle, is_vip, vip_level
								pszKey=lua_tostring(L,-2);
								type=lua_type(L,-1);

								if (type==LUA_TNUMBER) {
									dwValue=lua_tounsigned(L,-1);
									printf("%s(): tuin=%u %s=%u(u)\n",__FUNCTION__,tuin,pszKey,dwValue);
								} else {
									pszValue=lua_tostring(L,-1);
									printf("%s(): tuin=%u %s=%s(s)\n",__FUNCTION__,tuin,pszKey,pszValue);
								}

								if (!strcmp(pszKey,"MyHandle")) {
									DBWriteContactSettingUTF8String(hContact,"CList",pszKey,pszValue);
								} else if (!strcmp(pszKey,"signature")) {
									DBWriteContactSettingUTF8String(hContact,"CList","StatusMsg",pszValue);
									WRITEC_U8S(pszKey,pszValue);
								} else if (type==LUA_TNUMBER) {
									WRITEC_D(pszKey,dwValue);
								} else
									WRITEC_U8S(pszKey,pszValue);

								if (!strcmp(pszKey,"IsQun")) {
									WRITEC_W("Status",ID_STATUS_ONLINE);
								} else if (!strcmp(pszKey,"group") && dwValue!=0) {
									CallService(MS_CLIST_CONTACTCHANGEGROUP,(WPARAM)hContact,(LPARAM)m_localgroups[dwValue]);
								}

								lua_pop(L,1); // Remove value and keep key
							}

							if (isSession) {
								DBVARIANT dbv1={0};
								DBVARIANT dbv2={0};
								DWORD gtuin=READC_D2("sm_gtuin");
								HANDLE hContact2=FindOrAddContact(0,gtuin);

								if (hContact2) {
									READC_U8S2("Nick",&dbv1);
									if (dbv1.pszVal) {
										READ_U8S2(hContact2,"Nick",&dbv2);
										if (dbv2.pszVal) {
											char szTemp[MAX_PATH];
											strcpy(szTemp,dbv1.pszVal);
											sprintf(szTemp+strlen(szTemp),"(%s)",dbv2.pszVal);
											WRITEC_U8S("Nick",szTemp);
											DBFreeVariant(&dbv2);
										}
										DBFreeVariant(&dbv1);
									}
								}
							}
						} else {
							Log("%s(OPEVENT_CONTACTINFO): hContact==NULL!",__FUNCTION__);
						}

						// No need to remove key as last next popped it! lua_pop(L,1); // Remove key

						if (pszNick) free(pszNick);

						if (!hContact) break;
					}
				}
				break;
			case OPEVENT_CONTACTSTATUS:
				{
					DWORD tuin=*(LPDWORD)pcszStatus;
					DWORD param=*(LPDWORD)pAux;

					if (HANDLE hContact=FindOrAddContact(0,tuin)) {
						WRITEC_W("Status",LOWORD(param));
						WRITEC_W("client_type",HIWORD(param));
					} else {
						Log("%s(OPEVENT_CONTACTSTATUS): hContact==NULL for tUin %u!",__FUNCTION__,tuin);
					}
				}
				break;
			case OPEVENT_GROUPMESSAGE:
			case OPEVENT_CONTACTMESSAGE:
			case OPEVENT_SESSIONMESSAGE:
				{
					DWORD tuin;
					HANDLE hContact;
					lua_State* L=(lua_State*)pAux;

					if (nEvent==OPEVENT_SESSIONMESSAGE) {
						hContact=FindSessionContact(pcszStatus);
						if (hContact) tuin=READC_D2(KEY_TUIN);
					} else {
						tuin=*(LPDWORD)pcszStatus;
						hContact=FindOrAddContact(0,tuin);
					}

					if (hContact) {
						PROTORECVEVENT pre={PREF_UTF};
						CCSDATA ccs={hContact,PSR_MESSAGE,0,(LPARAM)&pre};
						BOOL isQun=(nEvent==OPEVENT_GROUPMESSAGE);
						BOOL isSession=(nEvent==OPEVENT_SESSIONMESSAGE);
						LPCSTR pszMsg=lua_tostring(L,(isQun||isSession)?4:3);
						LPCSTR pszQunMember=NULL;

						pre.timestamp=lua_tounsigned(L,(isQun||isSession)?3:2);
						if (pre.timestamp==0)
							pre.timestamp=time(NULL);
						else if (time(NULL)-pre.timestamp<180)
							// This is to prevent my message shows after new messages
							pre.timestamp=time(NULL);

						if (isQun && tuin!=0) pszQunMember=findQunMember(tuin,lua_tounsigned(L,2));

						int size=(isQun?pszQunMember?strlen(pszQunMember)+10:32:0)+strlen(pszMsg)+1;
						LPSTR pszMsg2=(LPSTR)mir_alloc(size);

						if (isQun && tuin!=0) {
							if (pszQunMember)
								sprintf(pszMsg2,"%s:\r\n%s",pszQunMember,pszMsg);
							else
								sprintf(pszMsg2,"%u:\r\n%s",lua_tounsigned(L,2),pszMsg);
						} else {
							strcpy(pszMsg2,pszMsg);
						}

						pre.szMessage=pszMsg2;

						CallService(MS_PROTO_CHAINRECV, 0, (LPARAM)&ccs);
						mir_free(pszMsg2);
					}
				}
				break;
			case OPEVENT_GROUPINFO:
				{
					DWORD dwTUIN=*(LPDWORD)pcszStatus;
					if (HANDLE hContact=FindOrAddContact(0,dwTUIN)) {
						lua_State* L=(lua_State*)pAux;
						DWORD dwValue;
						LPCSTR pcszKey;
						LPCSTR pcszValue;
						int type;

						lua_pushnil(L);

						while (lua_next(L,2)!=0) {
							// groupflag, group, MyHandle, is_vip, vip_level
							pcszKey=lua_tostring(L,-2);
							type=lua_type(L,-1);

							if (!strcmp(pcszKey,"signature")) {
								pcszValue=lua_tostring(L,-1);
								WRITEC_U8S(pcszKey,pcszValue);
								DBWriteContactSettingUTF8String(hContact,"CList","StatusMsg",pcszValue);
							} else if (!strcmp(pcszKey,"MyHandle")) {
								pcszValue=lua_tostring(L,-1);
								DBWriteContactSettingUTF8String(hContact,"CList",pcszKey,pcszValue);
							} else if (type==LUA_TNUMBER) {
								dwValue=lua_tounsigned(L,-1);
								WRITEC_D(pcszKey,dwValue);
							} else {
								pcszValue=lua_tostring(L,-1);
								WRITEC_U8S(pcszKey,pcszValue);
							}

							lua_pop(L,1);
						}
						lua_pop(L,1);
					}
				}
				break;
			case OPEVENT_GROUPMEMBERS:
				{
					DWORD tuin=*(LPDWORD)pcszStatus;
					lua_State* L=(lua_State*)pAux;
					freeQunMembers(tuin);

					LPLINKEDLIST lpLLHead=NULL;
					LPLINKEDLIST lpLLCurrent=NULL;
					LPLINKEDLIST lpLLNew;
					LPCSTR pcszKey;
					LPCSTR pcszValue;

					lua_pushnil(L);

					while (lua_next(L,2)!=0) {
						pcszKey=lua_tostring(L,-2);
						pcszValue=lua_tostring(L,-1);

						lpLLNew=(LPLINKEDLIST)malloc(sizeof(LINKEDLIST));
						memset(lpLLNew,0,sizeof(LINKEDLIST));
						if (lpLLHead==NULL)
							lpLLHead=lpLLNew;
						else
							lpLLCurrent->next=lpLLNew;

						lpLLNew->key=strtoul(pcszKey,NULL,10);
						lpLLNew->values[0]=strdup(pcszValue);
						lpLLCurrent=lpLLNew;

						lua_pop(L,1);
					}

					lua_pop(L,1);

					m_qunmembers[tuin]=lpLLHead;
				}
				break;
			case OPEVENT_VERYCODE:
				extern void ShowVeryCode(LPSTR pszPath);
				ShowVeryCode((LPSTR)pAux);
				break;
			case OPEVENT_TYPINGNOTIFY:
				if (HANDLE hContact=FindOrAddContact(0,*(LPDWORD)pcszStatus)) {
					CallService(MS_PROTO_CONTACTISTYPING, (WPARAM)hContact, 5); // 5 secs
				}
				break;
			case OPEVENT_ADDTEMPCONTACT:
				if (HANDLE hContact=FindOrAddContact(0,*(LPDWORD)pcszStatus,TRUE,FALSE,TRUE)) {
					WRITEC_W("Status",*(LPDWORD)pAux);
				}
				break;
			case OPEVENT_ADDSEARCHRESULT:
				{
					DWORD tuin=*(LPDWORD)pcszStatus;
					lua_State* L=(lua_State*)pAux;
					
					// isqun,account,name,email,token
					PROTOSEARCHRESULT psr={0};
					
					psr.flags=PSR_UNICODE;
					psr.cbSize=sizeof(psr);
					psr.nick=mir_utf8decodeW(lua_tostring(L,3));
					psr.email=mir_utf8decodeW(lua_tostring(L,4));
					psr.id=mir_utf8decodeW(lua_tostring(L,2));
					psr.firstName=(LPWSTR)(lua_tointeger(L,1)==1?L"1(Group)":L"0(User)");
					psr.lastName=mir_utf8decodeW(lua_tostring(L,5));
					ProtoBroadcastAck(m_szModuleName, NULL, ACKTYPE_SEARCH, ACKRESULT_DATA, (HANDLE) 1, (LPARAM)&psr);
					
					mir_free(psr.nick);
					mir_free(psr.email);
					mir_free(psr.id);
					mir_free(psr.lastName);
					
				}
				break;
			case OPEVENT_ENDOFSEARCH:
				ProtoBroadcastAck(m_szModuleName, NULL, ACKTYPE_SEARCH, ACKRESULT_SUCCESS, (HANDLE) 1, (LPARAM)0);
				break;
			case OPEVENT_REQUESTJOIN:
				{
					DWORD dwUIN=*(LPDWORD)pcszStatus;
					lua_State* L=(lua_State*)pAux;
					
					// isqun,account,name,msg
					
					CCSDATA ccs;
					PROTORECVEVENT pre;
					HANDLE hContact=FindOrAddContact(dwUIN,0);
					char* szBlob;
					char* pCurBlob;
					LPCSTR pcszNick=lua_tostring(L,3);
					LPCSTR pcszFN=lua_tostring(L,4);
					LPCSTR pcszLN=lua_tostring(L,5);
					LPCSTR pcszEMail=lua_tostring(L,6);
					LPCSTR pcszMsg=lua_tostring(L,7);
					
					if (!pcszNick || !*pcszNick) pcszNick=" ";
					if (!pcszFN || !*pcszFN) pcszFN=" ";
					if (!pcszLN || !*pcszLN) pcszLN=" ";
					if (!pcszEMail || !*pcszEMail) pcszEMail=" ";
					if (!pcszMsg || !*pcszMsg) pcszMsg=" ";
			
					if (!hContact) { // The buddy is not in my list, get information on buddy
						// hContact=AddOrFindContact(dwUIN,true,false);
						hContact=FindOrAddContact(dwUIN,dwUIN,TRUE,FALSE,TRUE);
						// TODO: UIN is mangled
						// m_webqq->web2_api_get_friend_info(dwUIN);
					}
					//util_log(0,"%s(): QQID=%d, msg=%s",__FUNCTION__,qqid,szMsg);
					
					WRITEC_D("IsQun",lua_tointeger(L,1));
			
					ccs.szProtoService=PSR_AUTH;
					ccs.hContact=hContact;
					ccs.wParam=0;
					ccs.lParam=(LPARAM)&pre;
					pre.flags=PREF_UTF;
					pre.timestamp=(DWORD)time(NULL);
					pre.lParam=sizeof(DWORD)+sizeof(HANDLE)+strlen(pcszNick)+strlen(pcszFN)+strlen(pcszLN)+strlen(pcszEMail)+strlen(pcszMsg)+5;
	
					/*blob is: uin(DWORD), hcontact(HANDLE), nick(ASCIIZ), first(ASCIIZ), last(ASCIIZ), email(ASCIIZ), reason(ASCIIZ)*/
					// Leak
					pCurBlob=szBlob=(char *)mir_alloc(pre.lParam);
					memcpy(pCurBlob,&dwUIN,sizeof(DWORD)); pCurBlob+=sizeof(DWORD);
					memcpy(pCurBlob,&hContact,sizeof(HANDLE)); pCurBlob+=sizeof(HANDLE);
					strcpy((char *)pCurBlob,pcszNick); pCurBlob+=strlen(pcszNick)+1;
					strcpy((char *)pCurBlob,pcszFN); pCurBlob+=strlen(pcszFN)+1;
					strcpy((char *)pCurBlob,pcszLN); pCurBlob+=strlen(pcszLN)+1;
					strcpy((char *)pCurBlob,pcszEMail); pCurBlob+=strlen(pcszEMail)+1;
					//strcpy((char *)pCurBlob,szMsg);
					strcpy((char *)pCurBlob,pcszMsg);
					pre.szMessage=(char *)szBlob;
	
					CallService(MS_PROTO_CHAINRECV,0,(LPARAM)&ccs);
				}
				break;
			default:
				m_protocol->print_debug("%s(): DEFAULT! nEvent=%d pcszStatus=%s pAux?=%s",__FUNCTION__,nEvent,pcszStatus,pAux==NULL?"no":"yes");
		}

		return 0;
	}
};

extern "C" {
	BOOL WINAPI DllMain(HINSTANCE hinst,DWORD fdwReason,LPVOID lpvReserved) {
		if (fdwReason==DLL_PROCESS_ATTACH) {
			g_hInst=hinst;
			DisableThreadLibraryCalls(hinst);
		}
		return TRUE;
	}

	// MirandaPluginInfo(): Retrieve plugin information
	// mirandaVersion: Version of running Miranda IM
	// Return: Pointer to PLUGININFO, or NULL to disallow plugin load
	__declspec(dllexport) PLUGININFOEX* MirandaPluginInfoEx(DWORD mirandaVersion) {
		if (mirandaVersion<PLUGIN_MAKE_VERSION( 0, 9, 0, 0 )) {
			MessageBoxA(NULL, "MirandaQQ4 plugin can only be loaded on Miranda IM 0.9.0.0 or later.", NULL, MB_OK|MB_ICONERROR|MB_SETFOREGROUND|MB_TOPMOST);
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

	tagPROTO_INTERFACE* InitAccount(LPCSTR szModuleName, LPCTSTR szUserName) {
		CProtocol* protocol=new CProtocol(szModuleName,szUserName);
		protocols.push_back(protocol);
		return protocol;
	}

	int UninitAccount(struct tagPROTO_INTERFACE* pInterface) {
		delete (CProtocol*)pInterface;
		protocols.remove(pInterface);
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
		mir_getMD5I(&md5i);
		//mir_getSHA1I(&sha1i);
		mir_getLP(&pluginInfo);

		if (ServiceExists("MIMQQ4C/PrevInstance")) {
			MessageBoxA(NULL,"This version of MirandaQQ can only be loaded once. This copy will be deactivated.",NULL,MB_ICONERROR);
			return -1;
		}
		
		CreateServiceFunction("MIMQQ4C/PrevInstance",NULL);

		// Register Protocol
		GetModuleFileNameA(g_hInst,szTemp,MAX_PATH);
		*strrchr(szTemp,'.')=0;
		strcpy(g_dllname,CharUpperA(strrchr(szTemp,'\\')+1));
		
		pd.szName=g_dllname;
		pd.type=PROTOTYPE_PROTOCOL;
		pd.fnInit=InitAccount;
		pd.fnUninit=UninitAccount;
		//pd.fnDestroy=&CNetwork::DestroyAccount;

		CallService(MS_PROTO_REGISTERMODULE,0,(LPARAM)&pd);

		// Initialize libeva emot map (This can be shared safely)
		//CProtocol::LoadSmileys(); // TODO

		return 0;
	}
} // extern "C"
