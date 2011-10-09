#include "StdAfx.h"

HINSTANCE CProtocol::g_hInstance;
// CHttpServer* CProtocol::g_httpServer=NULL;

CProtocol::CProtocol(LPCSTR szModuleName, LPCTSTR szUserName):
m_webqq(NULL) {
	WCHAR wszTemp[MAX_PATH];
	NETLIBUSER nlu = {sizeof(nlu)};

	m_szModuleName=m_szProtoName=mir_strdup(szModuleName);
	m_tszUserName=mir_wstrdup(szUserName);

	// Register NetLib User
	swprintf(wszTemp,TranslateT("%S plugin connections"),m_szModuleName);
	nlu.ptszDescriptiveName=wszTemp;
	nlu.flags=NUF_UNICODE|NUF_INCOMING|NUF_OUTGOING|NUF_NOHTTPSOPTION|NUF_HTTPCONNS; // NUF_INCOMING for HttpServer

	nlu.szSettingsModule=m_szModuleName;
	m_hNetlibUser=(HANDLE)CallService(MS_NETLIB_REGISTERUSER, 0, ( LPARAM )&nlu);

}

CProtocol::~CProtocol() {
	/*
	while (m_services.size()) {
		DestroyServiceFunction(m_services.top());
		m_services.pop();
	}

	while (m_hooks.size()) {
		UnhookEvent(m_hooks.top());
		m_hooks.pop();
	}

	if (g_httpServer) {
		delete g_httpServer;
		g_httpServer=NULL;
	}
*/
	mir_free(m_szModuleName);
	mir_free(m_tszUserName);
}

DWORD_PTR CProtocol::GetCaps( int type, HANDLE hContact ){
	switch (type) {
		case PFLAGNUM_1:
			return PF1_IM | PF1_SERVERCLIST | PF1_CHAT/* | PF1_ADDED*/ | PF1_NUMERICUSERID | PF1_BASICSEARCH/* | PF1_SEARCHBYEMAIL | PF1_SEARCHBYNAME*/ | PF1_ADDSEARCHRES | PF1_AUTHREQ | PF1_MODEMSG /*| PF1_FILE | PF1_BASICSEARCH*/;
		case PFLAGNUM_2: // Possible Status
			return PF2_ONLINE | PF2_INVISIBLE | PF2_SHORTAWAY | PF2_LIGHTDND; // | PF2_LONGAWAY | PF2_LIGHTDND; // PF2_SHORTAWAY=Away
			break;
		case PFLAGNUM_3:
			return PF2_ONLINE | PF2_INVISIBLE | PF2_SHORTAWAY | PF2_LIGHTDND; // | PF2_LONGAWAY | PF2_LIGHTDND; // Status that supports mode message
			break;
		case PFLAGNUM_4: // Additional Capabilities
			return PF4_FORCEAUTH | PF4_FORCEADDED | PF4_NOCUSTOMAUTH | PF4_AVATARS | PF4_IMSENDUTF | PF4_IMSENDOFFLINE; // PF4_FORCEADDED="Send you were added" checkbox becomes uncheckable
			break;
		case PFLAG_UNIQUEIDTEXT: // Description for unique ID (For search use)
			return (int)Translate("Fetion ID");
		case PFLAG_UNIQUEIDSETTING: // Where is my Unique ID stored in?
			return (int)UNIQUEIDSETTING;
		case PFLAG_MAXLENOFMESSAGE: // Maximum message length
			return 0;
		default:
			return 0;
	}
}

HICON CProtocol::GetIcon( int iconIndex ){
	return (iconIndex & 0xFFFF)==PLI_PROTOCOL?(HICON)LoadImage(g_hInstance, MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, GetSystemMetrics(iconIndex&PLIF_SMALL?SM_CXSMICON:SM_CXICON), GetSystemMetrics(iconIndex&PLIF_SMALL?SM_CYSMICON:SM_CYICON), 0):0;
}

int CProtocol::OnEvent( PROTOEVENTTYPE iEventType, WPARAM wParam, LPARAM lParam ) {
	QLog(__FUNCTION__"(iEventType=%d, wParam=%p, lParam=%p)",iEventType,wParam,lParam);
	switch (iEventType) {
		case EV_PROTO_ONLOAD: OnProtoLoad(); break;
		case EV_PROTO_ONOPTIONS: QLog(__FUNCTION__"(EV_PROTO_ONOPTIONS) Stub!"); break;
		case EV_PROTO_ONMENU: QLog(__FUNCTION__"(EV_PROTO_ONMENU) Stub!"); break;
		case EV_PROTO_ONRENAME:
			if (m_hMenuRoot) {
				CLISTMENUITEM clmi={sizeof(clmi)};
				clmi.flags=CMIM_NAME|CMIF_UNICODE;
				clmi.ptszName=m_tszUserName;
				CallService(MS_CLIST_MODIFYMENUITEM, (WPARAM)m_hMenuRoot, (LPARAM)&clmi);
			}
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

void CProtocol::OnProtoLoad() {
	CHAR szTemp[MAX_PATH]={0};
	LPSTR pszTemp;
	CLISTMENUITEM mi={sizeof(mi)};
	int c2=0;

	//add as a known module in DB Editor ++
	CallService("DBEditorpp/RegisterSingleModule",(WPARAM)m_szModuleName, 0);	

	// m_enableBBCode=(ServiceExists(MS_IEVIEW_EVENT) && (DBGetContactSettingDword(NULL,"IEVIEW","GeneralFlags",0) & 1));

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
	mi.hIcon=LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_ICON1));
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
		CRMI(QQ_MENU_DOWNLOADGROUP,DownloadGroup,TranslateT("&Download Group"));
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
		CRMI2(QQ_CNXTMENU_FORCESMS,ForceSMS,L"SMS Mode: ???");
		// CRMI2(QQ_CNXTMENU_POSTIMAGE,PostImage,TranslateT("Post &Image"));
		/*
		CRMI2(QQ_CNXTMENU_FORCEREFRESH,QunSpace,TranslateT("Qun &Space"));

		InitUpdater();
		*/
	InitFontService();
	InitFoldersService();
	InitAssocManager();

#define DECL(a) extern int a(WPARAM wParam, LPARAM lParam)
#define CSF(a,b) strcpy(pszTemp, a); this->QCreateService(szTemp,&CProtocol::b)
/*
	CSF(PS_GETNAME,GetName);
	CSF(PS_GETSTATUS,GetStatus);
	CSF(PS_SETMYNICKNAME,SetMyNickname);
	*/
	CSF(PS_GETAVATARINFO,GetAvatarInfo);
	/*

	CSF(PS_SET_LISTENINGTO,SetCurrentMedia);
	CSF(PS_GET_LISTENINGTO,GetCurrentMedia);
	*/
	CSF(PS_CREATEACCMGRUI,CreateAccMgrUI);

	CSF(PS_GETAVATARCAPS,GetAvatarCaps);
	CSF(PS_GETMYAVATAR,GetMyAvatar);
	CSF(PS_SETMYAVATAR,SetMyAvatar);

	SetContactsOffline();

	//this->QHookEvent(ME_SYSTEM_MODULESLOADED, &CNetwork::OnModulesLoadedEx);
	//this->QHookEvent(ME_DB_CONTACT_DELETED, &CNetwork::OnContactDeleted);
	this->QHookEvent(ME_CLIST_PREBUILDCONTACTMENU, &CProtocol::OnPrebuildContactMenu);
	// this->QHookEvent(ME_USERINFO_INITIALISE, &CProtocol::OnDetailsInit);

	QLog(__FUNCTION__" Complete");
}

int CProtocol::SetStatus(int iNewStatus) {
	/*
	// Debug start
	BroadcastStatus(iNewStatus);
	return 0;
	// Debug end
	*/

	if (/*m_iDesiredStatus*/m_iStatus==iNewStatus) return 0;

	if (m_webqq) {
		m_iDesiredStatus=iNewStatus;

		if (iNewStatus==ID_STATUS_OFFLINE) {
  			m_webqq->Stop();
		} else {
			if (m_webqq->WebIM_SetPresence(MapStatus(iNewStatus))) BroadcastStatus(iNewStatus);
		}
	} else if (iNewStatus!=ID_STATUS_OFFLINE) {
		DBVARIANT dbv={0};
		DBVARIANT dbvID={0};
		HANDLE hContact=NULL;
		
		READC_S2("LoginID",&dbvID);

		if (!dbvID.pszVal || READC_S2(QQ_LOGIN_PASSWORD,&dbv)!=0) {
			MessageBox(NULL,TranslateT("You must enter both ID and password to continue."),m_tszUserName,MB_ICONERROR);
		} else {
			m_iDesiredStatus=iNewStatus;
			// if (m_iDesiredStatus==ID_STATUS_ONLINE && READC_B2(QQ_LOGIN_INVISIBLE)==1) m_iDesiredStatus=ID_STATUS_INVISIBLE;

			CallService(MS_DB_CRYPT_DECODESTRING, lstrlenA(dbv.pszVal) + 1, (LPARAM)dbv.pszVal);
			m_webqq=new CLibWebFetion(dbvID.pszVal,dbv.pszVal,this,&CProtocol::_CallbackHub, m_hNetlibUser);
			DBFreeVariant(&dbv);

			NETLIBUSERSETTINGS nlus={sizeof(nlus)};
			CallService(MS_NETLIB_GETUSERSETTINGS,(WPARAM)m_hNetlibUser,(LPARAM)&nlus);
			char szProxy[MAX_PATH];

			if (nlus.useProxy==1) {
				switch (nlus.proxyType) {
					case PROXYTYPE_HTTP:
						strcpy(szProxy,"http=");
						break;
					case PROXYTYPE_SOCKS4:
					case PROXYTYPE_SOCKS5:
						strcpy(szProxy,"socks=");
						break;
					default:
						MessageBox(NULL,TranslateT("MIMFetion2 Only supports HTTP or SOCKS proxy."),m_tszUserName,MB_ICONERROR);
						return 0;
				}

				sprintf(szProxy+strlen(szProxy),"%s:%d",nlus.szProxyServer,nlus.wProxyPort);
				m_webqq->SetProxy(szProxy,nlus.szProxyAuthUser,nlus.szProxyAuthPassword);

			}

			WCHAR szPath[MAX_PATH];
			FoldersGetCustomPathW(m_folders[FOLDER_AVATARS],szPath,MAX_PATH,L"QQ");
			CreateDirectoryW(szPath,NULL);

			sprintf(szProxy,"%S",szPath);
			m_webqq->SetBasePath(szProxy);

			BroadcastStatus(ID_STATUS_CONNECTING);
			if (READC_B2(QQ_LOGIN_INVISIBLE)) m_iDesiredStatus=ID_STATUS_INVISIBLE;

			m_webqq->SetInitialStatus(MapStatus(m_iDesiredStatus));
			m_webqq->Start();
		}
		if (dbvID.pszVal) DBFreeVariant(&dbvID);
	} else
		QLog("Protocol already offline. Nothing to do!");

	return 0; 
}

int CProtocol::RecvMsg(HANDLE hContact, PROTORECVEVENT* evt){
	CCSDATA ccs={hContact, PSR_MESSAGE, 0, ( LPARAM )evt};
	return CallService(MS_PROTO_RECVMSG, 0, ( LPARAM )&ccs);
}

void CProtocol::BroadcastMsgAck(LPVOID lpParameter) {
	LPHANDLE pdwParameters=(LPHANDLE)lpParameter;

	Sleep(500);
	ProtoBroadcastAck(m_szModuleName, pdwParameters[0], ACKTYPE_MESSAGE, ACKRESULT_SUCCESS, pdwParameters[1], 0);
	mir_free(lpParameter);
}

typedef struct {
	HANDLE hContact;
	int flags;
	DWORD seq;
	LPSTR msg;
} SENDMSG, *PSENDMSG, *LPSENDMSG;

void __cdecl CProtocol::SendMsgThread(LPVOID lpParameter) {
	LPSENDMSG lpSM=(LPSENDMSG)lpParameter;
	HANDLE hContact=lpSM->hContact;
	LPSTR msg=lpSM->msg;
	LPSTR msgsend;

	int flags=lpSM->flags;
	// DWORD seq=lpSM->seq;

	if (!(flags & PREF_UTF)) {
		// MessageBox(NULL,L"Error: Not UTF!",NULL,0);
		QLog("Warning: Message not in UTF-8");
		msg=mir_utf8encode(msg);
	}

	msgsend=msg;

	bool sms=false;

	if (READC_B2("IsMe")) {
		sms=true;
	} else {
		DBVARIANT dbv;
		if (READC_U8S2("Mobile",&dbv)==0 && *dbv.pszVal) {
			switch (READC_B2("ForceSMS")) {
				case 0: // auto
					sms=READC_W2("Status")==ID_STATUS_OFFLINE;
					break;
				case 1: // sms
					sms=true;
					break;
			}
			DBFreeVariant(&dbv);

			if (!strnicmp(msgsend,"!sms ",5)) {
				sms=true;
				msgsend+=5;
			}
		}
	}

	bool ret=m_webqq->WebIM_SendMsg(READC_D2(UNIQUEIDSETTING),msgsend,sms);
	ProtoBroadcastAck(m_szModuleName, hContact, ACKTYPE_MESSAGE, ret?ACKRESULT_SUCCESS:ACKRESULT_FAILED, (HANDLE)lpSM->seq, 0);

	if (!(flags & PREF_UTF)) {
		mir_free((void*)msg);
	}

#if 0
	bool isqun=READC_B2("IsQun");

	// DWORD seq=0;

	FontID fid = {sizeof(fid)};
	LOGFONTA font;
	strcpy(fid.name,isqun?"Qun Messaging Font":"Contact Messaging Font");
	strcpy(fid.group,m_szModuleName);
	CallService(MS_FONT_GET,(WPARAM)&fid,(LPARAM)&font);
	COLORREF color=DBGetContactSettingDword(NULL,m_szModuleName,isqun?"font2Col":"font1Col",0);
	LPSTR pszFont=*font.lfFaceName?mir_utf8encodecp(font.lfFaceName,GetACP()):mir_utf8encodeW(L"宋体");

	if (color!=0) color=(color&0xff)<<16|(color&0xff00)|(color>>16);
	
	int fontsize=(int)DBGetContactSettingByte(NULL,m_szModuleName,isqun?"font2Size":"font1Size",12);
	if (fontsize<9) fontsize=9;

	LPSTR pszMsg=NULL;
	if (READ_B2(NULL,"EnableCustomSignature")==1 && !READC_B2("Composed")) {
		// 【提示：血桜の涙(648096556)正在使用WebQQ：http://web.qq.com/?w】
		LPSTR pszSig=NULL;
		DBVARIANT dbv={0};
		BOOL fRet;
		
		if (!(fRet=READ_U8S2(NULL,"CustomSignature",&dbv)) && *dbv.pszVal!=0) {
			pszSig=mir_strdup(dbv.pszVal);
		} else {
			pszSig=mir_utf8encodeW(L"【提示：%s(%u)正在使用MIMQQ4：http://www.studiokuma.com/mimqq/mimqq4b.html】");
		}

		if (!fRet) DBFreeVariant(&dbv);

		if (!READ_U8S2(NULL,"Nick",&dbv)) {
			pszMsg=(LPSTR)mir_alloc(strlen(pszSig)+13+strlen(dbv.pszVal)+strlen(msg));
			strcpy(pszMsg,msg);
			if (*pszSig) {
				strcat(pszMsg,"\n");
				sprintf(pszMsg+strlen(pszMsg),pszSig,dbv.pszVal,m_webqq->GetQQID());
			}
			DBFreeVariant(&dbv);
		}
		mir_free(pszSig);
		WRITEC_B("Composed",1);
	}

	if (!pszMsg) pszMsg=mir_strdup(msg);

	if (m_webqq->GetUseWeb2()) {
		bool hasImage=strstr(msg,"/cgi-bin/webqq_app/?cmd=2&bd=")!=NULL;
		DWORD dwExtID=isqun?READC_D2("ExternalID"):0;

		JSONNODE* jnContent=Web2ConvertMessage(isqun,dwExtID,pszMsg,fontsize,pszFont,color,font.lfWeight>FW_NORMAL,font.lfItalic,font.lfUnderline);

		if (isqun)
			/*seq=*/dwRet=m_webqq->SendClassMessage(READC_D2(UNIQUEIDSETTING),dwExtID,hasImage,jnContent);
		else
			/*seq=*/dwRet=m_webqq->SendContactMessage(READC_D2(UNIQUEIDSETTING),READC_W2("Face"),hasImage,jnContent);

		json_delete(jnContent);
#if 0 // Web1
	} else {
		if (isqun) {

			EncodeSmileys(pszMsg);
			EncodeQunImages(hContact,pszMsg);

			/*seq*/dwRet=m_webqq->SendClassMessage(READC_D2(UNIQUEIDSETTING),pszMsg,fontsize,pszFont,color,font.lfWeight>FW_NORMAL,font.lfItalic,font.lfUnderline);
			// m_sentMessages[seq]=hContact;
			// return (int)seq;
			if (READC_B2(QQ_SILENTQUN)) {
				SilentQun((WPARAM)hContact,0);
			}
		} else {
			EncodeSmileys(pszMsg);
			EncodeP2PImages(pszMsg);

			/*seq*/dwRet=m_webqq->SendContactMessage(READC_D2(UNIQUEIDSETTING),pszMsg,fontsize,pszFont,color,font.lfWeight>FW_NORMAL,font.lfItalic,font.lfUnderline);
			// m_sentMessages[seq]=hContact;
			// return (int)seq;
		}
#endif // Web1
	}
	mir_free(pszMsg);

	mir_free(pszFont);

	if (!(flags & PREF_UTF)) {
		mir_free((void*)msg);
	}

	/*
	if (seq) {
		LPHANDLE pdwParameters=(LPHANDLE)mir_alloc(sizeof(HANDLE)*2);
		pdwParameters[0]=hContact;
		pdwParameters[1]=(HANDLE)seq;
		CreateThreadObj(&CProtocol::BroadcastMsgAck,pdwParameters);
		return seq;
	}*/
	
	ProtoBroadcastAck(m_szModuleName, hContact, ACKTYPE_MESSAGE, dwRet==0?ACKRESULT_FAILED:ACKRESULT_SUCCESS, (HANDLE)lpSM->seq, 0);
#endif
	mir_free(lpSM->msg);
	mir_free(lpSM);
}

int CProtocol::SendMsg(HANDLE hContact, int flags, const char* msg){
	if (m_webqq) {
		LPSENDMSG lpSM=(LPSENDMSG)mir_alloc(sizeof(SENDMSG));
		DWORD seq=GetTickCount();

		lpSM->hContact=hContact;
		lpSM->flags=flags;
		lpSM->msg=mir_strdup(msg);
		lpSM->seq=seq;
		CreateThreadObj(&CProtocol::SendMsgThread,lpSM);

		return seq;
	}
	return 0; 
}

int CProtocol::TestService(WPARAM wParam, LPARAM lParam) {
	// GetAllAvatars();
	QLog("GetTickCount=%u%03d",(DWORD)(DWORD)time(NULL),GetTickCount()%1000);
	/*
	JSONNODE* jn=json_new(JSON_NODE);
	JSONNODE* jnSub;
	json_push_back(jn,jnSub=json_new_a("test","value\xe4\xb8\xad\xe6\x98\x87\xe8\xa8\x8a\xe6\x81\xaf""answer"));
	char* pszJSON=json_write(jn);

	json_free(pszJSON);
	json_delete(jn);
	*/
	/*
	BroadcastStatus(ID_STATUS_ONLINE);
	m_webqq=new CLibWebFetion(431533706,"",this,_CallbackHub);
	m_webqq->Test();
	*/
	/*
	HANDLE hFile=CreateFileA("C:\\My Documents\\My Pictures\\2.JPG",GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,0,NULL);
	if (hFile==INVALID_HANDLE_VALUE)
		QLog("Failed opening test file.");
	else {
		// m_webqq->UploadQunImage(hFile,"2.JPG",85379868);
		m_webqq->Web2UploadP2PImage(hFile,"2.JPG",85379868);
		// hFile closed by above function
	}
	*/
	/*
	CallbackHub(WEBQQ_CALLBACK_WEB2_ERROR,"testcommand1","test response 1");
	CallbackHub(WEBQQ_CALLBACK_WEB2_ERROR,"test/testcommand2","test response 2");
	*/
	/*
	HANDLE hFile=CreateFileA("C:\\webqq\\v2\\testjson.txt",GENERIC_READ,FILE_SHARE_WRITE|FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);
	LPSTR pszTemp=(LPSTR)LocalAlloc(LMEM_FIXED,65536);
	DWORD dwRead;
	ReadFile(hFile,pszTemp,65536,&dwRead,NULL);

	JSONNODE* jn=json_parse(pszTemp);
	JSONNODE* jnResult=json_get(jn,"result");

	json_delete(jn);

	LocalFree(pszTemp);
	CloseHandle(hFile);
	*/
	/*
	JSONNODE* jn=json_new(JSON_NODE);//json_new_a("vfwebqq","3a111410084da65fa99c73d579803b94b7a8533a6e739e5641e7ddf66cc7c3e8e437c169e8b9728c");
	json_set_a(jn,"1234");
	char* txt=json_write(jn);
	json_free(txt);
	json_delete(jn);
	*/
	return 0;
}

void CProtocol::FetchAvatar(HANDLE hContact) {
	PROTO_AVATAR_INFORMATION pai={sizeof(pai),hContact,PA_FORMAT_JPEG};
	DBVARIANT dbv;
	DBVARIANT dbvCrc;

	dbv.pszVal=dbvCrc.pszVal=NULL;

	READC_U8S2("Crc",&dbvCrc);

	if (!READC_U8S2("PendingAvatar",&dbv)) {
		if (int format=m_webqq->FetchUserHead(READ_D2(NULL,UNIQUEIDSETTING),READC_D2(UNIQUEIDSETTING),dbvCrc.pszVal,dbv.pszVal)) {
			if (hContact) {
				strcpy(pai.filename,dbv.pszVal);
			} else {
				if (ServiceExists(MS_AV_SETMYAVATAR)) CallService(MS_AV_SETMYAVATAR,(WPARAM)m_szModuleName,(LPARAM)pai.filename);
				// DBFreeVariant(&dbv);
			}
			pai.format=format==1?PA_FORMAT_BMP:format==2?PA_FORMAT_GIF:PA_FORMAT_JPEG;
		} else {
			QLog(__FUNCTION__"(): UserHead fetch failed for %u!",READC_D2(UNIQUEIDSETTING));
		}
		DBFreeVariant(&dbv);
		DELC("PendingAvatar");
	} else
		QLog(__FUNCTION__"(): Should not happen - PendingAvatar not found!");

	DBFreeVariant(&dbvCrc);

	if (hContact) ProtoBroadcastAck(m_szModuleName, hContact, ACKTYPE_AVATAR, *pai.filename?ACKRESULT_SUCCESS:ACKRESULT_FAILED, (HANDLE)&pai, (LPARAM)0);
}

int CProtocol::GetAvatarInfo(WPARAM wParam, LPARAM lParam) {
	PROTO_AVATAR_INFORMATION* lpPAI = (PROTO_AVATAR_INFORMATION*)lParam;
	HANDLE hContact=lpPAI->hContact;
	DBVARIANT dbv;
	DWORD uid=READC_D2(UNIQUEIDSETTING);

	if (READC_U8S2("Crc",&dbv)==0) {
		if (!strcmp(dbv.pszVal,"0")) {
			// No avatar
			QLog(__FUNCTION__"(): No avatar for %u",uid);
			DBFreeVariant(&dbv);
			return GAIR_NOAVATAR;
		} else {
			WCHAR szPath[MAX_PATH];

			FoldersGetCustomPathW(m_folders[FOLDER_AVATARS],szPath,MAX_PATH,L"Fetion");
			sprintf(lpPAI->filename,"%S\\%s.jpg",szPath,dbv.pszVal);
			lpPAI->format=PA_FORMAT_JPEG;
			DBFreeVariant(&dbv);

			if (GetFileAttributesA(lpPAI->filename)!=INVALID_FILE_ATTRIBUTES) {
				QLog(__FUNCTION__"(): Avatar is current for %u",uid);
				if (hContact) ProtoBroadcastAck(m_szModuleName, hContact, ACKTYPE_AVATAR, ACKRESULT_SUCCESS, (HANDLE)lpPAI, (LPARAM)0);
				return GAIR_SUCCESS;
			} else if (m_webqq) {
				QLog(__FUNCTION__"(): Update avatar for %u",uid);
				WRITEC_U8S("PendingAvatar",lpPAI->filename);
				CreateThreadObj(&CProtocol::FetchAvatar,lpPAI->hContact);
				return GAIR_WAITFOR;
			} else {
				QLog(__FUNCTION__"(): Avatar needs update for %u but protocol not online!",uid);
				DBFreeVariant(&dbv);
				return GAIR_NOAVATAR;
			}
		}
	} else {
		// This should not happen, however just handle it :)
		return GAIR_NOAVATAR;
	}
}

void __cdecl CProtocol::GetAllAvatarsThread(LPVOID) {
	HANDLE hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, (WPARAM)NULL, (LPARAM)NULL);
	WCHAR szPath[MAX_PATH];
	char szPath2[MAX_PATH];
	LPSTR pszPath;
	BOOL fUHDownloadType=READ_B2(NULL,"UHDownload");

	FoldersGetCustomPathW(m_folders[FOLDER_AVATARS],szPath,MAX_PATH,L"Fetion");
	sprintf(szPath2,"%S\\",szPath);
	pszPath=szPath2+strlen(szPath2);

	while (hContact) {
		if (!lstrcmpA(m_szModuleName,(char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact,(LPARAM)NULL))) {

			sprintf(pszPath,"%u.jpg",READC_D2(UNIQUEIDSETTING));
			if (fUHDownloadType==1 || READC_W2("Status")!=ID_STATUS_OFFLINE/* || GetFileAttributesA(szPath2)==INVALID_FILE_ATTRIBUTES*/) {
				WRITEC_U8S("PendingAvatar",szPath2);
				FetchAvatar(hContact);
				Sleep(1000); // Prevent unknown lockup
			}
		}

		hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, (LPARAM)NULL);
	}

	hContact=NULL;
	sprintf(pszPath,"%u.jpg",READC_D2(UNIQUEIDSETTING));
	WRITEC_U8S("PendingAvatar",szPath2);
	FetchAvatar(hContact);
}

void CProtocol::GetAllAvatars() {
	CreateThreadObj(&CProtocol::GetAllAvatarsThread,NULL);
}

int __cdecl CProtocol::GetAvatarCaps(WPARAM wParam, LPARAM lParam) {
	switch (wParam) {
		case AF_MAXSIZE:
			((LPPOINT)lParam)->x=64;
			((LPPOINT)lParam)->y=64;
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

int __cdecl CProtocol::SetMyAvatar(WPARAM wParam, LPARAM lParam) {
	/*
	wParam=0
	lParam=(const char *)Avatar file name or NULL to remove the avatar
	return=0 for sucess
	*/
	return 1; // Not supported yet
}

int __cdecl CProtocol::GetMyAvatar(WPARAM wParam, LPARAM lParam) {
	HANDLE hContact=NULL;
	WCHAR szPath[MAX_PATH];
	FoldersGetCustomPathW(m_folders[FOLDER_AVATARS],szPath,MAX_PATH,L"QQ");

	mir_snprintf((LPSTR)wParam,lParam,"%S\\%u.jpg",szPath,READC_D2(UNIQUEIDSETTING));
	return 0;
}

int __cdecl CProtocol::OnPrebuildContactMenu(WPARAM wParam, LPARAM lParam) {
	DWORD config=0;
	HANDLE hContact=(HANDLE)wParam;
	CLISTMENUITEM clmi={sizeof(clmi)};

	if (CallService(MS_PROTO_ISPROTOONCONTACT,(WPARAM)hContact,(LPARAM)m_szModuleName)==-1) {
		/*
		CRMI2(QQ_CNXTMENU_SILENTQUN,SilentQun,TranslateT("Toggle &Silent"));
		CRMI2(QQ_CNXTMENU_POSTIMAGE,PostImage,TranslateT("Post &Image"));
		*/

		DBVARIANT dbv;
		dbv.pszVal=NULL;

		if (m_iStatus>=ID_STATUS_ONLINE && READC_U8S2("Mobile",&dbv)==0 && *dbv.pszVal) {
			BYTE val=READC_B2("ForceSMS");
			config=1;

			clmi.flags=CMIM_FLAGS|CMIF_UNICODE|CMIM_NAME;
			clmi.ptszName=(val==0?TranslateT("Send Mode: Auto"):val==1?TranslateT("Send Mode: Force SMS"):TranslateT("Send Mode: Force IM"));
			CallService(MS_CLIST_MODIFYMENUITEM,(WPARAM)m_contextMenuItems.at(0),(LPARAM)&clmi);
		} else {
			config=0;
		}

		if (dbv.pszVal) DBFreeVariant(&dbv);

	}

	int c=0;

	for (vector<HANDLE>::iterator iter=m_contextMenuItems.begin(); iter!=m_contextMenuItems.end(); iter++, c++) {
		clmi.flags=CMIF_UNICODE|((config & (1<<c))?CMIM_FLAGS:CMIM_FLAGS|CMIF_HIDDEN);
		//if (c==2) clmi.flags+=CMIM_NAME;
		CallService(MS_CLIST_MODIFYMENUITEM,(WPARAM)*iter,(LPARAM)&clmi);
	}

	return 0;
}

int __cdecl CProtocol::DownloadGroup(WPARAM wParam, LPARAM lParam) {
	if (m_webqq) {
		WRITE_B(NULL,"GroupFetched",0);
		m_webqq->WebIM_GetContactList();
	}
	return 0;
}

int __cdecl CProtocol::ForceSMS(WPARAM wParam, LPARAM lParam) {
	HANDLE hContact=(HANDLE)wParam;
	WRITEC_B("ForceSMS",(READC_B2("ForceSMS")+1)%3);
	return 0;
}

void __cdecl CProtocol::SearchBasicThread(LPVOID lpParameter) {
	DWORD uin=strtoul((LPSTR)lpParameter,NULL,10);

	PROTOSEARCHRESULT psr={sizeof(psr)};
	psr.id=(LPWSTR)lpParameter;
	// psr.flags=PSR_UNICODE;

	Sleep(500);
	ProtoBroadcastAck(m_szModuleName, NULL, ACKTYPE_SEARCH, ACKRESULT_DATA, (HANDLE)1, (LPARAM)&psr);

	Sleep(500);
	ProtoBroadcastAck(m_szModuleName, NULL, ACKTYPE_SEARCH, ACKRESULT_SUCCESS, (HANDLE)1, 0);
	mir_free(lpParameter);
	return;
}

HANDLE CProtocol::SearchBasic(const PROTOCHAR* id) {
	if (/*true || */m_webqq) {
		LPCSTR pszID=(LPCSTR)id;
		CreateThreadObj(&CProtocol::SearchBasicThread,mir_strdup(pszID));
		return (HANDLE)1;
	} else
		return 0;
}

HANDLE CProtocol::AddToList(int flags, PROTOSEARCHRESULT* psr){
	QLog(__FUNCTION__"()");

	if (m_webqq) {
		LPSTR pszID=mir_u2a(psr->id);
		DBVARIANT dbv;
		HANDLE hContact=NULL;
		LPWSTR pszReply=NULL;
		dbv.pszVal=NULL;
		
		READC_U8S2("Nick",&dbv);
		
		switch (m_webqq->WebIM_AddBuddy(pszID,dbv.pszVal?dbv.pszVal:"???")) {
			case 200:
				// pszReply=TranslateT("Your request has been sent.");
				break;
			case 312:
				pszReply=TranslateT("Incorrect verification code!");
				break;
			case 404:
				pszReply=TranslateT("Specified ID does not exist!");
				break;
			case 512:
				pszReply=TranslateT("Specified ID already exists in your contact list!");
				break;
			default:
				pszReply=TranslateT("Unknown reply from server!");
				break;
		}

		ShowNotification(pszReply?pszReply:TranslateT("Your request has been sent."),pszReply?NIIF_ERROR:NIIF_INFO);
		mir_free(pszID);
		if (dbv.pszVal) DBFreeVariant(&dbv);
		return (HANDLE)1;
	}
	return 0; // No hContact returned
}

int CProtocol::Authorize(HANDLE hDbEvent){
	return AuthDeny(hDbEvent,NULL);
}

int CProtocol::AuthDeny(HANDLE hDbEvent, const PROTOCHAR* szReason){
	DBEVENTINFO dbei={sizeof(dbei)};
	unsigned int* uid;
	HANDLE hContact;
	LPSTR pszBlob;

	if ((dbei.cbBlob=CallService(MS_DB_EVENT_GETBLOBSIZE, (WPARAM)hDbEvent, 0))==-1) return 1;

	pszBlob=(LPSTR)(dbei.pBlob=(PBYTE)alloca(dbei.cbBlob));
	if (CallService(MS_DB_EVENT_GET, (WPARAM)hDbEvent, (LPARAM)&dbei)) return 1;
	if (dbei.eventType!=EVENTTYPE_AUTHREQUEST) return 1;
	if (strcmp(dbei.szModule, m_szModuleName)) return 1;

	uid=(unsigned int*) dbei.pBlob;

	uid=(unsigned int*)dbei.pBlob;pszBlob+=sizeof(DWORD);
	hContact=*(HANDLE*)(dbei.pBlob+sizeof(DWORD));pszBlob+=sizeof(HANDLE);

	if (m_webqq) {
		DBVARIANT dbv;
		dbv.pszVal=NULL;
		DBGetContactSettingUTF8String(hContact,"CList","MyHandle",&dbv);
		m_webqq->WebIM_HandleAddBuddy(*uid,szReason==NULL,dbv.pszVal?dbv.pszVal:"",0);
		if (dbv.pszVal) DBFreeVariant(&dbv);
		return 0;
	}
	return 1;
}

int CProtocol::AuthRecv(HANDLE hContact, PROTORECVEVENT* pre){
	if (pre) {
		DBEVENTINFO dbei;

		QLog(__FUNCTION__ "(): Received authorization request");
		// Show that guy
		DBDeleteContactSetting(hContact,"CList","Hidden");

		ZeroMemory(&dbei,sizeof(dbei));
		dbei.cbSize=sizeof(dbei);
		dbei.szModule=m_szModuleName;
		dbei.timestamp=pre->timestamp;
		dbei.flags=pre->flags & (PREF_CREATEREAD?DBEF_READ:0);
		dbei.eventType=EVENTTYPE_AUTHREQUEST;
		dbei.cbBlob=pre->lParam;
		dbei.pBlob=(PBYTE)pre->szMessage;
		CallService(MS_DB_EVENT_ADD,(WPARAM)NULL,(LPARAM)&dbei);
	} else
		QLog(__FUNCTION__ "(): ERROR: Received empty authorization request!");
	return 0;
}

int CProtocol::GetInfo(HANDLE hContact, int infoType){
	/*
	if (m_webqq) {
		CreateThreadObj(&CProtocol::GetInfoThread,(LPVOID)hContact);
	}

	return 0;
	*/
	return 1;
}

#if 0
void __cdecl CProtocol::GetInfoThread(LPVOID lpParameter) {
	HANDLE hContact=(HANDLE)lpParameter;

	if (READC_B2("IsQun")==0) {
		DWORD dwUIN=READC_D2(UNIQUEIDSETTING);
		m_webqq->web2_api_get_qq_level(dwUIN);
		m_webqq->web2_api_get_friend_info(dwUIN);
	}
	// TODO: Qun Info
}
#endif

// Stubs
int CProtocol::AuthRequest(HANDLE hContact, const PROTOCHAR* szMessage){ return 1; }
HANDLE CProtocol::AddToListByEvent(int flags, int iContact, HANDLE hDbEvent){ QLog(__FUNCTION__"()"); return 0; }
HANDLE CProtocol::SearchByName(const PROTOCHAR* nick, const PROTOCHAR* firstName, const PROTOCHAR* lastName) { return 0; }

HANDLE   CProtocol::ChangeInfo( int iInfoType, void* pInfoData ){ return 0; }

HANDLE CProtocol::SendFile(HANDLE hContact, const PROTOCHAR* szDescription, PROTOCHAR** ppszFiles) { return 0; }
HANDLE   CProtocol::FileAllow( HANDLE hContact, HANDLE hTransfer, const PROTOCHAR* szPath ){ return 0; }
int      CProtocol::FileCancel( HANDLE hContact, HANDLE hTransfer ){ return 0; }
int      CProtocol::FileDeny( HANDLE hContact, HANDLE hTransfer, const PROTOCHAR* szReason ){ return 0; }
int      CProtocol::FileResume( HANDLE hTransfer, int* action, const PROTOCHAR** szFilename ){ return 0; }

HANDLE    CProtocol::SearchByEmail( const PROTOCHAR* email ){ return 0; }
HWND      CProtocol::SearchAdvanced( HWND owner ){ return 0; }
HWND      CProtocol::CreateExtendedSearchUI( HWND owner ){ return 0; }

int       CProtocol::RecvContacts( HANDLE hContact, PROTORECVEVENT* ){ return 0; }
int       CProtocol::RecvFile( HANDLE hContact, PROTOFILEEVENT* ){ return 0; }
int       CProtocol::RecvUrl( HANDLE hContact, PROTORECVEVENT* ){ return 0; }

int       CProtocol::SendContacts( HANDLE hContact, int flags, int nContacts, HANDLE* hContactsList ){ return 0; }
int       CProtocol::SendUrl( HANDLE hContact, int flags, const char* url ){ return 0; }

int       CProtocol::SetApparentMode( HANDLE hContact, int mode ){ return 0; }

HANDLE    CProtocol::GetAwayMsg( HANDLE hContact ){ return 0; }
int       CProtocol::RecvAwayMsg( HANDLE hContact, int mode, PROTORECVEVENT* evt ){ return 0; }
int       CProtocol::SendAwayMsg( HANDLE hContact, HANDLE hProcess, const char* msg ){ return 0; }
int       CProtocol::SetAwayMsg( int iStatus, const PROTOCHAR* msg ){ return 0; }

int       CProtocol::UserIsTyping( HANDLE hContact, int type ){ return 0; }

