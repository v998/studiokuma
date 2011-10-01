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
/* Utils.cpp: Handy utility function
 */
#include "StdAfx.h"

extern "C" int SuppressQunMessages (WPARAM wParam, LPARAM lParam);
extern map<unsigned short,OutPacket*> pendingImList;

//short QQ_CLIENT_VERSION=0;

#define CODEPAGE_GB 936

// util_setcontactsoffline(): Set all contacts to offline
void CNetwork::SetContactsOffline() {	
	HANDLE hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
	while (hContact) {
		if (!lstrcmpA(m_szModuleName, (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0)))
			if (READC_W2("Status")!=ID_STATUS_OFFLINE) {
				if (READC_B2("IsQun")==1) {
					WRITEC_B("QunInit",0);
					WRITEC_B("ServerQun",0);
					WRITEC_D("QunVersion",0);
					WRITEC_D("CardVersion",0);
					if (READC_W2("Status")==ID_STATUS_INVISIBLE) 
						WRITEC_B("NoInit",1);
					else
						DELC("NoInit");
					if (READC_W2("Status")==ID_STATUS_DND)
						WRITEC_B("SilentQun",1);
					else
						DELC("SilentQun");
				}
				WRITEC_W("Status", ID_STATUS_OFFLINE);
			}

			hContact = ( HANDLE )CallService( MS_DB_CONTACT_FINDNEXT, ( WPARAM )hContact, 0 );
	}
}

// util_broadcaststatus(): Broadcast new protocol status
// newStatus: New Miranda's status to set
//
// * Note: This status is Miranda's status not QQ's status
void CNetwork::BroadcastStatus(int newStatus) {
	int oldStatus;
	oldStatus = m_iStatus;
	m_iStatus = newStatus;
	ProtoBroadcastAck(m_szModuleName,NULL,ACKTYPE_STATUS,ACKRESULT_SUCCESS,(HANDLE)oldStatus,newStatus);
}

// util_log(): General-Purpose logging function, very efficient (From Yahoo)
// level: Log message level (98 is warning, 99 is error)
// fmt: Message format (printf style)
// ...: va_list
extern "C" {
int util_log(unsigned char level, char *fmt,...)
{
	char str[ 4096 ];
	va_list ap;
	int tBytes;

	if (level<DEBUG_LEVEL) return 0;

	va_start(ap, fmt);
	
	tBytes = _vsnprintf( str, sizeof( str ), fmt, ap );
	if ( tBytes > 0 )
		str[ tBytes ] = 0;

	CallService( MS_NETLIB_LOG, ( WPARAM )hNetlibUser, ( LPARAM )str );

	va_end(ap);
	return 0;
}

int util_log2(char *fmt,...)
{
	char str[ 4096 ];
	va_list ap;
	int tBytes;

	va_start(ap, fmt);
	
	tBytes = _vsnprintf( str, sizeof( str ), fmt, ap );
	if ( tBytes > 0 )
		str[ tBytes ] = 0;

	CallService( MS_NETLIB_LOG, ( WPARAM )hNetlibUser, ( LPARAM )str );

	va_end(ap);
	return 0;
}

}

// FindContact(): Find a contact with specified QQID
// QQID: QQID of contact to find
// Return: HANDLE of contact if found, or NULL otherwise
HANDLE CNetwork::FindContact(const unsigned int QQID) {
	HANDLE hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, (WPARAM)NULL, (LPARAM)NULL);
	while (hContact) {
		if (!lstrcmpA(m_szModuleName,(char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact,(LPARAM)NULL)) && 
			READC_D2(UNIQUEIDSETTING)==QQID)
			 return hContact;

		hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, (LPARAM)NULL);
	}
	return NULL;
}
// AddContact(): Add a contact to database with specified QQID
// QQID: QQID of contact to add
// not_on_list: Set the user to be temporary (remove on exit)
// hidden: Set the user to be hidden
// Return: HANDLE of contact created
// * if contact with specified QQID already exists, the existing one will be used instead.
HANDLE CNetwork::AddContact(const unsigned int QQID, bool not_on_list, bool hidden) {
	HANDLE hContact=FindContact(QQID);
	BOOL fNew=FALSE;

	if (!hContact) {
		// Contact not exist, create it
		hContact=(HANDLE)CallService(MS_DB_CONTACT_ADD, (WPARAM)NULL, (LPARAM)NULL);

		if (hContact) {
			// Creation successful, associate protocol
			CallService(MS_PROTO_ADDTOCONTACT,(WPARAM)hContact,(LPARAM)m_szModuleName);
			util_log(0,"%s(): Added contact %d",__FUNCTION__,QQID);
		} /*else
			// Creation failed
			CallService(MS_DB_CONTACT_DELETE,(WPARAM)hContact,(LPARAM)NULL);
			*/
		fNew=TRUE;
	}

	if (hContact) {
		// Contact now exist, set flags
		DBWriteContactSettingDword(hContact,m_szModuleName,UNIQUEIDSETTING,QQID);
		if (fNew && not_on_list) DBWriteContactSettingByte(hContact,"CList","NotOnList",not_on_list?1:0);
		if (fNew && hidden) DBWriteContactSettingByte(hContact,"CList","Hidden",hidden?1:0);
		// DBWriteContactSettingWord(hContact,m_szModuleName,"Status",ID_STATUS_ONLINE);
	}

	return hContact;
}
#if 0
// ShowNotification(): Show message as notification (W2K/XP/2003 only)
// info: The message to show
// flags: One of the NIIF_XXXX flags
// * If this function is used on non-supported OS, MessageBox will be called if flags!=NIIF_INFO
int CNetwork::ShowNotification(const char *info, DWORD flags) {
	if (ServiceExists(MS_POPUP_ADDPOPUP)) {
		POPUPDATA ppd={0};
		strcpy(ppd.lpzContactName,"MirandaQQ");
		strcpy(ppd.lpzText,info);
		ppd.lchIcon=(HICON)LoadImage(hinstance, MAKEINTRESOURCE(IDI_TM), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0);
		CallService(MS_POPUP_ADDPOPUP,(WPARAM)&ppd,0);
	} else if (ServiceExists(MS_CLIST_SYSTRAY_NOTIFY)) {
		// Guest OS supporting tray notification
        MIRANDASYSTRAYNOTIFY err;
        err.szProto = m_szModuleName;
        err.cbSize = sizeof(err);
		err.szInfoTitle = m_szModuleName;
        err.szInfo = (char*) info;
        err.dwInfoFlags = flags;
        err.uTimeout = 1000 * 3;
        CallService(MS_CLIST_SYSTRAY_NOTIFY, 0, (LPARAM) & err);
        return 1;
    } else if (flags != NIIF_INFO) {
		// Gust OS does not support tray notification
		MessageBoxA(NULL,info,"MirandaQQ",flags==NIIF_ERROR?MB_ICONERROR:MB_ICONWARNING);
	}
    return 0;
}
#endif
int CNetwork::ShowNotification(LPCWSTR info, DWORD flags) {
	POPUPDATAW ppd={0};
	_tcscpy(ppd.lpwzContactName,m_tszUserName);
	_tcscpy(ppd.lpwzText,info);

	if (ServiceExists(MS_POPUP_ADDPOPUPW)) {
		ppd.lchIcon=(HICON)LoadImage(hinstance, MAKEINTRESOURCE(IDI_TM), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0);
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

// _nativeconvert(): Conversion of string using WINAPI, called by the next two functions
// szGBK: Message to be converted
// fromCP: Source Codepage
// toCP: Destination Codepage
// * Warning: Don't pass const char* to this function, as it modifies the string directly!
/*
void _nativeconvert(char* szGBK, int fromCP, int toCP) {
	if (fromCP==toCP) {
		util_log(0,"%s(): Not converting because OEMCP is the same (%d)",__FUNCTION__,fromCP);
	} else if (!szGBK) {
		util_log(0,"%s(): Not converting because szGBK is NULL!",__FUNCTION__);
	} else if (!*szGBK) {
		util_log(0,"%s(): Not converting because szGBK is NULL valued!",__FUNCTION__);
	} else {
		WCHAR* wszTemp;
		int size=MultiByteToWideChar(fromCP,0,szGBK,-1,NULL,0);
		wszTemp=(WCHAR*)mir_alloc(sizeof(WCHAR)*(size+1));
		if (!MultiByteToWideChar(fromCP,0,szGBK,-1,wszTemp,size)) {
			// Error occurred
			util_log(0,"%s(): Conversion failed on GBK->Unicode",__FUNCTION__);
		} else {
			// Conversion successful: Now back to ACP
			if (WideCharToMultiByte(toCP,0,wszTemp,size,szGBK,strlen(szGBK),NULL,NULL)) {
				// Error occurred, resulting message most likely contains question marks
				util_log(0,"%s(): Conversion failed on Unicode->OEMCP",__FUNCTION__);
			}
		}
		mir_free(wszTemp);
	}
}

// util_convertFromNative(): Conversion from UTF-16 or native charset to GBK
// szGBK: Mesage to be converted, in UTF-16/Native Char*
// Return: Converted string in Char*.
void util_convertFromNative(LPSTR *szDest, LPCTSTR szSource) {
	bool fNeedChange=_tcsstr(szSource,_T("¡E"));
	int size=WideCharToMultiByte(CODEPAGE_GB,WC_NO_BEST_FIT_CHARS,szSource,-1,NULL,0,fNeedChange?"\xA1\xA4":NULL,NULL);
	*szDest=(LPSTR)malloc(size+1);
	WideCharToMultiByte(CODEPAGE_GB,WC_NO_BEST_FIT_CHARS,szSource,size,*szDest,size,fNeedChange?"\xA1\xA4":NULL,NULL);
}

void util_convertToNative(LPTSTR *szDest, LPCSTR szSource, bool fGBK) {
	int size=MultiByteToWideChar(fGBK?CODEPAGE_GB:GetACP(),0,szSource,-1,NULL,0);
	*szDest=(LPWSTR)malloc(sizeof(WCHAR)*(size+1));
	MultiByteToWideChar(fGBK?CODEPAGE_GB:GetACP(),0,szSource,-1,*szDest,size);
}
*/
// util_convertFromGBK(): Conversion from server messages (GBK)
// szGBK: Mesage to be converted, in GBK
// * Warning: Don't pass const char* to this function, as it modifies the string directly!
void util_convertFromGBK(char* szGBK) {
	LPTSTR pszUnicode=mir_a2u_cp(szGBK,936);
	LPSTR pszANSI=mir_u2a_cp(pszUnicode,GetACP());
	strcpy(szGBK,pszANSI);
	mir_free(pszUnicode);
	mir_free(pszANSI);
	//_nativeconvert(szGBK,CODEPAGE_GB,GetOEMCP());
}

// util_convertToGBK(): Conversion to server messages (GBK)
// szGBK: Mesage to be converted, in native encoding
// * Warning: Don't pass const char* to this function, as it modifies the string directly!
void util_convertToGBK(char* szGBK) {
	//_nativeconvert(szGBK,GetOEMCP(),CODEPAGE_GB);
	LPTSTR pszUnicode=mir_a2u_cp(szGBK,GetACP());
	LPSTR pszANSI=mir_u2a_cp(pszUnicode,936);
	strcpy(szGBK,pszANSI);
	mir_free(pszUnicode);
	mir_free(pszANSI);
}

void __cdecl CNetwork::ThreadMsgBox(void* szMsg) {
	MessageBox(NULL,(LPTSTR)szMsg,_T("MIRANDAQQ"),MB_ICONINFORMATION);
	mir_free(szMsg);
}

// copied from groups.c - horrible, but only possible as this is not available as service
// NOTE: This function is in UTF8 as in MirandaQQ3
int util_group_name_exists(LPCSTR name,int skipGroup)
{
  char idstr[33];
  DBVARIANT dbv;
  int i;

  if (name == NULL) return -1; // no group always exists
  for(i=0;;i++)
  {
    if(i==skipGroup) continue;
    itoa(i,idstr,10);
	if(DBGetContactSettingUTF8String(NULL,"CListGroups",idstr,&dbv)) break;
	if(!strcmp(dbv.pszVal+1,name)) 
    {
      DBFreeVariant(&dbv);
      return i;
    }
    DBFreeVariant(&dbv);
  }
  return -1;
}

void util_clean_nickname(char* nickname) {
	unsigned char* pszNick=(unsigned char*)nickname;
	while (*pszNick) {
		if (*pszNick>127)
			pszNick+=2;
		else if (*pszNick<32)
			memmove(pszNick,pszNick+1,strlen((const char*)pszNick));
		else
			pszNick++;
	}
	//util_log(0,"%s(): -> %s",__FUNCTION__,nickname);
}

void util_clean_nickname(LPWSTR nickname) {
	LPWSTR pszNick=nickname;
	while (*pszNick) {
		if (*pszNick<32)
			memmove(pszNick,pszNick+2,wcslen(pszNick)*sizeof(WCHAR));
		else
			pszNick++;
	}
	//util_log(0,"%s(): -> %s",__FUNCTION__,nickname);
}
void CNetwork::RemoveQunPics() {
	TCHAR szFileName[MAX_PATH];
	LPTSTR pszFileName;
	WIN32_FIND_DATAW FindFileData;
	HANDLE hFind;

	// CallService(MS_UTILS_PATHTOABSOLUTEW,(WPARAM)L"QQ\\QunImages\\*.*",(LPARAM)szFileName);
	FoldersGetCustomPathW(CNetwork::m_folders[0],szFileName,MAX_PATH,L"QQ\\QunImages");
	wcscat(szFileName,L"\\*.*");

	pszFileName=wcsrchr(szFileName,L'\\')+1;

	hFind = FindFirstFile(szFileName, &FindFileData);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			if (*FindFileData.cFileName!=L'.') {
				wcscpy(pszFileName,FindFileData.cFileName);
				DeleteFile(szFileName);
				//printf ("Removing %s=%d\n", szFileName,DeleteFileA(szFileName));
			}
		} while (FindNextFile(hFind,&FindFileData));

		FindClose(hFind);
	}

}

// _trimChatTags(): Remove specified tags from message
// szSrc: 
// szTag: 
void util_trimChatTags(char* szSrc, char* szTag) {
	char* szPos=strstr(szSrc, szTag);
	while (szPos) {
		if (*(szPos-1)!='%') memmove(szPos,szPos+strlen(szTag),strlen(szPos+strlen(szTag))+1);
		if (szPos)
			szPos=strstr(szPos+1,szTag);
		else
			szPos=NULL;
	}
}

void CNetwork::EnableMenuItems(BOOL parEnable) {
	CLISTMENUITEM clmi={sizeof(clmi)};
	clmi.flags = CMIM_FLAGS;
	if (!parEnable)	clmi.flags|=CMIF_GRAYED;

#ifdef TESTSERVICE
	int c=0;
#endif

	for (list<HANDLE>::iterator iter=m_menuItemList.begin(); iter!=m_menuItemList.end(); iter++) {
#ifdef TESTSERVICE
		if (++c>=m_menuItemList.size()) break;
#endif
		CallService( MS_CLIST_MODIFYMENUITEM, (WPARAM)*iter, (LPARAM)&clmi);
	}

}

/////////////////////////////////////////////////////////////////////////////////////////
// MSN_GoOffline - performs several actions when a server goes offline
extern "C" int QQSetCurrentMedia(WPARAM wParam, LPARAM lParam);

// NOTE: By calling this function, the protocol should be already disconnected
void CNetwork::GoOffline() {
	BroadcastStatus(ID_STATUS_OFFLINE);

	if (!Miranda_Terminated()) EnableMenuItems(FALSE);

	SetCurrentMedia(1,NULL);

	if (isConnected()) qqclient_logout(&m_client);
	qqclient_cleanup(&m_client);

	if (!Miranda_Terminated()) SetContactsOffline();

	/*
	while (m_pendingImList.size()) {
		delete m_pendingImList.begin()->second;
		m_pendingImList.erase(m_pendingImList.begin());
	}
	*/
	/*
	if (m_currentMedia.ptszArtist) mir_free(m_currentMedia.ptszArtist);
	if (m_currentMedia.ptszTitle) mir_free(m_currentMedia.ptszTitle);
	ZeroMemory(&m_currentMedia, sizeof(m_currentMedia));
	*/

	if (m_userhead) m_userhead->disconnect();
	if (m_qunimage) m_qunimage->disconnect();

#if 0 // TODO
	Packet::clearAllKeys();
	if (!Miranda_Terminated()) CEvaAccountSwitcher::FreeAccount();
#endif

#ifdef MIRANDAQQ_IPC
	NotifyEventHooks(hIPCEvent,QQIPCEVT_PROTOCOL_OFFLINE,0);
#endif
}

void CNetwork::SetServerStatus(int newStatus) {
	util_log(0, "Setting QQ server status %d", newStatus);

	if (newStatus!=ID_STATUS_OFFLINE/* && qqStatusMode != ID_STATUS_CONNECTING*/ ) {
#if 1
		char status;

		switch (newStatus) {
			case ID_STATUS_OFFLINE:
				status=QQ_OFFLINE;
				break;
			case ID_STATUS_AWAY:
				status=QQ_AWAY;
				break;
			case ID_STATUS_INVISIBLE:
				status=QQ_HIDDEN;
				break;
			case ID_STATUS_NA:
				status=QQ_BUSY;
				break;
			case ID_STATUS_OCCUPIED:
				status=QQ_KILLME;
				break;
			case ID_STATUS_ONLINE:
				status=QQ_ONLINE;
				break;
		}

		qqclient_change_status(&m_client,status);
#else // TODO
		char qqStatus;

		switch (newStatus) {
			case ID_STATUS_AWAY: 
			case ID_STATUS_DND:
			case ID_STATUS_NA:
			case ID_STATUS_ONTHEPHONE:
			case ID_STATUS_OUTTOLUNCH:
			case ID_STATUS_OCCUPIED:
				m_iDesiredStatus=ID_STATUS_AWAY;
				qqStatus=QQ_FRIEND_STATUS_LEAVE; break;
			case ID_STATUS_INVISIBLE:
				qqStatus=QQ_FRIEND_STATUS_INVISIBLE; break;
			case ID_STATUS_ONLINE: 
			case ID_STATUS_FREECHAT:
			default: 
				m_iDesiredStatus=ID_STATUS_ONLINE;
				qqStatus=QQ_FRIEND_STATUS_ONLINE; break;
		}
		append(new ChangeStatusPacket(qqStatus));
#endif
	}
}

void util_fillClientKey(LPSTR pszDest) {
#if 0 // TODO
	const unsigned char* pszKey=Packet::getClientKey();
	for (int c=0; c<32; c++) {
		sprintf(pszDest+(c*2),"%02X",*pszKey++);
	}
#endif
}

void CNetwork::AddContactWithSend(DWORD qqid) {
	PROTOSEARCHRESULT psr={sizeof(psr)};
	CHAR szNick[16];
	psr.nick=szNick;
	ultoa(qqid,szNick,10);
	AddToList(0,&psr);
}

void CNetwork::ForkThread(ThreadFunc func, void* arg) {
	unsigned int threadid;
	mir_forkthreadowner(( pThreadFuncOwner ) *( void** )&func,this,arg,&threadid);
	util_log(0,"Created thread %x ep=%p",threadid,func);
}

#if 0 // TODO
// md5 wrappers
void md5_init(md5_state_t* ctx) { mir_md5_init((mir_md5_state_t*)ctx); }
void md5_append(md5_state_t* ctx, const md5_byte_t* in, int len) { mir_md5_append((mir_md5_state_t*)ctx,in,len); }
void md5_finish(md5_state_t* ctx, md5_byte_t* md5Buf) { mir_md5_finish((mir_md5_state_t*)ctx,md5Buf); }
#endif

LRESULT CALLBACK PopupWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) { 
	switch( message ) {
			case UM_INITPOPUP:
				*(HWND*)PUGetPluginData(hWnd)=hWnd;
				break;
			case WM_COMMAND:
			case WM_CONTEXTMENU:
				*(HWND*)PUGetPluginData(hWnd)=NULL;
				PUDeletePopUp( hWnd );
				break;
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

void CNetwork::RemoveAllCardNames(HANDLE hContact) {
	list<DWORD> idlist;
	char szTemp[20];

	DBCONTACTENUMSETTINGS dbces={_RemoveAllCardNamesProc,(LPARAM)&idlist,m_szModuleName};
	if (CallService(MS_DB_CONTACT_ENUMSETTINGS,(WPARAM)hContact,(LPARAM)&dbces)!=-1) {
		for (list<DWORD>::iterator iter=idlist.begin(); iter!=idlist.end(); iter++) {
			ultoa(*iter,szTemp,10);
			DELC(szTemp);
		}
	}

}

int CNetwork::_RemoveAllCardNamesProc(const char *szSetting,LPARAM lParam) {
	if (*szSetting>='1' && *szSetting<='9') ((list<DWORD>*)lParam)->push_back(strtoul(szSetting,NULL,10));
	return 0;
}
