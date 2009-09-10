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

//extern void __cdecl _get_infothread(HANDLE hContact);
extern "C" BOOL CALLBACK ModifySignatureDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);

void ftCallbackHub(int command, int subcommand, WPARAM wParam, LPARAM lParam);

void CNetwork::_updateQunCard(HANDLE hContact, const int qunid) {
	int qunCardUpdate=READC_D2("QunCardUpdate");

	if (qunCardUpdate!=-1 && time(NULL)-qunCardUpdate>60) {
		util_log(0,"_updateQunCard(): QunCardUpdate=%d, time=%d",READC_D2("QunCardUpdate"), time(NULL));
		WRITEC_D("QunCardUpdate",(DWORD)time(NULL));
		append(new QunRequestAllRealNames(qunid));
	}
}

LRESULT CALLBACK _noticePopupProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam){ 
	switch( message ) {
			case WM_COMMAND:
				{
					if (LPVOID data=PUGetPluginData(hWnd)) {
						//ShellExecuteW(NULL,NULL,(LPTSTR)data,NULL,NULL,SW_SHOWNORMAL);
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
		/*
		if (qqSettings->allowUpdateTime) {
			time_t newOffset=time(NULL)-sentTime;
			if (newOffset < 1800 && newOffset > -1800) {
				qqSettings->serverTimeOffset=newOffset;
				util_log(0,"Server offset updated (%d sec)",newOffset);
			}
		}
		*/

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

	itoa(senderQQ,szUID,10);
	char* pszMsg;
	DBVARIANT dbv;
	bool hideMessage=false;

	if (DBGetContactSetting(hContact,m_szModuleName,szUID,&dbv)) {
		// No Nick
		pszMsg=(char*)mir_alloc((strlen(szUID)+message.length()+60)*3);
		sprintf(pszMsg,"%s:\n",szUID);
	} else {
		// With Nick
		pszMsg=(char*)mir_alloc((strlen(dbv.pszVal)+strlen(szUID)+3+message.length()+60)*3);
		sprintf(pszMsg,"%s (%s):\n",dbv.pszVal,szUID);
		DBFreeVariant(&dbv);
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

	strcat(pszMsg,message.c_str());

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
		LPWSTR pszTempS=mir_a2u_cp(pszMsg,936);
		LPWSTR pszTempT=mir_tstrdup(pszTempS);
		LCMapString(GetUserDefaultLCID(),LCMAP_TRADITIONAL_CHINESE,pszTempS,wcslen(pszTempS)+1,pszTempT,wcslen(pszTempT)+1);
		pre.szMessage=mir_utf8encodeW(pszTempT);
		mir_free(pszTempS);
		mir_free(pszTempT);
	} else
		pre.szMessage=mir_utf8encodecp(pszMsg,936);

	if (READC_W2("Status")==ID_STATUS_DND) pre.flags+=PREF_CREATEREAD;
	pre.timestamp = (DWORD)sentTime+600<READ_D2(NULL,"LoginTS")?(DWORD)sentTime:(DWORD)time(NULL);

	CallService(MS_PROTO_CHAINRECV, 0, (LPARAM)&ccs);
#ifdef MIRANDAQQ_IPC
	ipcmessage_t ipcm={hContact,qunID,pre.timestamp,mir_a2u_cp(pszMsg,936)};
	NotifyEventHooks(hIPCEvent,QQIPCEVT_RECV_MESSAGE,(LPARAM)&ipcm);
	mir_free(ipcm.message);
#endif
	mir_free(pre.szMessage);
	mir_free(pszMsg);
}
#if 0 // UPGRADE_DISABLE
// errorCallback(): Callback Function for Connection Error
// errorCode: Error Code
// errorMsg: Error Message in GBK (If any)
#if 0
void errorCallback(int errorCode, const char* errorMsg) {
	switch (errorCode) {
#if 0
		case QQNetwork::QQNETERR_CONNLOST:
			util_log(0,"%s(): Lost Connection!",__FUNCTION__);
			ShowNotification(Translate("Connection to QQ server lost"),NIIF_ERROR);
			CallProtoService(m_szModuleName,PS_SETSTATUS,ID_STATUS_OFFLINE,0);
			break;
		case QQNetwork::QQQUNERR_NEEDAUTH:
			{
				HANDLE hContact=FindContact(qqSettings->addUID);
				DBVARIANT dbv={0};

				if (hContact) DBGetContactSetting(hContact,m_szModuleName,"AuthReason",&dbv);
				if (!hContact || !dbv.pszVal) {
					mir_forkthread(util_pthread_msgbox,strdup(Translate("Qun need authorization. Please enable authorization and enter your reason.")));
					qqSettings->addUID=0;

					DBWriteContactSettingByte(hContact,"CList","NotOnList",1);
				} else {
					QunAuthPacket* packet=new QunAuthPacket(qqSettings->addUID,QQ_QUN_AUTH_REQUEST);
					util_convertToGBK(dbv.pszVal);
					packet->setMessage(dbv.pszVal);
					network->append(packet);
					DBFreeVariant(&dbv);
				}
				break;
			}
#endif
		case QQNetwork::QQQUNERR_NOADD:
			mir_forkthread(util_pthread_msgbox,strdup(Translate("This Qun does not allow others to join")));
			break;
		case QQNetwork::QQQUNERR_MISC:
			{
				char* szMsg=strdup(errorMsg);
				util_convertFromGBK(szMsg);
				mir_forkthread(util_pthread_msgbox,szMsg);
			}
			break;
		case QQNetwork::QQQUNERR_IMAGE:
			{
				util_convertFromGBK((char*)errorMsg);
				mir_forkthread(util_pthread_msgbox,strdup(errorMsg));
			}
			break;
	}
}
#endif

extern int filter(unsigned int code, struct _EXCEPTION_POINTERS *ep);
#endif // UPGRADE_DISABLE
// IMCallback(): Callback Function for IM Retrieval
// imType: Type of IM received
// data: Different types of IM object depending on type of IM
void CNetwork::_imCallback(const int imType, const void* data) {
	switch (imType) {
		case QQ_RECV_IM_TO_BUDDY:
		case QQ_RECV_IM_FROM_BUDDY_2006:
		case QQ_RECV_IM_TO_UNKNOWN: 
		{
			ReceivedNormalIM * received = (ReceivedNormalIM*)data;
			HANDLE hContact=FindContact(received->getSender());
			if (!hContact) {
				hContact=AddContact(received->getSender(),true,false);
				append(new GetUserInfoPacket(received->getSender()));
			}

			char* msg;
			string sOut;

#if 0
			EvaHtmlParser parser;
			std::list<CustomizedPic> picList = parser.parseCustomizedPics((char*)received->getMessage().c_str(),false);

			if (picList.size()) {
				/*
				FILE* fp=fopen("C:\\imdebug.txt","wb");
				fwrite(received->getBodyData(),received->getBodyLength(),1,fp);
				fclose(fp);
				*/

				char szPath[MAX_PATH];
				std::list<CustomizedPic>::iterator iter;
				CallService(MS_FILE_GETRECEIVEDFILESFOLDER,(WPARAM)hContact,(LPARAM)szPath);
				strcat(szPath,"\\");

				for(iter=picList.begin(); iter!=picList.end(); iter++) {
					strcpy(strrchr(szPath,'\\'),iter->fileName.c_str());
					if (_access(szPath,0) && strrchr(szPath,'\\')[1]=='{') {
						// File not exist
						util_log(0,"ZDY file %s not found, put to queue",szPath);
						m_storedIM.push_back(*received);
						return;
					}
				}

				char* szMessage=mir_strdup(received->getMessage().c_str());
				char* pszMessage=szMessage;

				strrchr(szPath,'\\')[1]=0;

				// All files available
				while (strstr(pszMessage,"[ZDY]")) {
					*strstr(pszMessage,"[ZDY]")=0;
					sOut+=pszMessage;
					sOut+="[img]";
					sOut+=szPath;
					sOut+=picList.front().fileName;
					sOut+="[/img]";
					picList.pop_front();

					pszMessage=strstr(pszMessage+strlen(pszMessage)+1,"[/ZDY]")+6;

					if (!strstr(pszMessage,"[ZDY]")) {
						sOut+=pszMessage;
					} 
				}

				mir_free(szMessage);
				msg=(char*)mir_alloc((sOut.length()+64)*3);
			} else {
#endif
			{
				msg=(char*)mir_alloc((received->getMessage().length()+64)*3);
			}

			PROTORECVEVENT pre;
			CCSDATA ccs;
			*msg=0;
			if (g_enableBBCode && received->hasFontAttribute()) {
				sprintf(msg,"[color=#%02x%02x%02x]",(unsigned char)received->getRed(),(unsigned char)received->getGreen(),(unsigned char)received->getBlue());

				if (received->isBold()) strcat(msg,"[b]");
				if (received->isItalic()) strcat(msg,"[i]");
				if (received->isUnderline()) strcat(msg,"[u]");
			}

			if (sOut.empty()) {
				strcat(msg,received->getMessage().c_str());
			} else {
				strcat(msg,sOut.c_str());
			}

			if (g_enableBBCode && received->hasFontAttribute()) {
				if (received->isBold()) strcat(msg,"[/b]");
				if (received->isItalic()) strcat(msg,"[/i]");
				if (received->isUnderline()) strcat(msg,"[/u]");
				strcat(msg,"[/color]");
			}

			//LPWSTR pszUnicode=NULL;

			pre.flags=PREF_UTF;

			if (READ_B2(NULL,QQ_MESSAGECONVERSION) > 1) {
				LPWSTR pszTempS=mir_a2u_cp(msg,936);
				LPWSTR pszTempT=mir_tstrdup(pszTempS);
				LCMapString(GetUserDefaultLCID(),LCMAP_TRADITIONAL_CHINESE,pszTempS,wcslen(pszTempS)+1,pszTempT,wcslen(pszTempT)+1);
				pre.szMessage=mir_utf8encodeW(pszTempT);
				mir_free(pszTempS);
				mir_free(pszTempT);
			} else
				pre.szMessage=mir_utf8encodecp(msg,936);

			pre.timestamp = (DWORD) time(NULL);
			pre.lParam = 0;

			ccs.hContact=hContact;
			ccs.szProtoService = PSR_MESSAGE;
			ccs.wParam = 0;
			ccs.lParam = ( LPARAM )&pre;
			CallService(MS_PROTO_CHAINRECV, 0, ( LPARAM )&ccs );
			
#ifdef MIRANDAQQ_IPC
			ipcmessage_t ipcm={hContact,READC_D2(UNIQUEIDSETTING),pre.timestamp,mir_a2u_cp(pre.szMessage,936)};
			NotifyEventHooks(hIPCEvent,QQIPCEVT_RECV_MESSAGE,(LPARAM)&ipcm);
			mir_free(ipcm.message);
#endif

			mir_free(msg);
			mir_free(pre.szMessage);
			WRITEC_W("Sequence",received->getSequence());
			WRITEC_S("FileSessionKey",(char*)received->getBuddyFileSessionKey());

			break;
		}
		case 0x1f:
			{
				ReceivedTempSessionTextIMPacket* received = (ReceivedTempSessionTextIMPacket*)data;
				HANDLE hContact=FindContact(received->getSender()+0x80000000);
				PROTORECVEVENT pre;
				CCSDATA ccs;
				char msg2[1024]={0};

				pre.flags=0;
				ccs.hContact=hContact;
				ccs.szProtoService = PSR_MESSAGE;
				ccs.wParam = 0;
				ccs.lParam = ( LPARAM )&pre;
				pre.timestamp = (DWORD) time(NULL);
				pre.lParam = 0;

				if (!hContact) {
					LPWSTR pszTemp, pszTemp2;
					hContact=AddContact(received->getSender()+0x80000000,true,false);
					ccs.hContact=hContact;
					pre.szMessage = msg2;
					pre.flags=PREF_UNICODE;

					pszTemp=mir_a2u_cp(received->getNick().c_str(),936);
					pszTemp2=mir_a2u_cp(received->getSite().c_str(),936);
					_stprintf((LPWSTR)msg2,TranslateT("Temp Session: %s(%d) in %s"),pszTemp,received->getSender(),pszTemp2);
					mir_free(pszTemp);
					mir_free(pszTemp2);

				}

				//char* msg=(char*)malloc((received->getMessage().length()+64)*3);
				char* msg=(char*)mir_alloc(received->getMessage().length()+64);
				*msg=0;
				/*if (qqSettings->enableBBCode)*/ {
					COLORREF color=RGB((unsigned char)received->getRed(),(unsigned char)received->getGreen(),(unsigned char)received->getBlue());

					if (received->isBold()) strcat(msg,"[b]");
					if (received->isItalic()) strcat(msg,"[i]");
					if (received->isUnderline()) strcat(msg,"[u]");

					sprintf(msg+strlen(msg),"[color=#%02x%02x%02x]",(unsigned char)received->getRed(),(unsigned char)received->getGreen(),(unsigned char)received->getBlue());
				}
				strcat(msg,received->getMessage().c_str());

				/*if (qqSettings->enableBBCode)*/ {
					if (received->isBold()) strcat(msg,"[/b]");
					if (received->isItalic()) strcat(msg,"[/i]");
					if (received->isUnderline()) strcat(msg,"[/u]");
					strcat(msg,"[/color]");
				}

#if 0
				LPWSTR pszUnicode=NULL;

				/*if (qqSettings->unicode)*/ {
					pszUnicode=mir_a2u_cp(msg,936);
				}

				// TODO: !
				util_convertFromGBK(msg);

				if (pszUnicode) {
					pre.flags = PREF_UNICODE;
					wcscpy((LPWSTR)(msg+strlen(msg)+1),pszUnicode);
					mir_free(pszUnicode);
				}
#endif
				LPSTR pszUTF8=mir_utf8encodecp(msg,936);
				pre.szMessage = pszUTF8;
				pre.flags=PREF_UTF;
				pre.timestamp++;

				CallService(MS_PROTO_CHAINRECV, 0, ( LPARAM )&ccs );

				//free(msg);
				mir_free(pszUTF8);
				mir_free(msg);

				if (*msg2) {
					DBWriteContactSettingString(hContact,m_szModuleName,"Site",received->getSite().c_str());
					WRITEC_TS("Nick",(LPTSTR)msg2);
				}

				break;
			}
	}
}

#if 0
// buddyOnlineFinishedCallback(): Callback Function for Buddy Online enumeration finish
void buddyOnlineFinishedCallback() {
	HANDLE hContact;
	hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDFIRST,0,0);
	time_t now=time(NULL);

	while(hContact!=NULL) {
		if(!lstrcmpA(m_szModuleName,(char*)CallService(MS_PROTO_GETCONTACTBASEPROTO,(WPARAM)hContact,0))) {
			// I am responsible for this contact, check last update
			if (READC_B2("ChatRoom")==0 && 
				DBGetContactSettingWord(hContact,m_szModuleName,"Status",ID_STATUS_OFFLINE)!=ID_STATUS_OFFLINE &&
				READC_B2("IsQun")==0 &&
				now-READC_D2("TickTS")>30)
				// Buddy online not received, make him/her offline
				WRITEC_W("Status",ID_STATUS_OFFLINE);
		}
		hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDNEXT,(WPARAM)hContact,0);
	}
}
#endif
// ackCallback(): Callback Function for some acknowledge messages
#if 0
void ackCallback(const unsigned short ackType, const void* data, const void* data2) {
	GETSETTINGS();
	switch (ackType) {
		case QQNetwork::QQADDAUTH_SENT: // Authorization Request Sent
			WCHAR szMsg[MAX_PATH];
			swprintf(szMsg,TranslateT("Authorization request for User %d send successfully.\nThe user will be in your contact list when he/she approve your request."),(unsigned int)data);
			ShowNotification(szMsg,NIIF_INFO);
			qqSettings->addUID=0;
			break;
		case QQNetwork::QQNETCB_PERSONALSIGREMOVED: // Personal Signature Removed
			DBDeleteContactSetting(NULL,m_szModuleName,"PersonalSignature");
			break;
		case QQNetwork::QQNETCB_PERSONALSIGMODIFIED: // Personal Signature Modified
			//qqNetwork->requestSignature(qqSettings->myUID);
			//ShowNotification(Translate("Personal Signature Modified"),NIIF_INFO);
			break;
	}
}
#endif

// sysRequestJoinQunCallback(): Callback Function for application to join Qun
//                                     
void CNetwork::_sysRequestJoinQunCallback(int qunid, int extid, int userid, const char* msg, const unsigned char *token, const unsigned short tokenLen) {
	char *reason_utf8;
	Qun* qun=m_qunList.getQun(qunid);

	//CCSDATA ccs;
	//PROTORECVEVENT pre;
	HANDLE hContact;
	char* szBlob;
	char* pCurBlob;

	hContact=FindContact(qunid);

	if (hContact && qun) { // The qun is initialized, proceed
		char szEmail[MAX_PATH];

		reason_utf8=mir_strdup(msg);
		util_convertFromGBK(reason_utf8);

		sprintf(szEmail,"%s (%d)",qun->getDetails().getName().c_str(),qunid);
		util_convertFromGBK(szEmail);
		util_log(0,"%s(): QunID=%d, QQID=%d, msg=%s",__FUNCTION__,qunid,userid,reason_utf8);

#if 0
		ccs.szProtoService=PSR_AUTH;
		ccs.hContact=hContact;
		ccs.wParam=0;
		ccs.lParam=(LPARAM)&pre;
		pre.flags=0;
		pre.timestamp=(DWORD)time(NULL);
		pre.lParam=sizeof(DWORD)+3+sizeof(HANDLE)+strlen(reason_utf8)+strlen(szEmail)+5+tokenLen+2;

		/*blob is: uin(DWORD), hcontact(HANDLE), nick(ASCIIZ), first(ASCIIZ), last(ASCIIZ), email(ASCIIZ), reason(ASCIIZ)*/
		pCurBlob=szBlob=(char *)malloc(pre.lParam);
		memcpy(pCurBlob,&userid,sizeof(DWORD)); pCurBlob+=sizeof(DWORD);
		memcpy(pCurBlob,&hContact,sizeof(HANDLE)); pCurBlob+=sizeof(HANDLE);
		strcpy((char *)pCurBlob," "); pCurBlob+=2;
		strcpy((char *)pCurBlob," "); pCurBlob+=2;
		strcpy((char *)pCurBlob," "); pCurBlob+=2;
		strcpy((char *)pCurBlob,szEmail); pCurBlob+=(strlen(szEmail)+1);
		strcpy((char *)pCurBlob,reason_utf8); pCurBlob+=(strlen(reason_utf8)+1);
		*(unsigned short*)pCurBlob=tokenLen;
		memcpy(pCurBlob+2,token,tokenLen);
		
		pre.szMessage=(char *)szBlob;

		CallService(MS_PROTO_CHAINRECV,0,(LPARAM)&ccs);

		mir_free(reason_utf8);
#endif
		DBEVENTINFO dbei;

		util_log(0,"%s(): Received authorization request",__FUNCTION__);
		// Show that guy
		DBDeleteContactSetting(hContact,"CList","Hidden");

		ZeroMemory(&dbei,sizeof(dbei));
		dbei.cbSize=sizeof(dbei);
		dbei.szModule=m_szModuleName;
		dbei.timestamp=(DWORD)time(NULL);
		dbei.flags=0;
		dbei.eventType=EVENTTYPE_AUTHREQUEST;
		dbei.cbBlob=sizeof(DWORD)+3+sizeof(HANDLE)+strlen(reason_utf8)+strlen(szEmail)+5+tokenLen+2;;

		/*blob is: uin(DWORD), hcontact(HANDLE), nick(ASCIIZ), first(ASCIIZ), last(ASCIIZ), email(ASCIIZ), reason(ASCIIZ)*/
		pCurBlob=szBlob=(char *)malloc(dbei.cbBlob);
		memcpy(pCurBlob,&userid,sizeof(DWORD)); pCurBlob+=sizeof(DWORD);
		memcpy(pCurBlob,&hContact,sizeof(HANDLE)); pCurBlob+=sizeof(HANDLE);
		strcpy((char *)pCurBlob," "); pCurBlob+=2;
		strcpy((char *)pCurBlob," "); pCurBlob+=2;
		strcpy((char *)pCurBlob," "); pCurBlob+=2;
		strcpy((char *)pCurBlob,szEmail); pCurBlob+=(strlen(szEmail)+1);
		strcpy((char *)pCurBlob,reason_utf8); pCurBlob+=(strlen(reason_utf8)+1);
		*(unsigned short*)pCurBlob=tokenLen;
		memcpy(pCurBlob+2,token,tokenLen);

		dbei.pBlob=(PBYTE)szBlob;
		CallService(MS_DB_EVENT_ADD,(WPARAM)NULL,(LPARAM)&dbei);

		mir_free(reason_utf8);
	}

}

// sysRejectJoinQunCallback(): Callback Function for Qun Admin rejected your join Qun request
void CNetwork::_sysRejectJoinQunCallback(int qunid, int extid, int userid, const char* msg) {
	TCHAR szTemp[MAX_PATH];
	LPTSTR pszReason=mir_a2u_cp(msg,936);

	swprintf(szTemp,TranslateT("You request to join group %d has been rejected by admin %d.\nReason:\n\n%s"), extid,userid,pszReason);
	mir_free(pszReason);
	ForkThread((ThreadFunc)&CNetwork::ThreadMsgBox,szTemp);
}

#define WRITEQUNINFO_TS(k,i) pszTemp=mir_a2u_cp(i.c_str(),936); WRITEC_TS(k,pszTemp); mir_free(pszTemp)
#if 0 // UPGRADE_DISABLE
void _ftUpdateBuddyIP(const int qqid, const unsigned int ip, const unsigned short port) {
	HANDLE hContact=FindContact(qqid);
	if (hContact) {
		DBWriteContactSettingDword(hContact,m_szModuleName,"IP",ip);
		DBWriteContactSettingWord(hContact,m_szModuleName,"Port",port);
	}
}

void _ftReceivedFileRequest(const int qqid, const unsigned int sessionid, const char* file, const int fileSize, const unsigned char type) {
#if 0
	if(type == QQ_TRANSFER_FILE) {
		int tFileNameLen = strlen(file);
		char tComment[40];
		int tCommentLen = mir_snprintf(tComment, sizeof(tComment), Translate("%ld bytes"), fileSize);
		ft_t* ft=new ft_t();
		char* szBlob = (char*)malloc(sizeof(ft) + tFileNameLen + tCommentLen + 2 );

		ft->file=file;
		ft->qqid=qqid;
		ft->sessionid=sessionid;
		ft->size=fileSize;

		*( PDWORD )szBlob = ( DWORD )ft;
		util_convertFromGBK((char*)file);
		strcpy(szBlob + sizeof(DWORD), file);
		strcpy(szBlob + sizeof(DWORD) + tFileNameLen + 1, tComment );

		PROTORECVEVENT pre;
		pre.flags = 0;
		pre.timestamp = (DWORD)time(NULL);
		pre.szMessage = (char*)szBlob;
		pre.lParam = (LPARAM)type;

		CCSDATA ccs;
		ccs.hContact = FindContact(qqid);
		ccs.szProtoService = PSR_FILE;
		ccs.wParam = sessionid;
		ccs.lParam = ( LPARAM )&pre;
		CallService(MS_PROTO_CHAINRECV, 0, (LPARAM)&ccs);
	}

	HANDLE hContact=FindContact(qqid);
	if (hContact) {
		if(QQNetwork::fileManager){
			util_log(0,"EvaMain::slotReceivedFileRequest: -- new session: %d\n", sessionid);
			list<string> dirList;
			list<string> fileList;
			list<unsigned int> sizeList;
			char szPath[MAX_PATH];
			CallService(MS_FILE_GETRECEIVEDFILESFOLDER,(WPARAM)hContact,(LPARAM)szPath);

			dirList.push_back(szPath);
			fileList.push_back(file);
			sizeList.push_back(fileSize);
			// create udp download session, and wait for ip notification from your friend
			QQNetwork::fileManager->newSession(qqid, sessionid, dirList, fileList, sizeList, true, type);

			if(type == QQ_TRANSFER_IMAGE){
				EvaIPAddress eip(qqNsThread->qqn->getIP());
				util_log(0,"%s(): Recieved QQ_TRANSFER_IMAGE",__FUNCTION__);
				QQNetwork::fileManager->saveFileTo(qqid,sessionid,szPath);
				SendFileExRequestPacket *packet = new SendFileExRequestPacket(QQ_IM_EX_REQUEST_ACCEPTED);
				packet->setReceiver(qqid);
				packet->setTransferType(type);
				packet->setWanIp(htonl(eip.IP()));
				packet->setSessionId(sessionid);
				network->append(packet);
				util_log(0,"EvaPacketManager::doAcceptFileRequest -- sent\n");
				return;
			}

		}
	}
#endif
}

void _ftNotifyAddressRequest(const int qq, const unsigned int session, const unsigned int synsession, const unsigned int ip, const unsigned short port, const unsigned int myip, const unsigned short myport);

void _ftReceivedFileAccepted(const int qqid, const unsigned int sessionid, const unsigned int wanip, const bool accepted, const unsigned char type) {
#if 0
	if (ft_t* ft=ftSessions[sessionid]) {
		string filename=ft->file;
		if(filename.empty()) return; // we haven't got the request

		if(accepted){
			// here we start udp upload session while buddy is waiting your ip notification
			// if user is using proxy, we send ip notification directly
			NETLIBUSERSETTINGS nlus={sizeof(NETLIBUSERSETTINGS)};
			CallService(MS_NETLIB_GETUSERSETTINGS,(WPARAM)hNetlibUser,(LPARAM)&nlus);
			//if(nlus.useProxy==1)
			_ftNotifyAddressRequest(qqid, sessionid, 0, 0, 0, 0, 0);
			/*else
			QQNetwork::fileManager->startSession(qqid, sessionid);*/
		} else {
			QQNetwork::fileManager->stopThread(qqid, sessionid);
			if (ft_t* ft=ftSessions[sessionid]) {
				ProtoBroadcastAck(m_szModuleName,FindContact(qqid),ACKTYPE_FILE,ACKRESULT_FAILED,(HANDLE)ft,0);
				ftSessions.erase(sessionid);
				delete ft;
			}
		}
		util_log(0,"receivedFileAccepted.\n");
	}
#endif
}

void _ftReceivedFileAgentInfo(const int qqid, const short sequence, const unsigned int sessionid, const unsigned int wanip, const unsigned short port, const char* agentkey) {
#if 0
	if(ft_t* ft=ftSessions[sequence]){
		HANDLE hContact=FindContact(qqid);
		ProtoBroadcastAck(m_szModuleName,hContact,ACKTYPE_FILE,ACKRESULT_CONNECTING,(HANDLE)ft,0);
		/*
		in_addr ia;
		ia.S_un.S_addr=htonl(wanip);
		*/
		unsigned int ip=htonl(wanip);
		// buddy ask you to download the file from relay server
		QQNetwork::fileManager->changeToAgent(qqid, sequence);util_log(0,"EvaMain::slotReceivedFileAgentInfo -- changeToAgent\n");
		QQNetwork::fileManager->changeSessionTo(qqid, sequence, sessionid);util_log(0,"EvaMain::slotReceivedFileAgentInfo -- changeSessionTo\n");
		QQNetwork::fileManager->setBuddyAgentKey(qqid, sessionid, (unsigned char*)agentkey);util_log(0,"EvaMain::slotReceivedFileAgentInfo -- setBuddyAgentKey\n");
		QQNetwork::fileManager->setAgentServer(qqid, sessionid, htonl(wanip), port);util_log(0,"EvaMain::slotReceivedFileAgentInfo -- setAgentServer\n");
		QQNetwork::fileManager->startSession(qqid, sessionid); // we start agent download session
		util_log(0,"EvaMain::slotReceivedFileAgentInfo -- startSession\n");
		util_log(0,"EvaMain::slotReceivedFileAgentInfo: -- ip:%s\tport:%d\told:%d\tnew:%d\n", 
			inet_ntoa(/*ia*/*(in_addr*)&ip), port, sequence, sessionid);
		
		ftSessions.erase(sequence);
		ftSessions[sessionid]=ft;
		ft->sessionid=sessionid;
	}
#endif
}

void _ftReceivedFileNotifyIpEx(const int qqid, const bool isSender, const unsigned int sessionid, const unsigned char type, const unsigned int ip1, const unsigned int port1, const unsigned int ip2, const unsigned int port2, const unsigned int ip3, const unsigned int port3, const unsigned int lip1, const unsigned int lport1, const unsigned int lip2, const unsigned int lport2, const unsigned int lip3, const unsigned int lport3, const unsigned int syncip, const unsigned int syncport, const unsigned int syncsession) {
#if 0
	if(isSender) {  // true means buddy started udp upload session
		// we should start udp download session, but now
		// we simply ask buddy to use relay server :)
		HANDLE hContact=FindContact(qqid);
		ProtoBroadcastAck(m_szModuleName,hContact,ACKTYPE_FILE,ACKRESULT_INITIALISING,(HANDLE)ftSessions[sessionid],0);
		SendIpExNotifyPacket *packet = new SendIpExNotifyPacket(true);
		packet->setReceiver(qqid);
		packet->setSessionId(sessionid);
		packet->setSender(false);
		packet->setTransferType(type);

		packet->setWanIp1(0);
		packet->setWanPort1(0);
		packet->setWanIp2(0);
		packet->setWanPort2(0);
		packet->setWanIp3(0);
		packet->setWanPort3(0);

		packet->setLanIp1(0);
		packet->setLanPort1(0);
		packet->setLanIp2(0);
		packet->setLanPort3(0);
		packet->setLanIp3(0);
		packet->setLanPort3(0);

		packet->setSyncIp(0);
		packet->setSyncPort(0);
		packet->setSyncSession(0);

		network->append(packet);
		util_log(0,"IP Notification packet Sent.\n");

	} else { // if I am the sender, I should start udp session connecting to buddy directly
		// but now I ask relay server to do the job
		QQNetwork::fileManager->changeToAgent(qqid,sessionid);
		QQNetwork::fileManager->updateIp(qqid,sessionid,ip1);
		QQNetwork::fileManager->startSession(qqid,sessionid);
	}
#endif
}

void _ftNotifyTransferStatus(const int qq, const unsigned int session, const unsigned int size, const unsigned int sent, const int elapsed) {
#if 0
	if (ft_t* ft=ftSessions[session]) {
		PROTOFILETRANSFERSTATUS pfts={sizeof(PROTOFILETRANSFERSTATUS)};

		char szFile[MAX_PATH], szDir[MAX_PATH];

		//strcpy(szFile,ft->file.c_str()/*fileManager->getFileName(qq,session,true).c_str()*/);
		strcpy(szFile,QQNetwork::fileManager->getFileName(qq,session,true).c_str());
		if (!*szFile) return;

		strcpy(szDir,szFile);
		*strrchr(szDir,'\\')=0;

		pfts.currentFile=szFile;
		pfts.sending=QQNetwork::fileManager->getThread(qq,session)->getThreadType()%2;
		pfts.workingDir=szDir;
		pfts.currentFileProgress=pfts.totalProgress=sent;
		pfts.currentFileSize=pfts.totalBytes=size;
		pfts.hContact= FindContact(qq);
		pfts.totalFiles=1;
		ProtoBroadcastAck(m_szModuleName,pfts.hContact, ACKTYPE_FILE, ACKRESULT_DATA,(HANDLE)ft, (LPARAM) & pfts);
	}
#endif
}

// buddy qq, agent session id, agent ip, agent port
void _ftNotifyAgentRequest(const int qq, const unsigned int oldsession, const unsigned int newsession, const unsigned int ip, const unsigned short port, const unsigned char type) {
#if 0
	if (ft_t* ft=ftSessions[oldsession]) {
		ftSessions.erase(oldsession);
		ftSessions[newsession]=ft;
		ft->sessionid=newsession;

		util_log(0,"notifyAgentRequest: qq=%d, oldsession=%d, newsession=%d, ip=%d, port=%d, type=%d",qq,oldsession,newsession,ip,port,type);
		if (qq == Packet::getQQ()) return;

		if (ft->hWndPopup) PUChangeTextW(ft->hWndPopup,L"Notify agent to transfer");

		SendFileNotifyAgentPacket *packet = new SendFileNotifyAgentPacket();
		packet->setMsgSequence(oldsession);
		packet->setReceiver(qq);
		packet->setTransferType(type);
		packet->setAgentSession(newsession);
		packet->setAgentServer(htonl(ip), port);
		util_log(0,"EvaPacketManager::doNotifyAgentTransfer Sent. -- id:%d, ip:%8x, port:%d, session:0x%8x\n",qq, ip, port, newsession);
		network->append(packet);
	}
#endif
}

void _ftNotifyTransferSessionChanged(const int qq, const unsigned int oldsession, const unsigned int newsession) {
	util_log(0,"notifyTransferSessionChanged: qq=%d, oldsession=%d, newsession=%d",qq,oldsession,newsession);
}

#if 0
void _ftAskResume(void* data2) {
	ft_t* data=(ft_t*)data2;

	switch (MessageBox(NULL,_T("File already exists.\n\nChoose 'Yes' to resume, 'No' to overwrite or 'Cancel' to cancel operation."),_T("File Already Exists"),MB_ICONQUESTION | MB_YESNOCANCEL)) {
				case IDYES:
					QQNetwork::fileManager->slotFileTransferResume((unsigned int)data->qqid,data->sessionid/*m_FileNoList[(unsigned int)data]*/ /*ar->qq,ar->session*/,true);
					break;
				case IDNO:
					QQNetwork::fileManager->slotFileTransferResume((unsigned int)data->qqid,data->sessionid/*m_FileNoList[(unsigned int)data]*/ /*ar->qq,ar->session*/,false);
					break;
				default:
					QQNetwork::fileManager->stopThread((unsigned int)data->qqid,data->sessionid/*m_FileNoList[(unsigned int)data]*/ /*ar->qq,ar->session*/);
					CallProtoService(m_szModuleName,PSS_FILECANCEL,(WPARAM)(unsigned int)data,0);
					break;
	}
}
#endif

void _ftNotifyTransferNormalInfo(const int qq, const unsigned int session, EvaFileStatus status, const string dir, const string name, const unsigned int size, const unsigned char type) {
#if 0
	if (ft_t* ft=ftSessions[session]) {
		HANDLE hContact=FindContact(qq);
		switch (status) {
		case ESResume: 
			{
				mir_forkthread(_ftAskResume,(void*)ft/*ar*/);
				ProtoBroadcastAck(m_szModuleName,hContact,ACKTYPE_FILE, ACKRESULT_NEXTFILE, (HANDLE)ft, 0);
				break;
			}
		case ESSendFinished:
		case ESReceiveFinished:
			{
				if (type==QQ_TRANSFER_FILE) 
					ProtoBroadcastAck(m_szModuleName,hContact,ACKTYPE_FILE, ACKRESULT_SUCCESS, (HANDLE)ft, 0);
				else {
					list<ReceivedNormalIM>::iterator iter;
					char szPath[MAX_PATH];
					std::list<CustomizedPic>::iterator iter2;
					CallService(MS_FILE_GETRECEIVEDFILESFOLDER,(WPARAM)hContact,(LPARAM)szPath);
					strcat(szPath,"\\");

					for (iter=storedIM.begin(); iter!=storedIM.end(); iter++) {
						EvaHtmlParser parser;
						std::list<CustomizedPic> picList = parser.parseCustomizedPics((char*)iter->getMessage().c_str(),false);

						for(iter2=picList.begin(); iter2!=picList.end(); iter2++) {
							strcpy(strrchr(szPath,'\\'),iter2->fileName.c_str());
							if (_access(szPath,0) && *szPath=='{') break;
						}
						if (iter2==picList.end()) {
							ReceivedNormalIM im=*iter;
							// All found
							imCallback(QQ_RECV_IM_TO_BUDDY,&im);
							storedIM.erase(iter);
							iter=storedIM.begin();
						}
					}
				}
				ftSessions.erase(session);
				delete ft;
				break;
			}
		case ESError:
			{
				if (type==QQ_TRANSFER_FILE) ProtoBroadcastAck(m_szModuleName,hContact,ACKTYPE_FILE, ACKRESULT_FAILED, (HANDLE)ft, 0);
				ftSessions.erase(session);
				delete ft;

				break;
			}
		}
	}
#endif
}

void _ftNotifyAddressRequest(const int qq, const unsigned int session, const unsigned int synsession, const unsigned int ip, const unsigned short port, const unsigned int myip, const unsigned short myport) {
#if 0
	util_log(0,"notifyAddressRequest: qq=%d, session=%d, synsession=%d, ip=%d, port=%d, myip=%d, myport=%d",qq,session,synsession,ip,port,myip,myport);
	bool ok = false;
	if(ft_t* ft=ftSessions[session]){
		bool isSender = QQNetwork::fileManager->isSender(qq, session, &ok);
		if(!ok) return;
		if (ft->hWndPopup) PUChangeTextW(ft->hWndPopup,L"Sending IP Notify");
		unsigned char type = QQNetwork::fileManager->getTransferType(qq, session);
		// we send our ip information to buddy
		// 		packetManager->doNotifyIpEx(id, isSender, session, 
		// 					user->getLoginWanIp(), user->getLoginWanPort()+100,
		// 					user->getLoginWanIp(), user->getLoginWanPort()+101,
		// 					myIp, myPort,
		// 					0x0a010113, user->getLoginLanPort()+15,
		// 					0, 0, 0x0a010113, user->getLoginLanPort()+50, synIp, synPort, synSession);
		/*packetManager->doNotifyIpEx(id, isSender, session, type, 
			0, 0,
			0, 0,
			0, 0,
			0, 0,
			0, 0, 0, 0, 0, 0, 0);*/
		HANDLE hContact=FindContact(qq);
		ProtoBroadcastAck(m_szModuleName,hContact,ACKTYPE_FILE,ACKRESULT_INITIALISING,(HANDLE)ft,0);
		SendIpExNotifyPacket *packet = new SendIpExNotifyPacket(true);
		packet->setReceiver(qq);
		packet->setSessionId(session);
		packet->setSender(isSender);
		packet->setTransferType(type);

		packet->setWanIp1(0);
		packet->setWanPort1(0);
		packet->setWanIp2(0);
		packet->setWanPort2(0);
		packet->setWanIp3(0);
		packet->setWanPort3(0);

		packet->setLanIp1(0);
		packet->setLanPort1(0);
		packet->setLanIp2(0);
		packet->setLanPort3(0);
		packet->setLanIp3(0);
		packet->setLanPort3(0);

		packet->setSyncIp(0);
		packet->setSyncPort(0);
		packet->setSyncSession(0);

		network->append(packet);
		util_log(0,"IP Notification packet Sent.\n");

	}
#endif
}
#endif // UPGRADE_DISABLE

void CNetwork::_loginCallback(LoginReplyPacket* packet) {
	//GETSETTINGS();
	const unsigned char replyCode=packet->getReplyCode();

	switch (replyCode) {
		case QQ_LOGIN_REPLY_OK:
			{
				DBVARIANT dbv;

#if 0
				if (ServiceExists(MS_POPUP_ADDPOPUPW)) {
					POPUPDATAW ppd={0};
					time_t tm=(time_t)packet->getLastLoginTime();

					_tcscpy(ppd.lpwzContactName,m_tszUserName);
					swprintf(ppd.lpwzText,TranslateT("Last Login: %s\nClock Skew: %ds"),_tctime(&tm),m_clockSkew);
					/*
					swprintf(ppd.lpwzText+wcslen(ppd.lpwzText),TranslateT("Last IP: %S:%d\n"),EvaIPAddress(packet->getMyIP()).toString().c_str(),packet->getMyPort());
					swprintf(ppd.lpwzText+wcslen(ppd.lpwzText),TranslateT("Current IP: %S:%d"),EvaIPAddress(packet->getLastLoginIP()).toString().c_str(),packet->getLastLoginTime());
					*/
					ppd.lchIcon=(HICON)LoadImage(hinstance, MAKEINTRESOURCE(IDI_TM), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0);
					CallService(MS_POPUP_ADDPOPUPW,(WPARAM)&ppd,0);
				}
#endif
				EnableMenuItems(TRUE);

				/*
				if (!READ_B2(NULL,QQ_NOAUTOSERVER)) {
					char szTemp[MAX_PATH];
					sprintf(szTemp,"%s://%s:%d",Packet::isUDP()?"udp":"tcp",getHost().c_str(),getPort());
					WRITE_S(NULL,QQ_LOGINSERVER2,szTemp);
				}
				*/

				append(new EvaRequestKeyPacket(QQ_REQUEST_FILE_AGENT_KEY));

				m_myInfoRetrieved=false;
				m_downloadGroup=false;
				append(new GetUserInfoPacket(m_myqq));

				if (READ_B2(NULL,QQ_STATUSASPERSONAL)==1 && !READ_TS2(NULL,"PersonalSignature",&dbv)) {
					LPSTR pszMsg;
					pszMsg=mir_u2a_cp(dbv.ptszVal,936);
					//util_convertFromNative(&pszMsg,dbv.ptszVal);

					if (*pszMsg==0) {
						append(new SignaturePacket(QQ_SIGNATURE_DELETE));
					} else {
						SignaturePacket *packet = new SignaturePacket(QQ_SIGNATURE_MODIFY);
						packet->setSignature(pszMsg);
						append(packet);
					}
					//free(pszMsg);
					mir_free(pszMsg);
					DBFreeVariant(&dbv);
				} else {
					std::map<unsigned int, unsigned int> list;
					SignaturePacket *packet = new SignaturePacket(QQ_SIGNATURE_REQUEST);
					list[packet->getQQ()] = 0;
					packet->setMembers(list);
					append(packet);
				}

				{
					char szTemp[MAX_PATH]={0};
					const unsigned char* pszKey=Packet::getClientKey();
					for (int c=0; c<32; c++) {
						sprintf(szTemp+strlen(szTemp),"%02x",*pszKey++);
					}
					util_log(0,"Client key: %s",szTemp);
				}

#ifdef MIRANDAQQ_IPC
				NotifyEventHooks(hIPCEvent,QQIPCEVT_LOGGED_IN,NULL);
#endif
				break;
			}
		case QQ_LOGIN_REPLY_MISC_ERROR:
		case QQ_LOGIN_REPLY_PWD_ERROR:
		case QQ_LOGIN_REPLY_NEED_REACTIVATE:
			{
				LPTSTR szMsg;
				util_log(0,"%s(): Login failed: %s",__FUNCTION__,packet->getReplyMessage().c_str());
				//util_convertToNative(&szMsg,packet->getReplyMessage().c_str());
				szMsg=mir_a2u_cp(packet->getReplyMessage().c_str(),936);
				// Reconnect plugin does not recognize LOGINERR_WRONGPROTOCOL and will reconnect, so send LOGINERR_WRONTPASSWORD also
				ProtoBroadcastAck(m_szModuleName,NULL,ACKTYPE_LOGIN,ACKRESULT_FAILED,NULL,/*replyCode==QQ_LOGIN_REPLY_PWD_ERROR?*/LOGINERR_WRONGPASSWORD/*:LOGINERR_WRONGPROTOCOL*/);
				SetStatus(ID_STATUS_OFFLINE);

				MessageBox(NULL,szMsg,NULL,MB_ICONERROR|MB_SYSTEMMODAL);
				if (replyCode==QQ_LOGIN_REPLY_NEED_REACTIVATE) {
					if (wcsstr(szMsg,L"im.qq.com"))
						MessageBox(NULL,TranslateT("Please switch server. If it doesn't help, then your QQ number can only be used in QQ2007/TM2007. No unofficial clients can be used."),NULL,MB_ICONERROR);
					else
						CallService(MS_UTILS_OPENURL,(WPARAM)0,(LPARAM)"http://jihuo.qq.com/");
				}
				mir_free(szMsg);
				break;
			}
	}
}

void CNetwork::_changeStatusCallback(ChangeStatusReplyPacket* packet) {
	if (packet->isAccepted()) BroadcastStatus(m_iDesiredStatus);
}

void CNetwork::_getUserInfoCallback(GetUserInfoReplyPacket* packet) {
	ContactInfo info=packet->contactInfo();
	unsigned int qqid=atoi(info.at(info.Info_qqID).c_str());
	HANDLE hContact=FindContact(qqid);
	//GETSETTINGS();

	util_log(0,"TEST: QQID=%d",packet->getQQ());

	if (qqid>0) {
		for (int c=0; c<2; c++)
			if ((c==0 && hContact) || (c==1 && !m_myInfoRetrieved && qqid==m_myqq)) {
				if (c==1) {
					m_myInfoRetrieved=true;

					append(new GetFriendListPacket());
					append(new RequestExtraInfoPacket());
				} 

				if (c==1) hContact=0;

				LPTSTR pszTemp;

				// QQID
				WRITEINFO_TS("Nick",info.Info_nick);
				WRITEINFO_TS("Country",info.Info_country);
				WRITEINFO_TS("Province",info.Info_province);
				WRITEINFO_TS("ZIP",info.Info_zipcode);
				WRITEINFO_TS("Address", info.Info_address);
				WRITEINFO_TS("Telephone",info.Info_telephone);
				WRITEINFO_W("Age",info.Info_age);

				if ((unsigned char)*(info.at(info.Info_gender).c_str())==0xA8 || (unsigned char)*(info.at(info.Info_gender).c_str())==0xC4) // Male
					WRITEC_B("Gender",'M');
				else if ((unsigned char)*(info.at(info.Info_gender).c_str())==0xA4 || (unsigned char)*(info.at(info.Info_gender).c_str())==0xC5) // Female
					WRITEC_B("Gender",'F');
				else {
					WRITEC_B("Gender",'?');
				}

				WRITEINFO_TS("FirstName",info.Info_name);
				WRITEINFO_TS("Email",info.Info_email);
				WRITEINFO_TS("PagerSN",info.Info_pagerSn);
				WRITEINFO_TS("PagerNum",info.Info_pagerNum);
				WRITEINFO_TS("PagerSP",info.Info_pagerSp);
				WRITEINFO_W("PagerBaseNum",info.Info_pagerBaseNum);
				WRITEINFO_B("PagerType",info.Info_pagerType);
				WRITEINFO_B("Occupation",info.Info_occupation);
				WRITEINFO_TS("Homepage",info.Info_homepage);
				WRITEINFO_B("AuthType",info.Info_authType);
				WRITEINFO_TS("Unknown1",info.Info_unknown1);
				WRITEINFO_TS("Unknown2",info.Info_unknown2);
				WRITEINFO_W("Face",info.Info_face);
				WRITEINFO_TS("Mobile",info.Info_mobile);
				WRITEINFO_B("MobileType",info.Info_mobileType);
				WRITEINFO_TS("About",info.Info_intro);
				WRITEINFO_TS("City",info.Info_city);
				WRITEINFO_TS("Unknown3",info.Info_unknown3);
				WRITEINFO_TS("Unknown4",info.Info_unknown4);
				WRITEINFO_TS("Unknown5",info.Info_unknown5);
				WRITEINFO_B("OpenHP",info.Info_openHp);
				WRITEINFO_B("OpenContact",info.Info_openContact);
				WRITEINFO_TS("College",info.Info_college);
				WRITEINFO_B("Horoscope",info.Info_horoscope);
				WRITEINFO_B("Zodiac",info.Info_zodiac);
				WRITEINFO_B("Blood",info.Info_blood);
				WRITEINFO_W("QQShow",info.Info_qqShow);
				WRITEINFO_TS("Unknown6",info.Info_unknown5);
			}
			util_log(0,"%s(): nick=%s, udp=%s",__FUNCTION__,info.at(info.Info_nick).c_str(),packet->isUDP()?"TRUE":"FALSE");

			if (DBGetContactSettingByte(hContact,"CList","NotOnList",0)==0) {
				std::list<unsigned int> list;
				list.push_back(qqid);
				append(new EvaGetLevelPacket(list));
			} else {
				delayReport_t* dr=(delayReport_t*)mir_alloc(sizeof(delayReport_t));
				dr->hContact=hContact;
				dr->ackType=ACKTYPE_GETINFO;
				dr->ackResult=ACKRESULT_SUCCESS;
				dr->aux=1;
				dr->aux2=NULL;
				ForkThread((ThreadFunc)&CNetwork::delayReport,dr);
			}

	} else
		util_log(0,"%s(): Encountered invalid user info (qqid=0)!",__FUNCTION__);

}

void CNetwork::_getFriendListCallback(GetFriendListReplyPacket* packet) {
	friendItemList gotList = packet->getFriendList();
	friendItemList::iterator iter;
	QQFriend frd;
	HANDLE hContact;
	LPTSTR nick;
	bool fCleanNickname=READ_B2(NULL,QQ_REMOVENICKCHARS);
	//GETSETTINGS();

	for(iter = gotList.begin(); iter!= gotList.end(); ++iter){
		frd.setFriendItem(*iter);

		hContact=AddContact(frd.getQQ(),false,false);
		// Basic info
		WRITEC_W("Age",frd.getAge()); // This one should be Byte, Word for historical reason
		WRITEC_B("CommonFlag",frd.getCommonFlag());
		WRITEC_B("ExtFlag",frd.getExtFlag());
		WRITEC_W("Face",frd.getFace());
		WRITEC_B("Gender",frd.getGender());
		WRITEC_D("UpdateTS",(DWORD)time(NULL));

		nick=mir_a2u_cp(frd.getNick().c_str(),936);
		//util_convertToNative(&nick,frd.getNick().c_str());
		if (fCleanNickname) util_clean_nickname(nick);

		// Add identity if needed
#if 0
		if (!qqSettings->DisableContactIdentity) {
			DBVARIANT dbv;
			if (!READ_TS2(NULL,"Nick",&dbv)) {
				nick=(LPTSTR)realloc(nick,(_tcslen(nick)+_tcslen(dbv.ptszVal)+4)*sizeof(TCHAR));
				_stprintf(nick+_tcslen(nick),_T(" (%s)"),dbv.ptszVal);
				DBFreeVariant(&dbv);
			}
		}
#endif
		WRITEC_TS("Nick",nick);
		//free(nick);
		mir_free(nick);
	}
}

void CNetwork::_requestExtraInfoCallback(RequestExtraInfoReplyPacket* packet) {
	int count = 0;
	std::map<unsigned int, unsigned int> list;
	std::map<unsigned int, unsigned __int64> members = packet->getMembers();
	std::map<unsigned int, unsigned __int64>::iterator iter;
	HANDLE hContact;
	//GETSETTINGS();

	for(iter = members.begin(); iter!= members.end(); ++iter){
		hContact=FindContact(iter->first);
		if (hContact) {
			//util_log(0,"ASSERT: QQ %d, extra=0x%x",iter->first,iter->second);
			WRITEC_W("ExtraInfo",(WORD)iter->second);
			if(iter->second & 0x4000) {
				count++;
				list[iter->first] = 0;
			}
		}
	}
	if (list.size()) {
		SignaturePacket *packet = new SignaturePacket(QQ_SIGNATURE_REQUEST);
		packet->setMembers(list);
		append(packet);
	}

	if(packet->getOffset()!=QQ_FRIEND_LIST_POSITION_END){
		append(new RequestExtraInfoPacket(packet->getOffset()));
	} else if (READ_B2(NULL,QQ_AVATARTYPE)==0) {
		HANDLE hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, (WPARAM)NULL, (LPARAM)NULL);
		std::list<unsigned int> list;

		while (hContact) {
			if (!lstrcmpA(m_szModuleName, (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact,(LPARAM)NULL))) {
				if (READC_W2("ExtraInfo") & QQ_EXTAR_INFO_USER_HEAD)
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

#if 0
void CNetwork::_writeIP(HANDLE hContact, int ip) {
	char szPluginPath[MAX_PATH];
	LPWSTR szLocation;

	CallService(MS_UTILS_PATHTOABSOLUTE,(WPARAM)"Plugins\\QQWry.dat",(LPARAM)szPluginPath);
	EvaIPSeeker ipAddr(szPluginPath);
	//util_convertToNative(&szLocation,ipAddr.getIPLocation(ip).c_str());
	szLocation=mir_a2u_cp(ipAddr.getIPLocation(ip).c_str(),936);
	WRITEC_TS("Location",szLocation);
	mir_free(szLocation);
}
#endif

void CNetwork::_writeVersion(HANDLE hContact, int version, const char* iniFile) {
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
			//util_convertToNative(&wszVersion,szTextVersion,FALSE);
			wszVersion=mir_a2u_cp(szTextVersion,936);
		} else {
			*strchr(szTextVersion,'>')=0;
			wszVersion=mir_a2u_cp(szTextVersion+1,936);
		}

	}
	WRITEC_TS("MirVer",wszVersion);
	mir_free(wszVersion);
}

void CNetwork::_getOnlineFriendCallback(GetOnlineFriendReplyPacket* packet) {
	onlineList list = packet->getOnlineFriendList();
	char szPluginPath[MAX_PATH];

	map<unsigned int, unsigned int> qqlist;
	CallService(MS_UTILS_PATHTOABSOLUTE,(WPARAM)"Plugins\\qqVersion.ini",(LPARAM)szPluginPath);

	HANDLE hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, (WPARAM)NULL, (LPARAM)NULL);
	map<DWORD,UCHAR> unhandledqqlist;
	DWORD qqid;
	int oldStatus;
	int status;

	while (hContact) {
		if (!lstrcmpA(m_szModuleName, (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact,(LPARAM)NULL))) {
			if (qqid=READC_D2(UNIQUEIDSETTING)) if (!READC_B2("IsQun") && qqid<0x80000000) unhandledqqlist[qqid]=0;
		}

		hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, (LPARAM)NULL);
	}

	for(onlineList::iterator iter = list.begin(); iter!=list.end(); ++iter){
		unhandledqqlist[iter->getQQ()]=1;
		if (hContact=FindContact(iter->getQQ())) {
			oldStatus=READC_W2("Status");
			status=ID_STATUS_ONLINE;

			switch (iter->getStatus()) {
				case QQ_FRIEND_STATUS_OFFLINE: status=ID_STATUS_OFFLINE; break;
				case QQ_FRIEND_STATUS_LEAVE: status=ID_STATUS_AWAY; break;
				case QQ_FRIEND_STATUS_INVISIBLE: status=ID_STATUS_INVISIBLE; break;
			}

			if (status!=oldStatus && oldStatus!=ID_STATUS_INVISIBLE && status!=ID_STATUS_INVISIBLE) {
				WRITEC_W("Status",status);

				if (oldStatus!=ID_STATUS_OFFLINE)
					qqlist[iter->getQQ()]=0;
				else {
					// These things should only change when changing from offline to online
					WRITEC_B("CommFlag",iter->getCommFlag());
					WRITEC_B("ExtFlag",iter->getExtFlag());
					//WRITEC_D("TickTS",(DWORD)time(NULL)); // No use now

					_writeVersion(hContact,iter->getUnknown3_13_14(),szPluginPath);

#if 0
					if (iter->getIP()) {
						util_log(0,"QQ Contact %d shared IP: 0x%x",iter->getQQ(),iter->getIP());
						WRITEC_D("IP",iter->getIP());
						WRITEC_W("Port",iter->getPort());
						_writeIP(hContact,iter->getIP());
					}
#endif
				}
			}
		} else
			util_log(98,"%s(): Warning: Cannot find contact with QQ=%d! Possibly removed?",__FUNCTION__,iter->getQQ());
	}

	for (map<DWORD,UCHAR>::iterator iter=unhandledqqlist.begin(); iter!=unhandledqqlist.end(); iter++) {
		if (iter->second==0) {
			if (hContact=FindContact(iter->first)) {
				oldStatus=READC_W2("Status");
				if (oldStatus!=ID_STATUS_OFFLINE && (oldStatus!=ID_STATUS_INVISIBLE || (DWORD)time(NULL)-READC_D2("InvisibleTS")>300)) {
					DELC("InvisibleTS");
					WRITEC_W("Status",ID_STATUS_OFFLINE);
					util_log(0,"Set QQ contact %d to offline.",iter->first);
				}
			}
		}
	}

	if (qqlist.size()) {
		SignaturePacket *packet = new SignaturePacket(QQ_SIGNATURE_REQUEST);
		packet->setMembers(qqlist);
		append(packet);
	}
}

//extern HANDLE	hInitChat;
void CNetwork::_downloadGroupFriendCallback(DownloadGroupFriendReplyPacket* packet) {
	std::list<DownloadFriendEntry> friends = packet->getGroupedFriends();
	HANDLE hContact;
	//GETSETTINGS();

	for(list<DownloadFriendEntry>::iterator iter = friends.begin(); iter!= friends.end(); ++iter){
		if(!iter->isQun()){
			hContact=FindContact(iter->getQQ());
			if (m_downloadGroup && hContact) { // Only continues if download group is performing and user is in contact list
				DBVARIANT dbv={0};
				DBGetContactSetting(hContact,"CList","Group",&dbv);

				if (dbv.pszVal && !strcmp(dbv.pszVal+1,"MetaContacts Hidden Group")) { // Don't touch MetaContacts!
					DBVARIANT dbvNick={0};
					TCHAR szPath[MAX_PATH];
					DBGetContactSettingTString(hContact,m_szModuleName,"Nick",&dbv);

					swprintf(szPath,TranslateT("Not moving contact %s (%d) because he/she is maintained by MetaContacts"),dbv.ptszVal,iter->getQQ());
					ForkThread((ThreadFunc)&CNetwork::ThreadMsgBox,mir_tstrdup(szPath));
				} else {
					util_log(0,"%s(): Contact %d belongs to group %d",__FUNCTION__,iter->getQQ(), iter->getGroupID());

					if (iter->getGroupID()==0) // No group
						CallService(MS_CLIST_CONTACTCHANGEGROUP, (WPARAM)hContact, (LPARAM)0);
					else // In group
						CallService(MS_CLIST_CONTACTCHANGEGROUP, (WPARAM)hContact, (LPARAM)m_hGroupList[iter->getGroupID()-1]);
				}

				m_qqusers++;
			}
		}else{
			if (!m_qunList.getQun(iter->getQQ())) {
				Qun q(iter->getQQ());
				m_qunList.add(q);
			}
		}
	}

	if (packet->getNextStartID()==0) {
		list<Qun> quns=m_qunList.getQunList();
		HANDLE hContact;
		DBVARIANT dbv;
		//DWORD dwQunIgnore;

		// Process new quns from server list
		for (list<Qun>::iterator iter=quns.begin(); iter!=quns.end(); iter++) {
			hContact=FindContact(iter->getQunID());
			if (!hContact) {
				hContact=AddContact(iter->getQunID(),false,false);
			}

			if (hContact) {
				WRITEC_B("IsQun",1);
				WRITEC_B("ServerQun",1);
				//DBWriteContactSettingDword(hContact,"Ignore","Mask1",8);

				if (DBGetContactSettingTString(hContact,m_szModuleName,"Nick",&dbv)) {
					// Info not get
					WRITEC_B("QunInit",1);
					append(new QunGetInfoPacket(iter->getQunID()));
				} else
					DBFreeVariant(&dbv);
			}
		}

		bool fSuppressQunMsg=READ_B2(NULL,QQ_SUPPRESSQUNMSG);

		// Process quns not in list
		hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, (WPARAM)NULL, (LPARAM)NULL);
		while (hContact) {
			if (!lstrcmpA(m_szModuleName, (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact,(LPARAM)NULL)) && READ_B2(hContact,"IsQun")==1) {
				/*
				if (!((dwQunIgnore=DBGetContactSettingDword(hContact,"Ignore","Mask1",0))&8))
					DBWriteContactSettingDword(hContact,"Ignore","Mask1",dwQunIgnore|8);
				*/

				if (READC_B2("ServerQun")==0 && READC_B2("NoInit")==0) {
					WRITEC_B("QunInit",1);
					Qun q(READC_D2(UNIQUEIDSETTING));
					m_qunList.add(q);
					//network->append(new QunGetInfoPacket(*iter));
					m_qunInitList.push_back(q.getQunID());
				} else {
					DBDeleteContactSetting(hContact,"CList","Hidden");
					//dwQunIgnore=DBGetContactSettingDword(hContact,"Ignore","Mask1",0);
					//DBWriteContactSettingDword(hContact,"Ignore","Mask1",dwQunIgnore|8);

					WRITEC_W("Status",READC_B2("NoInit")?ID_STATUS_INVISIBLE:(fSuppressQunMsg?ID_STATUS_DND:DBGetContactSettingWord(hContact,m_szModuleName,"PrevStatus",ID_STATUS_ONLINE)));
					//DBWriteContactSettingDword(hContact,"Ignore","Mask1",dwQunIgnore);
#ifdef MIRANDAQQ_IPC
					NotifyEventHooks(hIPCEvent,(WPARAM)QQIPCEVT_QUN_ONLINE,(LPARAM)hContact);
#endif
					//chatInitQun((WPARAM)hContact,0);
					//NotifyEventHooks(hInitChat, (WPARAM)hContact,0);
				}
			}

			hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, (LPARAM)NULL);
		}

		/*
		if (isInvisible()) 
			qqDesiredStatus=ID_STATUS_INVISIBLE;
		*/

		if (m_iDesiredStatus==ID_STATUS_ONLINE||m_iDesiredStatus==ID_STATUS_INVISIBLE) {
			BroadcastStatus(m_iDesiredStatus);
		} else {
			//SetStatus(m_iDesiredStatus);
			SetServerStatus(m_iDesiredStatus);
		}

		if (m_qunInitList.size()) append(new QunGetInfoPacket(m_qunInitList.front()));

		if (m_downloadGroup) {
			WCHAR szTemp[MAX_PATH];
			swprintf(szTemp,TranslateT("Retrieved %d users, %d quns from %d groups."),m_qqusers,m_qunList.numQuns(),m_hGroupList.size());
			ShowNotification(szTemp,NIIF_INFO);
			m_hGroupList.clear();
			m_downloadGroup=false;
		}
	}
}

void CNetwork::_qunGetInfoCallback(QunReplyPacket* packet) {
	if (packet->isReplyOK()) {
		QunInfo info = packet->getQunInfo();
		int qunid=info.getQunID();
		Qun* qun=m_qunList.getQun(qunid);

		if (!qun) {
			m_qunList.add(Qun(qunid));
			qun=m_qunList.getQun(qunid);
		}

		std::list<FriendItem> l=qun->getMembers();

		//util_log(0,"Qun %d: Pre: l.size=%d, qun->...getVersionID()=%d, info.getVersionID()=%d",info.getExtID(),qun->getDetails().getVersionID(),info.getVersionID());

		//std::map<int, QunMember>::iterator iter;
		std::map<unsigned int, QunMember> lst = packet->getMemberList();
		int cardVersion=qun->getRealNamesVersion();

		util_log(0,"Qun %d: Version=%d, CardVersion=%d, Member Count(Stored:Recv)=%d:%d",info.getExtID(),info.getVersionID(),cardVersion,lst.size(),l.size());

		m_qunList.setDetails(info);
		m_qunList.setMemberArgs(qunid,lst);

		qun->setRealNamesVersion(cardVersion);

		for(list<FriendItem>::iterator it = l.begin(); it!= l.end(); ++it){
			for(map<unsigned int, QunMember>::iterator iter= lst.begin(); iter != lst.end(); ++iter)
				if (iter->second.qqNum==it->getQQ()) {
					qun->setMember(*it);
					break;
				}
		}

		HANDLE hContact=FindContact(qunid);
		if (!hContact) {
			hContact=AddContact(qunid,false,false);
			if (hContact) {
				WRITEC_B("IsQun",1);
				WRITEC_W("QunVersion",info.getVersionID());
				WRITEC_W("CardVersion",-1);
			}
		}

		std::list<unsigned int> toSend;
		int i=0;
		bool sent=false;
		std::map<unsigned int,QunMember>::iterator iter;
		FriendItem fi;
		LPTSTR szInfo;
		TCHAR szNick[MAX_PATH];
		LPTSTR pszTemp;

		szInfo=mir_a2u_cp(info.getName().c_str(),936);
		_stprintf(szNick,TranslateT("(QQ Qun) %s"),szInfo);
		WRITEC_TS("Nick",szNick);
		mir_free(szInfo);

		LPSTR pszInfo2;
		pszInfo2=mir_utf8encodecp(info.getNotice().c_str(),936);
		DBWriteContactSettingUTF8String(hContact,"CList","StatusMsg",pszInfo2);
		mir_free(pszInfo2);

		if (!READC_B2("HelperControlled")) DBDeleteContactSetting(hContact,"CList","Hidden");
		//DWORD dwQunIgnore;

		if (READC_W2("Status")==ID_STATUS_OFFLINE) {
			//dwQunIgnore=DBGetContactSettingDword(hContact,"Ignore","Mask1",0);
			//DBWriteContactSettingDword(hContact,"Ignore","Mask1",dwQunIgnore|8);

			WRITEC_W("Status",READ_B2(NULL,QQ_SUPPRESSQUNMSG)?ID_STATUS_DND:DBGetContactSettingWord(hContact,m_szModuleName,"PrevStatus",ID_STATUS_ONLINE));
			//DBWriteContactSettingDword(hContact,"Ignore","Mask1",dwQunIgnore);
#ifdef MIRANDAQQ_IPC
			NotifyEventHooks(hIPCEvent,(WPARAM)QQIPCEVT_QUN_ONLINE,(LPARAM)hContact);
#endif
		} else {
#ifdef MIRANDAQQ_IPC
			NotifyEventHooks(hIPCEvent,(WPARAM)QQIPCEVT_QUN_REFRESHINFO,(LPARAM)hContact);
#endif
		}

		WRITEC_B("AuthType",info.getAuthType());
		WRITEC_W("Category",info.getCategory());
		WRITEC_D("Creator",info.getCreator());
		WRITEQUNINFO_TS("Description",info.getDescription());
		WRITEC_D("ExternalID",info.getExtID());
		WRITEC_B("Type",info.getType());
		WRITEC_B("IsAdmin",qun->isAdmin(m_myqq));

		m_qunMemberCountList[qunid]=lst.size();
		m_currentQunMemberCountList[qunid]=0;

		for(iter= lst.begin(); iter != lst.end(); ++iter){
			//if (!qun->hasMember(iter->first)) {
				if(i>0 && i%30==0){
					QunGetMemberInfoPacket *out=new QunGetMemberInfoPacket(qunid);
					out->setMemberList(toSend);
					append(out);
					toSend.clear();
					util_log(0,"Qun %d: Sent 30-batch qun member update",qunid);
					//i = 0;
					sent=true;
				}

				toSend.push_back(iter->first);
				i++;
			//}
		}
		if(toSend.size()) {
			QunGetMemberInfoPacket *out=new QunGetMemberInfoPacket(qunid);
			out->setMemberList(toSend);
			append(out);
			util_log(0,"Qun %d: Sent last batch(%d) qun member update",qunid,toSend.size());
			toSend.clear();
			sent=true;
		} 

		if (!sent) {
			// No member update required, update Qun Card
			append(new QunRequestAllRealNames(qunid));
		}

		if (m_qunInitList.size() && m_qunInitList.front()==qunid) {
			m_qunInitList.pop_front();
			if (m_qunInitList.size()) append(new QunGetInfoPacket(m_qunInitList.front()));
		}
	} else if (m_qunInitList.size()) {
		HANDLE hContact=FindContact(m_qunInitList.front());
		if (hContact) {
			TCHAR szTemp[MAX_PATH];
			LPTSTR pszMsg;

			WRITEC_W("Status",ID_STATUS_INVISIBLE);
			pszMsg=mir_a2u_cp(packet->getErrorMessage().c_str(),936);
			DBWriteContactSettingTString(hContact,"CList","StatusMsg",pszMsg);

			_stprintf(szTemp,TranslateT("Unable to initialize qun %d:\n%s"),m_qunInitList.front(),pszMsg);
			ShowNotification(szTemp,NIIF_WARNING);
			mir_free(pszMsg);
		}

		m_qunInitList.pop_front();
		if (m_qunInitList.size()) append(new QunGetInfoPacket(m_qunInitList.front()));
	}
}

void CNetwork::_qunCommandCallback(QunReplyPacket* packet) {
	int qunid=packet->getQunID();

	switch (packet->getQunCommand()) {
		case QQ_QUN_CMD_GET_QUN_INFO: _qunGetInfoCallback(packet); break;
		case QQ_QUN_CMD_SEND_IM:
		case QQ_QUN_CMD_SEND_IM_EX:
		case QQ_QUN_CMD_SEND_TEMP_QUN_IM:
			{
				OutPacket* op=m_pendingImList[packet->getSequence()+1];

				util_log(0,"Received Qun IM callback, next seq=0x%x, op=0x%x",packet->getSequence()+1, op);

				if (op) {
					m_pendingImList.erase(op->getSequence());
					append(op);
				} else {
					HANDLE hContact=FindContact(qunid);
					util_log(0,"Received IM Send Reply, isReplyOK=%d",packet->isReplyOK()?1:0);

					delayReport_t* dr=(delayReport_t*)mir_alloc(sizeof(delayReport_t));
					dr->hContact=hContact;
					dr->ackType=ACKTYPE_MESSAGE;
					dr->aux=packet->getSequence();

					if (packet->isReplyOK()) {
						Sleep(500);
						if (READC_B2("NoInit")==1 || READC_W2("Status")==ID_STATUS_INVISIBLE) {
							DELC("NoInit");
							WRITEC_W("Status",ID_STATUS_ONLINE);
						}
						dr->ackResult=ACKRESULT_SUCCESS;
						dr->aux2=NULL;
						ForkThread((ThreadFunc)&CNetwork::delayReport,dr);

					} else {
						// Ack Failed
						LPWSTR pszUnicode=mir_a2u_cp(packet->getErrorMessage().c_str(),936);
						LPSTR pszANSI=mir_u2a_cp(pszUnicode,GetACP());
						ShowNotification(pszUnicode,NIIF_ERROR);
						dr->ackResult=ACKRESULT_FAILED;
						dr->aux2=NULL;
						ForkThread((ThreadFunc)&CNetwork::delayReport,dr);

						mir_free(pszUnicode);
					}
#ifdef MIRANDAQQ_IPC
					NotifyEventHooks(hIPCEvent,QQIPCEVT_QUN_MESSAGE_SENT,(LPARAM)hContact);
#endif
				}
			}
			break;
		case QQ_QUN_CMD_MODIFY_MEMBER:
			{
				TCHAR szMsg[MAX_PATH];
				LPCSTR szServerMsg=strdup(packet->getErrorMessage().c_str());
				LPTSTR pszServerMsg;
				HANDLE hContact=FindContact(qunid);

				pszServerMsg=mir_a2u_cp(szServerMsg,936);
				
				_stprintf(szMsg,TranslateT("Qun Modification Request for %d %s.\n%s"),qunid,packet->isReplyOK()?TranslateT("succeeded"):TranslateT("failed"),pszServerMsg);
				ShowNotification(szMsg,NIIF_INFO);
				WRITEC_W("CardVersion",-1);
				append(new QunGetInfoPacket(qunid));
				mir_free(pszServerMsg);
			}
			break;
		case QQ_QUN_CMD_EXIT_QUN: // Requested to exit a Qun or kicked by Qun Admin
			{
				TCHAR szMsg[MAX_PATH];
				LPCSTR szServerMsg=strdup(packet->getErrorMessage().c_str());
				LPTSTR pszServerMsg;
				HANDLE hContact=FindContact(qunid);
				int extID=READC_D2("ExternalID");

				if (!extID) extID=qunid;
				pszServerMsg=mir_a2u_cp(szServerMsg,936);

				_stprintf(szMsg,TranslateT("Qun Exit Request for %d %s.\n%s"),qunid,packet->isReplyOK()?TranslateT("succeeded"):TranslateT("failed"),pszServerMsg);
				ShowNotification(szMsg,NIIF_INFO);

				if(packet->isReplyOK()){
					m_qunList.remove(packet->getQunID());
					if (hContact) CallService(MS_DB_CONTACT_DELETE,(WPARAM)hContact,0);
				}
			}
			break;
		case QQ_QUN_CMD_GET_MEMBER_INFO: // Requested to get member info on a qun (Multiple members each reply)
			{
				if (packet->isReplyOK()) {
					if (Qun* qun=m_qunList.getQun(qunid)) {
						const list<FriendItem> lst=packet->getMemberInfoList();
						m_qunList.setMembers(qunid, lst);
						m_currentQunMemberCountList[qunid]+=lst.size();
						if (m_currentQunMemberCountList[qunid]>=m_qunMemberCountList[qunid]) {
							//CQunListV2* qunList=CQunListV2::getInstance(false);
							HANDLE hContact=FindContact(qunid);

							util_log(0,"Qun %d: Member info completed, ask for real names",qunid);

							/*
							if (qunList && qunList->getQunid()==qunid) {
								qunList->refresh();
							}
							*/

							append(new QunRequestAllRealNames(qunid));
							m_currentQunMemberCountList.erase(qunid);
						}
					}
				}
			}
			break;
		case QQ_QUN_CMD_GET_ONLINE_MEMBER: // Requested to get online members in Qun
			{
				if (packet->isReplyOK()) {
					HANDLE hContact=FindContact(packet->getQunID());
					m_qunList.setOnlineMembers(packet->getQunID(), packet->getQQNumberList());
					if (Qun* qun=m_qunList.getQun(packet->getQunID())) {
#if 0
						list<FriendItem> fil=qun->getMembers();
						qunmemberinfo_t qmi[201]={0};
						qunmemberinfo_t* pQmi=qmi;
						qunmemberinfo_outter_t qmio={packet->getQunID(),qmi};

						for (list<FriendItem>::iterator iter=fil.begin(); iter!=fil.end(); iter++,pQmi++) {
							pQmi->memberid=iter->getQQ();
							pQmi->flag=iter->isOnline()?QQIPCQMFLAG_ONLINE:0;
							if (iter->isAdmin()) pQmi->flag|=QQIPCQMFLAG_ADMIN;
							if (iter->isShareHolder()) pQmi->flag|=QQIPCQMFLAG_INVESTOR;
							if (qun->getDetails().getCreator()==pQmi->memberid) pQmi->flag|=QQIPCQMFLAG_CREATOR;
						}
#endif

#ifdef MIRANDAQQ_IPC
						ipconlinemembers_t iom={hContact,packet->getQunID(),packet->getQQNumberList()};
						NotifyEventHooks(hIPCEvent,(WPARAM)QQIPCEVT_QUN_UPDATE_ONLINE_MEMBERS,(LPARAM)&iom);
#endif

						/*
						if (CQunListV2* qList=CQunListV2::getInstance(false)) {
							if (qList->getQunid()==qunid) qList->refresh();
						}
						*/
					}
					WRITEC_B("GetInfoOnline",2);
					delayReport_t* dr=(delayReport_t*)mir_alloc(sizeof(delayReport_t));
					dr->hContact=hContact;
					dr->ackType=ACKTYPE_GETINFO;
					dr->ackResult=ACKRESULT_SUCCESS;
					dr->aux=1;
					dr->aux2=NULL;
					ForkThread((ThreadFunc)&CNetwork::delayReport,dr);

				}
			}
			break;
		case QQ_QUN_CMD_JOIN_QUN: // Requested to join a Qun
			switch (packet->getJoinReply()) {
				// case QQ_QUN_NO_AUTH: // Not seen
				case QQ_QUN_JOIN_OK: // Qun Admin approved request
					m_qunList.add(Qun(qunid));
					append(new QunGetInfoPacket(qunid));
					break;
				case QQ_QUN_JOIN_DENIED: // Qun Admin rejected request
					{
						TCHAR szMsg[MAX_PATH];
						LPTSTR pszServerMsg=mir_a2u_cp(packet->getErrorMessage().c_str(),936);

						_stprintf(szMsg,TranslateT("Administrator of Qun %d denied your join request\n%s"),qunid,pszServerMsg);
						ShowNotification(szMsg,NIIF_ERROR);
						mir_free(pszServerMsg);
					}
					break;
				case QQ_QUN_JOIN_NEED_AUTH: // This Qun need authorization
					{
						util_log(0,"2006: Qun need authorization (default), request auth info");

						if (HANDLE hContact=FindContact(m_addUID)) {
							EvaAddFriendGetAuthInfoPacket *packet = new EvaAddFriendGetAuthInfoPacket(READC_D2("ExternalID"), AUTH_INFO_CMD_INFO, true);
							packet->setVerificationStr("");
							packet->setSessionStr("");
							append(packet);
						} else {
							util_log(0,"ERROR: hContact==NULL");
						}
					}
					break;
				case QQ_QUN_CMD_JOIN_QUN_AUTH: // Requested to join a Qun with authorization message
					if (!packet->isReplyOK()) {
						TCHAR szMsg[MAX_PATH];
						LPTSTR pszServerMsg=mir_a2u_cp(packet->getErrorMessage().c_str(),936);

						_stprintf(szMsg,TranslateT("Administrator of Qun %d denied your join request\n%s"),qunid,pszServerMsg);
						ShowNotification(szMsg,NIIF_ERROR);
						mir_free(pszServerMsg);
					}
					break;
				default:
					{
						TCHAR szMsg[MAX_PATH];
						LPTSTR pszServerMsg=mir_a2u_cp(packet->getErrorMessage().c_str(),936);

						_stprintf(szMsg,TranslateT("Add qun result of qun %d:\n%s"),qunid,pszServerMsg);
						ShowNotification(szMsg,NIIF_INFO);
						mir_free(pszServerMsg);
					}
					break;
			}
			break;
		case QQ_QUN_CMD_SEARCH_QUN: // Search for a Qun
			{
				if (packet->isReplyOK()) {
					list<QunInfo> lst=packet->getQunInfoList();
					list<QunInfo>::iterator iter=lst.begin();
					PROTOSEARCHRESULT psr;
					char uid[32];

					if (lst.size()) {
						sprintf(uid,"%d (%d)",iter->getExtID(),iter->getQunID());

						ZeroMemory(&psr, sizeof(psr));
						psr.cbSize = sizeof(psr);
						psr.nick = uid;
						psr.firstName=mir_strdup(iter->getName().c_str());
						util_convertFromGBK(psr.firstName);
						psr.lastName=mir_strdup(iter->getDescription().c_str());
						util_convertFromGBK(psr.lastName);
						psr.email=mir_u2a(iter->getAuthType()==QQ_QUN_NEED_AUTH?TranslateT("Authentication Required"):iter->getAuthType()==QQ_QUN_NO_AUTH?TranslateT("Authentication Not Required"):TranslateT("No Add"));

						ProtoBroadcastAck(m_szModuleName, NULL, ACKTYPE_SEARCH, ACKRESULT_DATA, (HANDLE) 1, (LPARAM)&psr);			
						mir_free(psr.firstName);
						mir_free(psr.lastName);
						mir_free(psr.email);
					}

					ProtoBroadcastAck(m_szModuleName, NULL, ACKTYPE_SEARCH, ACKRESULT_SUCCESS, (HANDLE) 1, (LPARAM)0);
				} else {
					ProtoBroadcastAck(m_szModuleName, NULL, ACKTYPE_SEARCH, ACKRESULT_SUCCESS, (HANDLE) 1, (LPARAM)0);
				}
			}
			break;
		case QQ_QUN_CMD_REQUEST_ALL_REALNAMES:
			if(packet->isReplyOK()){
				Qun *qun = m_qunList.getQun(qunid);
				bool fUpdate=false;
				HANDLE hContact=FindContact(qunid);
				if(!qun) break;

				util_log(0,"Qun %d RealNames version: %d -> %d",qunid, qun->getRealNamesVersion(),packet->getCardVersion());

				if (packet->getCardVersion()!=READC_W2("CardVersion") || READC_D2("QunCardUpdate")==-1) {
					util_log(0,"Qun %d: Different card version(%d), perform update",qunid,packet->getCardVersion());
					fUpdate=true;
				} else if (time(NULL)-READC_D2("QunCardUpdate")>60) {
					// Same version, no need to update
					util_log(0,"Qun %d: Same card version(%d), no need to update",qunid,packet->getCardVersion());
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
					std::map<int, std::string> nameList = packet->getRealNames();
					std::map<int, std::string>::iterator it;
					for(it = nameList.begin(); it != nameList.end(); ++it){
						// Crash here
						if (qun->hasMember(it->first) && it->second.size()>0) {
							FriendItem item (*(qun->getMemberDetails( it->first)));
							item.setQunRealName( it->second);
							qun->setMember(item);
						}
					}
					if(packet->getNextStartIndex()){
						QunRequestAllRealNames *all = new QunRequestAllRealNames(packet->getQunID());
						all->setStartIndex(packet->getNextStartIndex());
						append(all);
					} else {
						char szQQID[16];
						std::list<FriendItem> fil=qun->getMembers();
						std::list<FriendItem>::iterator iter;
						string realName;
#ifdef MIRANDAQQ_IPC
						ipcmember_t ipcm;
						ipcmembers_t ipcms={hContact,qunid};
#endif

						qun->setRealNamesVersion(packet->getCardVersion());
						WRITEC_D("QunCardUpdate",(DWORD)time(NULL));
						WRITEC_W("CardVersion",packet->getCardVersion());

						util_log(0,"Writing names for Qun %d",qunid);

#ifdef MIRANDAQQ_IPC
						unsigned int creator=qun->getDetails().getCreator();
						LPWSTR pwszCreator;
#endif

						for (iter=fil.begin();iter!=fil.end();iter++) {
							itoa(iter->getQQ(),szQQID,10);
							realName=iter->getQunRealName();
							if (!realName.length()) realName=iter->getNick();

							DBWriteContactSettingString(hContact,m_szModuleName,szQQID,realName.c_str());
							
							if (iter->getQQ()==creator) {
								pwszCreator=mir_a2u_cp(realName.c_str(),936);
								DBWriteContactSettingWString(hContact,m_szModuleName,"CreatorName",pwszCreator);
								mir_free(pwszCreator);
							}
#ifdef MIRANDAQQ_IPC

							ipcm.qqid=iter->getQQ();
							ipcm.face=iter->getFace();
							ipcm.flag=IPCMFLAG_EXISTS+(iter->isOnline()?IPCMFLAG_ONLINE:0);
							if (creator==ipcm.qqid)
								ipcm.flag|=IPCMFLAG_CREATOR;
							else if (iter->isAdmin())
								ipcm.flag|=IPCMFLAG_MODERATOR;
							else if (iter->isShareHolder())
								ipcm.flag|=IPCMFLAG_INVESTOR;

							ipcm.name=realName;
							ipcms.members.push_back(ipcm);
#endif
						}

#ifdef MIRANDAQQ_IPC
						NotifyEventHooks(hIPCEvent,(WPARAM)QQIPCEVT_QUN_UPDATE_NAMES,(LPARAM)&ipcms);
#endif
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
			break;
		case QQ_QUN_CMD_REQUEST_CARD: // Requested qun card
			if (Qun* qun=m_qunList.getQun(qunid)) {
				FriendItem fi(*(qun->getMemberDetails(packet->getTargetQQ())));

				fi.setQunRealName(packet->getRealName());
				qun->setMember(fi);
			}
			break;
		case QQ_QUN_CMD_ADMIN:
			{
				TCHAR szMsg[MAX_PATH];
				LPCSTR szServerMsg=strdup(packet->getErrorMessage().c_str());
				LPTSTR pszServerMsg;
				HANDLE hContact=FindContact(qunid);
				int extID=READC_D2("ExternalID");

				if (!extID) extID=qunid;
				pszServerMsg=mir_a2u_cp(szServerMsg,936);

				_stprintf(szMsg,TranslateT("Qun Administration Request for %d %s.\n%s"),qunid,packet->isReplyOK()?TranslateT("succeeded"):TranslateT("failed"),pszServerMsg);
				ShowNotification(szMsg,NIIF_INFO);

				if(packet->isReplyOK()){
					append(new QunGetInfoPacket(qunid));
				}

			}
			break;
		case QQ_QUN_CMD_TRANSFER:
			{
				TCHAR szMsg[MAX_PATH];
				LPCSTR szServerMsg=strdup(packet->getErrorMessage().c_str());
				LPTSTR pszServerMsg;
				HANDLE hContact=FindContact(qunid);
				int extID=READC_D2("ExternalID");

				if (!extID) extID=qunid;
				pszServerMsg=mir_a2u_cp(szServerMsg,936);

				_stprintf(szMsg,TranslateT("Qun Transfer Request for %d %s.\n%s"),qunid,packet->isReplyOK()?TranslateT("succeeded"):TranslateT("failed"),pszServerMsg);
				ShowNotification(szMsg,NIIF_INFO);

				if(packet->isReplyOK()){
					append(new QunGetInfoPacket(qunid));
				}

			}
			break;
	}
}

void CNetwork::_imCallback(int subCommand, ReceiveIMPacket* packet, void* auxpacket) {
	if (subCommand==-1) {
		int qqid=(int)packet;
		if (HANDLE hContact=FindContact(qqid)) {
			if (READC_W2("Status")==ID_STATUS_OFFLINE) {
				util_log(0,"Set QQ contact %d to invisible",qqid);
				WRITEC_W("Status",ID_STATUS_INVISIBLE);
			}
			if (READC_W2("Status")==ID_STATUS_INVISIBLE) {
				util_log(0,"Updated invisible TS for QQ contact %d",qqid);
				WRITEC_D("InvisibleTS",(DWORD)time(NULL));
			}
		}
	} else 
		switch (packet->getIMType()) {
			case QQ_RECV_IM_NEWS:
				if (READ_B2(NULL,QQ_SHOWAD)==1 && ServiceExists(MS_POPUP_ADDPOPUPW)) {
					ReceivedQQNews news(packet->getBodyData(), packet->getBodyLength());
					LPTSTR pszMsg;
					POPUPDATAW ppd={0};
					pszMsg=mir_a2u_cp(news.getTitle().c_str(),936);
					_tcscpy(ppd.lpwzContactName,pszMsg);
					mir_free(pszMsg);
					pszMsg=mir_a2u_cp(news.getBrief().c_str(),936);
					_tcscpy(ppd.lpwzText,pszMsg);
					mir_free(pszMsg);
					ppd.lchIcon=(HICON)LoadImage(hinstance, MAKEINTRESOURCE(IDI_TM), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0);
					ppd.PluginWindowProc=_noticePopupProc;
					pszMsg=mir_a2u_cp(news.getURL().c_str(),936);
					ppd.PluginData=mir_wstrdup(pszMsg);
					ppd.iSeconds=60;
					mir_free(pszMsg);
					CallService(MS_POPUP_ADDPOPUPW,(WPARAM)&ppd,0);
				}
				break;
			case 0x0012: // QQ Mail
				if (*(char*)packet->getBodyData()==3) {
					ReceivedQQMailPacket qmp(packet->getBodyData(),packet->getBodyLength());

					CHAR szUrl[MAX_PATH]="http://mail.qq.com/cgi-bin/login?Fun=clientread&Uin=";
					LPTSTR pszSender,pszTitle,pszURL;
					
					pszSender=mir_a2u_cp(qmp.getFrom().c_str(),936);
					pszTitle=mir_a2u_cp(qmp.getTitle().c_str(),936);

					// http://mail.qq.com/cgi-bin/login?Fun=clientread&Uin=xxx&K=xxx
					itoa(m_myqq,szUrl+strlen(szUrl),10);
					strcat(szUrl,"&K=");
					util_fillClientKey(szUrl+strlen(szUrl));
					strcat(szUrl,"&Mailid=");
					strcat(szUrl,qmp.getMailID().c_str());
					pszURL=mir_a2u_cp(szUrl,936);

					POPUPDATAW ppd={0};
					_tcscpy(ppd.lpwzContactName,TranslateT("You got a new QQ Mail!"));
					_stprintf(ppd.lpwzText,TranslateT("Sender: %s\nTitle: %s"),pszSender,pszTitle);
					ppd.lchIcon=(HICON)LoadImage(hinstance, MAKEINTRESOURCE(IDI_TM), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0);
					ppd.PluginWindowProc=_noticePopupProc;
					ppd.PluginData=mir_wstrdup(pszURL);
					ppd.iSeconds=60;
					CallService(MS_POPUP_ADDPOPUPW,(WPARAM)&ppd,0);

					mir_free(pszSender);
					mir_free(pszTitle);
					mir_free(pszURL);
				}
				break;
			case QQ_RECV_IM_SYS_MESSAGE:
				{
					// Received System Message
					ReceivedSystemIM sys(packet->getBodyData(), packet->getBodyLength());		

					switch(sys.getSystemIMType())
					{
						case QQ_RECV_IM_KICK_OUT:	// Kick out by server due to duplicated login
							ShowNotification(TranslateT("You were logged out from QQ network due to duplicated login"),NIIF_ERROR);
							ProtoBroadcastAck(m_szModuleName,NULL,ACKTYPE_LOGIN,ACKRESULT_FAILED,NULL,LOGINERR_OTHERLOCATION);
							GoOffline();
							break;
						default:
							{
								LPTSTR pszTemp=mir_a2u_cp(sys.getMessage().c_str(),936);
								ShowNotification(pszTemp,NIIF_WARNING);
								mir_free(pszTemp);
							}
					}
				}
				break;
			case QQ_RECV_IM_QUN_IM:
			case QQ_RECV_IM_TEMP_QUN_IM:
			case QQ_RECV_IM_UNKNOWN_QUN_IM:
				{
					HANDLE hContact;
					ReceivedQunIM* im=(ReceivedQunIM*)auxpacket;
					int qunID=packet->getSender();
					
					_qunImCallback2(qunID,im->getSenderQQ(),im->hasFontAttribute(),im->isBold(),im->isItalic(),im->isUnderline(),im->getFontSize(),im->getRed(),im->getGreen(),im->getBlue(),im->getSentTime(),im->getMessage());
					hContact=FindContact(qunID);
					if (READC_W2("QunVersion")!=im->getVersionID()) {
						WRITEC_W("QunVersion",im->getVersionID());
						WRITEC_D("QunCardUpdate",-1);
						append(new QunGetInfoPacket(qunID));
					} else {
						util_log(0,"qunImCallback(): QunVersion=%d, versionID=%d",READC_W2("QunVersion"),im->getVersionID());
						_updateQunCard(hContact, qunID);
					}

				}
				break;
			case QQ_RECV_IM_TO_BUDDY:
			case QQ_RECV_IM_TO_UNKNOWN:
			case QQ_RECV_IM_FROM_BUDDY_2006:
			case 0x1f: // Temp
				switch (subCommand) {
					case QQ_IM_EX_UDP_REQUEST:
						{
							ReceivedFileIM* received=(ReceivedFileIM*)auxpacket;
							util_log(0,"QQ_IM_EX_UDP_REQUEST");
#if 0
							_ftUpdateBuddyIP(packet->getSender(),received->getWanIp(),received->getWanPort());
							_ftReceivedFileRequest(packet->getSender(), received->getSessionId(), received->getFileName().c_str(), received->getFileSize(), received->getTransferType());
#endif
							WCHAR szPath[MAX_PATH];
							DBVARIANT dbv;
							if (HANDLE hContact=FindContact(packet->getSender())) {
								LPWSTR szFileName=mir_a2u_cp(received->getFileName().c_str(),936);
								dbv.ptszVal=NULL;
								READC_TS2("Nick",&dbv);
								swprintf(szPath,TranslateT("You received a file/pic transfer request from %s(%d), which MIMQQ2 is not supported\n\nFilename: %s"),dbv.ptszVal?dbv.ptszVal:L"",packet->getSender(),szFileName);
								if (dbv.ptszVal) DBFreeVariant(&dbv);
								mir_free(szFileName);
								ShowNotification(szPath,NIIF_WARNING);

								SendFileExRequestPacket *packet2 = new SendFileExRequestPacket(QQ_IM_EX_REQUEST_CANCELLED);
								packet2->setReceiver(packet->getSender());
								packet2->setTransferType(received->getTransferType());
								packet2->setSessionId(received->getSessionId());
								append(packet2);

							}
						}
						break;
					case QQ_IM_EX_REQUEST_ACCEPTED:
						util_log(0,"QQ_IM_EX_REQUEST_ACCEPTED");
						/*
						{
							ReceivedFileIM* received=(ReceivedFileIM*)auxpacket;

							_ftReceivedFileAccepted(packet->getSender(), received->getSessionId(), 
								received->getWanIp(), true, received->getTransferType());
						}
						*/
						break;
					case QQ_IM_EX_REQUEST_CANCELLED:
						/*
						{
							ReceivedFileIM* received=(ReceivedFileIM*)auxpacket;
							util_log(0,"EvaPacketManager::processReceiveIM -- cmd canceled\n");

							_ftReceivedFileAccepted(packet->getSender(), received->getSessionId(), 
							received->getWanIp(), false, received->getTransferType());
						}
						*/
						break;
					case QQ_IM_NOTIFY_FILE_AGENT_INFO:
						/*
						{
							ReceivedFileIM* received=(ReceivedFileIM*)auxpacket;
							util_log(0,"QQ_IM_NOTIFY_FILE_AGENT_INFO");
							_ftReceivedFileAgentInfo(packet->getSender(), received->getSequence(), received->getSessionId(), 
								received->getWanIp(), received->getWanPort(), (char*)received->getAgentServerKey());
						}
						*/
						break;
					case QQ_IM_EX_NOTIFY_IP:
						/*
						{
							ReceivedFileExIpIM* received=(ReceivedFileExIpIM*)auxpacket;

							util_log(0,"QQ_IM_EX_NOTIFY_IP");
							_ftReceivedFileNotifyIpEx(packet->getSender(), received->isSender(),
								received->getSessionId(), received->getTransferType(),
								received->getWanIp1(), received->getWanPort1(),
								received->getWanIp2(), received->getWanPort2(),
								received->getWanIp3(), received->getWanPort3(),
								received->getLanIp1(), received->getLanPort1(),
								received->getLanIp2(), received->getLanPort2(),
								received->getLanIp3(), received->getLanPort3(),
								received->getSyncIp(), received->getSyncPort(), received->getSyncSession()
								);
						}
						*/
						break;
					default:
						_imCallback(packet->getIMType(),auxpacket);
				}
				break;

			//case QQ_RECV_IM_CREATE_QUN:
			case QQ_RECV_IM_ADDED_TO_QUN:
			case QQ_RECV_IM_DELETED_FROM_QUN:// these three type of notifications has no messages
			case QQ_RECV_IM_APPROVE_JOIN_QUN:
			case QQ_RECV_IM_REQUEST_JOIN_QUN:
			case QQ_RECV_IM_REJECT_JOIN_QUN:
			case QQ_RECV_IM_SET_QUN_ADMIN:
				{
					ReceivedQunIMJoinRequest join(packet->getIMType(),packet->getBodyData(), packet->getBodyLength());
					union {
						TCHAR wszTemp[MAX_PATH*2];
						CHAR szTemp[MAX_PATH*4];
					};

					*wszTemp=0;
					wszTemp[MAX_PATH]=0;

					switch (packet->getIMType()) {
						case QQ_RECV_IM_APPROVE_JOIN_QUN: 
							{
								swprintf(wszTemp,TranslateT("You have been approved to join Qun %d."),packet->getSender());
								ShowNotification(wszTemp,NIIF_INFO);
								append(new QunGetInfoPacket(packet->getSender()));
								break;
							}
						case QQ_RECV_IM_REQUEST_JOIN_QUN:
							_sysRequestJoinQunCallback(packet->getSender(),join.getExtID(),join.getSender(), join.getMessage().c_str(),join.getToken(),join.getTokenLength());
							break;
						case QQ_RECV_IM_REJECT_JOIN_QUN:
							_sysRejectJoinQunCallback(packet->getSender(),join.getExtID(),join.getSender(), join.getMessage().c_str());
							break;
						case QQ_RECV_IM_SET_QUN_ADMIN:
							switch (join.getType()) {
								case QQ_QUN_SET_ADMIN:
									swprintf(wszTemp+MAX_PATH,TranslateT("The Qun Creator assigned %d to be administrator."),join.getSender());
									break;
								case QQ_QUN_UNSET_ADMIN:
									swprintf(wszTemp+MAX_PATH,TranslateT("The Qun Creator revoked administrator identity of %d."),join.getSender());
									break;
							}
							break;
						case QQ_RECV_IM_ADDED_TO_QUN:
						case QQ_RECV_IM_DELETED_FROM_QUN:
							{
								LPTSTR pszComment=mir_a2u_cp(join.getMessage().c_str(),936);

								switch (packet->getIMType()) {
									case QQ_RECV_IM_DELETED_FROM_QUN:
										if (join.getCommander()==0)
											swprintf(wszTemp+MAX_PATH,TranslateT("%d left this Qun."),join.getSender());
										else
											swprintf(wszTemp+MAX_PATH,TranslateT("%d(%s) removed %d from this Qun."),join.getCommander(),pszComment,join.getSender());
										break;
									case QQ_RECV_IM_ADDED_TO_QUN:
										_stprintf(wszTemp+MAX_PATH,TranslateT("%d(%s) added %d to this Qun."),join.getCommander(),pszComment,join.getSender());
										break;
								}

								mir_free(pszComment);
							}
							break;

					}

					if (wszTemp[MAX_PATH]) {
						PROTORECVEVENT pre;
						CCSDATA ccs;
						HANDLE hContact=FindContact(packet->getSender());

						WideCharToMultiByte(CP_ACP,0,wszTemp+MAX_PATH,-1,szTemp,MAX_PATH*2,NULL,NULL);
						memmove(szTemp+strlen(szTemp)+1,wszTemp+MAX_PATH,sizeof(WCHAR)*(wcslen(wszTemp+MAX_PATH)+1));

						pre.szMessage = szTemp;
						pre.flags = PREF_UNICODE;
						pre.timestamp = (DWORD)time(NULL);
						pre.lParam = 0;

						ccs.hContact=hContact;
						ccs.szProtoService = PSR_MESSAGE;
						ccs.wParam = 0;
						ccs.lParam = ( LPARAM )&pre;
						CallService(MS_PROTO_CHAINRECV, 0, ( LPARAM )&ccs );

						WRITEC_W("CardVersion",-1);
						append(new QunGetInfoPacket(packet->getSender()));
					}

					break;
				}
				break;
			case QQ_RECV_IM_SIGNATURE_CHANGED:
				{
					SignatureChangedPacket sig(packet->getBodyData(), packet->getBodyLength());
					HANDLE hContact=FindContact(sig.getQQ());
					if (hContact) {
						LPTSTR szSign;
						LPCSTR pszSignature=sig.getSignature().c_str();
						bool fSong=pszSignature[strlen(pszSignature)-1]==1;

						szSign=mir_a2u_cp(string(pszSignature,strlen(pszSignature)-(fSong?1:0)).c_str(),936);
						WRITEC_TS("PersonalSignature",szSign);

						if (hContact) {
							if (fSong) {
								WRITEC_TS("ListeningTo",szSign);
							} else {
								DBWriteContactSettingTString(hContact,"CList","StatusMsg",szSign);
								DELC("ListeningTo");
							}
						}

						mir_free(szSign);

					}
					break;
				}
			case 0x1e: // Signature acknowledge
				break;
			/*
			case 0x70: // Unknown
			case 0x12:
				{
					FILE* fp=fopen("f:\\mimqq-0x70.txt","wb");
					fwrite(packet->getBodyData(),packet->getBodyLength(),1,fp);
					fclose(fp);
				}
				*/
				break;
		}
}

void CNetwork::_sendImCallback(SendIMReplyPacket* packet,SendTextIMPacket* im) {
	OutPacket* op=m_pendingImList[packet->getSequence()+1];

	util_log(0,"Received IM callback, next seq=0x%x, op=0x%x",packet->getSequence()+1, op);

	if (op) {
		m_pendingImList.erase(op->getSequence());
		append(op);
	} else {
		delayReport_t* dr=(delayReport_t*)mir_alloc(sizeof(delayReport_t));
		dr->hContact=FindContact(im->getReceiver());
		dr->ackType=ACKTYPE_MESSAGE;
		dr->ackResult=ACKRESULT_SUCCESS;
		dr->aux=packet->getSequence();
		dr->aux2=NULL;
		ForkThread((ThreadFunc)&CNetwork::delayReport,dr);
	}
}

void CNetwork::_signatureOpCallback(SignatureReplyPacket* packet) {
	switch(packet->getType()) {
		// TODO: Modify/Delete my signature
		case QQ_SIGNATURE_MODIFY:
			if(packet->isChangeAccepted()) {
				map<unsigned int, unsigned int> qqlist;
				SignaturePacket* sp=new SignaturePacket(QQ_SIGNATURE_REQUEST);
				qqlist[m_myqq]=0;
				sp->setMembers(qqlist);
				append(sp);
			}
			break;
		case QQ_SIGNATURE_DELETE:
			if(packet->isChangeAccepted()) {
				DBDeleteContactSetting(NULL,m_szModuleName,"PersonalSignature");
			}
			break;
		case QQ_SIGNATURE_REQUEST:{
			std::map<unsigned int, SignatureElement> members = packet->getMembers();
			std::map<unsigned int, SignatureElement>::iterator iter;
			HANDLE hContact;

			for(iter = members.begin(); iter!= members.end(); ++iter){
				// Time is iter->second.lastModifyTime, may not used by MirandaQQ
				hContact=FindContact(iter->first);
				if (hContact || iter->first==m_myqq) { // Only writes status message if contact is in my list
					LPTSTR szSign;
					LPCSTR pszSignature=iter->second.signature.c_str();
					bool fSong=pszSignature[strlen(pszSignature)-1]==1;

					szSign=mir_a2u_cp(string(pszSignature,strlen(pszSignature)-(fSong?1:0)).c_str(),936);
					//util_convertToNative(&szSign,string(pszSignature,strlen(pszSignature)-(fSong?1:0)).c_str());
					WRITEC_TS("PersonalSignature",szSign);

					if (hContact) {
						if (fSong) {
							WRITEC_TS("ListeningTo",szSign);
							//DBDeleteContactSetting(hContact,"CList","StatusMsg");
						} else {
							DBWriteContactSettingTString(hContact,"CList","StatusMsg",szSign);
							DELC("ListeningTo");
						}
					}
					if (iter->first==m_myqq) WRITEC_TS("StatusMsg",szSign);

					//free(szSign);
					mir_free(szSign);
				}
			}
			// TODO: Do this still needed as I request a subset of members each time?
			//doRequestSignature(packet->nextStartID());
			break;
		}
	}
}

void CNetwork::_getLevelCallback(EvaGetLevelReplyPacket* packet) {
	std::list<LevelUserItem> friends = packet->getLevelList();
	
	HANDLE hContact;
	
	for (std::list<LevelUserItem>::iterator iter=friends.begin(); iter!=friends.end(); iter++) {
		hContact=FindContact(iter->qqNum);
		struct tm *newtime;
		time_t ot=iter->onlineTime;
		int sun, moon, star;

		newtime = localtime(&ot);   // Convert time to struct tm form
		EvaUtil::calcSuns(iter->level,&sun,&moon,&star);

		util_log(0,"%d is Level %d, %d hrs online, Sun=%d, Moon=%d, Star=%d, remain=%d",iter->qqNum, iter->level,iter->onlineTime/3600,sun,moon,star,iter->timeRemainder);

		if(iter->qqNum == m_myqq){
			WRITE_D(NULL,"OnlineMins",iter->onlineTime);
			WRITE_W(NULL,"Level",iter->level);
			WRITE_W(NULL,"HoursToLevelUp",iter->timeRemainder);
		}

		if (hContact) {
			WRITEC_D("OnlineMins",iter->onlineTime);
			WRITEC_W("Level",iter->level);
			WRITEC_W("HoursToLevelUp",iter->timeRemainder);
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

void CNetwork::_friendChangeStatusCallback(FriendChangeStatusPacket* packet) {
	int newStatus;
	HANDLE hContact=FindContact(packet->getQQ());

	if (hContact) { // Only handles if the buddy is in my contact list
		int oldStatus=READ_W2(hContact,"Status");

		switch (packet->getStatus()) {
			case QQ_FRIEND_STATUS_INVISIBLE:	newStatus=ID_STATUS_INVISIBLE;	break;
			case QQ_FRIEND_STATUS_LEAVE:		newStatus=ID_STATUS_AWAY;		break;
			case QQ_FRIEND_STATUS_OFFLINE:		newStatus=ID_STATUS_OFFLINE;	break;
			case QQ_FRIEND_STATUS_ONLINE: default:	newStatus=ID_STATUS_ONLINE;	break;
		}

		if (oldStatus!=newStatus) {
			//in_addr ia;
			int ip=htonl(packet->getIP());

			//if (newStatus!=ID_STATUS_OFFLINE) {
			if (oldStatus==ID_STATUS_OFFLINE) {
				char szPluginPath[MAX_PATH];

				CallService(MS_UTILS_PATHTOABSOLUTE,(WPARAM)"Plugins\\qqVersion.ini",(LPARAM)szPluginPath);

				std::map<unsigned int, unsigned int> list;
				SignaturePacket *packet2 = new SignaturePacket(QQ_SIGNATURE_REQUEST);
				list[packet->getQQ()] = 0;
				packet2->setMembers(list);
				append(packet2);

				_writeVersion(hContact,packet->getUnknown3_13_14(),szPluginPath);

#if 0
				if (packet->getIP()) {
					//ia.S_un.S_addr=htonl(packet->getIP());
					WRITEC_D("IP",packet->getIP());
					WRITEC_W("Port",packet->getPort());
					_writeIP(hContact,packet->getIP());
				}
#endif
				util_log(0,"%s(): Contact %d changed status to %d. IP=%s",__FUNCTION__,packet->getQQ(),newStatus,/*packet->getIP()?inet_ntoa(ia):""*/inet_ntoa(*(in_addr*)&ip));
			}

			DBWriteContactSettingWord(hContact,m_szModuleName,"Status",newStatus);
		}
	}
}

void CNetwork::_tempSessionOpCallback(TempSessionOpReplyPacket* packet) {
	delayReport_t* dr=(delayReport_t*)mir_alloc(sizeof(delayReport_t));
	dr->hContact=FindContact(packet->getReceiver()+0x80000000);
	dr->ackType=ACKTYPE_MESSAGE;
	dr->ackResult=ACKRESULT_SUCCESS;
	dr->aux=1;
	dr->aux2=NULL;
	ForkThread((ThreadFunc)&CNetwork::delayReport,dr);
}

void CNetwork::_weatherOpCallback(WeatherOpReplyPacket* packet) {
	time_t tm;
	LPTSTR lpwzShortDesc, lpwzWind, lpwzHint;

	WCHAR szMsg[2048];
	list<Weather> weathers=packet->getWeathers();
	list<Weather>::iterator iter;

	LPWSTR lpwzProvince=mir_a2u_cp(packet->getProvince().c_str(),936);
	LPWSTR lpwzCity=mir_a2u_cp(packet->getCity().c_str(),936);
	LPWSTR lpwzTime;

	_stprintf(szMsg,TranslateT("Weather of %s %s:\n\n"),lpwzProvince,lpwzCity);
	mir_free(lpwzProvince);
	mir_free(lpwzCity);

	for (iter=weathers.begin(); iter!=weathers.end(); iter++) {
		tm=(time_t)(iter)->getTime();
		lpwzShortDesc=mir_a2u_cp(iter->getShortDesc().c_str(),936);
		lpwzWind=mir_a2u_cp(iter->getWind().c_str(),936);
		lpwzHint=mir_a2u_cp(iter->getHint().c_str(),936);
		lpwzTime=_tctime(&tm);
		*wcsstr(lpwzTime,L"00:")=0;
		_stprintf(szMsg+_tcslen(szMsg),TranslateT("%sCondition: %s\nWind: %s\nMax: %d Min: %d\n%s\n\n"),_tctime(&tm),lpwzShortDesc,lpwzWind,(iter)->getHighTemperature(),(iter)->getLowTemperature(),lpwzHint);
	}
	
	ForkThread((ThreadFunc)&CNetwork::ThreadMsgBox,mir_tstrdup(szMsg));
}

void CNetwork::_searchUserCallback(SearchUserReplyPacket* packet) {
	std::list<OnlineUser> list=packet->getUsers();
	std::list<OnlineUser>::iterator iter;

	PROTOSEARCHRESULT psr;
	char uid[16];

	for(iter = list.begin(); iter!=list.end(); ++iter) {
		sprintf(uid,"%d",iter->getQQ());

		ZeroMemory(&psr, sizeof(psr));
		psr.cbSize = sizeof(psr);
		psr.nick = uid;
		psr.firstName=mir_strdup(iter->getNick().c_str());
		util_convertFromGBK(psr.firstName);
		psr.lastName=mir_strdup(iter->getProvince().c_str());
		util_convertFromGBK(psr.lastName);

		ProtoBroadcastAck(m_szModuleName, NULL, ACKTYPE_SEARCH, ACKRESULT_DATA, (HANDLE) 1, (LPARAM)&psr);
		mir_free(psr.firstName);
		mir_free(psr.lastName);
	}

	if (m_searchUID) {
		QunSearchPacket* packet=new QunSearchPacket();
		packet->setExtID(m_searchUID);
		append(packet);
	} else
		ProtoBroadcastAck(m_szModuleName, NULL, ACKTYPE_SEARCH, ACKRESULT_SUCCESS, (HANDLE) 1, (LPARAM)0);
}

void CNetwork::_addFriendCallback(EvaAddFriendExReplyPacket* packet) {
	TCHAR szMsg[MAX_PATH];
	if (m_addUID) {
		switch (packet->getAuthStatus()) {
			case QQ_ADD_FRIEND_EX_ALREADY_IN_LIST:
				ForkThread((ThreadFunc)&CNetwork::ThreadMsgBox,mir_tstrdup(szMsg));
				break;
			case QQ_AUTH_NO_AUTH: // Add Successful
				util_log(0,"QQ_AUTH_NO_AUTH, refresh friend list");
				append(new GetFriendListPacket());
				break;
			case QQ_AUTH_NO_ADD: // Adding not allowed
				{
					swprintf(szMsg,TranslateT("QQ User %d does not allow others to add him/her."),m_addUID);
					ForkThread((ThreadFunc)&CNetwork::ThreadMsgBox,mir_tstrdup(szMsg));
					m_addUID=0;
					break;
				}
			case QQ_AUTH_NEED_AUTH: // Need authorization
				{
					HANDLE hContact=FindContact(m_addUID);
					DBVARIANT dbv={0};

					if (hContact) DBGetContactSetting(hContact,m_szModuleName,"AuthReason",&dbv);
					if (!hContact || !dbv.pszVal || !*dbv.pszVal) {
						ASKDLGPARAMS* adp=(ASKDLGPARAMS*)malloc(sizeof(ASKDLGPARAMS));
						adp->network=this;
						adp->command=QQ_CMD_ADD_FRIEND_AUTH;
						adp->hContact=hContact;
						DialogBoxParam(hinstance,MAKEINTRESOURCE(IDD_CHANGESIGNATURE),NULL,ModifySignatureDlgProc,(LPARAM)adp);
					} else {
						AddFriendAuthPacket *packet=new AddFriendAuthPacket(m_addUID,QQ_MY_AUTH_REQUEST);
						util_convertToGBK(dbv.pszVal);
						packet->setMessage(dbv.pszVal);
						util_log(0,"QQ_AUTH_NEED_AUTH, send auth request, reason=%s",dbv.pszVal);
						append(packet);
						DBFreeVariant(&dbv);
					}
					break;
				}
		}
	}
}

void CNetwork::_systemMessageCallback(SystemNotificationPacket* packet) {
	switch (packet->getType()) {
		case QQ_MSG_SYS_ADD_FRIEND_APPROVED: // Your friend approved you to add him/her
			{
				WCHAR szMsg[MAX_PATH];
				HANDLE hContact=FindContact(packet->getFromQQ());

				swprintf(szMsg,TranslateT("Your add friend request to %d have been approved."),packet->getFromQQ());
				DELC("Reauthorize");
				DELC("AuthReason");
				ShowNotification(szMsg,NIIF_INFO);
				append(new GetFriendListPacket());
			}
			break;
		case QQ_MSG_SYS_BEING_ADDED: // You are being added by other party
			{
				unsigned int uid=packet->getFromQQ();
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
		case QQ_MSG_SYS_ADD_FRIEND_REJECTED: // Your friend rejected your add request
			{
				LPWSTR lpwzMsg=mir_a2u_cp(packet->getMessage().c_str(),936);
				ShowNotification(lpwzMsg,NIIF_ERROR);
				mir_free(lpwzMsg);
			}
			break;
		case QQ_MSG_SYS_ADD_FRIEND_REQUEST: // Your friend would like to add you to his/her list
		case QQ_MSG_SYS_ADD_FRIEND_REQUEST_EX:
			if (READ_B2(NULL,QQ_BLOCKEMPTYREQUESTS)==0) {
				CCSDATA ccs;
				PROTORECVEVENT pre;
				int qqid=packet->getFromQQ();
				HANDLE hContact=FindContact(qqid);
				char* msg=mir_strdup(packet->getMessage().c_str());
				char* szBlob;
				char* pCurBlob;

				if (!hContact) { // The buddy is not in my list, get information on buddy
					hContact=AddContact(qqid,true,false);
					append(new GetUserInfoPacket(qqid));
				}
				//util_log(0,"%s(): QQID=%d, msg=%s",__FUNCTION__,qqid,szMsg);

				ccs.szProtoService=PSR_AUTH;
				ccs.hContact=hContact;
				ccs.wParam=0;
				ccs.lParam=(LPARAM)&pre;
				pre.flags=0;
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
				util_convertFromGBK(pCurBlob);
				pre.szMessage=(char *)szBlob;

				util_log(0,"%s(): QQID=%d, msg=%s",__FUNCTION__,qqid,pCurBlob);

				CallService(MS_PROTO_CHAINRECV,0,(LPARAM)&ccs);

				mir_free(szBlob);
				mir_free(msg);
			}
			break;
	}
}

void CNetwork::_deleteMeCallback(DeleteMeReplyPacket* packet) {
	TCHAR szMsg[MAX_PATH];

	swprintf(szMsg,TranslateT("Your request to be removed from %d %s."),packet->getQQ(),packet->isDeleted()?TranslateT("succeeded"):TranslateT("failed"));
	ShowNotification(szMsg,NIIF_INFO);
}

void CNetwork::_groupNameOpCallback(GroupNameOpReplyPacket* packet) {
	int hGroupMax=0;

	if(packet->isDownloadReply()){
		std::list<std::string>::iterator iter;
		std::list<std::string> names = packet->getGroupNames();
		CLIST_INTERFACE* ci=(CLIST_INTERFACE*)CallService(MS_CLIST_RETRIEVE_INTERFACE,0,0);

		for(iter = names.begin(); iter != names.end(); ++iter){
			int id;
			LPWSTR szGroupname=mir_a2u_cp(iter->c_str(),936);

			id=util_group_name_exists(szGroupname,-1);
			if (m_downloadGroup && ci) { // Download group is performing
				if (id==-1) { // This group is not in contact list
					util_log(0,"%s(): Creating new group: %s",__FUNCTION__,szGroupname);
					m_hGroupList[hGroupMax]=(HANDLE)CallService(MS_CLIST_GROUPCREATE, 0, 0);
					//CallService(MS_CLIST_GROUPRENAME, (WPARAM)m_hGroupList[m_hGroupMax], (LPARAM)groupname);
					ci->pfnRenameGroup((int)m_hGroupList[hGroupMax],szGroupname);
				} else { // This group is already in contact list, record group ID
					m_hGroupList[hGroupMax]=(HANDLE)(id+1);
				}
				hGroupMax++;
			}
			mir_free(szGroupname);
		}
		append(new DownloadGroupFriendPacket());
	}else{
		map<unsigned int, unsigned int> destlist;
		UploadGroupFriendPacket* packet=new UploadGroupFriendPacket();

		for(map<int,HANDLE>::iterator iter=m_hGroupList.begin(); iter!=m_hGroupList.end(); iter++) {
			destlist[(unsigned int)iter->first]=(unsigned int)iter->second;
		}

		packet->setGroupedFriends(destlist);
		append(packet);
	}

}

void CNetwork::_uploadGroupFriendCallback(UploadGroupFriendReplyPacket* packet) {
	if (packet->uploadOk()) {
		WCHAR szTemp[MAX_PATH];
		swprintf(szTemp,TranslateT("%d users and %d groups uploaded to server."),m_hGroupList.size(),m_qqusers);
		ShowNotification(szTemp,NIIF_INFO);
	} else {
		ShowNotification(TranslateT("Failed upload contact list"),NIIF_ERROR);
	}
	m_qqusers=0;
	m_downloadGroup=false;
	m_hGroupList.clear();
}

void CNetwork::_deleteFriendCallback(DeleteFriendReplyPacket* packet) {
	ShowNotification(packet->isDeleted()?TranslateT("Specified contact has been removed from server list."):TranslateT("Failed to remove specified contact from server list."),packet->isDeleted()?NIIF_INFO:NIIF_ERROR);
}

void CNetwork::_keepAliveCallback(KeepAliveReplyPacket*) {
	if (++m_keepaliveCount>2) {
		m_keepaliveCount=0;
		append(new GetOnlineFriendsPacket());
	}
}

void CNetwork::_requestLoginTokenExCallback(RequestLoginTokenExReplyPacket* packet) {
	if (packet->getReplyCode() == QQ_LOGIN_TOKEN_NEED_VERI) {
		XGraphicVerifyCode* code=new XGraphicVerifyCode();
		code->setSessionToken(packet->getToken(), packet->getTokenLength());
		code->setData(packet->getData(), packet->getDataLength());

		removePacket(0);

		CodeVerifyWindow* win=new CodeVerifyWindow(code);
		delete win;
	}
}
void __cdecl CNetwork::_addFriendAuthGraphicalVerification(LPVOID adp) {
	DialogBoxParam(hinstance,MAKEINTRESOURCE(IDD_CHANGESIGNATURE),NULL,ModifySignatureDlgProc,(LPARAM)adp);
}

void __cdecl CNetwork::_tempSessionGraphicalVerification(LPVOID adp) {
	CodeVerifyWindow cvw((ASKDLGPARAMS*)adp);
}

void CNetwork::_addFriendAuthInfoCallback(EvaAddFriendGetAuthInfoReplyPacket* packet) {
	ASKDLGPARAMS* adp=NULL;

	if(packet->getSubCommand() == AUTH_INFO_CMD_CODE){
		util_log(0,"auth info cmd code reply");
		if(packet->getReplyCode() == 0x01){
			PostMessage(CodeVerifyWindow::getHwnd(packet->getSequence()),WM_USER+1,0,0);
			return;
		}
	}

	switch (packet->getReplyCode()) {
		case AUTH_INFO_TYPE_GRAPHIC:
			if (!m_codeVerifyWindow) {
				adp=(ASKDLGPARAMS*)malloc(sizeof(ASKDLGPARAMS));
				adp->network=this;
				adp->fAux=true;
			}
		case AUTH_INFO_TYPE_CODE:
			{
				if (packet->getSubSubCommand()==AUTH_INFO_SUB_CMD_TEMP_SESSION && !adp) {
					if (CodeVerifyWindow::getHwnd(packet->getSequence())) {
						SendMessage(CodeVerifyWindow::getHwnd(packet->getSequence()),WM_USER+2,packet->getCodeLength(),(LPARAM)packet->getCode());
					} else {
						m_savedTempSessionMsg->setAuthInfo(packet->getCode(),packet->getCodeLength()); 
						append(m_savedTempSessionMsg);
						m_savedTempSessionMsg=NULL;
					}
				} else {
					HANDLE hContact=NULL;
					if (packet->getSubSubCommand()==AUTH_INFO_SUB_CMD_QUN || packet->getSubSubCommand()==AUTH_INFO_SUB_CMD_USER) {
						hContact=FindContact(m_addUID);
					} else
						hContact=FindContact(0x80000000+m_savedTempSessionMsg->getReceiver());
					/*else if (packet->getSubSubCommand()==AUTH_INFO_SUB_CMD_TEMP_SESSION)
						hContact=FindContact(m_savedTempSessionMsg->getReceiver());*/

					//DBVARIANT dbv={0};
					if (!hContact) return;

					if (CodeVerifyWindow::getHwnd(packet->getSequence())) {
						SendMessage(CodeVerifyWindow::getHwnd(packet->getSequence()),WM_USER+2,packet->getCodeLength(),(LPARAM)packet->getCode());
						break;
					}

					if (!adp) {
						adp=(ASKDLGPARAMS*)malloc(sizeof(ASKDLGPARAMS));
						adp->fAux=false;
					}
					adp->network=this;
					adp->command=packet->getSubSubCommand();
					adp->hContact=hContact;
					adp->nAux=packet->getCodeLength();
					adp->pAux=mir_alloc(adp->nAux);
					memcpy(adp->pAux,packet->getCode(),adp->nAux);
					if (packet->getSubSubCommand()==AUTH_INFO_SUB_CMD_QUN || packet->getSubSubCommand()==AUTH_INFO_SUB_CMD_USER)
						//mir_forkthread(_addFriendAuthGraphicalVerification,adp);
						ForkThread((ThreadFunc)&CNetwork::_addFriendAuthGraphicalVerification,adp);
					else
						//mir_forkthread(_tempSessionGraphicalVerification,adp);
						ForkThread((ThreadFunc)&CNetwork::_tempSessionGraphicalVerification,adp);
				}
			}
			break;
	}
}

void CNetwork::callbackHub(int command, int subcommand, WPARAM wParam, LPARAM lParam) {
	switch (command) {
		case QQ_CMD_REQUEST_LOGIN_TOKEN_EX: _requestLoginTokenExCallback((RequestLoginTokenExReplyPacket*)wParam);
		case QQ_CMD_LOGIN: _loginCallback((LoginReplyPacket*)wParam); break;
		case QQ_CMD_KEEP_ALIVE: _keepAliveCallback((KeepAliveReplyPacket*)wParam); break;
		case QQ_CMD_CHANGE_STATUS: _changeStatusCallback((ChangeStatusReplyPacket*)wParam); break;
		case QQ_CMD_GET_USER_INFO: _getUserInfoCallback((GetUserInfoReplyPacket*)wParam); break;
		case QQ_CMD_GET_FRIEND_LIST: _getFriendListCallback((GetFriendListReplyPacket*)wParam); break;
		case QQ_CMD_GET_FRIEND_ONLINE: _getOnlineFriendCallback((GetOnlineFriendReplyPacket*)wParam); break;
		case QQ_CMD_DOWNLOAD_GROUP_FRIEND: _downloadGroupFriendCallback((DownloadGroupFriendReplyPacket*)wParam); break;
		case QQ_CMD_REQUEST_EXTRA_INFORMATION: _requestExtraInfoCallback((RequestExtraInfoReplyPacket*)wParam); break;
		case QQ_CMD_SIGNATURE_OP: _signatureOpCallback((SignatureReplyPacket*)wParam); break;
		case QQ_CMD_RECV_IM: _imCallback(subcommand,(ReceiveIMPacket*)wParam,(void*)lParam); break;
		case QQ_CMD_QUN_CMD: _qunCommandCallback((QunReplyPacket*)wParam); break;
		case QQ_CMD_RECV_MSG_FRIEND_CHANGE_STATUS: _friendChangeStatusCallback((FriendChangeStatusPacket*)wParam); break;
		case QQ_CMD_SEARCH_USER: _searchUserCallback((SearchUserReplyPacket*)wParam); break;
		case QQ_CMD_GET_LEVEL: _getLevelCallback((EvaGetLevelReplyPacket*)wParam); break;
		case QQ_CMD_RECV_MSG_SYS: _systemMessageCallback((SystemNotificationPacket*)wParam); break;
		case QQ_CMD_ADD_FRIEND_EX: _addFriendCallback((EvaAddFriendExReplyPacket*)wParam); break;
		case QQ_CMD_ADD_FRIEND_AUTH_INFO: _addFriendAuthInfoCallback((EvaAddFriendGetAuthInfoReplyPacket*)wParam); break;
		case QQ_CMD_DELETE_FRIEND: _deleteFriendCallback((DeleteFriendReplyPacket*)wParam); break;
		case QQ_CMD_DELETE_ME: _deleteMeCallback((DeleteMeReplyPacket*)wParam); break;
		case QQ_CMD_GROUP_NAME_OP: _groupNameOpCallback((GroupNameOpReplyPacket*)wParam); break;
		case QQ_CMD_SEND_IM: _sendImCallback((SendIMReplyPacket*)wParam,(SendTextIMPacket*)lParam); break;
		case QQ_CMD_TEMP_SESSION_OP: _tempSessionOpCallback((TempSessionOpReplyPacket*)wParam); break;
		case QQ_CMD_WEATHER: _weatherOpCallback((WeatherOpReplyPacket*)wParam); break;
		case QQ_CMD_UPLOAD_GROUP_FRIEND: _uploadGroupFriendCallback((UploadGroupFriendReplyPacket*)wParam); break;
	}
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
#if 0
		case 0: // Received qunpic
			{
				ReceivedQunIM* im=(ReceivedQunIM*)aux;
				qunImCallback2(qunid,im->getSenderQQ(),im->hasFontAttribute(),im->isBold(),im->isItalic(),im->isUnderline(),im->getFontSize(),im->getRed(),im->getGreen(),im->getBlue(),im->getSentTime(),im->getMessage());
			}
			break;
#endif
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

									sendStr+=htmlParser.generateSendFormat(szMD5File, pqi.sessionid, pqi.ip, pqi.port);
								}
								pszSend2+=(strlen(pszSend2)+6);
							}
						} else {
							pszSend2=pszSend3+1;
						}
					}

					if (pszSend2)
						sendStr+=pszSend2;

					// Message is already in GBK encoding
					util_log(0,"Send String=%s",sendStr.c_str());
					QunSendIMExPacket* p=new QunSendIMExPacket(qunid);
					p->setMessage(sendStr.c_str());
					append(p);
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
void _ftCustomEvent(QCustomEvent* e) {
#if 0
	switch(e->type()){
	case Eva_FileNotifyAgentEvent:{
		EvaFileNotifyAgentEvent *ae = (EvaFileNotifyAgentEvent *)e;
		_ftNotifyAgentRequest(ae->getBuddyQQ(), ae->getOldSession(), ae->getAgentSession(),
			ae->getAgentIp(), ae->getAgentPort(), ae->getTransferType());
								  }
								  break;
	case Eva_FileNotifyStatusEvent:{
		EvaFileNotifyStatusEvent *se = (EvaFileNotifyStatusEvent *)e;
		_ftNotifyTransferStatus(se->getBuddyQQ(), se->getSession(),
			se->getFileSize(), se->getBytesSent(), se->getTimeElapsed());
								   }
								   break;
	case Eva_FileNotifySessionEvent:{
		EvaFileNotifySessionEvent *se = (EvaFileNotifySessionEvent *)e;
		_ftNotifyTransferSessionChanged(se->getBuddyQQ(), se->getOldSession(), se->getNewSession());
									}
									break;
	case Eva_FileNotifyNormalEvent:{
		EvaFileNotifyNormalEvent *ne = (EvaFileNotifyNormalEvent *)e;
		_ftNotifyTransferNormalInfo(ne->getBuddyQQ(), ne->getSession(), ne->getStatus(),
			ne->getDir(), ne->getFileName(), ne->getFileSize(), ne->getTransferType());
								   }
								   break;
	case Eva_FileNotifyAddressEvent:{
		printf("EvaFileManager::customEvent -- Eva_FileNotifyAddressEvent Got!");
		EvaFileNotifyAddressEvent *ae = (EvaFileNotifyAddressEvent *)e;
		_ftNotifyAddressRequest(ae->getBuddyQQ(), ae->getSession(), ae->getSynSession(), 
			ae->getIp(), ae->getPort(), ae->getMyIp(), ae->getMyPort());
									}
									break;
	default:
		break;
	}
#endif
}

void ftCallbackHub(int command, int subcommand, WPARAM wParam, LPARAM lParam) {
	switch (command) {
		case 0: // CustomEvent
			{
				_ftCustomEvent((QCustomEvent*)wParam);
				break;
			}
	}
}

void __cdecl CNetwork::delayReport(LPVOID lpData) {
	delayReport_t* dr=(delayReport_t*)lpData;

	Sleep(500);
	ProtoBroadcastAck(m_szModuleName, dr->hContact, dr->ackType, dr->ackResult, (HANDLE)dr->aux, (LPARAM)dr->aux2);
	if (dr->aux2) mir_free(dr->aux2);
	mir_free(dr);
}
