#include "StdAfx.h"

bool CChatQunList::fChatInit=false;
static COLORREF* pColors;

CChatQunList::CChatQunList(): hHookChatEvent(NULL) {
	/*if (!fChatInit)*/ {
#if 0
		//COLORREF colors[64]={0};
		GCREGISTER gcr={sizeof(gcr)};
		gcr.dwFlags=(0x0f-GC_COLOR)|GC_UNICODE ;
		gcr.pszModule=szMIMQQ;
		gcr.pszModuleDispName=szMIMQQ;
		/*
		gcr.nColors=64;
		gcr.pColors=colors;
		*/
		if (CallServiceSync(MS_GC_REGISTER, 0, (LPARAM)&gcr))
			MessageBoxA(NULL,"Failed to register chat session",szMIMQQError,MB_ICONERROR|MB_SYSTEMMODAL);
		else {
			hHookChatEvent=HookEvent(ME_GC_EVENT,CChatQunList::ChatEventProc);
			fChatInit=true;
		}
#endif
		int nAccount;
		PROTOACCOUNT** pAccount;
		CHAR szSvcName[MAX_PATH];

		GCREGISTER gcr={sizeof(gcr)};
		gcr.dwFlags=(0x0f-GC_COLOR)|GC_UNICODE ;

		CallService(MS_PROTO_ENUMACCOUNTS,(WPARAM)&nAccount,(LPARAM)&pAccount);
		for (int c=0; c<nAccount; c++) {
			if (!pAccount[c]->bIsEnabled) continue;
			strcpy(szSvcName, pAccount[c]->szModuleName);
			strcat(szSvcName, IPCSVC);
			if (!ServiceExists(szSvcName)) continue;

			gcr.pszModule=pAccount[c]->szModuleName;
			m_modulename=mir_strdup(gcr.pszModule);
			gcr.pszModuleDispName=mir_u2a(pAccount[c]->tszAccountName);
			CallServiceSync(MS_GC_REGISTER, 0, (LPARAM)&gcr);

			mir_free((LPSTR)gcr.pszModuleDispName);
		}

		hHookChatEvent=HookEvent(ME_GC_EVENT,CChatQunList::ChatEventProc);
		fChatInit=true;
	}
}

CChatQunList::~CChatQunList() {
	if (hHookChatEvent) {
		UnhookEvent(hHookChatEvent);

		HANDLE hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, (WPARAM)NULL, (LPARAM)NULL);
		LPSTR szMIMQQ;
		while (hContact) {
			//szMIMQQ=GETPROTO();
			szMIMQQ=(LPSTR)CallService(MS_PROTO_GETCONTACTBASEPROTO,(WPARAM)hContact,0);
			if (!strcmp(szMIMQQ,m_modulename)) {
				if (READC_B2("HelperControlled")) {
					DBWriteContactSettingByte(hContact,"CList","Hidden",DBGetContactSettingByte(hContact,"CList","Hidden_Helper",0));
					DBDeleteContactSetting(hContact,"CList","Hidden_Helper");
					DBWriteContactSettingDword(hContact,"Ignore","Mask1",DBGetContactSettingDword(hContact,"Ignore","Mask1_Helper",0));
					DBDeleteContactSetting(hContact,"Ignore","Mask1_Helper");
					DBDeleteContactSetting(hContact,szMIMQQ,"HelperControlled");
				}
			}
			hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, (LPARAM)NULL);
		}

		GCDEST gd={m_modulename,NULL,GC_EVENT_CONTROL};
		GCEVENT ge={sizeof(ge),&gd};
		CallServiceSync(MS_GC_EVENT, SESSION_OFFLINE, (LPARAM)&ge);

		mir_free(m_modulename);
	}
}

CChatQunList* CChatQunList::getInstance() {
	if (m_inst)
		return (CChatQunList*) m_inst;
	else
		return new CChatQunList();
}

void CChatQunList::QunOnline(HANDLE hContact) {
	GCSESSION gcs={sizeof(gcs)};
	DBVARIANT dbv;
	WCHAR szID[16];
	LPSTR szMIMQQ=GETPROTO();

	gcs.iType=GCW_CHATROOM;
	gcs.pszModule=szMIMQQ;
	gcs.dwFlags=GC_UNICODE;

	if (!DBGetContactSettingTString(hContact,szMIMQQ,"Nick",&dbv)) {
		gcs.ptszName=mir_wstrdup(dbv.ptszVal);
		DBFreeVariant(&dbv);
	} else {
		gcs.ptszName=mir_wstrdup(L"???");
	}

	_itow(DBGetContactSettingDword(hContact,szMIMQQ,"UID",0),szID,10);
	//gcs.ptszName=wcsdup(szID);
	gcs.ptszID=szID;

	if (!DBGetContactSettingTString(hContact,"CList","StatusMsg",&dbv)) {
		gcs.ptszStatusbarText=mir_wstrdup(dbv.ptszVal);
		DBFreeVariant(&dbv);
	}

	int ret=CallServiceSync(MS_GC_NEWSESSION, 0, (LPARAM)&gcs);
	mir_free((LPVOID)gcs.ptszName);

	GCDEST gcd={szMIMQQ};
	GCEVENT gce={sizeof(gce),&gcd};

	/*
	gcd.iType=GC_EVENT_SETITEMDATA;
	gce.dwItemData=(DWORD)hContact;
	CallServiceSync(MS_GC_EVENT,(WPARAM)0,(LPARAM)&gce);
	*/

	gcd.iType=GC_EVENT_ADDGROUP;
	gcd.ptszID=szID;
	gce.dwFlags=GC_UNICODE;

	gce.ptszStatus=TranslateT("Creator (Online)");
	CallServiceSync(MS_GC_EVENT,(WPARAM)0,(LPARAM)&gce);

	gce.ptszStatus=TranslateT("Moderators (Online)");
	CallServiceSync(MS_GC_EVENT,(WPARAM)0,(LPARAM)&gce);

	gce.ptszStatus=TranslateT("Shareholders (Online)");
	CallServiceSync(MS_GC_EVENT,(WPARAM)0,(LPARAM)&gce);

	gce.ptszStatus=TranslateT("Online");
	CallServiceSync(MS_GC_EVENT,(WPARAM)0,(LPARAM)&gce);

	gce.ptszStatus=TranslateT("Creator (Offline)");
	CallServiceSync(MS_GC_EVENT,(WPARAM)0,(LPARAM)&gce);

	gce.ptszStatus=TranslateT("Moderators (Offline)");
	CallServiceSync(MS_GC_EVENT,(WPARAM)0,(LPARAM)&gce);

	gce.ptszStatus=TranslateT("Shareholders (Offline)");
	CallServiceSync(MS_GC_EVENT,(WPARAM)0,(LPARAM)&gce);

#if 0
	gce.ptszStatus=TranslateT("Offline");
	CallServiceSync(MS_GC_EVENT,(WPARAM)0,(LPARAM)&gce);

	gce.ptszStatus=TranslateT("Creator");
	CallServiceSync(MS_GC_EVENT,(WPARAM)0,(LPARAM)&gce);

	gce.ptszStatus=TranslateT("Moderators");
	CallServiceSync(MS_GC_EVENT,(WPARAM)0,(LPARAM)&gce);

	gce.ptszStatus=TranslateT("Shareholders");
	CallServiceSync(MS_GC_EVENT,(WPARAM)0,(LPARAM)&gce);

	gce.ptszStatus=TranslateT("Online");
	CallServiceSync(MS_GC_EVENT,(WPARAM)0,(LPARAM)&gce);

#endif

	gcd.iType=GC_EVENT_CONTROL;
	char oldValue=DBGetContactSettingByte(NULL, "Chat", "PopupOnJoin", 0);
	DBWriteContactSettingByte(NULL, "Chat", "PopupOnJoin", 1);
	ret=CallServiceSync(MS_GC_EVENT, SESSION_INITDONE, (LPARAM)&gce);
	ret=CallServiceSync(MS_GC_EVENT, SESSION_ONLINE, (LPARAM)&gce);
	DBWriteContactSettingByte(NULL, "Chat", "PopupOnJoin", oldValue);
	//ret=CallServiceSync( MS_GC_EVENT, WINDOW_HIDDEN, (LPARAM)&gce );
	//ret=CallServiceSync( MS_GC_EVENT, WINDOW_VISIBLE, (LPARAM)&gce );

	if (gcs.ptszStatusbarText) mir_free((LPVOID)gcs.ptszStatusbarText);

	if (DBGetContactSettingByte(hContact,"CList","Hidden_Helper",0)==0) {
		DBWriteContactSettingByte(hContact,"CList","Hidden_Helper",DBGetContactSettingByte(hContact,"CList","Hidden",0));
	}
	DBWriteContactSettingByte(hContact,"CList","Hidden",1);

	if (DBGetContactSettingDword(hContact,"Ignore","Mask1_Helper",0)==0) {
		DBWriteContactSettingDword(hContact,"Ignore","Mask1_Helper",DBGetContactSettingDword(hContact,"Ignore","Mask1",0));
	}
	DBWriteContactSettingDword(hContact,"Ignore","Mask1",1);
	WRITEC_B("HelperControlled",1);
}

void CChatQunList::MessageReceived(ipcmessage_t* ipcm) {
	HANDLE hContact=ipcm->hContact;
	LPSTR pszModule=(LPSTR)CallService(MS_PROTO_GETCONTACTBASEPROTO,(WPARAM)ipcm->hContact,0);
	if (DBGetContactSettingByte(hContact,pszModule,"IsQun",0)==1) {
		GCDEST gcd={GETPROTO()};
		GCEVENT gce={sizeof(gce),&gcd};
		LPWSTR pszMessage=wcsstr(ipcm->message,L":\n");
		WCHAR szID[16];
		WCHAR szUID[16];
		wstring strConverted;
		LPWSTR pszImg;

		gcd.iType=GC_EVENT_MESSAGE;
		gce.dwFlags=GC_UNICODE;
		_itow(ipcm->qunid,szID,10);
		gcd.ptszID=szID;
		gce.ptszNick=ipcm->message;
		*pszMessage=0;
		pszMessage+=2;
		if (gce.ptszNick[wcslen(gce.ptszNick)-1]==L')') {
			wcscpy(szUID,wcsrchr(gce.ptszNick,L'(')+1);
			*wcschr(szUID,L')')=0;
		} else {
			wcscpy(szUID,gce.ptszNick);
		}
		gce.ptszUID=szUID;
		gce.time=(DWORD)ipcm->time;

		while (*pszMessage=='[') {
			if (!wcsncmp(pszMessage,L"[b]",3))
				strConverted+=L"%b";
			else if (!wcsncmp(pszMessage,L"[i]",3))
				strConverted+=L"%i";
			else if (!wcsncmp(pszMessage,L"[u]",3))
				strConverted+=L"%u";
			else if (wcsncmp(pszMessage,L"[color=",7) && wcsncmp(pszMessage,L"[size=",6))
				break;
			pszMessage=wcschr(pszMessage,L']')+1;
		}

		while (pszMessage[wcslen(pszMessage)-1]==']') {
			if (wcslen(pszMessage)>=8 && !wcsncmp(pszMessage+wcslen(pszMessage)-8,L"[/color]",8))
				pszMessage[wcslen(pszMessage)-8]=0;
			else if (wcslen(pszMessage)>=7 && !wcsncmp(pszMessage+wcslen(pszMessage)-7,L"[/size]",7))
				pszMessage[wcslen(pszMessage)-7]=0;
			else if (wcslen(pszMessage)>=4 && (pszMessage[wcslen(pszMessage)-4]==L'['))
				pszMessage[wcslen(pszMessage)-4]=0;
			else
				break;
		}

		pszImg=wcsstr(pszMessage,L"[img]");
		while (pszImg) {
			pszImg[4]=0;
			strConverted+=pszMessage;
			strConverted+=L"]";
			pszMessage+=wcslen(pszMessage)+1;
			pszImg=wcsstr(pszMessage,L"[img]");
		}
		strConverted+=pszMessage;

		gce.ptszText=strConverted.c_str();
		CallService(MS_GC_EVENT,0,(LPARAM)&gce);
	}
}

void CChatQunList::NamesUpdated(ipcmembers_t* ipcms) {
	HANDLE hContact=ipcms->hContact;
	GCDEST gcd={GETPROTO()};
	GCEVENT gce={sizeof(gce),&gcd};
	map<UINT,ipcmember_t> members=quns[ipcms->qunid];
	WCHAR szTemp[MAX_PATH]=L"1";
	WCHAR szUID[16];
	WCHAR szID[16];
	int ret;
	gcd.iType=GC_EVENT_PART;
	gce.dwFlags=GC_UNICODE;
	_itow(ipcms->qunid,szID,10);
	gcd.ptszID=szID;
	//gce.time=time(NULL);

	gce.ptszUID=szUID;
	gce.ptszNick=szTemp;

	for (map<UINT,ipcmember_t>::iterator iter=members.begin(); iter!=members.end(); iter++) {
		if (iter->first==1) continue;
		_itow(iter->first,szUID,10);
		CallService(MS_GC_EVENT,0,(LPARAM)&gce);
	}

	gcd.iType=GC_EVENT_JOIN;
	gce.ptszNick=szTemp;

	members.clear();

	for (list<ipcmember_t>::iterator iter=ipcms->members.begin(); iter!=ipcms->members.end(); iter++) {
		MultiByteToWideChar(936,0,iter->name.c_str(),-1,szTemp,MAX_PATH);
		_itow(iter->qqid,szUID,10);

		switch (iter->flag-IPCMFLAG_EXISTS) {
			case IPCMFLAG_ONLINE|IPCMFLAG_CREATOR:
				gce.ptszStatus=TranslateT("Creator (Online)");
				break;
			case IPCMFLAG_ONLINE|IPCMFLAG_MODERATOR:
				gce.ptszStatus=TranslateT("Moderators (Online)");
				break;
			case IPCMFLAG_ONLINE|IPCMFLAG_INVESTOR:
				gce.ptszStatus=TranslateT("Shareholders (Online)");
				break;
			case IPCMFLAG_ONLINE:
				gce.ptszStatus=TranslateT("Online");
				break;
			case IPCMFLAG_CREATOR:
				gce.ptszStatus=TranslateT("Creator (Offline)");
				break;
			case IPCMFLAG_MODERATOR:
				gce.ptszStatus=TranslateT("Moderators (Offline)");
				break;
			case IPCMFLAG_INVESTOR:
				gce.ptszStatus=TranslateT("Shareholders (Offline)");
				break;
			default:
				gce.ptszStatus=TranslateT("Offline");
		}
#if 0
		if (iter->flag&IPCMFLAG_CREATOR)
			gce.ptszStatus=TranslateT("Creator");
		else if (iter->flag&IPCMFLAG_MODERATOR)
			gce.ptszStatus=TranslateT("Moderators");
		else if (iter->flag&IPCMFLAG_INVESTOR)
			gce.ptszStatus=TranslateT("Shareholders");
		else
			gce.ptszStatus=NULL;
#endif
		members[iter->qqid]=*iter;iter->flag;
		ret=CallService(MS_GC_EVENT,0,(LPARAM)&gce);

#if 0
		if (iter->flag&IPCMFLAG_ONLINE) {
			gcd.iType=GC_EVENT_ADDSTATUS;
			gce.ptszStatus=TranslateT("Online");
			ret=CallService(MS_GC_EVENT,0,(LPARAM)&gce);
		}
#endif
	}
	quns[ipcms->qunid]=members;

	//CallService(szIPCService,QQIPCSVC_QUN_UPDATE_ONLINE_MEMBERS,ipcms->qunid);
	CallContactService(hContact,IPCSVC,QQIPCSVC_QUN_UPDATE_ONLINE_MEMBERS,ipcms->qunid);
}

void CChatQunList::OnlineMembersUpdated(ipconlinemembers_t* ipcms) {
	map<UINT,ipcmember_t> oldmembers=quns[ipcms->qunid];
	map<UINT,ipcmember_t> newmembers=oldmembers;
	int currentstatus;
	HANDLE hContact=ipcms->hContact;
	GCDEST gcd={GETPROTO()};
	GCEVENT gce={sizeof(gce),&gcd};
	WCHAR szUID[16];
	WCHAR szID[16];
	LPWSTR pszNewStatus;
	int ret;

	if (oldmembers.size()==0) return;

	gce.dwFlags=GC_UNICODE;
	_itow(ipcms->qunid,szID,10);
	gcd.ptszID=szID;
	gce.ptszUID=szUID;
	//gce.ptszNick=szTemp;

	onlinemembers[ipcms->qunid]=(UCHAR)ipcms->members.size();

	// For online members
	for (list<UINT>::iterator iter=ipcms->members.begin(); iter!=ipcms->members.end(); iter++) {
		currentstatus=oldmembers[*iter].flag;
		if (currentstatus&IPCMFLAG_EXISTS) {
			if (!(currentstatus&IPCMFLAG_ONLINE)) {
				switch (currentstatus-IPCMFLAG_EXISTS) {
				case IPCMFLAG_CREATOR:
					gce.ptszStatus=TranslateT("Creator (Offline)");
					pszNewStatus=TranslateT("Creator (Online)");
					break;
				case IPCMFLAG_MODERATOR:
					gce.ptszStatus=TranslateT("Moderators (Offline)");
					pszNewStatus=TranslateT("Moderators (Online)");
					break;
				case IPCMFLAG_INVESTOR:
					gce.ptszStatus=TranslateT("Shareholders (Offline)");
					pszNewStatus=TranslateT("Shareholders (Online)");
					break;
				default:
					gce.ptszStatus=TranslateT("Offline");
					pszNewStatus=TranslateT("Online");
				}
				_itow(*iter,szUID,10);
				gcd.iType=GC_EVENT_REMOVESTATUS;
				ret=CallService(MS_GC_EVENT,0,(LPARAM)&gce);
				gce.ptszStatus=pszNewStatus;
				gcd.iType=GC_EVENT_ADDSTATUS;
				ret=CallService(MS_GC_EVENT,0,(LPARAM)&gce);
				newmembers[*iter].flag|=IPCMFLAG_ONLINE;
			}
			oldmembers[*iter].flag=0;
		}
	}

	// For offline members
	for (map<UINT,ipcmember_t>::iterator iter=oldmembers.begin(); iter!=oldmembers.end(); iter++) {
		if ((iter->second.flag&IPCMFLAG_EXISTS)&&(iter->second.flag&IPCMFLAG_ONLINE)) {
			switch (currentstatus-IPCMFLAG_EXISTS) {
			case IPCMFLAG_CREATOR:
				gce.ptszStatus=TranslateT("Creator (Online)");
				pszNewStatus=TranslateT("Creator (Offline)");
				break;
			case IPCMFLAG_MODERATOR:
				gce.ptszStatus=TranslateT("Moderators (Online)");
				pszNewStatus=TranslateT("Moderators (Offline)");
				break;
			case IPCMFLAG_INVESTOR:
				gce.ptszStatus=TranslateT("Shareholders (Online)");
				pszNewStatus=TranslateT("Shareholders (Offline)");
				break;
			default:
				gce.ptszStatus=TranslateT("Online");
				pszNewStatus=TranslateT("Offline");
			}

			_itow(iter->first,szUID,10);
			gcd.iType=GC_EVENT_REMOVESTATUS;
			ret=CallService(MS_GC_EVENT,0,(LPARAM)&gce);
			gce.ptszStatus=pszNewStatus;
			gcd.iType=GC_EVENT_ADDSTATUS;
			ret=CallService(MS_GC_EVENT,0,(LPARAM)&gce);

			newmembers[iter->first].flag-=IPCMFLAG_ONLINE;
		}
	}

	quns[ipcms->qunid]=newmembers;
}

void CChatQunList::MessageSent(HANDLE hContact) {
#if 0
	LPSTR szMIMQQ=GETPROTO();
	GCDEST gcd={szMIMQQ};
	GCEVENT gce={sizeof(gce),&gcd};
	WCHAR szID[16];
	int ret;
	gcd.iType=GC_EVENT_ACK;
	gce.dwFlags=GC_UNICODE;
	_itow(READC_D2("UID"),szID,10);
	gcd.ptszID=szID;
	gce.time=(DWORD)time(NULL);

	ret=CallService(MS_GC_EVENT,0,(LPARAM)&gce);
#endif
}

#define DM_QUERYHCONTACT    (WM_USER+41)

void CChatQunList::TabSwitched(CWPRETSTRUCT* cps) {
	HWND hWnd=(HWND)cps->wParam;
	/*hContact=NULL;
	if (hWnd)
		SendMessage(hWnd,DM_QUERYHCONTACT,NULL,(LPARAM)&hContact);
	else if (cps->message==WM_USER+100) {
		//struct MessageWindowData *dat = (struct MessageWindowData *)GetWindowLong(cps->hwnd, GWL_USERDATA);
		HWND hWndMsg=(HWND)GetWindowLong(cps->hwnd,GWL_USERDATA);
		MessageWindowData* dat=(MessageWindowData*)GetWindowLong(hWndMsg,GWL_USERDATA);
		hWnd=cps->hwnd;
		hContact=dat->hContact;
	}*/

	if (IsWindow(hWnd)) {
		SendMessage(hWnd,DM_QUERYHCONTACT,NULL,(LPARAM)&hContact);
		LPSTR szMIMQQ=GETPROTO();

		if (READC_B2("ChatRoom")==1) {
			DBVARIANT dbv;
			int qunid;
			OutputDebugString(L"CChatQunList::TabSwitched\n");
			READC_TS2("ChatRoomID",&dbv);
			qunid=_wtoi(dbv.ptszVal);
			DBFreeVariant(&dbv);
			if (m_qunid!=qunid) {
				m_qunid=qunid;
				if (quns[m_qunid].size()==0) {
					ipcmember_t ipcm={0};
					quns[m_qunid][1]=ipcm;
					//CallService(szIPCService,QQIPCSVC_QUN_UPDATE_INFORMATION,m_qunid);
					CallContactService(hContact,IPCSVC,QQIPCSVC_QUN_UPDATE_INFORMATION,m_qunid);
				} else
					//CallService(szIPCService,QQIPCSVC_QUN_UPDATE_ONLINE_MEMBERS,m_qunid);
					CallContactService(hContact,IPCSVC,QQIPCSVC_QUN_UPDATE_ONLINE_MEMBERS,m_qunid);

				if (m_timerEvent) SetEvent(m_timerEvent);
				mir_forkthread(TimerThread,(LPVOID)m_qunid);
			}
		}
	}
}

INT CChatQunList::ChatEventProc(WPARAM wParam, LPARAM lParam) {
	GCHOOK* gch=(GCHOOK*)lParam;


	switch (gch->pDest->iType) {
		case GC_USER_MESSAGE:
			{
				GCDEST gcd=*gch->pDest;
				GCEVENT gce={sizeof(gce),&gcd};

				HANDLE hContact=util_find_contact(gch->pDest->pszModule,_ttoi(gch->pDest->ptszID));
				LPSTR szMIMQQ=GETPROTO();

				if (hContact==NULL || DBGetContactSettingByte(hContact,szMIMQQ,"IsQun",0)==0) return 0;

				WCHAR szUID[16];
				CHAR nszUID[16];
				WCHAR szNick[MAX_PATH];
				DBVARIANT dbv;
				ipcsendmessage_t ism={_wtoi(gch->pDest->ptszID),gch->ptszText};
				//CallService(szIPCService,QQIPCSVC_QUN_SEND_MESSAGE,(LPARAM)&ism);
				CallContactService(hContact,IPCSVC,QQIPCSVC_QUN_SEND_MESSAGE,(LPARAM)&ism);

				_itow(DBGetContactSettingDword(NULL,szMIMQQ,"UID",0),szUID,10);
				//gcd.ptszID=gch->pDest->ptszID;
				gcd.iType=GC_EVENT_MESSAGE;
				gce.dwFlags=GC_UNICODE;
				gce.ptszUID=szUID;
				gce.bIsMe=true;
				gce.time=(DWORD)time(NULL);

				WideCharToMultiByte(CP_ACP,0,szUID,-1,nszUID,16,NULL,NULL);
				if (DBGetContactSetting(hContact,szMIMQQ,nszUID,&dbv)) {
					gce.ptszNick=szUID;
				} else {
					MultiByteToWideChar(936,0,dbv.pszVal,-1,szNick,MAX_PATH);
					gce.ptszNick=szNick;
					wsprintf(szNick+wcslen(szNick),L" (%s)",szUID);
				}

				gce.ptszText=gch->ptszText;
				CallService(MS_GC_EVENT,0,(LPARAM)&gce);

				if (quns[_wtoi(gch->pDest->ptszID)].size()==0) {
					ipcmember_t ipcm={0};
					quns[_wtoi(gch->pDest->ptszID)][1]=ipcm;
					//CallService(szIPCService,QQIPCSVC_QUN_UPDATE_INFORMATION,_wtoi(gch->pDest->ptszID));
					CallContactService(hContact,IPCSVC,QQIPCSVC_QUN_UPDATE_INFORMATION,_wtoi(gch->pDest->ptszID));
				}
			}
			break;
		case GC_USER_PRIVMESS:
			break;
		case GC_USER_NICKLISTMENU:
			break;
		case GC_USER_LEAVE:
			break;
	}
	return 0;
}
