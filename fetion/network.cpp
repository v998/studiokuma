#include "stdafx.h"

CNetwork::CNetwork(LPCSTR szModuleName, LPCTSTR szUserName):
CClientConnection("CNETWORK",1000), m_ssic(NULL), m_uri(NULL), m_username(NULL) {
	m_szModuleName=m_szProtoName=mir_strdup(szModuleName);
	if (szUserName) {
		m_tszUserName=(LPWSTR)mir_alloc((wcslen(szUserName)+9)*sizeof(wchar_t));
		swprintf(m_tszUserName,L"Fetion(%s)",szUserName);
	} else {
		//m_tszUserName=mir_a2u(m_szModuleName);
		m_tszUserName=(LPWSTR)mir_alloc((strlen(m_szModuleName)+9)*sizeof(wchar_t));
		swprintf(m_tszUserName,L"Fetion(%S)",m_szModuleName);
	}

	m_iStatus=ID_STATUS_OFFLINE;
	m_iXStatus=0;

	SetResident();
	LoadAccount();
}

CNetwork::~CNetwork() {
	UnloadAccount();
	mir_free(m_szModuleName);
	mir_free(m_tszUserName);
	if (m_ssic) mir_free(m_ssic);
	util_log("[CNetwork] Instance Destruction Complete");
}

#define SETRESIDENTVALUE(a) strcpy(pszTemp,a); CallService(MS_DB_SETSETTINGRESIDENT,TRUE,(LPARAM)szTemp)
void CNetwork::SetResident() {
	CHAR szTemp[MAX_PATH];
	LPSTR pszTemp;
	strcpy(szTemp,m_szProtoName);
	strcat(szTemp,"/");
	pszTemp=szTemp+strlen(szTemp);

	SETRESIDENTVALUE("Updated");
	/*
	SETRESIDENTVALUE("LointS");
	SETRESIDENTVALUE("IP");
	SETRESIDENTVALUE("LastLointS");
	SETRESIDENTVALUE("LastLoginIP");
	SETRESIDENTVALUE("LointS");
	*/
}

void CNetwork::connectionError() {
	util_log("[%d:CNetwork] Connection Error",m_myqq);
#if 0
	if (!Packet::isClientKeySet()) {
		ShowNotification(TranslateT("Failed connecting to server"),NIIF_ERROR);
		QQ_SendBroadcast( NULL, ACKTYPE_LOGIN, ACKRESULT_FAILED, NULL, LOGINERR_NOSERVER );
	}
#endif
	//QQ_GoOffline();
	//disconnect();
	connectionClosed();
}

void CNetwork::connectionEstablished() {
	if (!m_notdisconnect) {
		SOCKET s=(SOCKET)CallService(MS_NETLIB_GETSOCKET,(WPARAM)m_connection,0);
		sockaddr_in sa;
		int namelen=sizeof(sa);
		getsockname(s,(sockaddr*)&sa,&namelen);

		m_myip = sa.sin_addr; // purple_network_get_my_ip(sip->fd);
		m_myport = htons(sa.sin_port); // purple_network_get_port_from_fd(sip->fd);

		do_register();
	}
}

void CNetwork::connectionClosed() {
	util_log("[%d:CNetwork] Connection Closed",m_myqq);
	if (!m_notdisconnect) 
		GoOffline();
	else {
		connect();
	}
}

void CNetwork::waitTimedOut() {
	fetion_keep_alive();
}

int CNetwork::dataReceived(NETLIBPACKETRECVER* nlpr) {
	util_log(__FUNCTION__);

	LPBYTE lpBuffer=nlpr->buffer;
	LPBYTE cur;
	LPSTR dummy;
	struct sipmsg *msg;
	int restlen;

	/* according to the RFC remove CRLF at the beginning */
	while(*lpBuffer == '\r' || *lpBuffer == '\n')
	{
		lpBuffer++;
		nlpr->bytesUsed++;
	}

	do
	{

		/* Received a full Header? */
		if((cur = (LPBYTE)strstr((LPSTR)lpBuffer, "\r\n\r\n")) != NULL)
		{
			time_t currtime = time(NULL);
			cur += 2;
			cur[0] = '\0';
			//util_log("\n\nreceived - %s\n######\n%s\n#######\n\n", ctime(&currtime), nlpr->buffer);
			msg = sipmsg_parse_header((LPSTR)lpBuffer);
			cur[0] = '\r';
			cur += 2;
			restlen = nlpr->bytesAvailable- nlpr->bytesUsed - (cur - lpBuffer);
			if(restlen >= msg->bodylen)
			{
				dummy = (LPSTR)mir_alloc(msg->bodylen + 1);
				memcpy(dummy, cur, msg->bodylen);
				dummy[msg->bodylen] = '\0';
				msg->body = dummy;
				cur += msg->bodylen;
				nlpr->bytesUsed+=cur-lpBuffer;
				lpBuffer+=cur-lpBuffer;
			} else {
				sipmsg_free(msg);
				return 0;
			}
			util_log("in process response response: %d", msg->response);
			m_curmsg=msg;
			process_input_message(msg);
			m_curmsg=NULL;
			sipmsg_free(msg);
		} 
		else {
			util_log("received a incomplete sip msg: %s", nlpr->buffer);
			break;
		}

	}while(nlpr->bytesAvailable-nlpr->bytesUsed!=0);

	return 0;
}

HANDLE __cdecl CNetwork::AddToList(int flags, PROTOSEARCHRESULT* psr) {
	if (flags & PALF_TEMPORARY) return NULL;
	if (int uid=atoi(psr->email+((*psr->email=='s' && psr->email[4]!='P')?4:6))) {
		HANDLE hContact=FindContact(uid);
		DBVARIANT dbv;
		ezxml_t root, buddy;
		LPSTR xml;
		// <args><contacts><buddies><buddy uri="x" buddy-lists="1" desc="mynick" /></buddies></contacts></args>
		if (!hContact) {
			hContact=AddContact(uid,true,true);
			int hGroup=util_group_name_exists(L"我的好友",0);
			if (hGroup!=-1) CallService(MS_CLIST_CONTACTCHANGEGROUP,(WPARAM)hContact,hGroup+1);
		} else {
			if (DBGetContactSettingByte(hContact,"CList","NotOnList",0)==1) {
				ShowNotification(TranslateT("The contact/group is already in your contact list."),NIIF_ERROR);
				return NULL;
			}
		}
		DBGetContactSettingUTF8String(NULL,m_szModuleName,"Nick",&dbv);

		root=ezxml_new("args");

		if (psr->email[4]=='P') {
			// <args><group uri="sip:PG4843690@fetion.com.cn;p=4609" nickname="" desc="add me" /></args>

			WRITEC_B("IsGroup",1);
			/*
			buddy=ezxml_add_child(root,"group",0);
			ezxml_set_attr(buddy,"uri",psr->email);
			ezxml_set_attr(buddy,"nickname","");
			ezxml_set_attr(buddy,"desc","");
			xml=ezxml_toxml(root,false);
			send_sip_request("S","","","N: PGApplyGroup\r\n",xml,NULL,&CNetwork::PGApplyGroup_cb);
			*/
			WRITEC_S("uri",psr->email);
			ShowNotification(TranslateT("NOTE: Group join request will only be sent when authorization reason is enabled."),NIIF_INFO);
		} else {
			buddy=ezxml_add_child(ezxml_add_child(ezxml_add_child(root,"contacts",0),"buddies",0),"buddy",0);
			ezxml_set_attr(buddy,"uri",psr->email);
			ezxml_set_attr(buddy,"buddy-lists","1");
			ezxml_set_attr(buddy,"desc",dbv.pszVal);
			xml=ezxml_toxml(root,false);
			DBFreeVariant(&dbv);
			send_sip_request("S","","","N: AddBuddy\r\n",xml,NULL,&CNetwork::AddBuddy_cb);
			free(xml);
		}
		ezxml_free(root);
		return hContact;
	}
	return 0;
}

HANDLE __cdecl CNetwork::AddToListByEvent(int flags, int iContact, HANDLE hDbEvent) {
#if 0
	DBEVENTINFO dbei={sizeof(dbei)};
	char* nick;
	HANDLE hContact;
	int qqid;
	char* reason;

	if (m_iStatus==ID_STATUS_OFFLINE) return 0;

	if ((dbei.cbBlob=CallService(MS_DB_EVENT_GETBLOBSIZE, (WPARAM)hDbEvent, 0))==-1) {
		util_log("%s(): ERROR: Can't get blob size.",__FUNCTION__);
		return 0;
	}

	util_log("Blob size: %lu", dbei.cbBlob);
	dbei.pBlob=(PBYTE)_alloca(dbei.cbBlob);
	if (CallService(MS_DB_EVENT_GET, (WPARAM)hDbEvent, (LPARAM)&dbei)) {
		util_log("%s(): ERROR: Can't get event.",__FUNCTION__);
		return 0;
	}

	if (dbei.eventType != EVENTTYPE_AUTHREQUEST) {
		util_log("%s(): ERROR: Not authorization.",__FUNCTION__);
		return 0;
	}

	if (strcmp(dbei.szModule, m_szModuleName)) {
		util_log("%s(): ERROR: Not for MIMFetion.",__FUNCTION__);
		return 0;
	}

	//Adds a contact to the contact list given an auth, added or contacts event
	//wParam=MAKEWPARAM(flags,iContact)
	//lParam=(LPARAM)(HANDLE)hDbEvent
	//Returns a HANDLE to the new contact, or NULL on failure
	//hDbEvent must be either EVENTTYPE_AUTHREQ or EVENTTYPE_ADDED
	//flags are the same as for PS_ADDTOLIST.
	//iContact is only used for contacts events. It is the 0-based index of the
	//contact in the event to add. There is no way to add two or more contacts at
	//once, you should just do lots of calls.

	/* TYPE ADDED
	blob is: uin(DWORD), hcontact(HANDLE), nick(ASCIIZ), first(ASCIIZ), 
	last(ASCIIZ), email(ASCIIZ) 

	TYPE AUTH REQ
	blob is: uin(DWORD), hcontact(HANDLE), nick(ASCIIZ), first(ASCIIZ), 
	last(ASCIIZ), email(ASCIIZ), reason(ASCIIZ)
	*/
	//hContact = (HANDLE) ( dbei.pBlob + sizeof( DWORD ));

	qqid=*(int*)dbei.pBlob;
	hContact=*(HANDLE*)(dbei.pBlob+sizeof(DWORD));
	nick = (char*)( dbei.pBlob + sizeof( DWORD )*2 );
	{
		char* firstName = nick + lstrlenA(nick) + 1;
		char* lastName = firstName + lstrlenA(firstName) + 1;
		char* email = lastName + lstrlenA(lastName) + 1;
		reason = email + lstrlenA(email) + 1;

		util_log("buddy:%s first:%s last:%s e-mail:%s", nick,
			firstName, lastName, email);
		util_log("reason:%s ", reason);

		/* we need to send out a packet to request an add */
		if (email[4]=='P') {
			LPSTR pszReasonUTF8=mir_utf8encode(reason);
			// <args><group uri="sip:PG4843690@fetion.com.cn;p=4609" nickname="" desc="add me" /></args>
			ezxml_t root=ezxml_new("args");
			ezxml_t buddy=ezxml_add_child(root,"group",0);
			ezxml_set_attr(buddy,"uri",email);
			ezxml_set_attr(buddy,"nickname","");
			ezxml_set_attr(buddy,"desc",pszReasonUTF8);
			LPSTR xml=ezxml_toxml(root,false);
			send_sip_request("S","","","N: PGApplyGroup\r\n",xml,NULL,&CNetwork::PGApplyGroup_cb);
			mir_free(pszReasonUTF8);
			ezxml_free(root);
			free(xml);
			return hContact;
		}

	}
#endif
	return 0;
}

int __cdecl CNetwork::Authorize(HANDLE hContact) {
	if (m_iStatus==ID_STATUS_OFFLINE) return 1;

	DBEVENTINFO dbei={sizeof(dbei)};

	if ((int)(dbei.cbBlob=CallService(MS_DB_EVENT_GETBLOBSIZE, (WPARAM)hContact, 0))==-1) return 1;

	dbei.pBlob=(PBYTE)alloca(dbei.cbBlob);
	if (CallService(MS_DB_EVENT_GET, (WPARAM)hContact, (LPARAM)&dbei)) return 1;
	if (dbei.eventType!=EVENTTYPE_AUTHREQUEST) return 1;
	if (strcmp(dbei.szModule, m_szModuleName)) return 1;

	LPSTR nick=(LPSTR)(dbei.pBlob+sizeof(DWORD)*2);
	LPSTR uri=nick+strlen(nick)+3;
	char szBody[MAX_PATH];
	sprintf(szBody,"<args><contacts><buddies><buddy uri=\"%s\" result=\"1\" buddy-lists=\"1\" expose-mobile-no=\"1\" expose-name=\"1\" /></buddies></contacts></args>",uri);
	send_sip_request("S","","","N: HandleContactRequest\r\n",szBody,NULL,NULL);
	/*
	char* firstName = nick + strlen( nick ) + 1;
	char* lastName = firstName + strlen( firstName ) + 1;
	char* email = lastName + strlen( lastName ) + 1;
	*/

	//<args><contacts><buddies><buddy uri="sip:726036475@fetion.com.cn;p=4609" result="1" buddy-lists="1" expose-mobile-no="1" expose-name="1" /></buddies></contacts></args>
	return 0;
}

int __cdecl CNetwork::AuthRecv(HANDLE hContact, PROTORECVEVENT* pre) {
	DBEVENTINFO dbei = { 0 };

	dbei.cbSize = sizeof(dbei);
	dbei.szModule = m_szProtoName;
	dbei.timestamp = pre->timestamp;
	dbei.flags = pre->flags & ( PREF_CREATEREAD ? DBEF_READ : 0 );
	dbei.eventType = EVENTTYPE_AUTHREQUEST;

	/* Just copy the Blob from PSR_AUTH event. */
	dbei.cbBlob = pre->lParam;
	dbei.pBlob = (PBYTE)pre->szMessage;
	CallService(MS_DB_EVENT_ADD, 0,(LPARAM)&dbei);

	return 0;
}

int __cdecl CNetwork::AuthRequest(HANDLE hContact, const char* szMessage) {
	DBVARIANT dbv;
	bool ret=false;

	if (!READC_S2("uri",&dbv)) {
		if (dbv.pszVal[4]=='P') {
			LPSTR pszReasonUTF8=mir_utf8encodecp(szMessage,CP_ACP);
			// <args><group uri="sip:PG4843690@fetion.com.cn;p=4609" nickname="" desc="add me" /></args>
			ezxml_t root=ezxml_new("args");
			ezxml_t buddy=ezxml_add_child(root,"group",0);
			ezxml_set_attr(buddy,"uri",dbv.pszVal);
			ezxml_set_attr(buddy,"nickname","");
			ezxml_set_attr(buddy,"desc",pszReasonUTF8);
			LPSTR xml=ezxml_toxml(root,false);
			send_sip_request("S","","","N: PGApplyGroup\r\n",xml,NULL,&CNetwork::PGApplyGroup_cb);
			mir_free(pszReasonUTF8);
			ezxml_free(root);
			free(xml);
			ret=true;
		}
		DBFreeVariant(&dbv);
	}

	return ret?0:1;
}

int __cdecl CNetwork::AuthDeny(HANDLE hContact, const char* szReason) {
	if (m_iStatus==ID_STATUS_OFFLINE) return 1;

	DBEVENTINFO dbei={sizeof(dbei)};

	if ((int)(dbei.cbBlob=CallService(MS_DB_EVENT_GETBLOBSIZE, (WPARAM)hContact, 0))==-1) return 1;

	dbei.pBlob=(PBYTE)alloca(dbei.cbBlob);
	if (CallService(MS_DB_EVENT_GET, (WPARAM)hContact, (LPARAM)&dbei)) return 1;
	if (dbei.eventType!=EVENTTYPE_AUTHREQUEST) return 1;
	if (strcmp(dbei.szModule, m_szModuleName)) return 1;

	LPSTR nick=(LPSTR)(dbei.pBlob+sizeof(DWORD)*2);
	char szBody[MAX_PATH];
	LPSTR uri=nick+strlen(nick)+3;
	sprintf(szBody,"<args><contacts><buddies><buddy uri=\"%s\" result=\"0\" /></buddies></contacts></args>",uri);
	send_sip_request("S","","","N: HandleContactRequest\r\n",szBody,NULL,NULL);
	return 0;
}

HANDLE __cdecl CNetwork::ChangeInfo(int iInfoType, void* pInfoData) {
	return NULL;
}

int __cdecl CNetwork::FileAllow(HANDLE hContact, HANDLE hTransfer, const char* szPath) {
	return 1;
}

int __cdecl CNetwork::FileCancel(HANDLE hContact, HANDLE hTransfer) {
	return 1;
}

int __cdecl CNetwork::FileDeny(HANDLE hContact, HANDLE hTransfer, const char* szReason) {
	return 1;
}

int __cdecl CNetwork::FileResume(HANDLE hTransfer, int* action, const char** szFilename) {
	return 1;
}

int __cdecl CNetwork::RecvFile(HANDLE hContact, PROTORECVFILE*) {
	return 1;
}

int __cdecl CNetwork::SendFile(HANDLE hContact, const char* szDescription, char** ppszFiles) {
	return 1;
}

DWORD __cdecl CNetwork::GetCaps(int type, HANDLE hContact) {
	switch (type) {
		case PFLAGNUM_1: // Protocol Capabilities
			return PF1_IM | PF1_SERVERCLIST | PF1_ADDED | PF1_BASICSEARCH | PF1_ADDSEARCHRES | PF1_AUTHREQ | PF1_CHAT | PF1_BASICSEARCH | PF1_NUMERICUSERID | PF1_MODEMSG;
			break;
		case PFLAGNUM_2: // Possible Status
			return PF2_ONLINE | PF2_INVISIBLE | PF2_LONGAWAY | PF2_LIGHTDND; // PF2_SHORTAWAY=Away
			break;
		case PFLAGNUM_3:
			return PF2_ONLINE | PF2_INVISIBLE | PF2_SHORTAWAY; // Status that supports mode message
			break;
		case PFLAGNUM_4: // Additional Capabilities
			return PF4_FORCEADDED|PF4_IMSENDUTF|PF4_AVATARS; // PF4_FORCEADDED="Send you were added" checkbox becomes uncheckable
			break;
		case PFLAG_UNIQUEIDTEXT: // Description for unique ID (For search use)
			return (int)Translate("Fetion ID");
			break;
		case PFLAG_UNIQUEIDSETTING: // Where is my Unique ID stored in?
			return (int)UNIQUEIDSETTING;
		case PFLAG_MAXLENOFMESSAGE: // Maximum message length
			return 0;
			break;
		case 10000: // MIMQQ: IPCSupport
			return 1;
	}
	return 0;
}

HICON __cdecl CNetwork::GetIcon(int iconIndex) {
	return (iconIndex & 0xFFFF)==PLI_PROTOCOL?(HICON)LoadImage(g_hInstance, MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, GetSystemMetrics(iconIndex&PLIF_SMALL?SM_CXSMICON:SM_CXICON), GetSystemMetrics(iconIndex&PLIF_SMALL?SM_CYSMICON:SM_CYICON), 0):0;
	return 0;
}

int __cdecl CNetwork::GetInfo(HANDLE hContact, int /*infoType*/) {
	DBVARIANT dbv;

	if (m_iStatus==ID_STATUS_OFFLINE) return 1;

	if (!READC_S2("uri",&dbv)) {
		if (dbv.pszVal[4]!='P' && hContact!=NULL/* !READC_B2("IsMe")*/) {
			GetBuddyInfo(dbv.pszVal);
			DBFreeVariant(&dbv);
			return 0;
		}
		DBFreeVariant(&dbv);
	}
	return 1;
}

void __cdecl CNetwork::SearchBasicDeferred(LPSTR pszID) {
	char szBody[MAX_PATH];
	if (*pszID=='s') {
		DBVARIANT dbv;
		HANDLE hContact=NULL;
		READC_S2("get-uri",&dbv);
		CHAR szUrl[MAX_PATH];
		CHAR szSSIC[MAX_PATH]="ssic=";
		CHAR szPost[MAX_PATH]="Sid=";
		NETLIBHTTPHEADER nlhh[]={
			{"User-Agent",FETION_USER_AGENT},
			{"Content-Type","application/x-www-form-urlencoded; charset=utf-8"},
			{"Cookie",szSSIC}
		};
		NETLIBHTTPREQUEST nlhr={
			sizeof(nlhr),
			REQUEST_POST,
			NLHRF_GENERATEHOST|NLHRF_REMOVEHOST|NLHRF_HTTP11,
			szUrl,
			nlhh,
			3,
			szPost,
			0
		};
		strcat(szSSIC,m_ssic);
		strcpy(szUrl,dbv.pszVal);
		DBFreeVariant(&dbv);
		strcat(szPost,pszID+4);
		nlhr.dataLength=strlen(szPost);

		if (NETLIBHTTPREQUEST* nlhrr=(NETLIBHTTPREQUEST*)CallService(MS_NETLIB_HTTPTRANSACTION,(WPARAM)g_hNetlibUser,(LPARAM)&nlhr)) {
			if (nlhrr->resultCode==200) {
				if (ezxml_t f1=ezxml_parse_str(nlhrr->pData,nlhrr->dataLength)) {
					if (ezxml_t f2=ezxml_child(f1,"uri")) {
						PROTOSEARCHRESULT psr={sizeof(psr)};
						//psr.nick=(LPSTR)ezxml_attr(f2,"value");
						psr.email=(LPSTR)ezxml_attr(f2,"value");
						psr.nick=mir_strdup(psr.email+4);
						*strrchr(psr.nick,';')=0;
						ProtoBroadcastAck(m_szModuleName, NULL, ACKTYPE_SEARCH, ACKRESULT_DATA, (HANDLE) 1, (LPARAM)&psr);
						mir_free(psr.nick);
					}
					ezxml_free(f1);
				}
			}
			CallService(MS_NETLIB_FREEHTTPREQUESTSTRUCT,(WPARAM)nlhrr,0);			
		}

		//ProtoBroadcastAck(m_szModuleName, NULL, ACKTYPE_SEARCH, ACKRESULT_SUCCESS, (HANDLE) 1, (LPARAM)0);
		sprintf(szBody,"<args><group sid=\"%s\" /><result-fields attributes=\"all\" /></args>",pszID+4);
		send_sip_request("S","","","N: PGSearchGroup\r\n",szBody,NULL,&CNetwork::SearchBasicDeferred_cb2);
	} else {
		sprintf(szBody,"<args><contacts attributes=\"provisioning;impresa;mobile-no;nickname;name;portrait-crc;ivr-enabled\" extended-attributes=\"score-level\"><contact uri=\"%s\" /></contacts></args>",pszID);
		send_sip_request("S","","","N: GetContactsInfo\r\n",szBody,NULL,&CNetwork::SearchBasicDeferred_cb);
	}
	mir_free(pszID);
}

bool __cdecl CNetwork::SearchBasicDeferred_cb(sipmsg *msg, transaction *tc) {
	if (msg->response==200) {
		if (ezxml_t f1=ezxml_parse_str(msg->body,msg->bodylen)) {
			if (ezxml_t f2=ezxml_get(f1,"contacts",0,"contact",-1)) {
				PROTOSEARCHRESULT psr={sizeof(psr)};
				//psr.nick=(LPSTR)ezxml_attr(f2,"uri");
				psr.email=(LPSTR)ezxml_attr(f2,"uri");
				psr.nick=mir_strdup(psr.email+4);
				ProtoBroadcastAck(m_szModuleName, NULL, ACKTYPE_SEARCH, ACKRESULT_DATA, (HANDLE) 1, (LPARAM)&psr);
				mir_free(psr.nick);
			}
			ezxml_free(f1);
		}
	}

	ProtoBroadcastAck(m_szModuleName, NULL, ACKTYPE_SEARCH, ACKRESULT_SUCCESS, (HANDLE) 1, (LPARAM)0);
	return true;
}

bool __cdecl CNetwork::SearchBasicDeferred_cb2(sipmsg *msg, transaction *tc) {
	if (msg->response==200) {
		if (ezxml_t f1=ezxml_parse_str(msg->body,msg->bodylen)) {
			if (ezxml_t f2=ezxml_child(f1,"groups")) {
				LPCSTR pszCount=ezxml_attr(f2,"group-count");
				if (pszCount && !strcmp(pszCount,"1")) {
					ezxml_t f3=ezxml_child(f2,"group");
					PROTOSEARCHRESULT psr={sizeof(psr)};
					psr.email=(LPSTR)ezxml_attr(f3,"uri");
					psr.nick=mir_utf8decodecp(mir_strdup((LPSTR)ezxml_attr(f3,"name")),CP_ACP,NULL);
					psr.lastName=(LPSTR)ezxml_attr(f3,"category");
					psr.firstName=mir_utf8decodecp(mir_strdup((LPSTR)ezxml_attr(f3,"introduce")),CP_ACP,NULL);
					ProtoBroadcastAck(m_szModuleName, NULL, ACKTYPE_SEARCH, ACKRESULT_DATA, (HANDLE) 1, (LPARAM)&psr);
					mir_free(psr.nick);
					mir_free(psr.firstName);
				}
			}
			ezxml_free(f1);
		}
	}

	ProtoBroadcastAck(m_szModuleName, NULL, ACKTYPE_SEARCH, ACKRESULT_SUCCESS, (HANDLE) 1, (LPARAM)0);
	return true;
}

HANDLE __cdecl CNetwork::SearchBasic(const char* id) {
	CHAR szNick[16];

	if (m_iStatus==ID_STATUS_OFFLINE) return 0;

	if (strlen(id)==11 && IsCMccNo(id)) {
		sprintf(szNick,"tel:%s",id);
	} else if (strlen(id)<11) {
		sprintf(szNick,"sip:%s",id);
	} else {
		ShowNotification(TranslateT("You entered an invalid search criteria. ID must be either Fetion ID or CMCC Mobile Phone Number."),NIIF_ERROR);
		return 0;
	}

	ForkThread((ThreadFunc)&CNetwork::SearchBasicDeferred,mir_strdup(szNick));

	return (HANDLE)1;
}

HANDLE __cdecl CNetwork::SearchByEmail(const char* email) {
	return 0;
}

HANDLE __cdecl CNetwork::SearchByName(const char* nick, const char* firstName, const char* lastName) {
	return 0;
}

HWND __cdecl CNetwork::SearchAdvanced(HWND owner) {
	return 0;
}

HWND __cdecl CNetwork::CreateExtendedSearchUI(HWND owner) {
	return 0;
}

int __cdecl CNetwork::RecvContacts(HANDLE hContact, PROTORECVEVENT*) {
	return 1;
}

int __cdecl CNetwork::RecvMsg(HANDLE hContact, PROTORECVEVENT* evt) {
	CCSDATA ccs={hContact, PSR_MESSAGE, 0, ( LPARAM )evt};
	return CallService(MS_PROTO_RECVMSG, 0, ( LPARAM )&ccs);
}

int __cdecl CNetwork::RecvUrl(HANDLE hContact, PROTORECVEVENT*) {
	return 1;
}

int __cdecl CNetwork::SendContacts(HANDLE hContact, int flags, int nContacts, HANDLE* hContactsList) {
	return 1;
}

int __cdecl CNetwork::SendMsg(HANDLE hContact, int flags, const char* msg) {
	DBVARIANT dbv;
	if (!READC_S2("uri",&dbv)) {
		bool fSMS=false;
		LPCSTR pMsg=msg;

		if (*dbv.pszVal=='t' || READC_B2(SETTING_FORCESMS))
			fSMS=true;
		else if (!strncmp(msg,"!SMS ",5)) {
			fSMS=true;
			pMsg+=5;
		}

		fetion_send_message(dbv.pszVal, pMsg, NULL,fSMS);

		/*
		if (!strncmp(dbv.pszVal,"tel:",4)) {
			delayReport_t* dr=(delayReport_t*)mir_alloc(sizeof(delayReport_t));
			dr->hContact=hContact;
			dr->ackType=ACKTYPE_MESSAGE;
			dr->ackResult=ACKRESULT_SUCCESS;
			dr->aux=1;
			dr->aux2=NULL;
			ForkThread((ThreadFunc)&CNetwork::delayReport,dr);
		}
		*/
		DBFreeVariant(&dbv);
		return 1;
	} else {
		ForkThread(&CNetwork::ThreadMsgBox,mir_tstrdup(TranslateT("Error: This contact doesn't have a uri!")));
		delayReport_t* dr=(delayReport_t*)mir_alloc(sizeof(delayReport_t));
		dr->hContact=hContact;
		dr->ackType=ACKTYPE_MESSAGE;
		dr->ackResult=ACKRESULT_FAILED;
		dr->aux=NULL;
		dr->aux2=NULL;
		ForkThread((ThreadFunc)&CNetwork::delayReport,dr);
		return 0;
	}
	return 0;
}

int __cdecl CNetwork::SendUrl(HANDLE hContact, int flags, const char* url) {
	return 1;
}

int __cdecl CNetwork::SetApparentMode(HANDLE hContact, int mode) {
	return 1;
}

int __cdecl CNetwork::SetStatus(int iNewStatus) {
	m_iDesiredStatus=iNewStatus;
	util_log(__FUNCTION__ "(): %d=>%d", m_iStatus,iNewStatus);

	if (m_iStatus==ID_STATUS_OFFLINE) {
		if (m_iDesiredStatus!=ID_STATUS_OFFLINE) {
			fetion_login();
			//test();
		}
	} else {
		if (m_iDesiredStatus==ID_STATUS_OFFLINE) {
			disconnect();
		} else {
			char szBody[MAX_PATH];
			sprintf(szBody,"<args><presence><basic value=\"%d\" /></presence></args>",ConvertStatus(m_iDesiredStatus,false));

			send_sip_request("S", NULL, "", "N: SetPresence\r\n", szBody, NULL, (TransCallback)&CNetwork::SetPresence_cb);

		}
	}
	return 1;
}

int __cdecl CNetwork::GetAwayMsg(HANDLE hContact) {
	return 1;
}

int __cdecl CNetwork::RecvAwayMsg(HANDLE hContact, int mode, PROTORECVEVENT* evt) {
	return 1;
}

int __cdecl CNetwork::SendAwayMsg(HANDLE hContact, HANDLE hProcess, const char* msg) {
	return 1;
}

int __cdecl CNetwork::SetAwayMsg(int iStatus, const char* msg) {
	if (m_iStatus>=ID_STATUS_ONLINE) {
		LPSTR szU8=mir_utf8encode(msg);
		SetImpresa(szU8);
		mir_free(szU8);
	}
	return 0;
}

int __cdecl CNetwork::UserIsTyping(HANDLE hContact, int type) {
	return 1;
}

/////////////////////////////////////////////////////////////////////////////////////////
// OnEvent - maintain protocol events

int __cdecl CNetwork::OnEvent(PROTOEVENTTYPE eventType, WPARAM wParam, LPARAM lParam) {
	switch( eventType ) {
		case EV_PROTO_ONLOAD:  return OnModulesLoadedEx(0,0);
		case EV_PROTO_ONEXIT:  return OnPreShutdown(0,0);
		//case EV_PROTO_ONOPTIONS: return OnOptionsInit( wParam, lParam ); // This is only called from AcctMgr
		case EV_PROTO_ONRENAME:
			{	
				CLISTMENUITEM clmi={sizeof(clmi)};
				clmi.flags=CMIM_NAME|CMIF_TCHAR;
				clmi.ptszName=m_tszUserName;
				CallService(MS_CLIST_MODIFYMENUITEM, (WPARAM)m_hMenuRoot, (LPARAM)&clmi);
			}	
			//case EV_PROTO_ONREADYTOEXIT:
		default: return 1;
	}	
}

int __cdecl CNetwork::OnPreShutdown(WPARAM wParam, LPARAM lParam) {
	util_log(__FUNCTION__);
	return 0;
}

int __cdecl CNetwork::GetName(WPARAM wParam, LPARAM lParam) {
	char* pszResult=(char*)lParam;
	char* pszANSI=mir_u2a(m_tszUserName);
	mir_snprintf(pszResult,wParam,"%s",pszANSI);
	mir_free(pszANSI);

	return 0;
}

int __cdecl CNetwork::GetStatus(WPARAM wParam, LPARAM lParam) {
	return m_iStatus;
}

void __cdecl CNetwork::delayReport(LPVOID lpData) {
	delayReport_t* dr=(delayReport_t*)lpData;

	Sleep(1000);
	ProtoBroadcastAck(m_szModuleName, dr->hContact, dr->ackType, dr->ackResult, (HANDLE)dr->aux, (LPARAM)dr->aux2);
	if (dr->aux2 && (LPARAM)dr->aux2>1000) mir_free(dr->aux2);
	mir_free(dr);
}

void CNetwork::ReceivedConnection(HANDLE hNewConnection, DWORD dwRemoteIP, void *pExtra) {
	((CNetwork*)pExtra)->ResumeConnection(hNewConnection);
}

void CNetwork::ResumeConnection(HANDLE hNewConnection) {
	m_connection=hNewConnection;
	m_redirect=true;
	connect();
}

bool CNetwork::crashRecovery() {
	util_log("CNetwork Crash Recovery");
	if (m_curmsg) {
		FILE* fp=fopen("c:\\mimfetion.log","wb");
		fwrite(m_curmsg->body,m_curmsg->bodylen,1,fp);
		fclose(fp);
		ShowNotification(L"MIMFetion error packet dump has been written to c:\\mimfetion.log. Please sumbit for support.",NIIF_ERROR);
		mir_free(m_curmsg);
		m_curmsg=NULL;
	}
	return false;
}

int __cdecl CNetwork::OnContactDeleted(WPARAM wParam, LPARAM)
{
	LPCSTR szProto=(LPCSTR)CallService(MS_PROTO_GETCONTACTBASEPROTO, wParam, 0);
	HANDLE hContact=(HANDLE)wParam;
	DBVARIANT dbvUri;

	if (!szProto || strcmp(szProto, m_szModuleName)) return 0;
	if (m_iStatus==ID_STATUS_OFFLINE) return 1;
	if (DBGetContactSettingByte(hContact,"CList","NotOnList",0)==1) return 1;
	if (READC_S2("uri",&dbvUri)) return 1;

	char szTemp[MAX_PATH];
	if (dbvUri.pszVal[4]=='P') {
		sprintf(szTemp,"<args><group uri=\"%s\" /></args>",dbvUri.pszVal);
		send_sip_request("S","","","N: PGQuitGroup\r\n",szTemp,NULL,NULL);
	} else {
		sprintf(szTemp,"<args><contacts><buddies><buddy uri=\"%s\" /></buddies></contacts></args>",dbvUri.pszVal);
		send_sip_request("S","","","N: DeleteBuddy\r\n",szTemp,NULL,NULL);
	}
	DBFreeVariant(&dbvUri);
	return 0;
}

int  __cdecl CNetwork::OnPrebuildContactMenu(WPARAM wParam, LPARAM) {
	DWORD config=0;
	HANDLE hContact=(HANDLE)wParam;
	CLISTMENUITEM clmi={sizeof(clmi)};

	clmi.flags=CMIF_UNICODE|CMIM_FLAGS|CMIF_HIDDEN;

	if (!strcmp((LPSTR)CallService(MS_PROTO_GETCONTACTBASEPROTO,(WPARAM)hContact,0),m_szModuleName)) {
		/*
		CRMI2(CTXMENU_SMS,ForceSMS,TranslateT("Switch Send Mode to SMS"));
		*/
		if (!READC_B2("IsGroup") && !READC_B2("IsMe")) {
			clmi.flags&=~CMIF_HIDDEN;
			clmi.flags|=CMIM_NAME;

			if (READC_B2(SETTING_FORCESMS)==0)
				clmi.ptszName=TranslateT("Switch Send Mode to SMS");
			else
				clmi.ptszName=TranslateT("Switch Send Mode to Auto");
		}
	}
	CallService(MS_CLIST_MODIFYMENUITEM,(WPARAM)m_contextMenuItemList.front(),(LPARAM)&clmi);

	return 0;
}

int __cdecl CNetwork::ForceSMS(WPARAM wParam, LPARAM lParam) {
	HANDLE hContact=(HANDLE)wParam;
	bool newvalue=!(bool)READC_B2(SETTING_FORCESMS);
	WRITEC_B(SETTING_FORCESMS,newvalue);

	ShowNotification(wcsdup(newvalue?TranslateT("Send Mode for this contact changed to SMS."):TranslateT("Send Mode for this contact changed to Auto.")),NIIF_INFO);
	return 0;
}