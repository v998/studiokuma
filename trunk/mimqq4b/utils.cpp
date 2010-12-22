#include "StdAfx.h"

CProtocol::QQ_SMILEY CProtocol::g_smileys[256];
int CProtocol::g_smileysCount;

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
			CLibWebQQ* qq=m_webqq;
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
		ID_STATUS_OFFLINE, CLibWebQQ::WEBQQ_PROTOCOL_STATUS_OFFLINE,
		ID_STATUS_ONLINE, CLibWebQQ::WEBQQ_PROTOCOL_STATUS_ONLINE,
		ID_STATUS_AWAY, CLibWebQQ::WEBQQ_PROTOCOL_STATUS_AWAY,
		ID_STATUS_DND, CLibWebQQ::WEBQQ_PROTOCOL_STATUS_BUSY,
		ID_STATUS_INVISIBLE, CLibWebQQ::WEBQQ_PROTOCOL_STATUS_HIDDEN,
		ID_STATUS_FREECHAT, CLibWebQQ::WEBQQ_PROTOCOL_STATUS_ONLINE,
		ID_STATUS_OCCUPIED, CLibWebQQ::WEBQQ_PROTOCOL_STATUS_BUSY,

		CLibWebQQ::WEBQQ_PROTOCOL_STATUS_OFFLINE, ID_STATUS_OFFLINE,
		CLibWebQQ::WEBQQ_PROTOCOL_STATUS_ONLINE, ID_STATUS_ONLINE,
		CLibWebQQ::WEBQQ_PROTOCOL_STATUS_AWAY, ID_STATUS_AWAY,
		CLibWebQQ::WEBQQ_PROTOCOL_STATUS_BUSY, ID_STATUS_DND,
		CLibWebQQ::WEBQQ_PROTOCOL_STATUS_HIDDEN, ID_STATUS_INVISIBLE,
		0
	};

	for (int* pStatus=allstatus; *pStatus; pStatus+=2) {
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
	HANDLE hContact2=NULL;
	int ret;

	while (hContact) {
		if (!lstrcmpA(m_szModuleName, (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0))) {
			hContact2=hContact;
		}

		hContact = ( HANDLE )CallService( MS_DB_CONTACT_FINDNEXT, ( WPARAM )hContact, 0 );
		if (hContact2) {
			ret=CallService(MS_DB_CONTACT_DELETE,(WPARAM)hContact2,0);
			hContact2=NULL;
		}
	}

}

void CProtocol::LoadSmileys() {
	char szPath[MAX_PATH];
	GetModuleFileNameA(NULL,szPath,MAX_PATH);
	strcpy(strrchr(szPath,'\\')+1,"plugins\\qqemot_w.txt");

	if (FILE* fp=fopen(szPath,"r")) {
		char szTemp[MAX_PATH];
		int c=0;
		LPQQ_SMILEY lpQS=g_smileys;

		while (fgets(szTemp,MAX_PATH,fp)) {
			if (*szTemp==';' || *szTemp==0 || *szTemp=='\r' || *szTemp=='\n') continue;

			sscanf(szTemp,"%x\t%x\t%s\t%s",&lpQS->qq2009,&lpQS->qq2006,lpQS->en,lpQS->py);
			// lpQS->webqq=c++;
			lpQS++;
		}
		fclose(fp);
		g_smileysCount=lpQS-g_smileys;
	}
}

int CProtocol::CountSmileys(LPCSTR pszSrc, BOOL encoded) {
	int ret=0;
	char chk=encoded?0x14:'/';

	while (pszSrc=strchr(pszSrc,chk)) {
		ret++;
		pszSrc++;
	}

	return ret;
}

void CProtocol::DecodeSmileys(LPSTR pszSrc) {
	char szTemp[16];
	int smiley;

	while (pszSrc=strchr(pszSrc,0x14)) {
		strncpy(szTemp,pszSrc+1,2);
		szTemp[2]=0;

		smiley=strtol(szTemp,NULL,16);
		if (smiley>g_smileysCount) {
			pszSrc++;
			continue;
		} else {
			LPQQ_SMILEY lpQS=g_smileys+smiley;
			int len;
			sprintf(szTemp,"[face:%u]",(DWORD)lpQS->qq2006);
			len=(int)strlen(szTemp);
			memmove(pszSrc+len,pszSrc+3,strlen(pszSrc+2));
			strncpy(pszSrc,szTemp,len);
			pszSrc+=len;
		}
	}
}

void CProtocol::EncodeSmileys(LPSTR pszSrc) {
	char szTemp[16];
	LPSTR pszTerm;
	int len;
	LPQQ_SMILEY lpQS;

	while (pszSrc=strchr(pszSrc,'/')) {
		if (!(pszTerm=strchr(pszSrc,' ')))
			pszTerm=pszSrc+strlen(pszSrc);
		len=pszTerm-pszSrc;
		if (len<16) {
			strncpy(szTemp,pszSrc,len);
			szTemp[len]=0;
			strlwr(szTemp);

			lpQS=g_smileys;
			for (unsigned char c=0; c<(unsigned char)g_smileysCount; c++) {
				if (!strcmp(lpQS->en,szTemp) || !strcmp(lpQS->py,szTemp)) {
					*pszSrc=0x09;
					sprintf(szTemp,"%02x",lpQS->qq2009);
					memcpy(pszSrc+1,szTemp,2);
					memmove(pszSrc+3,pszTerm,strlen(pszTerm)+1);
					break;
				}

				lpQS++;
			}
		}
		pszSrc+=len;
	}
}

void CProtocol::EncodeP2PImages(LPSTR pszSrc) {
	LPSTR pszTerm;
	LPSTR pszFileName;

	while (pszSrc=strstr(pszSrc,"[img]")) {
		if (!(pszTerm=strstr(pszSrc,"[/img]")))
			pszSrc+=5;
		else {
			*pszTerm=0;
			pszTerm+=6;

			if (!(pszFileName=strstr(pszSrc,"/cgi-bin/webqq_app/?cmd=2&bd="))) {
				pszTerm[-6]='[';
				pszSrc=pszTerm-6;
			} else {
				pszFileName+=29;
				*pszSrc++=0x15;
				*pszSrc++='3';
				*pszSrc++='2';
				memmove(pszSrc,pszFileName,strlen(pszFileName)+1);
				pszSrc+=strlen(pszSrc);
				*pszSrc++=stricmp(pszSrc-4,".gif")?'B':'A';
				*pszSrc++=0x1f;
				memmove(pszSrc,pszTerm,strlen(pszTerm)+1);
				// pszSrc=pszTerm;
			}
		}
	}
}

void CProtocol::EncodeQunImages(HANDLE hContact, LPSTR pszSrc) {
	DWORD intid=READC_D2(UNIQUEIDSETTING);
	DWORD extid=READC_D2(QQ_INFO_EXTID);

	char szUrl[384];
	LPSTR pszTerm;
	LPSTR pszFileName;
	LPSTR pszKeys=m_webqq->GetStorage(WEBQQ_STORAGE_QUNSIG);

	while (pszSrc=strstr(pszSrc,"[img]")) {
		if (!(pszTerm=strstr(pszSrc,"[/img]")))
			pszSrc+=5;
		else {
			*pszTerm=0;
			pszTerm+=6;

			if (!(pszFileName=strstr(pszSrc,"/cgi-bin/webqq_app/?cmd=2&bd="))) {
				pszTerm[-6]='[';
				pszSrc=pszTerm-6;
			} else {
				pszFileName+=29;

				sprintf(szUrl,"http://file1.web.qq.com/%u/%u/%u/gs/?files=%s%s%s&ft=c&cb=WEBQQ.obj.QQClient.mainPanel.handleUploadQunPic&key=%s&sig=%s&go=send",m_webqq->GetQQID(),intid,extid,"%7B%220%22%3A%20%22",pszFileName,"%22%7D",m_webqq->GetArgument(pszKeys,1),m_webqq->GetArgument(pszKeys,2));
				QLog("URL=%s",szUrl);

				DWORD dwLength;
				LPSTR pszResult=m_webqq->GetHTMLDocument(szUrl,m_webqq->GetReferer(CLibWebQQ::WEBQQ_REFERER_WEBQQ),&dwLength);
				if (dwLength!=0xffffffff && pszResult!=NULL) {
					// WEBQQ.obj.QQClient.mainPanel.handleUploadQunPic(2138914413, [{"ret":0,"svrip":3356463991.0,"svrport":443,"fileid":1341718546,"filename":"0B6064AD4F83671CF4275319B8A9AE50.jPg"}])
					if (strstr(pszResult,"\"ret\":0")) {
						DWORD dwServer=strtoul(strstr(pszResult,"\"svrip\":")+8,NULL,10);
						DWORD dwPort=strtoul(strstr(pszResult,"\"svrport\":")+10,NULL,10);
						DWORD dwFileID=strtoul(strstr(pszResult,"\"fileid\":")+9,NULL,10);
						LPSTR pszFileName=strstr(pszResult,"\"filename\":")+12;
						*strrchr(pszFileName,'"')=0;

						*pszSrc++=0x15;
						strcpy(pszSrc,"6 86eA1A");
						pszSrc+=8;
						pszSrc+=sprintf(pszSrc,"%8x%8x%8x%s%s%s",dwFileID,dwServer,dwPort,m_webqq->GetArgument(pszKeys,1),pszFileName,stricmp(pszFileName+strlen(pszFileName)-4,".gif")?"A":"B");
						memmove(pszSrc,pszTerm,strlen(pszTerm)+1);
					} else {
						QLog("Upload failed: Msg=%s",pszResult);

						strcpy(pszSrc,"[X]");
						pszSrc+=3;
						memmove(pszSrc,pszTerm,strlen(pszTerm)+1);
						// pszSrc=pszTerm;
					}
					LocalFree(pszResult);
				}
			}
		}
	}
}

void CProtocol::ProcessQunPics(LPSTR pszSrc, DWORD extid, DWORD ts, BOOL isQun) {
	// return; // Tencent server doesn't like requests without cookies
	
	LPSTR pszTerm;
	int len;
	char szUrl[MAX_PATH+88];

	if (!isQun) {
		while (pszSrc=strchr(pszSrc,0x15)) {
			if (!(pszTerm=strchr(pszSrc,0x1f))) break;
			if (pszSrc[1]!=0x33) {
				pszSrc=pszTerm+1;
				continue;
			} else {
				*pszTerm=0;
				len=sprintf(szUrl,"[img]http://127.0.0.1:%u/p2p?sender=%u&ts=%u&pic=%s[/img]",g_httpServer->GetPort(),extid,ts,pszSrc+2);
				*strstr(szUrl,"[/img]")=0;
				g_httpServer->RegisterQunImage(strstr(szUrl,"/p2p"),this);
				szUrl[strlen(szUrl)]='[';

				memmove(pszSrc+len,pszTerm+1,strlen(pszTerm+1)+1);
				memcpy(pszSrc,szUrl,len);
				pszSrc+=len;
			}
		}
	} else {
		while (pszSrc=strchr(pszSrc,0x15)) {
			if (!(pszTerm=strchr(pszSrc,0x1f))) break;
			if (pszSrc[1]!=0x36) {
				pszSrc=pszTerm+1;
				continue;
			} else {
				*pszTerm=0;
				len=sprintf(szUrl,"[img]http://127.0.0.1:%u/cgi/svr/chatimg/get?pic=%s&gid=%u&time=%u[/img]",g_httpServer->GetPort(),pszSrc+2,extid,ts);
				*strstr(szUrl,"[/img]")=0;
				g_httpServer->RegisterQunImage(strstr(szUrl,"/cgi"),this);
				szUrl[strlen(szUrl)]='[';

				memmove(pszSrc+len,pszTerm+1,strlen(pszTerm+1)+1);
				memcpy(pszSrc,szUrl,len);
				pszSrc+=len;
			}
		}
	}
}

int CProtocol::CountQunPics(LPCSTR pszSrc) {
	// +88 (with [img])
	int ret=0;

	while (pszSrc=strchr(pszSrc,0x15)) {
		if (pszSrc[1]==0x36 || pszSrc[1]==0x33) ret++; // 0x33 is P2P image
		pszSrc+=2;
	}

	return ret;
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

int CProtocol::Web2StatusToMIM(LPSTR pszStatus) {
	switch (*pszStatus) {
		case 'o': return pszStatus[1]=='n'?ID_STATUS_ONLINE:ID_STATUS_OFFLINE;
		case 'a': return ID_STATUS_AWAY;
		case 'h': return ID_STATUS_INVISIBLE;
		case 'b': return ID_STATUS_DND; // busy
		case 'c': return ID_STATUS_FREECHAT; // callme
		case 's': return ID_STATUS_OCCUPIED; // silent
		default : return ID_STATUS_ONLINE; // unknown status
	}
}

void CProtocol::WriteClientType(HANDLE hContact, int type) {
	LPWSTR pszType;
	WRITEC_W("ClientType",type);

	switch (type) {
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 10:
		case 10000:
			pszType=TranslateT("PC");
			break;
		case 21:
		case 22:
		case 23:
		case 24:
			pszType=TranslateT("Phone");
			break;
		case 41:
			pszType=TranslateT("WebQQ");
			break;
		default:
			pszType=L"?";
			break;

	}

	WRITEC_TS("ClientTypeText",pszType);

	WRITEC_TS("Interest2Cat",TranslateT("Client Type"));
	WRITEC_TS("Interest2Text",pszType);
}

LPCSTR CProtocol::Web2StatusFromMIM(int status) {
	static LPCSTR pszStatus[]={
		"online",
		"offline",
		"away",
		"hidden",
		"busy",
		"callme",
		"silent"
	};

	switch (status) {
		case ID_STATUS_ONLINE: return pszStatus[0];
		case ID_STATUS_OFFLINE: return pszStatus[1];
		case ID_STATUS_AWAY: return pszStatus[2];
		case ID_STATUS_INVISIBLE: return pszStatus[3];
		case ID_STATUS_DND: return pszStatus[4];
		case ID_STATUS_FREECHAT: return pszStatus[5];
		case ID_STATUS_OCCUPIED: return pszStatus[6];
		default: return pszStatus[0];
	}
}

string CProtocol::Web2ParseMessage(JSONNODE* jnContent, HANDLE hContact, DWORD gid, DWORD uin) {
	JSONNODE* jnItem;
	bool hasFormat=false;
	int nItems=json_size(jnContent);
	string str;
	char biu=0;
	char szTemp[MAX_PATH];
	LPSTR pszTemp, pszTemp2;
	int face;

	if (hContact!=NULL && gid!=0 && uin!=0) {
		DBVARIANT dbv;
		ultoa(uin,szTemp,10);
		if (!READC_U8S2(szTemp,&dbv)) {
			sprintf(szTemp,"%s (%u):\r\n",dbv.pszVal,uin);
			DBFreeVariant(&dbv);
		} else
			sprintf(szTemp,"%u:\r\n",uin);
		str+=szTemp;
	}

	for (int c=0; c<nItems; c++) {
		jnItem=json_at(jnContent,c);
		if (json_type(jnItem)==JSON_ARRAY) {
			pszTemp=json_as_string(json_at(jnItem,0));
			if (!strcmp(pszTemp,"font")) {
				if (hasFormat) {
					QLog(__FUNCTION__"(): Ignore duplicated format");
				} else {
					// Translate pixel to pt
					HDC hdc=GetDC(NULL);
					int lpsy=GetDeviceCaps(hdc, LOGPIXELSY);
					ReleaseDC(NULL,hdc);

					hasFormat=true;
					jnItem=json_at(jnItem,1);
					json_free(pszTemp);

					face=json_as_int(json_get(jnItem,"size"));
					sprintf(szTemp,"[size=%d]",(face>0?face:9)*lpsy/72);
					str+=szTemp;

					sprintf(szTemp,"[color=%s]",pszTemp=json_as_string(json_get(jnItem,"color")));
					str+=szTemp;

					jnItem=json_get(jnItem,"style");
					for (int d=0; d<3; d++)
						biu|=json_as_int(json_at(jnItem,d))<<d;

					if (biu & (1<<0)) str+="[b]";
					if (biu & (1<<1)) str+="[i]";
					if (biu & (1<<2)) str+="[u]";
				}
			} else if (gid!=0 && !strcmp(pszTemp,"cface")) {
				// http://web2.qq.com/cgi-bin/get_group_pic?gid=58914413&uin=431533686&rip=124.115.1.114&rport=443&fid=3873906531&pic=E9D8263BAEE04E06D7BA65153F54F15C.jpg
				// http://web2-b.qq.com/channel/get_cface?lcid=13910&guid=A661B29654962A3F9744E94F951AA5FA.jpg&to=85379868&count=5&time=1&clientid=35300933
				jnItem=json_at(jnItem,1);
				json_free(pszTemp);

				if (hContact==NULL) {
					// P2P
					// http://web2-b.qq.com/channel/get_cface?lcid=13910&guid=A661B29654962A3F9744E94F951AA5FA.jpg&to=85379868&count=5&time=1&clientid=35300933
					pszTemp2=szTemp+sprintf(szTemp,"[img]http://127.0.0.1:%d/channel/get_cface?lcid=%u&guid=%s&to=%u&count=5&time=1&clientid=%s",g_httpServer->GetPort(),gid,pszTemp=json_as_string(jnItem),uin,m_webqq->GetWeb2ClientID());
				} else {
					// Qun
					pszTemp2=szTemp+sprintf(szTemp,"[img]http://127.0.0.1:%d/cgi-bin/get_group_pic?gid=%u&uin=%u&rip=",g_httpServer->GetPort(),gid,uin);

					*strchr(pszTemp=json_as_string(json_get(jnItem,"server")),':')=0;

					pszTemp2+=strlen(strcpy(pszTemp2,pszTemp));
					pszTemp2+=strlen(strcpy(pszTemp2,"&rport="));
					pszTemp2+=strlen(strcpy(pszTemp2,pszTemp+strlen(pszTemp)+1));
					json_free(pszTemp);

					pszTemp2+=strlen(strcpy(pszTemp2,"&fid="));
					pszTemp2+=strlen(ultoa(json_as_float(json_get(jnItem,"file_id")),pszTemp2,10));

					pszTemp2+=strlen(strcpy(pszTemp2,"&pic="));
					pszTemp2+=strlen(strcpy(pszTemp2,pszTemp=json_as_string(json_get(jnItem,"name"))));
				}

				g_httpServer->RegisterQunImage(strstr(szTemp,"/c"),this);

				strcpy(pszTemp2,"[/img]");
				str+=szTemp;

			} else if (!strcmp(pszTemp,"offpic")) {
				// ["offpic",{"success":1,"file_path":"/f5e6fda4-1310-4233-90a5-ed83086bae59"}]]
				// http://web2-b.qq.com/channel/get_offpic?file_path=/70aaf782-2e12-49e3-9f23-95eb66ae8785&f_uin=431533706&clientid=2387771	R=http://web2.qq.com/
				jnItem=json_at(jnItem,1);

				if (json_as_int(json_get(jnItem,"success"))==1) {
					json_free(pszTemp);
					pszTemp=json_as_string(json_get(jnItem,"file_path"));

					pszTemp2=szTemp+sprintf(szTemp,"[img]http://127.0.0.1:%d/channel/get_offpic?file_path=%s&f_uin=%u&clientid=%s",g_httpServer->GetPort(),pszTemp,uin,m_webqq->GetWeb2ClientID());

					g_httpServer->RegisterQunImage(strstr(szTemp,"/c"),this);

					strcpy(pszTemp2,"[/img]");
					str+=szTemp;
				}
			} else if (!strcmp(pszTemp,"face")) {
				jnItem=json_at(jnItem,1);

				face=json_as_int(jnItem);
				if (face>g_smileysCount) {
					QLog(__FUNCTION__"(): ERROR - Face outside range (%d > %d)",face,g_smileysCount);
				} else {
					LPQQ_SMILEY lpQS=g_smileys;
					for (int d=0; d<g_smileysCount; d++) {
						if (lpQS->qq2009==face) {
							sprintf(szTemp,"[face:%u]",(DWORD)lpQS->qq2006);
							str+=szTemp;
							break;
						}
						lpQS++;
					}
					// while (lpQS->qq2009!=0 && lpQS->qq2009!=nItems) lpQS++;
				}
				
			} else
				QLog(__FUNCTION__"(): Unknown content type: %s",pszTemp);
		} else {
			pszTemp2=pszTemp=json_as_string(jnItem);
			while (pszTemp2=strchr(pszTemp2,'\r')) {
				if (pszTemp2[1]!='\n')
					*pszTemp2='\n';
				else
					pszTemp2++;
			}
			str+=pszTemp;
		}
		json_free(pszTemp);
	}

	if (hasFormat) {
		if (biu & (1<<2)) str+="[/u]";
		if (biu & (1<<1)) str+="[/i]";
		if (biu & (1<<0)) str+="[/b]";
		
		str+="[/color][/size]";
	}

	return str;
}

JSONNODE* CProtocol::Web2ConvertMessage(bool isqun, DWORD qunid, LPCSTR message, int fontsize, LPSTR font, DWORD color, BOOL bold, BOOL italic, BOOL underline) {
	char szTemp[1024];
	JSONNODE* jn=json_new(JSON_ARRAY);
	LPSTR pszMessage=mir_strdup(message);
	//LPSTR pszCheck=pszMessage;
	LPSTR pszCheckS;
	LPSTR pszCheckCS;
	LPSTR pszStart=pszMessage;
	LPSTR pszTerm;
	int len;
	LPQQ_SMILEY lpQS;
	string strMessage;

	JSONNODE* jnNode;
	JSONNODE* jnSubNode;
	JSONNODE* jnSubNode2;

	// pszCustom=strstr(pszSrc,"/cgi-bin/webqq_app/?cmd=2&bd=");
	// [img]http://127.0.0.1:
	pszCheckS=strchr(pszStart,'/');
	pszCheckCS=strstr(pszStart,"[img]http://127.0.0.1");

	while (pszCheckS || pszCheckCS) {
		if (pszCheckCS==NULL || pszCheckS<pszCheckCS) {
			if (!(pszTerm=strchr(pszCheckS,' '))) {
				// Modified because when ending, space is not necessary
				// pszCheckS=pszTerm;
				pszTerm=pszCheckS+strlen(pszCheckS);
			} /*else*/ {
				len=pszTerm-pszCheckS;
				if (len<16) {
					strncpy(szTemp,pszCheckS,len);
					szTemp[len]=0;
					strlwr(szTemp);

					lpQS=g_smileys;
					for (unsigned char c=0; c<(unsigned char)g_smileysCount; c++) {
						if (!strcmp(lpQS->en,szTemp) || !strcmp(lpQS->py,szTemp)) {
							if (pszCheckS!=pszStart) {
								*pszCheckS=0;
								jnNode=json_new(JSON_STRING);
								json_set_a(jnNode,pszStart);
								json_push_back(jn,jnNode);
							}

							jnNode=json_new(JSON_ARRAY);
							json_push_back(jn,jnNode);
							json_push_back(jnNode,json_new_a(NULL,"face"));
							json_push_back(jnNode,json_new_i(NULL,lpQS->qq2009));

							pszStart=pszTerm;
							break;
						}

						lpQS++;
					}
					// pszCheckS=pszTerm;
				} //else {
					pszCheckS=pszTerm;
				//}
			}
			if (pszCheckS) pszCheckS=strchr(pszCheckS,'/');
		} else {
			if (!(pszTerm=strstr(pszCheckCS,"[/img]"))) {
				pszCheckCS=pszTerm;
			} else {
				len=pszTerm-pszCheckCS;
				strncpy(szTemp,pszCheckCS,len);
				szTemp[len]=0;

				if (!strstr(szTemp,isqun?"/cgi-bin/webqq_app/?cmd=2&bd=":"/web2p2pimg/")) {
					pszCheckCS=pszTerm;
				} else {
					if (pszCheckCS!=pszStart) {
						*pszCheckCS=0;
						jnNode=json_new(JSON_STRING);
						json_set_a(jnNode,pszStart);
						json_push_back(jn,jnNode);
					}

					jnNode=json_new(JSON_ARRAY);
					json_push_back(jn,jnNode);

					if (isqun) {
						json_push_back(jnNode,json_new_a(NULL,"cface"));
						json_push_back(jnNode,json_new_a(NULL,"group"));
						json_push_back(jnNode,json_new_a(NULL,strstr(szTemp,"&bd=")+4));
					} else {
						// http://127.0.0.1:170/web2p2pimg/124.115.10.40//?ver=2173&rkey=9DBD38754BE09BF9B82480B6AA9D3727FD932F90C5A0A27AF246F551771841C031164240922D0D764B1430127713CA4852F65EB079AA507F96CBCEBBDBA4BC28&file_path=xxx&file_name=yyy&file_size=123456
						LPSTR pszFilename=strstr(szTemp,"&file_name=");
						LPSTR pszFilesize=strstr(szTemp,"&file_size=");
						*pszFilename=0;
						*pszFilesize=0;

						json_push_back(jnNode,json_new_a(NULL,"offpic"));
						json_push_back(jnNode,json_new_a(NULL,strstr(szTemp,"&file_path=")+11));
						json_push_back(jnNode,json_new_a(NULL,pszFilename+11));
						json_push_back(jnNode,json_new_f(NULL,strtoul(pszFilesize+11,NULL,10)));
					}

					pszStart=pszCheckCS=pszTerm+6;

				}
			}
			if (pszCheckCS) pszCheckCS=strstr(pszCheckCS,"[img]http://127.0.0.1");
		}

		/*
		if (pszStart) {
			pszCheckS=strchr(pszStart,'/');
			pszCheckCS=strstr(pszStart,"[img]http://127.0.0.1");
		}
		*/
	}

	if (*pszStart) {
		jnNode=json_new(JSON_STRING);
		json_set_a(jnNode,pszStart);
		json_push_back(jn,jnNode);
	}

	/*
	jnNode=json_new(JSON_STRING);
	json_set_a(jnNode,message);
	json_push_back(jn,jnNode);
	*/

	jnNode=json_new(JSON_ARRAY);
	json_push_back(jn,jnNode);

	json_set_a(jnSubNode=json_new(JSON_STRING),"font");
	json_push_back(jnNode,jnSubNode);
	jnSubNode=json_new(JSON_NODE);
	json_push_back(jnNode,jnSubNode);
	json_push_back(jnSubNode,json_new_a("name",font));
	json_push_back(jnSubNode,json_new_a("size",itoa(fontsize,szTemp,10)));
	json_push_back(jnSubNode,jnSubNode2=json_new(JSON_ARRAY));
	json_set_name(jnSubNode2,"style");
	json_push_back(jnSubNode2,json_new_i(NULL,bold?1:0));
	json_push_back(jnSubNode2,json_new_i(NULL,italic?1:0));
	json_push_back(jnSubNode2,json_new_i(NULL,underline?1:0));
	sprintf(szTemp,"%06x",color);
	json_push_back(jnSubNode,json_new_a("color",szTemp));

	return jn;
}

