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
#include <math.h>

extern "C" BOOL CALLBACK ModifySignatureDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK _noticePopupProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam){ 
	switch( message ) {
			case WM_COMMAND:
				{
					if (LPVOID data=PUGetPluginData(hWnd)) {
						char* pszTemp=mir_u2a_cp((LPTSTR)data,936);
						CallService(MS_UTILS_OPENURL,0,(LPARAM)pszTemp);
						mir_free(pszTemp);
					}
				}

			case WM_CONTEXTMENU:
				PUDeletePopUp( hWnd );
				break;
			case UM_FREEPLUGINDATA: 
				if (LPVOID data=PUGetPluginData(hWnd))
					mir_free(data);
				break;
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

// qunImCallback(): Callback Function for Qun IM Receive
void CNetwork::_qunImCallback2(const unsigned int qunID, const unsigned int senderQQ, const bool hasFontAttribute, const bool isBold, const bool isItalic, const bool isUnderline, const char fontSize, const char red, const char green, const char blue, const int sentTime, const std::string message) {
	HANDLE hContact = FindContact(qunID);
	char szUID[16];

	if (!hContact) {
		hContact=AddContact(qunID,false,false);
		WRITEC_B("IsQun",1);
		WRITEC_W("Status",ID_STATUS_ONLINE);
		//DBWriteContactSettingDword(hContact,"Ignore","Mask1",8);
		return;
	} else if (READC_B2("NoInit")==1 || READC_W2("Status")==ID_STATUS_INVISIBLE) {
		DELC("NoInit");
		WRITEC_W("Status",ID_STATUS_ONLINE);
	}

	if (READC_W2("Status")==ID_STATUS_DND && READ_B2(NULL,QQ_NOSILENTQUNHISTORY))
		return;

	if (senderQQ==m_myqq) {
		// This message is sent by me, update time offset and broadcast ack
		if (strstr(message.c_str(),"[img]")) {
			delayReport_t* dr=(delayReport_t*)mir_alloc(sizeof(delayReport_t));
			dr->hContact=hContact;
			dr->ackType=ACKTYPE_MESSAGE;
			dr->ackResult=ACKRESULT_SUCCESS;
			dr->aux=1;
			dr->aux2=NULL;
			ForkThread((ThreadFunc)&CNetwork::delayReport,dr);
		}
		return;
	}

	ultoa(senderQQ,szUID,10);
	LPSTR pszMsg=NULL;
	DBVARIANT dbv;
	bool hideMessage=false;

	qqqun* qq=qun_get(&m_client,qunID,0);
	qunmember* qm=qun_member_get(&m_client,qq,senderQQ,0);
	LPCSTR pszUtf8=message.c_str();
	int cbUtf8=strlen(pszUtf8);

	if (!READC_U8S2(szUID,&dbv)) {
		if (*dbv.pszVal) {
			pszMsg=(char*)mir_alloc((strlen(dbv.pszVal)+strlen(szUID)+3+cbUtf8+120));
			sprintf(pszMsg,"%s (%s):\n",dbv.pszVal,szUID);
		}
		DBFreeVariant(&dbv);
	}
	if (!pszMsg && qm && *qm->nickname) {
		// With Nick
		pszMsg=(char*)mir_alloc((strlen(qm->nickname)+strlen(szUID)+3+cbUtf8+120));
		sprintf(pszMsg,"%s (%s):\n",qm->nickname,szUID);
	}
	if (!pszMsg) {
		// No Nick
		pszMsg=(char*)mir_alloc((strlen(szUID)+cbUtf8+120));
		sprintf(pszMsg,"%s:\n",szUID);
	}

	if (g_enableBBCode && hasFontAttribute) { // BBCode enabled or Chat Plugin in use
		sprintf(pszMsg+strlen(pszMsg),"[color=%02x%02x%02x]", (unsigned char)red, (unsigned char)green, (unsigned char)blue);
		if (isBold) strcat(pszMsg,"[b]");
		if (isItalic) strcat(pszMsg,"[i]");
		if (isUnderline) strcat(pszMsg,"[u]");
		if (fontSize>0) {
			HDC hdc=GetDC(NULL);

			// Translate pixel to pt
			int lpsy=GetDeviceCaps(hdc, LOGPIXELSY);
			ReleaseDC(NULL,hdc);

			sprintf(pszMsg+strlen(pszMsg),"[size=%d]",fontSize*lpsy/72);
		}
	}

	strcat(pszMsg,pszUtf8);

	if (g_enableBBCode && hasFontAttribute) { // Close Tags for BBCode
		if (fontSize>0) strcat(pszMsg,"[/size]");
		if (isUnderline) strcat(pszMsg,"[/u]");
		if (isItalic) strcat(pszMsg,"[/i]");
		if (isBold) strcat(pszMsg,"[/b]");
		strcat(pszMsg,"[/color]");
	}

	PROTORECVEVENT pre={PREF_UTF};
	CCSDATA ccs={hContact,PSR_MESSAGE,NULL,(LPARAM)&pre};

	if (READ_B2(NULL,QQ_MESSAGECONVERSION) > 1) {
		LPWSTR pszTempS=mir_utf8decodeW(pszMsg);
		LPWSTR pszTempT=mir_tstrdup(pszTempS);
		LCMapString(GetUserDefaultLCID(),LCMAP_TRADITIONAL_CHINESE,pszTempS,wcslen(pszTempS)+1,pszTempT,wcslen(pszTempT)+1);
		pre.szMessage=mir_utf8encodeW(pszTempT);
		mir_free(pszTempS);
		mir_free(pszTempT);
	} else
		pre.szMessage=mir_strdup(pszMsg);

	if (READC_W2("Status")==ID_STATUS_DND) pre.flags+=PREF_CREATEREAD;
	pre.timestamp = (DWORD)sentTime+600<READ_D2(NULL,"LoginTS")?(DWORD)sentTime:(DWORD)time(NULL);

	CallService(MS_PROTO_CHAINRECV, 0, (LPARAM)&ccs);
	mir_free(pre.szMessage);
	mir_free(pszMsg);
}

// sysRequestJoinQunCallback(): Callback Function for application to join Qun
//                                     
void CNetwork::_sysRequestJoinQunCallback(int qunid, int extid, int userid, LPCSTR msg, const unsigned char *token, const WORD tokenLen) {
	qqqun* qun=qun_get(&m_client,qunid,0);

	HANDLE hContact;
	LPSTR szBlob;
	LPSTR pCurBlob;

	hContact=FindContact(qunid);

	if (hContact && qun) { // The qun is initialized, proceed
		char szEmail[MAX_PATH];

		sprintf(szEmail,"%s (%d)",qun->name,qunid);
		mir_utf8decodecp(szEmail,CP_ACP,NULL);
		util_log(0,"%s(): QunID=%d, QQID=%d, msg=%s",__FUNCTION__,qunid,userid,msg);

		DBEVENTINFO dbei;

		util_log(0,"%s(): Received authorization request",__FUNCTION__);

		// Show that guy
		DBDeleteContactSetting(hContact,"CList","Hidden");

		ZeroMemory(&dbei,sizeof(dbei));
		dbei.cbSize=sizeof(dbei);
		dbei.szModule=m_szModuleName;
		dbei.timestamp=(DWORD)time(NULL);
		dbei.flags=DBEF_UTF;
		dbei.eventType=EVENTTYPE_AUTHREQUEST;
		dbei.cbBlob=sizeof(DWORD)+3+sizeof(HANDLE)+strlen(msg)+strlen(szEmail)+5+tokenLen+2;;

		/*blob is: uin(DWORD), hcontact(HANDLE), nick(ASCIIZ), first(ASCIIZ), last(ASCIIZ), email(ASCIIZ), reason(ASCIIZ)*/
		pCurBlob=szBlob=(char *)malloc(dbei.cbBlob);
		memcpy(pCurBlob,&userid,sizeof(DWORD)); pCurBlob+=sizeof(DWORD);
		memcpy(pCurBlob,&hContact,sizeof(HANDLE)); pCurBlob+=sizeof(HANDLE);
		strcpy((char *)pCurBlob," "); pCurBlob+=2;
		strcpy((char *)pCurBlob," "); pCurBlob+=2;
		strcpy((char *)pCurBlob," "); pCurBlob+=2;
		strcpy((char *)pCurBlob,szEmail); pCurBlob+=(strlen(szEmail)+1);
		strcpy((char *)pCurBlob,msg); pCurBlob+=(strlen(msg)+1);
		*(unsigned short*)pCurBlob=tokenLen;
		memcpy(pCurBlob+2,token,tokenLen);

		dbei.pBlob=(PBYTE)szBlob;
		CallService(MS_DB_EVENT_ADD,(WPARAM)NULL,(LPARAM)&dbei);
	}

}

// sysRejectJoinQunCallback(): Callback Function for Qun Admin rejected your join Qun request
void CNetwork::_sysRejectJoinQunCallback(int qunid, int extid, int userid, LPCSTR msg) {
	TCHAR szTemp[MAX_PATH];
	LPTSTR pszReason=mir_utf8decodeW(msg);

	swprintf(szTemp,TranslateT("You request to join group %d has been rejected by admin %d.\nReason:\n\n%s"), extid,userid,pszReason);
	mir_free(pszReason);
	ForkThread((ThreadFunc)&CNetwork::ThreadMsgBox,szTemp);
}

#define WRITEQUNINFO_TS(k,i) pszTemp=mir_a2u_cp(i.c_str(),936); WRITEC_TS(k,pszTemp); mir_free(pszTemp)

void CNetwork::_writeVersion(HANDLE hContact, int version, LPCSTR iniFile) {
	char szTextVersion[MAX_PATH];
	char szVersion[5]={0};
	LPWSTR wszVersion;
	WRITEC_W("ClientVersion",version);

	if (version==0x05a8)
		wszVersion=mir_wstrdup(TranslateW(L"MIMQQ Project Nagato Beta"));
	else {
		sprintf(szVersion,"%04x",version);
		GetPrivateProfileStringA("Versions",szVersion,"",szTextVersion,MAX_PATH,iniFile);
		if (!*szTextVersion) {
			sprintf(szTextVersion,"QQ Unknown Version (0x%04x)",version);
			wszVersion=mir_a2u_cp(szTextVersion,936);
		} else {
			*strchr(szTextVersion,'>')=0;
			wszVersion=mir_a2u_cp(szTextVersion+1,936);
		}

	}
	WRITEC_TS("MirVer",wszVersion);
	mir_free(wszVersion);
}

bool CNetwork::uhCallbackHub(int msg, int qqid, const char* md5, unsigned int session) {
	switch (msg) {
		case CUserHead::Buddy_File:
			{
				// Broadcast
				util_log(0,"Received uh id=%d",qqid);
				HANDLE hContact=FindContact(qqid);
				if (hContact || qqid==m_myqq) {
					DBVARIANT dbv;
					if (!DBGetContactSetting(hContact,m_szModuleName,"UserHeadMD5",&dbv)) {
						char szFileName[MAX_PATH];
						// CallService(MS_UTILS_PATHTOABSOLUTE,(WPARAM)"QQ\\",(LPARAM)szFileName);
						FoldersGetCustomPath(m_avatarFolder,szFileName,MAX_PATH,"QQ");
						strcat(szFileName,"\\");
						strcat(szFileName,dbv.pszVal);
						strcat(szFileName,".bmp");

						if (GetFileAttributesA(szFileName)!=INVALID_FILE_ATTRIBUTES) {
							PROTO_AVATAR_INFORMATION pai={sizeof(pai)};
							strcpy(pai.filename,szFileName);
							pai.format=PA_FORMAT_BMP;
							pai.hContact=hContact;
							WRITEC_D("AvatarUpdateTS",DBGetContactSettingDword(NULL,m_szModuleName,"LoginTS",0));
							WRITEC_B("UserHeadCurrent",1);
							if (READ_B2(NULL,QQ_AVATARTYPE)==0)
								ProtoBroadcastAck(m_szModuleName, (HANDLE)hContact, ACKTYPE_AVATAR, ACKRESULT_SUCCESS, (HANDLE)&pai, (LPARAM)0);
						}

						DBFreeVariant(&dbv);
					}
				}

			}
			break;
		case CUserHead::Buddy_Info:
			{
				util_log(0,"Received uh info, id=%d, session=%d",qqid,session);
				HANDLE hContact=FindContact(qqid);
				if (hContact || qqid==m_myqq) {
					DBVARIANT dbv;
					if (!DBGetContactSetting(hContact,m_szModuleName,"UserHeadMD5",&dbv)) {
						char szFileName[MAX_PATH];
						// CallService(MS_UTILS_PATHTOABSOLUTE,(WPARAM)"QQ\\",(LPARAM)szFileName);
						FoldersGetCustomPath(m_avatarFolder,szFileName,MAX_PATH,"QQ");
						strcat(szFileName,"\\");
						strcat(szFileName,md5);
						strcat(szFileName,".bmp");

						// Old MD5 avail
						if (!strcmp(dbv.pszVal,md5) && GetFileAttributesA(szFileName)!=INVALID_FILE_ATTRIBUTES) {
							DBFreeVariant(&dbv);
							return false;
						} else {
							// MD5 Changed/File not exists
							DBFreeVariant(&dbv);
						}
					}

					WRITEC_S("UserHeadMD5",md5);
					DELC("UserHeadCurrent");
				}
				return true;

			}
			break;
		case -1:
			m_userhead=NULL;
			break;
	}

	return false;
}

void CNetwork::qunPicCallbackHub(int msg, int qunid, void* aux) {
	switch (msg) {
		case 1: // Posted qunpic
			{
				HANDLE hContact=FindContact(qunid);
				if (hContact) {
					//postqunimage_t* pqi=(postqunimage_t*) aux;
					EvaHtmlParser htmlParser;
					string sendStr="";
					//char* pszSend=strdup(pqi->message.c_str());
					char szMD5[16];
					std::string szMD5File;
					char* s;
					list<postqunimage_t>* sentList=(list<postqunimage_t>*)aux;
					char* pszSend=strdup(sentList->front().message.c_str());
					char* pszSend2=pszSend;
					char* pszSend3;
					postqunimage_t pqi;

					while (strstr(pszSend2,"[img]")) {
						pszSend3=strstr(pszSend2,"[img]");
						if (strnicmp(pszSend3,"[img]http",9)) {
							*pszSend3=0;
							sendStr+=pszSend2;
							pszSend2=pszSend3+5;

							if (strstr(pszSend2,"[/img]")) {
								//postqunimage_t* pqi=&sentList->front();
								*strstr(pszSend2,"[/img]")=0;
								s=strrchr(pszSend2,'\\')+1;
								EvaHelper::getFileMD5(pszSend2,szMD5);
								szMD5File.assign(EvaHelper::md5ToString(szMD5));
								pqi.sessionid=0;
								for (list<postqunimage_t>::iterator iter=sentList->begin(); iter!=sentList->end(); iter++)
									if (!strcmp(iter->md5.c_str(),szMD5File.c_str())) {
										pqi=*iter;
									}

								if (pqi.sessionid==0) {
									util_log(0,"Sent file not in list!");
								} else {
									szMD5File.append(strrchr(pszSend2,'.'));

									sendStr+=htmlParser.generateSendFormat(&m_client, szMD5File, pqi.sessionid, pqi.ip, pqi.port);
								}
								pszSend2+=(strlen(pszSend2)+6);
							}
						} else {
							pszSend2=pszSend3+1;
						}
					}

					if (pszSend2)
						sendStr+=pszSend2;

					// Message is already in UTF-8 encoding
					util_log(0,"Send String=%s",sendStr.c_str());
					
					HANDLE hContact=FindContact(qunid);
					if (hContact) {
						SendMsg(hContact,PREF_UTF | (1<<30), sendStr.c_str());
					}

					free(pszSend);
					sentList->clear();
				}
			}
			break;
		case 2: // Error message, aux is LPSTR
			{
				HANDLE hContact=FindContact(qunid);
				LPWSTR lpwzMsg=mir_a2u_cp((LPSTR)aux,936);
				ProtoBroadcastAck(m_szModuleName, hContact, ACKTYPE_MESSAGE, ACKRESULT_FAILED, (HANDLE) 1, (LPARAM)lpwzMsg);	
				ForkThread((ThreadFunc)&CNetwork::ThreadMsgBox,lpwzMsg);
			}
		case -1:
			m_qunimage=NULL;
			break;
	}
}

void __cdecl CNetwork::delayReport(LPVOID lpData) {
	delayReport_t* dr=(delayReport_t*)lpData;

	Sleep(500);
	ProtoBroadcastAck(m_szModuleName, dr->hContact, dr->ackType, dr->ackResult, (HANDLE)dr->aux, (LPARAM)dr->aux2);
	if (dr->aux2) mir_free(dr->aux2);
	mir_free(dr);
}

void CNetwork::_buddyMsgCallback(qqclient* qq, uint uid, time_t t, char* msg) {
	// Dummy function: Handled with libeva
}

void CNetwork::_qunMsgCallback(qqclient* qq, uint uid, uint int_uid, time_t t, char* msg) {
	// Dummy function: Handled with libeva
}

void CNetwork::processProcess(LPSTR pszArgs) {
	bool handled=false;

	switch (atoi(pszArgs)) {
		case P_INIT:
			// BroadcastStatus(ID_STATUS_CONNECTING);
			break;
		case P_LOGGING:
			// BroadcastStatus(m_iStatus+1);
			break;
		case P_VERIFYING: 
			if (m_deferActionType!='A') {
				XGraphicVerifyCode* code=new XGraphicVerifyCode();
				code->m_network=this;
				code->setType(XGVC_TYPE_LOGIN);

				CodeVerifyWindow* win=new CodeVerifyWindow(code);
				delete win;
			} else
				m_client.process=P_LOGIN;

			break;
		case P_LOGIN:
			// BroadcastStatus(m_iDesiredStatus);
			m_client.login_finish = 1;	//we can recv message now.
			EnableMenuItems(TRUE);
			WRITE_D(NULL,"LoginTS",time(NULL));

			if (!this->m_conservative) {
				// TODO: Leave for now
				// append(new RequestExtraInfoPacket());

				if (!*(LPDWORD)m_client.data.file_key) {
					prot_user_get_key(&m_client,QQ_REQUEST_FILE_AGENT_KEY);
				}
			}
			break;
		case P_DENIED:
			ShowNotification(TranslateT("The server does not allow you to login."),NIIF_ERROR);
			handled=true;
		case P_WRONGPASS:
			if (!handled) {
				ShowNotification(TranslateT("Password incorrect."),NIIF_ERROR);
				handled=true;
			}
		case P_ERROR: // Packet timeout or generic error
			if (!handled) {
				if (m_iStatus<ID_STATUS_ONLINE || m_client.login_finish==0) {
					ShowNotification(TranslateT("Connection to server timed out."),NIIF_ERROR);
				}
				handled=true;
			}
		case P_BUSY:
			if (!handled) {
				ShowNotification(TranslateT("You have logged in from another location."),NIIF_ERROR);
				handled=true;
			}
			ProtoBroadcastAck(m_szModuleName,NULL,ACKTYPE_LOGIN,ACKRESULT_FAILED,NULL,LOGINERR_WRONGPASSWORD);
			SetStatus(ID_STATUS_OFFLINE);
			EnableMenuItems(FALSE);
			break;
	}
}

void CNetwork::processClusterInfo(LPSTR pszArgs) {
	if (qqqun* q=qun_get_by_ext(&m_client,strtoul(pszArgs,NULL,10))) {
		HANDLE hContact=AddContact(q->number,false,false);
		
		// MIMQQ2 aware: remove qunver/cardver
		if (READC_B2("MIMQQVersion")!=3) {
			util_log(0,"%s(): Found incompatible qun %d, requesting new info.",__FUNCTION__,q->number);
			DELC("QunVersion");
			DELC("CardVersion");
			
			if (READC_B2("MIMQQVersion")==2) RemoveAllCardNames(hContact);
			WRITEC_B("MIMQQVersion",3);
		}
					
		WCHAR wszTemp[MAX_PATH];
		CHAR szTemp[MAX_PATH];
		wcscpy(wszTemp,TranslateT("QQ Qun"));
		LPSTR pszTemp=mir_utf8encodeW(wszTemp);
		sprintf(szTemp,"(%s) %s",pszTemp,q->name);
		mir_free(pszTemp);
		WRITEC_U8S("Nick",szTemp);
		DBWriteContactSettingUTF8String(hContact,"CList","StatusMsg",q->ann);
		WRITEC_B("AuthType",q->auth_type);
		WRITEC_W("Category",q->category);
		WRITEC_D("Creator",q->owner);
		WRITEC_U8S("Description",q->intro);
		WRITEC_D("ExternalID",q->ext_number);
		WRITEC_B("Type",q->type);
		WRITEC_W("MemberCount",q->member_list.count);
		WRITEC_W("MaxMember",q->max_member);
		WRITEC_W("Status",ID_STATUS_ONLINE);
		WRITEC_B("IsQun",1);
		
		WRITEC_D("QunVersion",q->version);
		// WRITEC_D("CardVersion",q->realnames_version);
		q->realnames_version=READC_D2("CardVersion");

		if (qunmember* qm=qun_member_get(&m_client,q,m_myqq,0)) {
			WRITEC_B("IsAdmin",(qm->role & 1)?1:0);
		}

		if (READC_B2("SilentQun")==1) WRITEC_W("Status",ID_STATUS_DND);

		if (q->realnames_version==0) {
			q->realnames_version=-1;
			WRITEC_D("CardVersion",-1);
			util_log(0,"Qun %d: Member info completed, ask for real names",q->number);
			// append(new QunRequestAllRealNames(q->number));
			prot_qun_get_membername(&m_client,q->number,0);
		} else {
			delayReport_t* dr=(delayReport_t*)mir_alloc(sizeof(delayReport_t));
			dr->hContact=hContact;
			dr->ackType=ACKTYPE_GETINFO;
			dr->ackResult=ACKRESULT_SUCCESS;
			dr->aux=1;
			dr->aux2=NULL;
			ForkThread((ThreadFunc)&CNetwork::delayReport,dr);
		}
	}
}

void CNetwork::processBuddyList(LPSTR pszArgs) {
	qqbuddy* qb;
	HANDLE hContact;
	time_t t=time(NULL);
	int oldStatus;
	int status;
	char szPluginPath[MAX_PATH];

	CallService(MS_UTILS_PATHTOABSOLUTE,(WPARAM)"Plugins\\qqVersion.ini",(LPARAM)szPluginPath);

	for (int c=0; c<m_client.buddy_list.count; c++) {
		qb=(qqbuddy*)m_client.buddy_list.items[c];
		hContact=AddContact(qb->number,false,false);
		if (*qb->nickname && strtoul(qb->nickname,NULL,10)!=qb->number) {
			if (READC_D2("UpdateTS")>=t) {
				DBWriteContactSettingUTF8String(hContact,"CList","StatusMsg",qb->signature);
			} else {
				WRITEC_U8S("Nick",qb->nickname);
				WRITEC_B("Gender",qb->sex);
				WRITEC_U8S("Alias",qb->alias);
				WRITEC_B("GID",qb->gid);
				WRITEC_D("UpdateTS",t);

				LPCSTR szSign=qb->signature;
				bool fSong=szSign[strlen(szSign)-1]==1;

				WRITEC_U8S("PersonalSignature",szSign);

				if (hContact) {
					if (fSong) {
						WRITEC_U8S("ListeningTo",szSign);
					} else {
						DBWriteContactSettingUTF8String(hContact,"CList","StatusMsg",szSign);
						DELC("ListeningTo");
					}
				}
				if (qb->number==m_myqq) WRITEC_U8S("StatusMsg",szSign);

				oldStatus=READC_W2("Status");
				status=ID_STATUS_OFFLINE; //ID_STATUS_ONLINE;

				switch (qb->status) {
					case QQ_OFFLINE:		status=ID_STATUS_OFFLINE;	break;
					case QQ_AWAY:			status=ID_STATUS_AWAY;		break;
					case QQ_HIDDEN:			status=ID_STATUS_INVISIBLE;	break;
					case QQ_BUSY:			status=ID_STATUS_NA;			break;
					case QQ_KILLME:			status=ID_STATUS_OCCUPIED;	break;
					case QQ_QUIET:			status=ID_STATUS_INVISIBLE;	break;
					case QQ_ONLINE: /*default:*/status=ID_STATUS_ONLINE;		break;
				}

				if (status!=oldStatus && oldStatus!=ID_STATUS_INVISIBLE && status!=ID_STATUS_INVISIBLE) {
					WRITEC_W("Status",status);

					if (oldStatus==ID_STATUS_OFFLINE) {
						// These things should only change when changing from offline to online
						_writeVersion(hContact,qb->version,szPluginPath);
					}
				}
			}
		}
	}

	qqqun* qq;

	for (int c=0; c<m_client.qun_list.count; c++) {
		qq=(qqqun*)m_client.qun_list.items[c];

		if (hContact=AddContact(qq->number,false,false)) WRITEC_W("Status",ID_STATUS_ONLINE);
	}

	if (m_downloadGroup) {
		HANDLE hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
		int gid;
		int moved=0;

		while (hContact) {
			if (!lstrcmpA(m_szModuleName, (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0)))
				if (READC_B2("IsQun")==0) {
					gid=READC_B2("GID");
					DBVARIANT dbv={0};
					DBGetContactSettingUTF8String(hContact,"CList","Group",&dbv);
					unsigned int uin=READC_D2(UNIQUEIDSETTING);

					if (dbv.pszVal && !strcmp(dbv.pszVal+1,"MetaContacts Hidden Group")) { // Don't touch MetaContacts!
						TCHAR szPath[MAX_PATH];

						swprintf(szPath,TranslateT("Not moving contact %d because he/she is maintained by MetaContacts"),uin);
						ForkThread((ThreadFunc)&CNetwork::ThreadMsgBox,mir_tstrdup(szPath));
					} else {
						util_log(0,"%s(): Contact %d belongs to group %d",__FUNCTION__,uin,gid);

						if (gid==0) // No group
							CallService(MS_CLIST_CONTACTCHANGEGROUP, (WPARAM)hContact, (LPARAM)0);
						else // In group
							CallService(MS_CLIST_CONTACTCHANGEGROUP, (WPARAM)hContact, (LPARAM)m_hGroupList[gid-1]);
					}
					moved++;
				}

			hContact = ( HANDLE )CallService( MS_DB_CONTACT_FINDNEXT, ( WPARAM )hContact, 0 );
		}

		WCHAR szTemp[MAX_PATH];
		swprintf(szTemp,TranslateT("Process %d users in %d groups."),moved,m_hGroupList.size());
		ShowNotification(szTemp,NIIF_INFO);
		m_hGroupList.clear();
		m_downloadGroup=false;

	}

	if (!m_uhTriggered) {
		m_uhTriggered=true;

		// TODO: Move back to requestextrainfo
		std::list<unsigned int> list;
		std::list<unsigned int> quns;
		HANDLE hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, (WPARAM)NULL, (LPARAM)NULL);
		while (hContact) {
			if (!lstrcmpA(m_szModuleName, (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact,(LPARAM)NULL))) {
				/*if (READC_W2("ExtraInfo") & QQ_EXTAR_INFO_USER_HEAD)
					list.push_back(READC_D2(UNIQUEIDSETTING));
				else*/ if (READC_B2("IsQun")==1)
					quns.push_back(READC_D2(UNIQUEIDSETTING));
				else
					list.push_back(READC_D2(UNIQUEIDSETTING));
			}

			hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, (LPARAM)NULL);
		}

		if (list.size()) {
			m_userhead=new CUserHead(this);
			m_userhead->setQQList(list);
			m_userhead->connect();
		}
	}
}

void CNetwork::processGroupList(LPSTR pszArgs) {
	qqgroup* qg;
	int id;
	unsigned short currentGroupSize=0;
	LPSTR pszGroupWrite=NULL;
	int hGroupMax=0;

	for (int c=0; c<m_client.group_list.count; c++) {
		qg=(qqgroup*)m_client.group_list.items[c];

		id=util_group_name_exists(qg->name,-1);
		if (m_downloadGroup/* && ci*/) { // Download group is performing
			if (id==-1) { // This group is not in contact list
				util_log(0,"%s(): Creating new group: %s",__FUNCTION__,qg->name);
				m_hGroupList[hGroupMax]=(HANDLE)CallService(MS_CLIST_GROUPCREATE, 0, 0);
				//CallService(MS_CLIST_GROUPRENAME, (WPARAM)m_hGroupList[m_hGroupMax], (LPARAM)groupname);
				//ci->pfnRenameGroup((int)m_hGroupList[hGroupMax],szGroupname);
				DBVARIANT dbv;
				char szIndex[16];
				itoa((int)m_hGroupList[hGroupMax]-1,szIndex,10);
				DBGetContactSettingUTF8String(NULL,"CListGroups",szIndex,&dbv);
				if (currentGroupSize<=strlen(qg->name)+1) {
					currentGroupSize=strlen(qg->name)+2;
					if (pszGroupWrite) {
						pszGroupWrite=(LPSTR)mir_realloc(pszGroupWrite,currentGroupSize);
					} else {
						pszGroupWrite=(LPSTR)mir_alloc(currentGroupSize);
					}
				}
				*pszGroupWrite=*dbv.pszVal;
				DBFreeVariant(&dbv);
				strcpy(pszGroupWrite+1,qg->name);
				DBWriteContactSettingUTF8String(NULL,"CListGroups",szIndex,pszGroupWrite);
			} else { // This group is already in contact list, record group ID
				m_hGroupList[hGroupMax]=(HANDLE)(id+1);
			}
			hGroupMax++;
		}

	}

	if (pszGroupWrite) mir_free(pszGroupWrite);

}

void CNetwork::processBuddyStatus(LPSTR pszArgs) {
	int newStatus;
	uint qqid;
	HANDLE hContact=FindContact(qqid=strtoul(pszArgs,NULL,10));
	qqbuddy* qb=buddy_get(&m_client,qqid,false);

	if (hContact!=NULL && qb!=NULL) { // Only handles if the buddy is in my contact list
		int oldStatus=READ_W2(hContact,"Status");

		switch (qb->status) {
			case QQ_OFFLINE:		newStatus=ID_STATUS_OFFLINE;	break;
			case QQ_AWAY:			newStatus=ID_STATUS_AWAY;		break;
			case QQ_HIDDEN:			newStatus=ID_STATUS_INVISIBLE;	break;
			case QQ_BUSY:			newStatus=ID_STATUS_NA;			break;
			case QQ_KILLME:			newStatus=ID_STATUS_OCCUPIED;	break;
			case QQ_QUIET:			newStatus=ID_STATUS_INVISIBLE;	break;
			case QQ_ONLINE: default:newStatus=ID_STATUS_ONLINE;		break;
		}

		if (oldStatus!=newStatus) {
			if (oldStatus==ID_STATUS_OFFLINE) {
				int ip=htonl(qb->ip);
				char szPluginPath[MAX_PATH];

				CallService(MS_UTILS_PATHTOABSOLUTE,(WPARAM)"Plugins\\qqVersion.ini",(LPARAM)szPluginPath);

				std::map<unsigned int, unsigned int> list;
#if 0 // TODO
				SignaturePacket *packet2 = new SignaturePacket(QQ_SIGNATURE_REQUEST);
				list[packet->getQQ()] = 0;
				packet2->setMembers(list);
				append(packet2);
#endif
				_writeVersion(hContact,qb->version,szPluginPath);

				util_log(0,"%s(): Contact %d changed status to %d. IP=%s",__FUNCTION__,qb->number,newStatus,/*packet->getIP()?inet_ntoa(ia):""*/inet_ntoa(*(in_addr*)&ip));
			}

			WRITEC_W("Status",newStatus);
		}
	}
}

void CNetwork::processStatus(LPSTR pszArgs) {
	BroadcastStatus(m_iDesiredStatus);
}

void CNetwork::processBuddyInfo(LPSTR pszArgs) {
	uint qqid;
	HANDLE hContact=FindContact(qqid=strtoul(pszArgs,NULL,10));
	qqbuddy* qb=buddy_get(&m_client,qqid,false);

	if (hContact!=NULL && qb!=NULL) {
		WRITEC_U8S("Nick",qb->nickname);
		WRITEC_U8S("ZIP",qb->post_code);
		WRITEC_U8S("Address",qb->address);
		WRITEC_U8S("Telephone",qb->homephone);
		WRITEC_U8S("Email",qb->email);
		WRITEC_U8S("Occupation",qb->occupation);
		WRITEC_U8S("Homepage",qb->homepage);
		WRITEC_U8S("Mobile",qb->mobilephone);
		WRITEC_U8S("About",qb->brief);
		WRITEC_U8S("College",qb->school);
		LPSTR pszTemp=qb->birth;
		char szTemp[MAX_PATH];
		sprintf(szTemp,"%d/%d/%d",htonl(*(unsigned int*)pszTemp),pszTemp[4],pszTemp[5]);
		WRITEC_U8S("Birthday",szTemp);
		WRITEC_U8S("Country",qb->country);
		WRITEC_U8S("City",qb->city);
		WRITEC_U8S("Province",qb->province);
		WRITEC_W("Age",qb->age);
		WRITEC_B("Flag",qb->flag);

		delayReport_t* dr=(delayReport_t*)mir_alloc(sizeof(delayReport_t));
		dr->hContact=hContact;
		dr->ackType=ACKTYPE_GETINFO;
		dr->ackResult=ACKRESULT_SUCCESS;
		dr->aux=1;
		dr->aux2=NULL;
		ForkThread((ThreadFunc)&CNetwork::delayReport,dr);
	}
}

void CNetwork::processSearchUid(LPSTR pszArgs) {
	PROTOSEARCHRESULT psr={sizeof(psr)};
	LPSTR ppszArgs[5];
	int c=0;
	union {
		char uid[16];
		wchar_t wuid[16];
	};
	bool fUTF8=CallService(MS_SYSTEM_GETVERSION,NULL,NULL)>=0x00090000;

	// sprintf( event, "search_uid_reply^$%d^$%d^$%s^$%d%$%s", num, (int)status, name, no_auth_len, no_auth);

	if (strcmp(pszArgs,"0")) {
		while (pszArgs) {
			ppszArgs[c++]=pszArgs;
			if (pszArgs=strstr(pszArgs,"^$")) {
				*pszArgs=0;
				pszArgs+=2;
			}
		}

		psr.nick=uid;

		if (fUTF8) {
			swprintf(wuid,L"%S",ppszArgs[0]);
			psr.firstName=(LPSTR)mir_utf8decodeW(ppszArgs[2]);
			psr.flags=PSR_UNICODE;
		} else {
			strcpy(uid,ppszArgs[0]);
			psr.firstName=mir_utf8decodecp(mir_strdup(ppszArgs[2]),CP_ACP,NULL);
		}

		ProtoBroadcastAck(m_szModuleName, NULL, ACKTYPE_SEARCH, ACKRESULT_DATA, (HANDLE) 1, (LPARAM)&psr);
		mir_free(psr.firstName);
	}

	if (m_deferActionType=='s') {
		prot_qun_search(&m_client,m_deferActionData);
	} else
		ProtoBroadcastAck(m_szModuleName, NULL, ACKTYPE_SEARCH, ACKRESULT_SUCCESS, (HANDLE) 1, (LPARAM)0);
}

void CNetwork::processQunSearch(LPSTR pszArgs) {
	PROTOSEARCHRESULT psr={sizeof(psr)};
	LPSTR ppszArgs[9];
	union {
		char uid[32];
		wchar_t wuid[32];
	};
	bool fUTF8=CallService(MS_SYSTEM_GETVERSION,NULL,NULL)>=0x00090000;
	int c=0;
	// sprintf( event, "search_uid_reply^$%d^$%d^$%s^$%d%$%s", num, (int)status, name, no_auth_len, no_auth);

	if (strcmp(pszArgs,"0")) {
		while (pszArgs) {
			ppszArgs[c++]=pszArgs;
			if (pszArgs=strstr(pszArgs,"^$")) {
				*pszArgs=0;
				pszArgs+=2;
			}
		}

		psr.nick = uid;

		if (fUTF8) {
			psr.flags=PSR_UNICODE;
			swprintf(wuid,L"%S (%S)",ppszArgs[1],ppszArgs[2]);
			psr.firstName=(LPSTR)mir_utf8decodeW(ppszArgs[6]);
			psr.lastName=(LPSTR)mir_utf8decodeW(ppszArgs[8]);
			psr.email=(LPSTR)(*ppszArgs[7]=='2'/*QQ_QUN_NEED_AUTH*/?TranslateT("Authentication Required"):*ppszArgs[7]=='1'/*QQ_QUN_NO_AUTH*/?TranslateT("Authentication Not Required"):TranslateT("No Add"));
		} else {
			sprintf(uid,"%s (%s)",ppszArgs[1],ppszArgs[2]);
			psr.firstName=mir_utf8decodecp(mir_strdup(ppszArgs[6]),CP_ACP,NULL);
			psr.lastName=mir_utf8decodecp(mir_strdup(ppszArgs[8]),CP_ACP,NULL);
			psr.email=mir_u2a(*ppszArgs[7]=='2'/*QQ_QUN_NEED_AUTH*/?TranslateT("Authentication Required"):*ppszArgs[7]=='1'/*QQ_QUN_NO_AUTH*/?TranslateT("Authentication Not Required"):TranslateT("No Add"));
		}

		ProtoBroadcastAck(m_szModuleName, NULL, ACKTYPE_SEARCH, ACKRESULT_DATA, (HANDLE) 1, (LPARAM)&psr);
		mir_free(psr.firstName);
		mir_free(psr.lastName);
		mir_free(psr.email);
	}

	ProtoBroadcastAck(m_szModuleName, NULL, ACKTYPE_SEARCH, ACKRESULT_SUCCESS, (HANDLE) 1, (LPARAM)0);
}

void CNetwork::processRequestAddBuddy(LPSTR pszArgs) {
	if (*pszArgs=='2') {
		LPSTR pszUin=strstr(pszArgs,"^$");
		WCHAR szMsg[MAX_PATH];
		*pszUin=0;
		pszUin+=2;

		swprintf(szMsg,TranslateT("QQ User %s does not allow others to add him/her."),pszUin);
		ForkThread((ThreadFunc)&CNetwork::ThreadMsgBox,mir_tstrdup(szMsg));
		m_deferActionType=0;
	} else if (*pszArgs=='3') {
		LPSTR pszUin=strstr(pszArgs,"^$");
		WCHAR szMsg[MAX_PATH];
		*pszUin=0;
		pszUin+=2;

		swprintf(szMsg,TranslateT("You need to answer a question in order to add QQ User %s, however MirandaQQ does not support it yet."),pszUin);
		ForkThread((ThreadFunc)&CNetwork::ThreadMsgBox,mir_tstrdup(szMsg));
		m_deferActionType=0;
	} else {
		util_log(0,"Unknown response",pszArgs);
	}
}

void CNetwork::processRequestToken(LPSTR pszArgs) {
	if (*pszArgs=='0') {
		ForkThread((ThreadFunc)&CNetwork::ThreadMsgBox,mir_tstrdup(TranslateT("You have entered an incorrect verification code.")));
	} else {
		XGraphicVerifyCode* code=new XGraphicVerifyCode();
		code->m_network=this;
		code->setType(XGVC_TYPE_ADDUSER);

		CodeVerifyWindow* win=new CodeVerifyWindow(code);
		delete win;
	}
}

void CNetwork::processDelBuddy(LPSTR pszArgs) {
	ShowNotification(*pszArgs=='0'?TranslateT("Specified contact has been removed from server list."):TranslateT("Failed to remove specified contact from server list."),*pszArgs=='0'?NIIF_INFO:NIIF_ERROR);
}

void CNetwork::processBroadcast(LPSTR pszArgs) {
	LPSTR pszFrom=strstr(pszArgs,"^$")+2;
	LPSTR pszTo=strstr(pszFrom,"^$")+2;
	LPSTR pszMsg=strstr(pszTo,"^$")+2;

	switch (atoi(pszArgs)) {
		case QQ_SERVER_BUDDY_ADDED_DEPRECATED: // You are being added by other party
			{
				DWORD uid=strtoul(pszFrom,NULL,10);
				DBEVENTINFO dbei={sizeof(dbei),m_szModuleName,(DWORD)time(NULL),0,EVENTTYPE_ADDED,sizeof(DWORD)+sizeof(HANDLE)+4};
				PBYTE pCurBlob;
				HANDLE hContact=FindContact(uid);

				if (!hContact) AddContact(uid,true,false);

				pCurBlob=dbei.pBlob=(PBYTE)malloc(dbei.cbBlob);
				/*blob is: uin(DWORD), hContact(HANDLE), nick(ASCIIZ), first(ASCIIZ), last(ASCIIZ), email(ASCIIZ) */
				memcpy(pCurBlob,&uid,sizeof(DWORD)); pCurBlob+=sizeof(DWORD);
				memcpy(pCurBlob,&hContact,sizeof(HANDLE)); pCurBlob+=sizeof(HANDLE);
				*(char *)pCurBlob = 0; pCurBlob++;
				*(char *)pCurBlob = 0; pCurBlob++;
				*(char *)pCurBlob = 0; pCurBlob++;
				*(char *)pCurBlob = 0;

				CallService(MS_DB_EVENT_ADD,(WPARAM)(HANDLE)NULL,(LPARAM)&dbei);
			}
			break;
		case QQ_SERVER_BUDDY_ADD_REQUEST_DEPRECATED:  // Your friend would like to add you to his/her list
		case QQ_SERVER_BUDDY_ADD_REQUEST:
			if (READ_B2(NULL,QQ_BLOCKEMPTYREQUESTS)==0) {
				CCSDATA ccs;
				PROTORECVEVENT pre;
				pre.flags=CallService(MS_SYSTEM_GETVERSION,NULL,NULL)<0x00090000?0:PREF_UTF;
				DWORD qqid=strtoul(pszFrom,NULL,10);
				HANDLE hContact=FindContact(qqid);
				char* msg=(pre.flags&PREF_UTF)?mir_strdup(pszMsg):mir_strdup(mir_utf8decodecp(pszMsg,936,NULL));
				char* szBlob;
				char* pCurBlob;

				if (!hContact) { // The buddy is not in my list, get information on buddy
					hContact=AddContact(qqid,true,false);
					// append(new GetUserInfoPacket(qqid));
				}
				//util_log(0,"%s(): QQID=%d, msg=%s",__FUNCTION__,qqid,szMsg);

				ccs.szProtoService=PSR_AUTH;
				ccs.hContact=hContact;
				ccs.wParam=0;
				ccs.lParam=(LPARAM)&pre;
				pre.timestamp=(DWORD)time(NULL);

				pre.lParam=sizeof(DWORD)+4+sizeof(HANDLE)+strlen(msg)+5;

				/*blob is: uin(DWORD), hcontact(HANDLE), nick(ASCIIZ), first(ASCIIZ), last(ASCIIZ), email(ASCIIZ), reason(ASCIIZ)*/
				// Leak
				pCurBlob=szBlob=(char *)mir_alloc(pre.lParam);
				memcpy(pCurBlob,&qqid,sizeof(DWORD)); pCurBlob+=sizeof(DWORD);
				memcpy(pCurBlob,&hContact,sizeof(HANDLE)); pCurBlob+=sizeof(HANDLE);
				strcpy((char *)pCurBlob," "); pCurBlob+=2;
				strcpy((char *)pCurBlob," "); pCurBlob+=2;
				strcpy((char *)pCurBlob," "); pCurBlob+=2;
				strcpy((char *)pCurBlob," "); pCurBlob+=2;
				//strcpy((char *)pCurBlob,szMsg);
				strcpy((char *)pCurBlob,msg);
				if (!(pre.flags&PREF_UTF)) {
					util_convertFromGBK(pCurBlob);
				}
				pre.szMessage=(char *)szBlob;

				util_log(0,"%s(): QQID=%u, msg=%s",__FUNCTION__,qqid,pCurBlob);

				CallService(MS_PROTO_CHAINRECV,0,(LPARAM)&ccs);

				mir_free(szBlob);
				mir_free(msg);
			}
			break;

		case QQ_SERVER_BUDDY_ADDED_ME: // Your friend approved you to add him/her
		case QQ_SERVER_BUDDY_ADDED: // Successful add actively
			{
				WCHAR szMsg[MAX_PATH];
				DWORD qqid=strtoul(pszFrom,NULL,10);
				HANDLE hContact=FindContact(qqid);

				swprintf(szMsg,TranslateT("Your add friend request to %u have been approved."),qqid);
				DELC("Reauthorize");
				DELC("AuthReason");
				ShowNotification(szMsg,NIIF_INFO);
			}
			break;
		case QQ_SERVER_BUDDY_REJECTED_ME: // Your friend rejected your add request
			{
				LPWSTR lpwzMsg=mir_utf8decodeW(pszMsg);
				ShowNotification(lpwzMsg,NIIF_ERROR);
				mir_free(lpwzMsg);
			}
			break;
		case QQ_SERVER_BUDDY_ADDING_EX: // Other side added you succfully
		case QQ_SERVER_BUDDY_ADDED_ANSWER:
			{
				// WCHAR szMsg[MAX_PATH];
				DWORD qqid=strtoul(pszFrom,NULL,10);
				HANDLE hContact=AddContact(qqid,true,false);

				DWORD cbBlob;
				PBYTE pBlob, pCurBlob;

				/*blob is: uin(DWORD), hcontact(HANDLE), nick(ASCIIZ), first(ASCIIZ), last(ASCIIZ), email(ASCIIZ) */
				cbBlob=sizeof(DWORD)+sizeof(HANDLE)+4;
				pCurBlob=pBlob=(PBYTE)_alloca(cbBlob);
				memcpy(pCurBlob,&qqid,sizeof(DWORD)); pCurBlob+=sizeof(DWORD);
				memcpy(pCurBlob,&hContact,sizeof(HANDLE)); pCurBlob+=sizeof(HANDLE);
				*(pCurBlob++)=0;
				*(pCurBlob++)=0;
				*(pCurBlob++)=0;
				*(pCurBlob++)=0;

				DBEVENTINFO dbei = {0};

				dbei.cbSize = sizeof(dbei);
				dbei.szModule = m_szModuleName;
				dbei.timestamp = time(NULL);
				dbei.flags = 0;
				dbei.eventType = EVENTTYPE_ADDED;
				dbei.cbBlob = cbBlob;
				dbei.pBlob = pBlob;

				CallService(MS_DB_EVENT_ADD, (WPARAM)NULL, (LPARAM)&dbei);

				/*
				swprintf(szMsg,TranslateT("Your friend %u added you."),qqid);
				ShowNotification(szMsg,NIIF_INFO);
				*/
			}
			break;
		case QQ_SERVER_NOTICE:
			{
				LPWSTR lpwzMsg=mir_utf8decodeW(pszMsg);
				ShowNotification(lpwzMsg,NIIF_INFO);
				mir_free(lpwzMsg);
			}
			break;
		case QQ_SERVER_NEW_CLIENT:
			util_log(0, "QQ Server has newer client version: %s",pszMsg);
			break;
		default:
			util_log(0, "Received unknown system message: %02x",atoi(pszArgs));
			break;
	}
}

void CNetwork::processLoginTouchRedirect(LPSTR pszArgs) {
	m_client.server_port=getPort();
	redirect(m_client.server_ip,m_client.server_port);
}

void CNetwork::processClusterMemberNames(LPSTR pszArgs) {
	DWORD qunid=strtoul(pszArgs,NULL,10);
	if (qqqun* qun=qun_get(&m_client,qunid,0)) {
		bool fUpdate=true;
		HANDLE hContact=FindContact(qunid);
		if (!hContact) return;

		util_log(0,"Qun %u RealNames version: %u=>%u",qunid, READC_D2("CardVersion"), qun->realnames_version);

		if (READC_D2("CardVersion")==qun->realnames_version && time(NULL)-READC_D2("QunCardUpdate")>60) {
			// Same version, no need to update
			util_log(0,"Qun %u: Same card version(%u), no need to update",qunid,qun->realnames_version);
			WRITEC_B("GetInfoOnline",1);
			delayReport_t* dr=(delayReport_t*)mir_alloc(sizeof(delayReport_t));
			dr->hContact=hContact;
			dr->ackType=ACKTYPE_GETINFO;
			dr->ackResult=ACKRESULT_SUCCESS;
			dr->aux=1;
			dr->aux2=NULL;
			ForkThread((ThreadFunc)&CNetwork::delayReport,dr);

			fUpdate=false;
		}

		if (fUpdate) {
			plist members=qun->member_list;
			char szID[16];
			
			WRITEC_D("QunCardUpdate",(DWORD)time(NULL));
			WRITEC_D("CardVersion",qun->realnames_version);

			util_log(0,"Writing names for Qun %u(%u)",qun->ext_number,qunid);

			qunmember* qm;

			for (int c=0; c<members.count; c++) {
				qm=(qunmember*)members.items[c];
				ultoa(qm->number,szID,10);
				WRITEC_U8S(szID,qm->nickname);

				if (qm->number==qun->owner) {
					WRITEC_U8S("CreatorName",qm->nickname);
				}

				qm++;
			}

			WRITEC_B("GetInfoOnline",1);
			delayReport_t* dr=(delayReport_t*)mir_alloc(sizeof(delayReport_t));
			dr->hContact=hContact;
			dr->ackType=ACKTYPE_GETINFO;
			dr->ackResult=ACKRESULT_SUCCESS;
			dr->aux=1;
			dr->aux2=NULL;
			ForkThread((ThreadFunc)&CNetwork::delayReport,dr);
		}
	}
}

void CNetwork::processIMSendMsgReply(LPSTR pszArgs) {
	DWORD dwSeq=strtoul(pszArgs,NULL,10);

	/*
	OutPacket* op=m_pendingImList[dwSeq+1];

	util_log(0,"Received IM callback, next seq=0x%x, op=0x%x",dwSeq+1, op);

	if (op) {
		m_pendingImList.erase(op->getSequence());
		m_imSender[op->getSequence()]=m_imSender[dwSeq];
		append(op);
	} else*/ {
		delayReport_t* dr=(delayReport_t*)mir_alloc(sizeof(delayReport_t));
		dr->hContact=FindContact(m_imSender[dwSeq]/*im->getReceiver()*/);
		dr->ackType=ACKTYPE_MESSAGE;
		dr->ackResult=ACKRESULT_SUCCESS;
		dr->aux=dwSeq;
		dr->aux2=NULL;
		ForkThread((ThreadFunc)&CNetwork::delayReport,dr);
	}
	// m_imSender.erase(dwSeq);
}

#define READDWORD(x) x=strtoul(ppszArgs,NULL,10); ppszArgs=strstr(ppszArgs,"^$")+2

void CNetwork::processBuddyIMTextAdv(LPSTR pszArgs) {
	// sprintf(tmp,"buddyimtextadv^$%u^$%d^$%d^$%d^$%d^$%d^$%02x^$%02x^$%02x^$%d^$%d^$%s^$%s",msg->from,msg->msg_id,msg->slice_count,msg->slice_no,(unsigned char)msg->auto_reply,hasFontAttribute,red,green,blue,fontSize,format,szFont,msg->msg_content);
	LPSTR ppszArgs=pszArgs;
	DWORD dwFrom, dwMsgID, dwSliceCount, dwSliceNumber, dwAutoReply, dwHasFontAttr;
	int red=0, green=0, blue=0, fontSize=12, format=0;
	LPSTR pszFont;
	LPSTR pszMsg;

	READDWORD(dwFrom);
	READDWORD(dwMsgID);
	READDWORD(dwSliceCount);
	READDWORD(dwSliceNumber);
	READDWORD(dwAutoReply);
	READDWORD(dwHasFontAttr);
	READDWORD(red);
	READDWORD(green);
	READDWORD(blue);
	READDWORD(fontSize);
	READDWORD(format);

	pszFont=ppszArgs;
	ppszArgs=strstr(ppszArgs,"^$")+2;
	ppszArgs[-2]=0;

	pszMsg=ppszArgs;

	/////////
	LPSTR msg=(LPSTR)mir_alloc(strlen(pszMsg)+64);

	PROTORECVEVENT pre;
	CCSDATA ccs;

	*msg=0;
	if (g_enableBBCode && dwHasFontAttr!=0) {
		sprintf(msg,"[color=#%02x%02x%02x]",(BYTE)red,(BYTE)green,(BYTE)blue);

		if (format & 1) strcat(msg,"[b]");
		if (format & 2) strcat(msg,"[i]");
		if (format & 4) strcat(msg,"[u]");
	}

	strcat(msg,pszMsg);

	if (g_enableBBCode && dwHasFontAttr!=0) {
		if (format & 4) strcat(msg,"[/u]");
		if (format & 2) strcat(msg,"[/i]");
		if (format & 1) strcat(msg,"[/b]");
		strcat(msg,"[/color]");
	}

	HANDLE hContact=FindContact(dwFrom);
	if (!hContact) {
		hContact=AddContact(dwFrom,true,false);
		prot_buddy_get_info(&m_client,dwFrom);
	}

	pre.flags=PREF_UTF;
	pre.szMessage=msg;

	pre.timestamp = (DWORD) time(NULL);
	pre.lParam = 0;

	ccs.hContact=hContact;
	ccs.szProtoService = PSR_MESSAGE;
	ccs.wParam = 0;
	ccs.lParam = ( LPARAM )&pre;
	CallService(MS_PROTO_CHAINRECV, 0, ( LPARAM )&ccs );
			
	mir_free(msg);
	// WRITEC_W("Sequence",received->getSequence());
	// WRITEC_S("FileSessionKey",(char*)received->getBuddyFileSessionKey());
}

std::string convertToShow(const std::string &src, const unsigned char type)
{
	std::string converted = "";
	unsigned int start = 0;
	char *uuid = NULL; // uuid format: {12345678-1234-1234-1234-123456789ABC}
	if(type == QQ_IM_IMAGE_REPLY){
		uuid = new char[39]; // 38 + 1
		memcpy(uuid, src.c_str(), 38);
		uuid[38] = 0x00; // just make it as a string
		start+=38;
		start+=2; // 2 unknown bytes: 0x13 0x4c (always)
	}
	for(uint i=start; i<src.length()-1; i++){
	// Here it is length()-1 to cut off the extra space at the end of every message (always)
		if(src[i]==0x14){
			converted+=EvaUtil::smileyToText((unsigned short)(unsigned char)src[++i]);
			converted+=' ';
		}else if(src[i]==0x15){
				int t=0;
				char *tmp = new char[src.length()+1];
				strcpy(tmp, src.c_str());
				converted+=EvaUtil::customSmileyToText(tmp + i, &t, uuid);
				i+=(t-1); // get rid of 0x15
				delete tmp;
			} else
				converted+=src[i];
	}
	// starkwong: oldherl's patch on SVN 47 damages long messages, so add a dirty fix here (not perfect)
	if (src[src.length()-1]!=0x20) converted+=src[src.length()-1];

	if(uuid) delete []uuid;
	return converted;
}

void CNetwork::processQunIMAdv(LPSTR pszArgs) {
	// sprintf(tmp,"qunimadv^$%u^$%u^$%d^$%d^$%d^$%u^$%u^$%d^$%02x^$%02x^$%02x^$%d^$%d^$%s^$%s",msg->qun_number,msg->from,msg->msg_id,msg->slice_count,msg->slice_no,qun_version,sent_time,has_font_attr,red,green,blue,font_size,format,font_name,msg->msg_content);

	LPSTR ppszArgs=pszArgs;
	DWORD dwQun, dwFrom, dwMsgID, dwSliceCount, dwSliceNumber, dwQunVersion, dwSentTime, dwHasFontAttr;
	int red=0, green=0, blue=0, fontSize=12, format=0;
	LPSTR pszFont;
	LPSTR pszMsg;

	READDWORD(dwQun);
	READDWORD(dwFrom);
	READDWORD(dwMsgID);
	READDWORD(dwSliceCount);
	READDWORD(dwSliceNumber);
	READDWORD(dwQunVersion);
	READDWORD(dwSentTime);
	READDWORD(dwHasFontAttr);
	READDWORD(red);
	READDWORD(green);
	READDWORD(blue);
	READDWORD(fontSize);
	READDWORD(format);

	pszFont=ppszArgs;
	ppszArgs=strstr(ppszArgs,"^$")+2;
	ppszArgs[-2]=0;

	pszMsg=ppszArgs;

	/////////
	string strShow=convertToShow(string(pszMsg), QQ_IM_NORMAL_REPLY);

	// TODO: Fragments
#if 0
	if (received->getNumFragments()>1)
	{
		unsigned short ropID = received->getMessageID();

		//push fragment inormation into map
		unsigned short total = received->getNumFragments();
		pcMsgCache[ropID].total = total;
		pcMsgCache[ropID].content[received->getSeqOfFragments()] = received->getMessage().c_str();

		if (total == pcMsgCache[ropID].content.size())
		{
			//all fragments received
			string allText = "";
			for (int idx=0; idx<total; idx++) 
				allText += pcMsgCache[ropID].content[idx];
			pcMsgCache.erase(ropID);
			received->setMessage(allText);
		} else
		{
			delete received;
			return;
		}

	}
#endif

	if (!this->m_conservative) {
		EvaHtmlParser parser;
		std::list<CustomizedPic> picList = parser.parseCustomizedPics((char*)strShow.c_str(),true);

		if (dwFrom==m_myqq) {
			if (strstr(parser.getConvertedMessage().c_str(),"[img]")) {
				delayReport_t* dr=(delayReport_t*)mir_alloc(sizeof(delayReport_t));
				dr->hContact=FindContact(dwQun);
				dr->ackType=ACKTYPE_MESSAGE;
				dr->ackResult=ACKRESULT_SUCCESS;
				dr->aux=1;
				dr->aux2=NULL;
				ForkThread((ThreadFunc)&CNetwork::delayReport,dr);
			}

			return;
		}

		if(picList.size()){
			LPWSTR pwszFileName;
			strShow.assign(parser.getConvertedMessage());

			for (list<CustomizedPic>::iterator iter=picList.begin(); iter!=picList.end();) {
				pwszFileName=mir_utf8decodeW(iter->tmpFileName.c_str());
				if (GetFileAttributesW(pwszFileName)!=INVALID_FILE_ATTRIBUTES)
					// File exists
					iter=picList.erase(iter);
				else if (iter->sessionID==0 || iter->ip==0 || iter->port==0)
					// Error on sending side (Continue may crash MIMQQ2 QunImage thread)
					iter=picList.erase(iter);
				else
					iter++;
				mir_free(pwszFileName);
			}
			if (picList.size()) {
				// Still items in picList
				EvaAskForCustomizedPicEvent *event = new EvaAskForCustomizedPicEvent();
				event->setPicList(picList);
				event->setQunID(dwQun);
				//CQunImage::PostEvent(event);
				if (!m_qunimage) m_qunimage=new CQunImage(this);
				m_qunimage->customEvent(event);
			}
		}

		HANDLE hContact;
		hContact=FindContact(dwQun);
		if (READC_D2("QunVersion")!=dwQunVersion) {
			util_log(0,"Qun %u: QunVersion differ (%u!=%u), requesting update",dwQun,READC_D2("QunVersion"),dwQunVersion);
			WRITEC_D("QunVersion",dwQunVersion);
			WRITEC_D("QunCardUpdate",-1);

			prot_qun_get_info(&m_client,dwQun,0);
		}
	}

	_qunImCallback2(dwQun,dwFrom,dwHasFontAttr!=0,format&0x01,format&0x02,format&0x04,fontSize,red,green,blue,dwSentTime,strShow);

}

void CNetwork::processNews(LPSTR pszArgs) {
	// news^$title^$brief^$url

	LPSTR pszNews, pszBrief, pszUrl;
	pszNews=pszArgs;
	pszBrief=strstr(pszNews,"^$")+2; pszBrief[-2]=0;
	pszUrl=strstr(pszBrief,"^$")+2; pszUrl[-2]=0;

	// **********

	if (/*READ_B2(NULL,QQ_SHOWAD)==1 &&*/ ServiceExists(MS_POPUP_ADDPOPUPW)) {
		POPUPDATAW ppd={0};
		LPWSTR pwszMsg;
		_tcscpy(ppd.lpwzContactName,pwszMsg=mir_utf8decodeW(pszNews));
		mir_free(pwszMsg);
		_tcscpy(ppd.lpwzText,pwszMsg=mir_utf8decodeW(pszBrief));
		mir_free(pwszMsg);
		ppd.lchIcon=(HICON)LoadImage(hinstance, MAKEINTRESOURCE(IDI_TM), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0);
		ppd.PluginWindowProc=_noticePopupProc;
		ppd.PluginData=mir_utf8decodeW(pszUrl);
		ppd.iSeconds=60;
		CallService(MS_POPUP_ADDPOPUPW,(WPARAM)&ppd,0);
	}
}

void CNetwork::processMail(LPSTR pszArgs) {
	// mail^$mailid^$sender^$title

	LPSTR pszMailID, pszSender, pszTitle;
	pszMailID=pszArgs;
	pszSender=strstr(pszMailID,"^$")+2; pszSender[-2]=0;
	pszTitle=strstr(pszSender,"^$")+2; pszTitle[-2]=0;

	// **********
	WCHAR szUrl[MAX_PATH]=L"http://mail.qq.com";
	LPWSTR pwszSender=mir_utf8decodeW(pszSender);
	LPWSTR pwszTitle=mir_utf8decodeW(pszTitle);

	// swprintf(szUrl,L"http://mail.qq.com/cgi-bin/login?Fun=clientread&Uin=%u&K=%S&Mailid=%S",m_myqq,EvaUtil:: m_client.data.im_key, pszMailID);
	POPUPDATAW ppd={0};
	_tcscpy(ppd.lpwzContactName,TranslateT("You got a new QQ Mail!"));
	_stprintf(ppd.lpwzText,TranslateT("Sender: %s\nTitle: %s"),pwszSender,pwszTitle);
	ppd.lchIcon=(HICON)LoadImage(hinstance, MAKEINTRESOURCE(IDI_TM), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0);
	ppd.PluginWindowProc=_noticePopupProc;
	ppd.PluginData=mir_wstrdup(szUrl);
	ppd.iSeconds=60;
	CallService(MS_POPUP_ADDPOPUPW,(WPARAM)&ppd,0);

	mir_free(pwszSender);
	mir_free(pwszTitle);
}

void CNetwork::processQunJoin(LPSTR pszArgs) {
	// qunjoin^$intid^$imtype^$extid^$subject^$commander^$type^$exttype^$msg^$codelen^$code^$tokenlen^$token
	unsigned char imtype, type, exttype;
	DWORD intid;
	DWORD extid;
	DWORD subject;
	DWORD commander;
	WORD codelen, tokenlen;
	LPSTR pszCode, pszToken;
	LPSTR pszMsg;
	LPSTR ppszArgs=pszArgs;

	intid=strtoul(ppszArgs,NULL,10);
	ppszArgs=strstr(ppszArgs,"^$")+2;

	imtype=atoi(ppszArgs);
	ppszArgs=strstr(ppszArgs,"^$")+2;

	extid=strtoul(ppszArgs,NULL,10);
	ppszArgs=strstr(ppszArgs,"^$")+2;

	subject=strtoul(ppszArgs,NULL,10);
	ppszArgs=strstr(ppszArgs,"^$")+2;

	commander=strtoul(ppszArgs,NULL,10);
	ppszArgs=strstr(ppszArgs,"^$")+2;

	type=atoi(ppszArgs);
	ppszArgs=strstr(ppszArgs,"^$")+2;

	exttype=atoi(ppszArgs);
	ppszArgs=strstr(ppszArgs,"^$")+2;

	pszMsg=ppszArgs;
	ppszArgs=strstr(ppszArgs,"^$")+2;
	ppszArgs[-2]=0;

	codelen=atoi(ppszArgs);
	ppszArgs=strstr(ppszArgs,"^$")+2;

	pszCode=ppszArgs;
	ppszArgs+=codelen+2;

	tokenlen=atoi(ppszArgs);
	ppszArgs=strstr(ppszArgs,"^$")+2;

	pszToken=ppszArgs;
	ppszArgs+=tokenlen+2;

	// *********
	TCHAR wszTemp[MAX_PATH]={0};

	switch (imtype) {
		case QQ_RECV_IM_APPROVE_APPLY_ADD_TO_QUN: 
			{
				swprintf(wszTemp,TranslateT("You have been approved to join Qun %u."),extid);
				ShowNotification(wszTemp,NIIF_INFO);
				qun_update_info(&m_client,qun_get(&m_client,intid,1));
				break;
			}
		case QQ_RECV_IM_APPLY_ADD_TO_QUN:
			_sysRequestJoinQunCallback(intid,extid,subject,pszMsg,(LPBYTE)pszToken,tokenlen/*,(LPBYTE)pszCode,codelen*/);
			break;
		case QQ_RECV_IM_REJECT_JOIN_QUN:
			_sysRejectJoinQunCallback(intid,extid,subject,pszMsg);
			break;
		case QQ_RECV_IM_SET_QUN_ADMIN:
			switch (exttype) {
				case QQ_QUN_SET_ADMIN:
					swprintf(wszTemp+1,TranslateT("The Qun Creator assigned %u to be administrator."),subject);
					break;
				case QQ_QUN_UNSET_ADMIN:
					swprintf(wszTemp+1,TranslateT("The Qun Creator revoked administrator identity of %u."),subject);
					break;
			}
			break;
		case QQ_RECV_IM_ADDED_TO_QUN:
		case QQ_RECV_IM_DELETED_FROM_QUN:
			{
				LPWSTR pwszComment=mir_utf8decodeW(pszMsg);

				switch (imtype) {
					case QQ_RECV_IM_DELETED_FROM_QUN:
						if (commander==0)
							swprintf(wszTemp+1,TranslateT("%u left this Qun."),subject);
						else
							swprintf(wszTemp+1,TranslateT("%u(%s) removed %d from this Qun."),commander,pwszComment,subject);
						break;
					case QQ_RECV_IM_ADDED_TO_QUN:
						_stprintf(wszTemp+1,TranslateT("%u(%s) added %d to this Qun."),commander,pwszComment,subject);
						break;
				}

				mir_free(pwszComment);
			}
			break;
	}

	if (wszTemp[1]) {
		PROTORECVEVENT pre;
		CCSDATA ccs;
		HANDLE hContact=FindContact(intid);
		LPSTR pszTemp=mir_utf8encodeW(wszTemp+1);

		pre.szMessage = pszTemp;
		pre.flags = PREF_UTF;
		pre.timestamp = (DWORD)time(NULL);
		pre.lParam = 0;

		ccs.hContact=hContact;
		ccs.szProtoService = PSR_MESSAGE;
		ccs.wParam = 0;
		ccs.lParam = ( LPARAM )&pre;
		CallService(MS_PROTO_CHAINRECV, 0, ( LPARAM )&ccs );

		WRITEC_D("CardVersion",-1);
		qun_update_info(&m_client,qun_get(&m_client,intid,1));

		mir_free(pszTemp);
	}
}

void CNetwork::processBuddySignature(LPSTR pszArgs) {
	// buddysignature^$uin^$time^$signature

	DWORD uid;
	DWORD tm;
	LPSTR pszSignature;
	LPSTR ppszArgs=pszArgs;

	uid=strtoul(ppszArgs,NULL,10);
	ppszArgs=strstr(ppszArgs,"^$")+2;

	tm=strtoul(ppszArgs,NULL,10);
	ppszArgs=strstr(ppszArgs,"^$")+2;

	pszSignature=ppszArgs;

	// **********

	HANDLE hContact=FindContact(uid);
	if (hContact) {
		bool fSong=pszSignature[strlen(pszSignature)-1]==1;

		WRITEC_U8S("PersonalSignature",pszSignature);

		if (fSong) {
			WRITEC_U8S("ListeningTo",pszSignature);
		} else {
			DBWriteContactSettingUTF8String(hContact,"CList","StatusMsg",pszSignature);
			DELC("ListeningTo");
		}
	}
}

void CNetwork::processBuddyWriting(LPSTR pszArgs) {
	DWORD uid=strtoul(pszArgs,NULL,10);

	util_log(0,"Received typing notification from %u",uid);

	if (HANDLE hContact=FindContact(uid)) {
		CallService(MS_PROTO_CONTACTISTYPING, (WPARAM)hContact, 5); // 5 secs
	}
}

void CNetwork::processWeather(LPSTR pszArgs) {
	// weather^$province^$city^$count^$time^$short_desc^$wind^$low^$high^$hints^$...
	LPSTR pszProvince;
	LPSTR pszCity;
	int count;
	DWORD tm;
	LPSTR pszShortDesc;
	LPSTR pszWind;
	int low;
	int high;
	LPSTR pszHints;
	LPSTR ppszArgs=pszArgs;

	WCHAR szMsg[2048];
	LPWSTR pszMsg=szMsg;

	// **********
	pszProvince=ppszArgs;
	ppszArgs=strstr(ppszArgs,"^$")+2;
	ppszArgs[-2]=0;

	pszCity=ppszArgs;
	ppszArgs=strstr(ppszArgs,"^$")+2;
	ppszArgs[-2]=0;

	count=atoi(ppszArgs);
	ppszArgs=strstr(ppszArgs,"^$")+2;

	LPWSTR lpwzProvince=mir_utf8decodeW(pszProvince);
	LPWSTR lpwzCity=mir_utf8decodeW(pszCity);
	LPWSTR lpwzTime;
	LPWSTR lpwzShortDesc;
	LPWSTR lpwzWind;
	LPWSTR lpwzHint;

	_stprintf(szMsg,TranslateT("Weather of %s %s:\n\n"),lpwzProvince,lpwzCity);
	mir_free(lpwzProvince);
	mir_free(lpwzCity);

	for (int c=0; c<count; c++) {
		tm=strtoul(ppszArgs,NULL,10);
		ppszArgs=strstr(ppszArgs,"^$")+2;

		pszShortDesc=ppszArgs;
		ppszArgs=strstr(ppszArgs,"^$")+2;
		ppszArgs[-2]=0;

		pszWind=ppszArgs;
		ppszArgs=strstr(ppszArgs,"^$")+2;
		ppszArgs[-2]=0;

		low=atoi(ppszArgs);
		ppszArgs=strstr(ppszArgs,"^$")+2;

		high=atoi(ppszArgs);
		ppszArgs=strstr(ppszArgs,"^$")+2;

		pszHints=ppszArgs;
		ppszArgs=strstr(ppszArgs,"^$")+2;
		ppszArgs[-2]=0;

		lpwzShortDesc=mir_utf8decodeW(pszShortDesc);
		lpwzWind=mir_utf8decodeW(pszWind);
		lpwzHint=mir_utf8decodeW(pszHints);
		lpwzTime=_tctime((time_t*)&tm);
		*wcsstr(lpwzTime,L"00:")=0;
		pszMsg+=_stprintf(pszMsg,TranslateT("%sCondition: %s\nWind: %s\nMax: %d Min: %d\n%s\n\n"),lpwzTime,lpwzShortDesc,lpwzWind,high,low,lpwzHint);
		mir_free(lpwzShortDesc);
		mir_free(lpwzWind);
		mir_free(lpwzHint);
	}

	ForkThread((ThreadFunc)&CNetwork::ThreadMsgBox,mir_tstrdup(szMsg));
}

void CNetwork::processQunMyJoin(LPSTR pszArgs) {
	// qunjoin2^$intid^$code^$extid^$result
	// qunjoin2^$intid^$code^$msg

	DWORD dwIntID, dwExtID;
	unsigned char code, result;
	LPSTR ppszArgs=pszArgs;

	dwIntID=strtoul(ppszArgs,NULL,10);
	ppszArgs=strstr(ppszArgs,"^$")+2;

	code=atoi(ppszArgs);
	ppszArgs=strstr(ppszArgs,"^$")+2;

	if (code==0) {
		dwExtID=strtoul(ppszArgs,NULL,0);
		ppszArgs=strstr(ppszArgs,"^$")+2;

		result=atoi(ppszArgs);

		switch (result) {
			// case QQ_QUN_NO_AUTH: // Not seen
			case QQ_QUN_JOIN_OK: // Qun Admin approved request
				qun_update_info(&m_client,qun_get(&m_client,dwIntID,1));
				break;
			case QQ_QUN_JOIN_DENIED: // Qun Admin rejected request
				{
					// TODO: How does it happen with error msg filled? Impossible
					TCHAR szMsg[MAX_PATH];
					// LPTSTR pszServerMsg=mir_a2u_cp(packet->getErrorMessage().c_str(),936);

					_stprintf(szMsg,TranslateT("Administrator of Qun %u denied your join request"),dwExtID);
					ShowNotification(szMsg,NIIF_ERROR);
					//mir_free(pszServerMsg);
				}
				break;

			case QQ_QUN_JOIN_NEED_AUTH: // This Qun need authorization
				{
					util_log(0,"2006: Qun need authorization (default), request auth info");

					// TODO: No longer valid
					/*
					if (HANDLE hContact=FindContact(m_deferActionData)) {
						prot_user_request_token(&m_client,m_deferActionData,
						EvaAddFriendGetAuthInfoPacket *packet = new EvaAddFriendGetAuthInfoPacket(READC_D2("ExternalID"), AUTH_INFO_CMD_INFO, true);
						packet->setVerificationStr("");
						packet->setSessionStr("");
						append(packet);
					} else {
						util_log(0,"ERROR: hContact==NULL");
					}
					*/
				}
				break;
				/* TODO: code==0 can't go here!
			case QQ_QUN_CMD_JOIN_QUN_AUTH: // Requested to join a Qun with authorization message
				if (!packet->isReplyOK()) {
					TCHAR szMsg[MAX_PATH];
					LPTSTR pszServerMsg=mir_a2u_cp(packet->getErrorMessage().c_str(),936);

					_stprintf(szMsg,TranslateT("Administrator of Qun %d denied your join request\n%s"),qunid,pszServerMsg);
					ShowNotification(szMsg,NIIF_ERROR);
					mir_free(pszServerMsg);
				}
				break;
				*/
		}
	} else {
		TCHAR szMsg[MAX_PATH];
		LPTSTR pszServerMsg=mir_utf8decodeW(ppszArgs);

		_stprintf(szMsg,TranslateT("Add qun result of qun %d:\n%s"),dwIntID,pszServerMsg);
		ShowNotification(szMsg,NIIF_INFO);
		mir_free(pszServerMsg);
	}
}

void CNetwork::registerCallbacks() {
	callbacks["process"]=&CNetwork::processProcess;
	callbacks["clusterinfo"]=&CNetwork::processClusterInfo;
	callbacks["buddylist"]=&CNetwork::processBuddyList;
	callbacks["grouplist"]=&CNetwork::processGroupList;
	callbacks["buddystatus"]=&CNetwork::processBuddyStatus;
	callbacks["status"]=&CNetwork::processStatus;
	callbacks["buddyinfo"]=&CNetwork::processBuddyInfo;
	callbacks["search_uid_reply"]=&CNetwork::processSearchUid;
	callbacks["qun_search_reply"]=&CNetwork::processQunSearch;
	callbacks["request_addbuddy_reply"]=&CNetwork::processRequestAddBuddy;
	callbacks["request_token_reply"]=&CNetwork::processRequestToken;
	callbacks["del_buddy_reply"]=&CNetwork::processDelBuddy;
	callbacks["misc_broadcast"]=&CNetwork::processBroadcast;
	callbacks["login_touch_redirect"]=&CNetwork::processLoginTouchRedirect;
	callbacks["clustermembernames"]=&CNetwork::processClusterMemberNames;
	callbacks["imsendmsg"]=&CNetwork::processIMSendMsgReply;
	callbacks["buddyimtextadv"]=&CNetwork::processBuddyIMTextAdv;
	callbacks["qunimadv"]=&CNetwork::processQunIMAdv;
	callbacks["news"]=&CNetwork::processNews;
	callbacks["mail"]=&CNetwork::processMail;
	callbacks["qunjoin"]=&CNetwork::processQunJoin;
	callbacks["buddysignature"]=&CNetwork::processBuddySignature;
	callbacks["buddywriting"]=&CNetwork::processBuddyWriting;
	callbacks["weather"]=&CNetwork::processWeather;
	callbacks["qunmyjoin"]=&CNetwork::processQunMyJoin;
}

void CNetwork::_eventCallback(char* msg) {
	util_log(0,"_eventCallback: %s",msg);

	LPSTR pszArgs=strstr(msg,"^$");
	*pszArgs=0;
	pszArgs+=2;

	for (map<LPCSTR,CallbackFunc>::iterator iter=callbacks.begin(); iter!=callbacks.end(); iter++) {
		if (!strcmp(msg,iter->first)) {
			(this->*(iter->second))(pszArgs);
			break;
		}
	}
}
