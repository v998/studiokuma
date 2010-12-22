#include "StdAfx.h"
/*
CProtocol::QQ_SMILEY CProtocol::g_smileys[256];
int CProtocol::g_smileysCount;
*/
void CProtocol::QLog(char *fmt,...) {
	if (m_hNetlibUser) {
		static CHAR szLog[1024];
		va_list vl;

		va_start(vl, fmt);
		
		szLog[_vsnprintf(szLog, sizeof(szLog)-1, fmt, vl)]=0;
		CallService( MS_NETLIB_LOG, (WPARAM)m_hNetlibUser, (LPARAM)szLog);

		va_end(vl);
	}
}

HANDLE CProtocol::QCreateService(LPCSTR pszService, ServiceFunc pFunc) {
	HANDLE hRet=NULL;
	if (hRet=CreateServiceFunctionObj(pszService,(MIRANDASERVICEOBJ)*(void**)&pFunc,this))
		m_services.push(hRet);
	
	return hRet;
}

HANDLE CProtocol::QHookEvent(LPCSTR pszEvent, EventFunc pFunc) {
	HANDLE hRet=NULL;
	if (hRet=HookEventObj(pszEvent,(MIRANDAHOOKOBJ)*(void**)&pFunc,this))
		m_hooks.push(hRet);

	return hRet;
}

void CProtocol::BroadcastStatus(int newStatus) {
	int oldStatus=m_iStatus;
	m_iStatus=newStatus;
	ProtoBroadcastAck(m_szModuleName,NULL,ACKTYPE_STATUS,ACKRESULT_SUCCESS,(HANDLE)oldStatus,newStatus);
	/*
	if (m_iStatus==ID_STATUS_OFFLINE) {
		if (m_webqq) {
			CLibWebFetion* qq=m_webqq;
			m_webqq=NULL;
			delete qq;
		}
	}
	*/
}

// copied from groups.c - horrible, but only possible as this is not available as service
int CProtocol::FindGroupByName(LPCSTR name)
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

HANDLE CProtocol::FindContact(DWORD qqid) {
	HANDLE hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, (WPARAM)NULL, (LPARAM)NULL);
	while (hContact) {
		if (!lstrcmpA(m_szModuleName,(char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact,(LPARAM)NULL)) && 
			READC_D2(UNIQUEIDSETTING)==qqid)
			 return hContact;

		hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, (LPARAM)NULL);
	}
	return NULL;
}

// NOTE: Change in this function
// If contact already exists, not_on_list and hidden state is untouched
HANDLE CProtocol::AddOrFindContact(DWORD qqid, bool not_on_list, bool hidden) {
	HANDLE hContact=FindContact(qqid);

	if (!hContact) {
		// Contact not exist, create it
		if (hContact=(HANDLE)CallService(MS_DB_CONTACT_ADD, (WPARAM)NULL, (LPARAM)NULL)) {
			// Creation successful, associate protocol
			CallService(MS_PROTO_ADDTOCONTACT,(WPARAM)hContact,(LPARAM)m_szModuleName);
			WRITEC_D(UNIQUEIDSETTING,qqid);
			DBWriteContactSettingByte(hContact,"CList","NotOnList",not_on_list?1:0);
			DBWriteContactSettingByte(hContact,"CList","Hidden",hidden?1:0);

			QLog(__FUNCTION__"(): Added contact %u, NotOnList=%d Hidden=%d",qqid,not_on_list,hidden);
		}
	}

	return hContact;
}

int CProtocol::MapStatus(int status) {
	static int allstatus[]={
		ID_STATUS_OFFLINE, WEBIM_STATUS_OFFLINE,
		ID_STATUS_ONLINE, WEBIM_STATUS_ONLINE,
		ID_STATUS_AWAY, WEBIM_STATUS_AWAY,
		ID_STATUS_DND, WEBIM_STATUS_BUSY,
		ID_STATUS_INVISIBLE, WEBIM_STATUS_OFFLINE,

		WEBIM_STATUS_OFFLINE, ID_STATUS_OFFLINE,
		WEBIM_STATUS_ONLINE, ID_STATUS_ONLINE,
		WEBIM_STATUS_AWAY, ID_STATUS_AWAY,
		WEBIM_STATUS_BUSY, ID_STATUS_DND,
		/*CLibWebFetion::WEBQQ_PROTOCOL_STATUS_HIDDEN, ID_STATUS_INVISIBLE,*/
		-1
	};

	for (int* pStatus=allstatus; *pStatus!=-1; pStatus+=2) {
		if (status==*pStatus) return pStatus[1];
	}

	return 0;
}

void CProtocol::CreateThreadObj(ThreadFunc func, void* arg) {
	unsigned int threadid;
	mir_forkthreadowner((pThreadFuncOwner) *(void**)&func,this,arg,&threadid);
}

void CProtocol::SetContactsOffline() {	
	HANDLE hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
	while (hContact) {
		if (!lstrcmpA(m_szModuleName, (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0)))
			if (READC_W2("Status")!=ID_STATUS_OFFLINE) {
				WRITEC_W("Status", ID_STATUS_OFFLINE);
			}

		hContact = ( HANDLE )CallService( MS_DB_CONTACT_FINDNEXT, ( WPARAM )hContact, 0 );
	}
}

void CProtocol::GetModuleName(LPSTR pszOutput) {
	LPSTR pszTemp;
	GetModuleFileNameA(g_hInstance,pszOutput,MAX_PATH);
	pszTemp=strrchr(pszOutput,'\\')+1;
	memmove(pszOutput,pszTemp,strlen(pszTemp)+1);
	*strrchr(CharUpperA(pszOutput),'.')=0;
}

int CProtocol::ShowNotification(LPCWSTR info, DWORD flags) {
	POPUPDATAW ppd={0};
	_tcscpy(ppd.lpwzContactName,m_tszUserName);
	_tcscpy(ppd.lpwzText,info);

	if (ServiceExists(MS_POPUP_ADDPOPUPW)) {
		ppd.lchIcon=(HICON)LoadImage(g_hInstance, MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0);
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
