#include "StdAfx.h"

HINSTANCE CProtocol::g_hInstance;
CHttpServer* CProtocol::g_httpServer=NULL;

CProtocol::CProtocol(LPCSTR szModuleName, LPCTSTR szUserName):
m_webqq(NULL),
/*
m_qunsToInit(0),
m_qunsInited(0),
*/
m_msgseqCurrent(0) {
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

	memset(m_msgseq,0,sizeof(DWORD)*10);

	g_httpServer=CHttpServer::GetInstance(this);

	registerCallbacks();
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
*/
	if (g_httpServer) {
		delete g_httpServer;
		g_httpServer=NULL;
	}

	mir_free(m_szModuleName);
	mir_free(m_tszUserName);
}

DWORD_PTR CProtocol::GetCaps( int type, HANDLE hContact ){
	switch (type) {
		case PFLAGNUM_1:
			return PF1_IM | PF1_SERVERCLIST | PF1_CHAT/* | PF1_ADDED*/ | PF1_BASICSEARCH/* | PF1_SEARCHBYEMAIL*/ | PF1_SEARCHBYNAME | PF1_NUMERICUSERID | PF1_ADDSEARCHRES | PF1_AUTHREQ | PF1_MODEMSG | PF1_FILE /*| PF1_BASICSEARCH*/;
		case PFLAGNUM_2: // Possible Status
			return PF2_ONLINE | PF2_INVISIBLE | PF2_SHORTAWAY | PF2_LONGAWAY | PF2_LIGHTDND | PF2_HEAVYDND | PF2_FREECHAT; // | PF2_LONGAWAY | PF2_LIGHTDND; // PF2_SHORTAWAY=Away
			break;
		case PFLAGNUM_3:
			return PF2_ONLINE | PF2_INVISIBLE | PF2_SHORTAWAY | PF2_LONGAWAY | PF2_LIGHTDND | PF2_HEAVYDND | PF2_FREECHAT; // | PF2_LONGAWAY | PF2_LIGHTDND; // Status that supports mode message
			break;
		case PFLAGNUM_4: // Additional Capabilities
			return PF4_FORCEAUTH | PF4_FORCEADDED/* | PF4_NOCUSTOMAUTH*/ | PF4_AVATARS | PF4_IMSENDUTF | PF4_OFFLINEFILES | PF4_IMSENDOFFLINE; // PF4_FORCEADDED="Send you were added" checkbox becomes uncheckable
			break;
		case PFLAG_UNIQUEIDTEXT: // Description for unique ID (For search use)
			return (int)Translate("QQ ID");
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
			if (Miranda_Terminated()) {
				if (g_httpServer) {
					delete g_httpServer;
					g_httpServer=NULL;
				}
			}
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

	m_enableBBCode=(ServiceExists(MS_IEVIEW_EVENT) && (DBGetContactSettingDword(NULL,"IEVIEW","GeneralFlags",0) & 1));

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
		CRMI2(QQ_CNXTMENU_SILENTQUN,SilentQun,TranslateT("Toggle &Silent"));
		CRMI2(QQ_CNXTMENU_POSTIMAGE,PostImage,TranslateT("Post &Image"));
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

	if (m_iDesiredStatus==iNewStatus) return 0;

	if (m_webqq) {
		m_iDesiredStatus=iNewStatus;

		if (iNewStatus==ID_STATUS_OFFLINE) {
  			m_webqq->Stop();
		} else {
			if (m_webqq->GetUseWeb2())
				m_webqq->web2_channel_change_status(Web2StatusFromMIM(iNewStatus));
#if 0 // Web1
			else
				m_webqq->SetOnlineStatus((CLibWebQQ::WEBQQPROTOCOLSTATUSENUM)MapStatus(m_iDesiredStatus));
#endif // Web1
			BroadcastStatus(iNewStatus);
		}
	} else if (iNewStatus!=ID_STATUS_OFFLINE) {
		DBVARIANT dbv={0};
		HANDLE hContact=NULL;
		DWORD qqid=READC_D2(UNIQUEIDSETTING);

		if (qqid==0 || READC_S2(QQ_LOGIN_PASSWORD,&dbv)!=0) {
			MessageBox(NULL,TranslateT("You must enter both QQID and password to continue."),m_tszUserName,MB_ICONERROR);
		} else {
			m_iDesiredStatus=iNewStatus;
			if (m_iDesiredStatus==ID_STATUS_ONLINE && READC_B2(QQ_LOGIN_INVISIBLE)==1) m_iDesiredStatus=ID_STATUS_INVISIBLE;

			CallService(MS_DB_CRYPT_DECODESTRING, lstrlenA(dbv.pszVal) + 1, (LPARAM)dbv.pszVal);
			m_webqq=new CLibWebQQ(qqid,dbv.pszVal,this,&CProtocol::_CallbackHub, m_hNetlibUser);
			m_webqq->SetUseWeb2(true/*READC_B2("Protocol")==0*/);
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
						MessageBox(NULL,TranslateT("MIMQQ4 Only supports HTTP or SOCKS proxy."),m_tszUserName,MB_ICONERROR);
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
			m_webqq->SetLoginHide(m_iDesiredStatus==ID_STATUS_INVISIBLE);
			m_webqq->Start();
		}
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
	int flags=lpSM->flags;
	// DWORD seq=lpSM->seq;
	DWORD dwRet;

	if (!(flags & PREF_UTF)) {
		// MessageBox(NULL,L"Error: Not UTF!",NULL,0);
		QLog("Warning: Message not in UTF-8");
		msg=mir_utf8encode(msg);
	}

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
	CWebQQ2* wq2=new CWebQQ2(m_hNetlibUser,"431533706","dSEjfFh6",this);
		wq2->SetAutoDelete(TRUE);
	wq2->Start();
	//wq2->Test();
	/*
	JSONNODE* jn=json_parse("{\"retcode\":0,\"result\":{\"face\":0,\"birthday\":{\"month\":0,\"year\":0,\"day\":0},\"occupation\":\"0\",\"phone\":\"-\",\"allow\":1,\"college\":\"-\",\"reg_time\":1114844684,\"uin\":431533706,\"constel\":0,\"blood\":0,\"homepage\":\"-\",\"stat\":10,\"vip_info\":0,\"country\":\"\",\"city\":\"\",\"personal\":\"U+6DB4模鳴竭擱,U+59A6U+7E6B飲羶衄隱U+72DFU+FE5D\",\"nick\":\"^_^2\",\"shengxiao\":0,\"email\":\"431533706@qq.com\",\"client_type\":41,\"province\":\"\",\"gender\":\"unknown\",\"mobile\":\"\"}}");
	if (jn) {
		QLog("JSON Parse OK, Start dump");
		JSONNODE* jn2=json_get(jn,"retcode");
		char* pszResult=json_as_string(jn2);
		QLog("retcode=%s",pszResult);
		json_free(pszResult);

		jn2=json_get(jn,"result");
		int count=json_size(jn2);
		char* psz1;
		JSONNODE* jn3;
		for (int c=0; c<count; c++) {
			jn3=json_at(jn2,c);
			psz1=json_name(jn3);
			pszResult=json_as_string(jn3);
			QLog("%s=%s",psz1,pszResult);
			json_free(psz1);
			json_free(pszResult);
		}
		json_delete(jn);
	} else
		QLog("JSON Parse Failed!");
	*/
	/*
	HANDLE hJSON=(HANDLE)CallService(MS_JSON_PARSE,(WPARAM)"{\"retcode\":0,\"result\":{\"face\":0,\"birthday\":{\"month\":0,\"year\":0,\"day\":0},\"occupation\":\"0\",\"phone\":\"-\",\"allow\":1,\"college\":\"-\",\"reg_time\":1114844684,\"uin\":431533706,\"constel\":0,\"blood\":0,\"homepage\":\"-\",\"stat\":10,\"vip_info\":0,\"country\":\"\",\"city\":\"\",\"personal\":\"U+6DB4模鳴竭擱,U+59A6U+7E6B飲羶衄隱U+72DFU+FE5D\",\"nick\":\"^_^2\",\"shengxiao\":0,\"email\":\"431533706@qq.com\",\"client_type\":41,\"province\":\"\",\"gender\":\"unknown\",\"mobile\":\"\"}}",0);
	if (hJSON) {
		QLog("JSON Parse OK, Start dump");
		HANDLE hJSON2=(HANDLE)CallService(MS_JSON_GET,(WPARAM)hJSON,(LPARAM)"retcode");
		char* pszResult=(char*)CallService(MS_JSON_AS_STRING,(WPARAM)hJSON2,0);
		QLog("retcode=%s",pszResult);
		CallService(MS_JSON_FREE,(WPARAM)pszResult,0);

		hJSON2=(HANDLE)CallService(MS_JSON_GET,(WPARAM)hJSON,(LPARAM)"result");
		int count=CallService(MS_JSON_SIZE,(WPARAM)hJSON2,0);
		char* psz1;
		HANDLE hJSON3;
		for (int c=0; c<count; c++) {
			hJSON3=(HANDLE)CallService(MS_JSON_AT,(WPARAM)hJSON2,c);
			psz1=(char*)CallService(MS_JSON_NAME,(WPARAM)hJSON3,0);
			pszResult=(char*)CallService(MS_JSON_AS_STRING,(WPARAM)hJSON3,0);
			QLog("%s=%s",psz1,pszResult);
			CallService(MS_JSON_FREE,(WPARAM)psz1,0);
			CallService(MS_JSON_FREE,(WPARAM)pszResult,0);
		}
		CallService(MS_JSON_DELETE,(WPARAM)hJSON,0);
	} else
		QLog("JSON Parse Failed!");
	*/
	return 0;
	if (1==1) {
		CWebQQ2* wq2=new CWebQQ2(m_hNetlibUser,"431533706","dSEjfFh6",this);
		wq2->SetAutoDelete(TRUE);
		wq2->Start();
	} else {
		CWebQQ2* wq2=new CWebQQ2(m_hNetlibUser,"431533706","dSEjfFh6",this);
		wq2->Test();
		delete wq2;
	}

	/*
	CHttpClient chc(m_hNetlibUser);
	chc.SetCookie("clienttest=value2;");
	LPSTR pszResponse;

	pszResponse=chc.GetRequest("http://127.0.0.1/cookietest/set.php",wq2_referrers[CHCREFERER_PTLOGIN2]);
	MessageBoxA(NULL,pszResponse,NULL,0);
	mir_free(pszResponse);

	pszResponse=chc.GetRequest("http://127.0.0.1/cookietest/view.php",wq2_referrers[CHCREFERER_PTLOGIN2]);
	MessageBoxA(NULL,pszResponse,NULL,0);
	mir_free(pszResponse);
	*/

	// GetAllAvatars();
	// SetContactsOffline();
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
	m_webqq=new CLibWebQQ(431533706,"",this,_CallbackHub);
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
	if (!READC_U8S2("PendingAvatar",&dbv)) {
		if (int format=m_webqq->FetchUserHead(READC_D2(READC_B2("IsQun")==0?UNIQUEIDSETTING:"ExternalID"),READC_B2("IsQun")==0?CLibWebQQ::WEBQQ_USERHEAD_USER:CLibWebQQ::WEBQQ_USERHEAD_CLASS,dbv.pszVal)) {
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

	if (hContact) ProtoBroadcastAck(m_szModuleName, hContact, ACKTYPE_AVATAR, *pai.filename?ACKRESULT_SUCCESS:ACKRESULT_FAILED, (HANDLE)&pai, (LPARAM)0);
}

int CProtocol::GetAvatarInfo(WPARAM wParam, LPARAM lParam) {
	if (m_webqq) {
		PROTO_AVATAR_INFORMATION* lpPAI = (PROTO_AVATAR_INFORMATION*)lParam;
		HANDLE hContact=lpPAI->hContact;
		DWORD uid=READC_D2(UNIQUEIDSETTING);
		WCHAR szPath[MAX_PATH];

		QLog("GetAvatarInfo(): %u",uid);

		FoldersGetCustomPathW(m_folders[FOLDER_AVATARS],szPath,MAX_PATH,L"QQ");

		sprintf(lpPAI->filename,"%S\\%u.jpg",szPath,uid);
		lpPAI->format=PA_FORMAT_JPEG;

		if (READ_B2(NULL,"UHDownload")==2) {
			// Download on demand
			WRITEC_U8S("PendingAvatar",lpPAI->filename);
			CreateThreadObj(&CProtocol::FetchAvatar,lpPAI->hContact);
			return GAIR_WAITFOR;
		} else if (GetFileAttributesA(lpPAI->filename)!=INVALID_FILE_ATTRIBUTES) {
			return GAIR_SUCCESS;
		}
	}
	return GAIR_NOAVATAR;
}

void __cdecl CProtocol::GetAllAvatarsThread(LPVOID) {
	HANDLE hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, (WPARAM)NULL, (LPARAM)NULL);
	WCHAR szPath[MAX_PATH];
	char szPath2[MAX_PATH];
	LPSTR pszPath;
	BOOL fUHDownloadType=READ_B2(NULL,"UHDownload");

	FoldersGetCustomPathW(m_folders[FOLDER_AVATARS],szPath,MAX_PATH,L"QQ");
	sprintf(szPath2,"%S\\",szPath);
	pszPath=szPath2+strlen(szPath2);

	if (fUHDownloadType!=2) {
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

void CProtocol::PostImageInner(HANDLE hContact, LPWSTR pszFilename) {
	HANDLE hFile=CreateFileW(pszFilename,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,0,NULL);
	if (hFile==INVALID_HANDLE_VALUE) {
		MessageBoxW(NULL,TranslateT("Failed opening specified file for upload."),m_tszUserName,MB_ICONERROR|MB_APPLMODAL);
	} else {
		POPUPDATAW pd={hContact};
		LPSTR pszFileNameOnly=mir_utf8encodeW(wcsrchr(pszFilename,'\\')+1);
		pd.lchIcon=LoadIcon(g_hInstance,MAKEINTRESOURCE(IDI_ICON1));
		wcscpy(pd.lptzContactName,m_tszUserName);
		wcscpy(pd.lptzText,L"Your image file is being uploaded. When uploading completes, an image tag will be added to your compose area.");
		PUAddPopUpW(&pd);

		if (READC_B2("IsQun"))
			m_webqq->UploadQunImage(hFile,pszFileNameOnly,READC_D2(UNIQUEIDSETTING));
		else
			m_webqq->Web2UploadP2PImage(hFile,pszFileNameOnly,READC_D2(UNIQUEIDSETTING));

		// hFile closed by above function
		mir_free(pszFileNameOnly);
	}
}

int __cdecl CProtocol::PostImage(WPARAM wParam, LPARAM lParam) {
	OPENFILENAMEW ofn={sizeof(OPENFILENAMEW)};
	WCHAR szFile[MAX_PATH]={0};       // buffer for file name
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrFilter = TranslateT("Image Files (*.jpg; *.gif)\0*.jpg;*.gif\0");
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	// Display the Open dialog box. 

	if (GetOpenFileNameW(&ofn)==TRUE) {
		PostImageInner((HANDLE)wParam,szFile);
	}

	return 0;
}

int __cdecl CProtocol::OnPrebuildContactMenu(WPARAM wParam, LPARAM lParam) {
	DWORD config=0;
	HANDLE hContact=(HANDLE)wParam;
	CLISTMENUITEM clmi={sizeof(clmi)};
	if (!strcmp((LPSTR)CallService(MS_PROTO_GETCONTACTBASEPROTO,(WPARAM)hContact,0),m_szModuleName)) {
		/*
		CRMI2(QQ_CNXTMENU_SILENTQUN,SilentQun,TranslateT("Toggle &Silent"));
		CRMI2(QQ_CNXTMENU_POSTIMAGE,PostImage,TranslateT("Post &Image"));
		*/

		/*if (m_iStatus>ID_STATUS_OFFLINE)*/ {
			if (READC_B2("IsQun")==1) {
				/*
				//Qun* qun=m_qunList.getQun(READC_D2(UNIQUEIDSETTING));
				config=0x74; //0x1b4;
				if (READC_D2("Creator")==m_myqq || READC_B2("IsAdmin")) { // Show "Add member to Qun"
					config+=0x2;
				}

				//clmi.flags=CMIM_FLAGS|CMIM_NAME;
				if (READC_W2("Status")!=ID_STATUS_ONLINE)
					clmi.ptszName=TranslateT("Allow &receive of Qun Message");
				else
					clmi.ptszName=TranslateT("Keep this Qun &silent");
				//CallService(MS_CLIST_MODIFYMENUITEM,(WPARAM)qqCnxtMenuItems[2],(LPARAM)&clmi);
				*/
				config=0x3;
			} else
				// config=0x09;
				config=0x2;
		}
		//config=-1; // For debug only
	}

	int c=0;

	for (vector<HANDLE>::iterator iter=m_contextMenuItems.begin(); iter!=m_contextMenuItems.end(); iter++, c++) {
		clmi.flags=CMIF_UNICODE|((config & (1<<c))?CMIM_FLAGS:CMIM_FLAGS|CMIF_HIDDEN);
		//if (c==2) clmi.flags+=CMIM_NAME;
		CallService(MS_CLIST_MODIFYMENUITEM,(WPARAM)*iter,(LPARAM)&clmi);
	}

	return 0;
}
/*
int __cdecl CProtocol::OnDetailsInit(WPARAM wParam, LPARAM lParam) {
	HWND hWnd=GetForegroundWindow();
	CHAR szTitle[MAX_PATH];
	GetWindowTextA(hWnd,szTitle,MAX_PATH);
	QLog(__FUNCTION__"(): szTitle=%s",szTitle);
	return 0;
}
*/
int __cdecl CProtocol::SilentQun(WPARAM wParam, LPARAM lParam) {
	HANDLE hContact=(HANDLE)wParam;
	BYTE bCurrent=READC_B2(QQ_SILENTQUN)==0?1:0;

	WRITEC_B(QQ_SILENTQUN,bCurrent);
	WRITEC_W(QQ_STATUS,bCurrent==0?ID_STATUS_ONLINE:ID_STATUS_DND);

	return 0;
}

HANDLE CProtocol::SendFile(HANDLE hContact, const PROTOCHAR* szDescription, PROTOCHAR** ppszFiles) {
	if (m_iStatus>ID_STATUS_OFFLINE) {
		LPSTR file=(LPSTR)*ppszFiles;
		LPWSTR wfile=NULL;
		LPWSTR wfileAlloc=NULL;

		if (ppszFiles[1]!=NULL) {
			MessageBoxW(NULL,TranslateT("Only 1 file is allowed"),NULL,MB_ICONERROR);
			return 0;
		}

		if (READC_B2("IsQun")==1) {
			// Qun
			HWND hWndFT=GetForegroundWindow();
			//SendMessage(GetForegroundWindow(),WM_CLOSE,NULL,NULL);

			if (file[1]==0) {
				// Miranda IM 0.9: ppszFiles maybe in Unicode
				wfile=(LPWSTR)file;
			} else {
				wfileAlloc=wfile=mir_a2u_cp(file,GetACP());
			}

			// Test file first
			LPWSTR pszExt=wcsrchr(wfile,'.')+1;
			if (wcsicmp(pszExt,L"jpg") && wcsicmp(pszExt,L"gif")) {
				MessageBoxW(NULL,TranslateT("Only GIF and JPG images are supported."),NULL,MB_ICONERROR);
			} else {
				PostImageInner(hContact,wfile);
			}

			PostMessage(hWndFT,WM_CLOSE,NULL,NULL);
			if (wfileAlloc) mir_free(wfileAlloc);
		} else {
			MessageBoxW(NULL,TranslateT("This MirandaQQ4 build does not support file transfer."),NULL,MB_ICONERROR);
		}
	}

	return 0;
}

int __cdecl CProtocol::DownloadGroup(WPARAM wParam, LPARAM lParam) {
	if (m_webqq) {
		WRITE_B(NULL,"GroupFetched",0);
		if (m_webqq->GetUseWeb2())
			m_webqq->web2_api_get_user_friends();
#if 0 // Web1
		else
			m_webqq->GetGroupInfo();
#endif // Web1
	}
	return 0;
}

void CProtocol::HandleQunSearchResult(JSONNODE* jn, HANDLE hSearch) {
	JSONNODE* jnResults=json_get(jn,"results");
	JSONNODE* jnItem;
	LPSTR pszValue;
	LPWSTR pszTemp;
	int nItems=json_size(jnResults);

	QLog(__FUNCTION__"(): Results=%d",nItems);

	for (int c=0; c<nItems; c++) {
		PROTOSEARCHRESULT psr={sizeof(psr)};
		TCHAR szID[32];
		LPTSTR pwszTemp;

		jnItem=json_at(jnResults,c);
		psr.id=wcscpy(szID,TranslateT("(Qun)"));
		wcscat(szID,L" ");
		wcscat(szID,pwszTemp=mir_utf8decodeW(pszValue=json_as_string(json_get(jnItem,"GE"))));
		mir_free(pwszTemp);

		/*
		psr.id=wcscpy(szID,pwszTemp=mir_utf8decodeW(pszValue=json_as_string(json_get(jnItem,"GE"))));
		json_free(pszValue);
		mir_free(pwszTemp);
		wcscat(szID,L" ");
		wcscat(szID,TranslateT("(Qun)"));
		*/

		psr.nick=mir_utf8decodeW(pszValue=json_as_string(json_get(jnItem,"TI")));
		json_free(pszValue);
		
		pszTemp=psr.nick;
		while (pszTemp=wcsstr(pszTemp,L"<font  color=#C60A00>")) memmove(pszTemp,pszTemp+21,(wcslen(pszTemp)-20)*2);
		
		pszTemp=psr.nick;
		while (pszTemp=wcsstr(pszTemp,L"</font>")) memmove(pszTemp,pszTemp+7,(wcslen(pszTemp)-6)*2);
		

		psr.firstName=mir_utf8decodeW(pszValue=json_as_string(json_get(jnItem,"TX")));
		json_free(pszValue);
		
		pszTemp=psr.firstName;
		while (pszTemp=wcsstr(pszTemp,L"<font  color=#C60A00>")) memmove(pszTemp,pszTemp+21,(wcslen(pszTemp)-20)*2);

		pszTemp=psr.firstName;
		while (pszTemp=wcsstr(pszTemp,L"</font>")) memmove(pszTemp,pszTemp+7,(wcslen(pszTemp)-6)*2);
		

		psr.flags=PSR_UNICODE;

		ProtoBroadcastAck(m_szModuleName, NULL, ACKTYPE_SEARCH, ACKRESULT_DATA, hSearch, (LPARAM)&psr);
		mir_free(psr.nick);
		mir_free(psr.firstName);
	}

	json_delete(jn);

	// Debug
	/*
	PROTOSEARCHRESULT psr={sizeof(psr)};
	psr.id=L"(Qun) 24564026";
	psr.nick=L"Test";
	psr.firstName=L"Test";
	psr.flags=PSR_UNICODE;

	ProtoBroadcastAck(m_szModuleName, NULL, ACKTYPE_SEARCH, ACKRESULT_DATA, hSearch, (LPARAM)&psr);
	*/
}

void __cdecl CProtocol::SearchBasicThread(LPVOID lpParameter) {
	JSONNODE* jn;
	DWORD uin=strtoul((LPSTR)lpParameter,NULL,10);

	/*
	// Debug start
		PROTOSEARCHRESULT psr={sizeof(psr)};
		psr.id=L"12345678";

		// Nick field reserved for qun extid
		psr.nick=L"Nick";

		psr.flags=PSR_UNICODE;

		Sleep(500);
		ProtoBroadcastAck(m_szModuleName, NULL, ACKTYPE_SEARCH, ACKRESULT_DATA, (HANDLE)m_searchuin, (LPARAM)&psr);

		Sleep(500);
		ProtoBroadcastAck(m_szModuleName, NULL, ACKTYPE_SEARCH, ACKRESULT_SUCCESS, (HANDLE)m_searchuin, 0);
		return;
	// Debug end
	*/

	if (m_webqq->web2_api_get_single_info(uin,&jn)) {
		JSONNODE* jnResult=json_get(jn,"result");
		JSONNODE* jnItem;
		LPSTR pszValue;

		QLog(__FUNCTION__"(): User Received search result");

		PROTOSEARCHRESULT psr={sizeof(psr)};
		psr.id=mir_utf8decodeW((LPSTR)lpParameter);

		// Nick field reserved for qun extid
		jnItem=json_get(jnResult,"nick");
		psr.nick=mir_utf8decodeW(pszValue=json_as_string(jnItem));
		json_free(pszValue);

		jnItem=json_get(jnResult,"province");
		psr.firstName=mir_utf8decodeW(pszValue=json_as_string(jnItem));
		json_free(pszValue);

		jnItem=json_get(jnResult,"email");
		psr.email=mir_utf8decodeW(pszValue=json_as_string(jnItem));
		json_free(pszValue);

		jnItem=json_get(jnResult,"token");
		psr.lastName=mir_utf8decodeW(pszValue=json_as_string(jnItem));
		json_free(pszValue);

		psr.flags=PSR_UNICODE;

		ProtoBroadcastAck(m_szModuleName, NULL, ACKTYPE_SEARCH, ACKRESULT_DATA, (HANDLE)uin, (LPARAM)&psr);
		mir_free(psr.id);
		mir_free(psr.firstName);
		mir_free(psr.email);

		json_delete(jn);
	}

	Sleep(500);

	if (jn=m_webqq->qun_air_search(uin)) {
		// {"responseHeader": {"Status":"0","CostTime":"2826","TotalNum":"1","CurrentNum":"1","CurrentPage":"1"},"results": [{"MD":"0","TI":"Miranda IM7)op讨论群","QQ":"753633","RQ":"1119325824","CL":"12:187;","DT":"1285005781","UR":"http://qun.qq.com/air/#2571213","TA":"","GA":"10","GB":"4","GC":"125","GD":"0","GE":"2571213","GF":"0","BU":"http://qun.qq.com/air/#2571213","TX":"本群不解答其它任何打包MIM问题 加入须有软件使用基础及DIY精神 还有本群长期成员推荐","HA":"2","HB":"0","HC":"0","HD":"0","HE":"0","HF":"0","PA":"","PB":"","PC":"","PD":""}],"QcResult": [],"HintResult": [],"SmartResult": []}
		QLog(__FUNCTION__"(): Qun Received search result");

		HandleQunSearchResult(jn,(HANDLE)uin);
	}

	ProtoBroadcastAck(m_szModuleName, NULL, ACKTYPE_SEARCH, ACKRESULT_SUCCESS, (HANDLE)uin, 0);

	mir_free(lpParameter);
}

HANDLE CProtocol::SearchBasic(const PROTOCHAR* id) {
	if (/*true || */m_webqq) {
		LPCSTR pszID=(LPCSTR)id;
		CreateThreadObj(&CProtocol::SearchBasicThread,mir_strdup(pszID));
		return (HANDLE)strtoul(pszID,NULL,10);	
	} else
		return 0;
}

void __cdecl CProtocol::SearchNickThread(LPVOID lpParameter) {
	JSONNODE* jn;
	LPSTR pszNick=(LPSTR)lpParameter;


	if (m_webqq->web2_api_search_qq_by_nick(pszNick,&jn)) {
		JSONNODE* jnResult=json_get(jn,"result");
		if (json_type(jnResult)!=JSON_ARRAY) {
			JSONNODE* jnItem;
			LPSTR pszValue;
			int count=json_as_int(json_get(jnResult,"count"));
			JSONNODE* jnUinlist=json_get(jnResult,"uinlist");
			WCHAR szID[16];

			/*
			{
				"endflag":0,
				"page":0,
				"count":25,
				"uinlist":[
					{
						"uin":22619484,
						"sex":2,
						"stat":10,
						"age":23,
						"nick":"miranda",
						"country":"\u4E2D\u56FD",
						"province":"\u6E56\u5317",
						"city":"\u8346\u5DDE",
						"face":0
					},...
				]
			}
			*/

			QLog(__FUNCTION__"(): User Received search result");

			for (int c=0; c<count; c++) {
				jnItem=json_at(jnUinlist,c);

				PROTOSEARCHRESULT psr={sizeof(psr)};
				
				psr.id=_ultow(json_as_float(json_get(jnItem,"uin")),szID,10);

				psr.nick=mir_utf8decodeW(pszValue=json_as_string(json_get(jnItem,"nick")));
				json_free(pszValue);

				psr.firstName=mir_utf8decodeW(pszValue=json_as_string(json_get(jnItem,"province")));
				json_free(pszValue);

				psr.flags=PSR_UNICODE;
				ProtoBroadcastAck(m_szModuleName, NULL, ACKTYPE_SEARCH, ACKRESULT_DATA, (HANDLE)lpParameter, (LPARAM)&psr);
				mir_free(psr.nick);
				mir_free(psr.firstName);
			}
		} else
			QLog(__FUNCTION__"(): No hit");

		json_delete(jn);
	}

	Sleep(500);

	if (jn=m_webqq->qun_air_search(pszNick)) {
		// {"responseHeader": {"Status":"0","CostTime":"2826","TotalNum":"1","CurrentNum":"1","CurrentPage":"1"},"results": [{"MD":"0","TI":"Miranda IM7)op讨论群","QQ":"753633","RQ":"1119325824","CL":"12:187;","DT":"1285005781","UR":"http://qun.qq.com/air/#2571213","TA":"","GA":"10","GB":"4","GC":"125","GD":"0","GE":"2571213","GF":"0","BU":"http://qun.qq.com/air/#2571213","TX":"本群不解答其它任何打包MIM问题 加入须有软件使用基础及DIY精神 还有本群长期成员推荐","HA":"2","HB":"0","HC":"0","HD":"0","HE":"0","HF":"0","PA":"","PB":"","PC":"","PD":""}],"QcResult": [],"HintResult": [],"SmartResult": []}
		QLog(__FUNCTION__"(): Qun Received search result");

		HandleQunSearchResult(jn,(HANDLE)lpParameter);
	}

	ProtoBroadcastAck(m_szModuleName, NULL, ACKTYPE_SEARCH, ACKRESULT_SUCCESS, (HANDLE)lpParameter, 0);

	mir_free(lpParameter);
}

HANDLE CProtocol::SearchByName(const PROTOCHAR* nick, const PROTOCHAR* firstName, const PROTOCHAR* lastName) {
	if (/*true || */m_webqq) {
		LPSTR pszNick=mir_utf8encode((LPSTR)nick);
		CreateThreadObj(&CProtocol::SearchNickThread,pszNick);
		return (HANDLE)pszNick;
	} else
		return 0;
}

HANDLE CProtocol::AddToList(int flags, PROTOSEARCHRESULT* psr){
	QLog(__FUNCTION__"()");
	bool isqun=wcschr(psr->id,' ')!=NULL;

	if (DWORD uid=wcstoul(isqun?wcsrchr(psr->id,' ')+1:psr->id,NULL,10)) {
		HANDLE hContact=AddOrFindContact(isqun?uid+3000000000:uid,flags&PALF_TEMPORARY,flags&PALF_TEMPORARY);
		if (isqun)
			WRITEC_B("IsQun",1);
		else {
			// LPSTR pszToken=mir_utf8encodeW(psr->lastName);
			WRITEC_U8S("Token",(LPSTR)psr->lastName);
			// mir_free(pszToken);
		}

		return hContact;
	} else
		QLog(__FUNCTION__"(): Error - uid==0!");

	return 0; 
}

int CProtocol::AuthRequest(HANDLE hContact, const PROTOCHAR* szMessage){
	if (m_webqq) {
		DWORD uid=READC_D2(UNIQUEIDSETTING);

		if (uid && szMessage) {
			LPSTR pszMsg=mir_utf8encode((LPCSTR)szMessage); // mir_utf8encodeW(szMessage);

			if (READC_B2("IsQun")) {
				if (JSONNODE* jn=m_webqq->qun_air_join(uid-3000000000,pszMsg)) {
					// {"r":{"domain":"qun.qq.com","server":"149.19","client":"112.120.135.211","elapsed":"0.0053","memory":"0.33MB","profile":"T_LOAD: 0.0008S|T_ROUTE: 0.0002S|T_DISPATCH: 0.0034S|","module":"default","controller":"index","action":"join","env":"live","way":"async","language":"zh-cn","user":{"id":85379868,"nick":"j85379868","gkey":"v0H+jYmLPTndIDki7VqITlmDrIJDXB6nEqjTYj5XT5Vf+3SDi54CSqAJvXP2inGIBn5Z6EAN1rM=","skey":"@rxMxViqNl","skeyt":1,"age":22,"gender":1,"face":252,"logintime":1288147097,"lastaccess":1288148306,"passport":"\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000","mail":""},"group":{"id":"0","auth":0,"permission":2,"type":1},"params":null},"c":{"code":"-8"}}
					JSONNODE* jnC=json_get(jn,"c");
					LPSTR pszCode=json_as_string(json_get(jnC,"code"));

					if (!strcmp(pszCode,"-8")) {
						ShowNotification(TranslateT("Wrong verification code, please try again."),NIIF_ERROR);
					} else
						ShowNotification(TranslateT("Authorization request sent."),NIIF_INFO);

					json_free(pszCode);
					json_delete(jn);
				} else
					ShowNotification(TranslateT("Authorization request failed"),NIIF_ERROR);
			} else {
				if (m_webqq->GetUseWeb2()) {
					JSONNODE* jn;
					DBVARIANT dbv;
					
					if (!READC_U8S2("Token",&dbv)) {
						if (m_webqq->web2_api_add_need_verify(uid,0,pszMsg,dbv.pszVal,&jn)) {
							int errno;

							if ((errno=json_as_int(json_get(jn,"result")))==0)
								ShowNotification(TranslateT("Authorization request sent."),NIIF_INFO);
							else {
								QLog(__FUNCTION__"(): Error - result=%d",errno);
								ShowNotification(TranslateT("Server replied an error code for authorization request."),NIIF_ERROR);
							}
						} else
							ShowNotification(TranslateT("Failed to send authorization request. Please try again later."),NIIF_ERROR);
						DBFreeVariant(&dbv);
					} else {
						QLog(__FUNCTION__"(): Error - No Token!");
						ShowNotification(TranslateT("No Auth Token. Please try again."),NIIF_ERROR);
					}
#if 0 // Web1
				} else {
					m_webqq->AddFriendPassive(uid,pszMsg);
					ShowNotification(TranslateT("Authorization request sent."),NIIF_INFO);
#endif // Web1
				}
			}

			mir_free(pszMsg);
		} else
			QLog(__FUNCTION__"(): Error - uid==0 or szMesage==NULL!");

		return 0; 
	} else
		return 1;
}

HANDLE CProtocol::AddToListByEvent(int flags, int iContact, HANDLE hDbEvent){ QLog(__FUNCTION__"()"); return 0; }

int CProtocol::Authorize(HANDLE hDbEvent){
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
		if (READC_B2("IsQun")==0) {
			DBGetContactSettingUTF8String(hContact,"CList","MyHandle",&dbv);
			m_webqq->web2_api_allow_and_add(*uid,0,dbv.pszVal?dbv.pszVal:"");
			if (dbv.pszVal) DBFreeVariant(&dbv);
		}
		return 0;
	}
	return 1;
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

	// szReason is most likely in native charset, not UTF8 nor Unicode!
	if (m_webqq) {
		LPSTR pszMsg=mir_utf8encode((LPSTR)szReason);
		if (READC_B2("IsQun")==0) {
			m_webqq->web2_api_deny_add_request(*uid,pszMsg);
		}
		mir_free(pszMsg);
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
	if (m_webqq) {
		CreateThreadObj(&CProtocol::GetInfoThread,(LPVOID)hContact);
	}

	return 0;
}

void __cdecl CProtocol::GetInfoThread(LPVOID lpParameter) {
	HANDLE hContact=(HANDLE)lpParameter;

	if (READC_B2("IsQun")==0) {
		DWORD dwUIN=READC_D2(UNIQUEIDSETTING);
		m_webqq->web2_api_get_qq_level(dwUIN);
		m_webqq->web2_api_get_friend_info(dwUIN);
	}
	// TODO: Qun Info
}

// CWebQQ2
void CProtocol::GetGroupListMask2(CWebQQ2* webqq2, LPSTR pszArgs, JSONNODE* jn) {
	if (json_as_int(json_get(jn,"retcode"))==0) {
		/*
		{
		   "retcode":0,
		   "result":{
			  "gmasklist":[

			  ],
			  "gnamelist":[
				 {"flag":1049617, "name":"凉宫春日的旧封绝", "gid":329399655, "code":2190184963},
				 {"flag":17826817, "name":"Miranda IM", "gid":1826181312, "code":3122999881},
				 {"flag":17825793, "name":"測試群", "gid":3387867626, "code":4092567878},
				 {"flag":34603025, "name":"叶色的闭锁空间", "gid":867899790, "code":1933612015},
				 {"flag":1064961, "name":"測試群2", "gid":1461164908, "code":4172861839}
			  ],
			  "gmarklist":[
				 {"uin":1461164908, "markname":"Notice3_Group"}
			  ]
		   }
		}
		*/
		JSONNODE* jnResult=json_get(jn,"result");
		JSONNODE* jnSection;
		JSONNODE* jnItem;
		LPSTR pszSection;
		DWORD dwFlag;
		DWORD dwCode;
		DWORD dwGid;
		LPSTR pszName;

		for (int c=0; c<json_size(jnResult); c++) {
			jnSection=json_at(jnResult,c);
			pszSection=json_name(jnSection);
			if (!strcmp(pszSection,"gnamelist")) {
				for (int c2=0; c2<json_size(jnSection); c2++) {
					jnItem=json_at(jnSection,c2);
					dwFlag=(DWORD)json_as_float(json_get(jnItem,"flag"));
					dwCode=(DWORD)json_as_float(json_get(jnItem,"code"));
					dwGid=(DWORD)json_as_float(json_get(jnItem,"gid"));
					pszName=json_as_string(json_get(jnItem,"name"));

					if (HANDLE hContact=AddOrFindQunContact(webqq2,dwFlag,pszName,dwGid,true)) {
						// Flag and name already set
						WRITEC_D("code",dwCode);
						WRITEC_D("pseudo_uin",dwGid);
						WRITEC_W("Status",READC_B2(QQ_SILENTQUN)?ID_STATUS_INVISIBLE:ID_STATUS_ONLINE);
					}
					json_free(pszName);
				}
			} else if (!strcmp(pszSection,"gmarklist")) {
				// Discussion Group
				for (int c2=0; c2<json_size(jnSection); c2++) {
					jnItem=json_at(jnSection,c2);
					dwGid=(DWORD)json_as_float(json_get(jnItem,"uin"));
					pszName=json_as_string(json_get(jnItem,"markname"));

					if (HANDLE hContact=AddOrFindContact(dwGid,true,false)) {
						WRITEC_U8S("Nick",pszName);
					}
					json_free(pszName);
				}
			} else {
				QLog(__FUNCTION__"(): Ignored unknown section %s",pszSection);
			} /*else if (!strcmp(pszSection,"gmasklist")) {
				// ???
			}*/
			json_free(pszSection);
		}
	} else {
		QLog(__FUNCTION__"(): retcode!=0!");
	}
}

void CProtocol::GetUserFriends2(CWebQQ2* webqq2, LPSTR pszArgs, JSONNODE* jn) {
	/*
	{
	   "retcode":0,
	   "result":{
		  "friends":[
			 {"flag":0, "uin":2989632540, "categories":0},
			 {"flag":0, "uin":937960405, "categories":0},
			 {"flag":0, "uin":1758353101, "categories":0},
			 {"flag":0, "uin":375567902, "categories":0},
			 {"flag":0, "uin":3451767722, "categories":0},
			 {"flag":0, "uin":3709177032, "categories":0},
			 {"flag":4, "uin":2522194210, "categories":1},
			 {"flag":0, "uin":1197379730, "categories":0},
			 {"flag":8, "uin":2469015452, "categories":2},
			 {"flag":0, "uin":2814673687, "categories":0},
			 {"flag":0, "uin":824286078, "categories":0},
			 {"flag":8, "uin":1829831559, "categories":2},
			 {"flag":0, "uin":624209290, "categories":0},
			 {"flag":0, "uin":22931682, "categories":0},
			 {"flag":4, "uin":2480456437, "categories":1}
		  ],
		  "marknames":[
			 {"uin":824286078, "markname":"My Notice 2"},
			 {"uin":2401036228, "markname":""},
			 {"uin":1461164908, "markname":"Notice3_Group"}
		  ],
		  "categories":[
			 {"index":1, "sort":0, "name":"新增群組"},
			 {"index":2, "sort":0, "name":"测试分组"}
		  ],
		  "vipinfo":[
			 {"vip_level":6, "u":2989632540, "is_vip":1},
			 {"vip_level":6, "u":937960405, "is_vip":1},
			 {"vip_level":0, "u":1758353101, "is_vip":0},
			 {"vip_level":0, "u":375567902, "is_vip":0},
			 {"vip_level":0, "u":3451767722, "is_vip":0},
			 {"vip_level":0, "u":3709177032, "is_vip":0},
			 {"vip_level":6, "u":2522194210, "is_vip":1},
			 {"vip_level":0, "u":1197379730, "is_vip":0},
			 {"vip_level":0, "u":2469015452, "is_vip":0},
			 {"vip_level":0, "u":2814673687, "is_vip":0},
			 {"vip_level":0, "u":824286078, "is_vip":0},
			 {"vip_level":0, "u":1829831559, "is_vip":0},
			 {"vip_level":0, "u":624209290, "is_vip":0},
			 {"vip_level":0, "u":22931682, "is_vip":0},
			 {"vip_level":0, "u":2480456437, "is_vip":0}
		  ],
		  "info":[
			 {"face":0, "flag":4194886, "nick":"心之镜", "uin":2989632540},
			 {"face":0, "flag":16777798, "nick":"  草从", "uin":937960405},
			 {"face":0, "flag":16384, "nick":"｡◕‿◕｡", "uin":1758353101},
			 {"face":303, "flag":8389186, "nick":"小路", "uin":375567902},
			 {"face":0, "flag":13124098, "nick":"梦在千年", "uin":3451767722},
			 {"face":0, "flag":12582978, "nick":"【平】", "uin":3709177032},
			 {"face":0, "flag":4735558, "nick":"    道莲", "uin":2522194210},
			 {"face":252, "flag":4194304, "nick":"j85379868", "uin":1197379730},
			 {"face":39, "flag":12582912, "nick":"wuchang", "uin":2469015452},
			 {"face":0, "flag":8389186, "nick":"ヤミノカケラ", "uin":2814673687},
			 {"face":48, "flag":4194368, "nick":"忧郁的凉宫遥", "uin":824286078},
			 {"face":33, "flag":12582912, "nick":"伴月?孤影", "uin":1829831559},
			 {"face":0, "flag":4194816, "nick":"^_^", "uin":624209290},
			 {"face":525, "flag":4735490, "nick":"法珞", "uin":22931682},
			 {"face":0, "flag":512, "nick":"魔法少女逆岛", "uin":2480456437}
		  ]
	   }
	}
	*/
	typedef struct {
		BYTE categories;
		LPSTR markname;
		BYTE is_vip;
		BYTE vip_level;
		WORD face;
		DWORD flag;
		LPSTR nick;
	} FRIEND, *PFRIEND, *LPFRIEND;

	if (json_as_int(json_get(jn,"retcode"))==0) {
		map<DWORD,LPFRIEND> friends;
		JSONNODE* jnResult=json_get(jn,"result");
		JSONNODE* jnSection;
		JSONNODE* jnItem;
		LPSTR pszSection;
		LPFRIEND lpFriend;
		DWORD uin;
		LPSTR pszTemp;
		int id;

		for (int c=0; c<json_size(jnResult); c++) {
			jnSection=json_at(jnResult,c);
			pszSection=json_name(jnSection);
			if (!strcmp(pszSection,"friends")) {
				// {"flag":0, "uin":2989632540, "categories":0},
				for (int c2=0; c2<json_size(jnSection); c2++) {
					jnItem=json_at(jnSection,c2);
					lpFriend=(LPFRIEND)mir_alloc(sizeof(FRIEND));
					memset(lpFriend,0,sizeof(FRIEND));
					lpFriend->categories=json_as_int(json_get(jnItem,"categories"));
					friends[(DWORD)json_as_float(json_get(jnItem,"uin"))]=lpFriend;
				}
			} else if (!strcmp(pszSection,"marknames")) {
				// {"uin":824286078, "markname":"My Notice 2"},
				for (int c2=0; c2<json_size(jnSection); c2++) {
					jnItem=json_at(jnSection,c2);
					lpFriend=friends[uin=(DWORD)json_as_float(json_get(jnItem,"uin"))];
					pszTemp=json_as_string(json_get(jnItem,"markname"));
					if (!lpFriend) {
						if (HANDLE hContact=FindContactWithPseudoUIN(uin)) {
							DBWriteContactSettingStringUtf(hContact,"CList","MyHandle",pszTemp);
						} else {
							QLog(__FUNCTION__"(%s): Pseudo UIN %u not contact nor qun!",pszSection,uin);
						}
						json_free(pszTemp);
					} else {
						lpFriend->markname=pszTemp;
					}
				}
			} else if (!strcmp(pszSection,"categories")) {
				// {"index":1, "sort":0, "name":"新增群組"},
				for (int c2=0; c2<json_size(jnSection); c2++) {
					jnItem=json_at(jnSection,c2);
					pszTemp=json_as_string(json_get(jnItem,"name"));

					if ((id=FindGroupByName(pszTemp))==-1) {
						LPWSTR pszGroup=mir_utf8decodeW(pszTemp);
						id=CallService(MS_CLIST_GROUPCREATE,0,(LPARAM)pszGroup);
						mir_free(pszGroup);
					}
					m_groups[json_as_int(json_get(jnItem,"index"))]=id;
					json_free(pszTemp);
				}
			} else if (!strcmp(pszSection,"vipinfo")) {
				// {"vip_level":6, "u":2989632540, "is_vip":1},
				for (int c2=0; c2<json_size(jnSection); c2++) {
					jnItem=json_at(jnSection,c2);
					if (lpFriend=friends[uin=(DWORD)json_as_float(json_get(jnItem,"u"))]) {
						lpFriend->vip_level=json_as_int(json_get(jnItem,"vip_level"));
						lpFriend->is_vip=json_as_int(json_get(jnItem,"is_vip"));
					} else {
						QLog(__FUNCTION__"(%s): Pseudo UIN %u not found!",pszSection,uin);
					}
				}
			} else if (!strcmp(pszSection,"info")) {
				// {"face":0, "flag":4194886, "nick":"心之镜", "uin":2989632540},
				for (int c2=0; c2<json_size(jnSection); c2++) {
					jnItem=json_at(jnSection,c2);

					if (lpFriend=friends[uin=(DWORD)json_as_float(json_get(jnItem,"uin"))]) {
						lpFriend->flag=(DWORD)json_as_float(json_get(jnItem,"flag"));
						lpFriend->nick=json_as_string(json_get(jnItem,"nick"));

						if (HANDLE hContact=AddOrFindQunContact(webqq2,lpFriend->flag,lpFriend->nick,uin,false)) {
							lpFriend->face=json_as_int(json_get(jnItem,"face"));
							if (lpFriend->categories>0 && m_groups[lpFriend->categories]!=0 && READC_W2("categories")!=lpFriend->categories) {
								// This contact is new, move group
								WRITEC_W("categories",lpFriend->categories);
								CallService(MS_CLIST_CONTACTCHANGEGROUP,(WPARAM)hContact,(LPARAM)m_groups[lpFriend->categories]);
							}

							if (lpFriend->markname) {
								DBWriteContactSettingStringUtf(hContact,"CList","MyHandle",lpFriend->markname);
							}

							WRITEC_B("is_vip",lpFriend->is_vip);
							WRITEC_B("vip_level",lpFriend->vip_level);
							WRITEC_W("face",lpFriend->face);
							WRITEC_D("pseudo_uin",uin);
						}
					} else {
						QLog(__FUNCTION__"(%s): Pseudo UIN %u not found!",pszSection,uin);
					}
				}
			}

			json_free(pszSection);
		}

		for (map<DWORD,LPFRIEND>::iterator iter=friends.begin(); iter!=friends.end(); iter++) {
			if (lpFriend=iter->second) {
				if (lpFriend->nick) json_free(lpFriend->nick);
				if (lpFriend->markname) json_free(lpFriend->markname);
				mir_free(lpFriend);
			}
		}
	} else {
		QLog(__FUNCTION__"(): retcode!=0!");
	}
}

void CProtocol::GetFriendInfo2(CWebQQ2* webqq2, LPSTR pszArgs, JSONNODE* jn) {
	if (json_as_int(json_get(jn,"retcode"))==0) {
		JSONNODE* jnResult=json_get(jn,"result");
		JSONNODE* jnItem;
		int size=json_size(jnResult);
		int nTemp;
		DWORD dwUin;
		LPSTR pszName, pszValue;

		dwUin=(DWORD)json_as_float(json_get(jnResult,"uin"));
		HANDLE hContact=FindContact(dwUin);
		if (hContact!=NULL || dwUin==webqq2->GetTUin()) {
			for (int c=0; c<size; c++) {
				jnItem=json_at(jnResult,c);
				pszName=json_name(jnItem);
				pszValue=NULL;

				if (!strcmp(pszName,"face")) WRITEC_W("Face",json_as_int(jnItem));
				else if (!strcmp(pszName,"birthday")) {
					// "birthday":{"year":0,"month":0,"day":0},
					WRITEC_W("BirthYear",json_as_int(json_get(jnItem,"year")));
					WRITEC_B("BirthMonth",json_as_int(json_get(jnItem,"month")));
					WRITEC_B("BirthDay",json_as_int(json_get(jnItem,"day")));
				}
				else if (!strcmp(pszName,"occupation")) { WRITEC_U8S("Occupation2",pszValue=json_as_string(jnItem)); WRITEC_U8S("CompanyPosition",pszValue=json_as_string(jnItem)); }
				else if (!strcmp(pszName,"phone")) { WRITEC_U8S("Telephone",pszValue=json_as_string(jnItem)); WRITEC_U8S("Phone",pszValue); }
				else if (!strcmp(pszName,"allow")) WRITEC_B("AuthType",json_as_int(jnItem));
				else if (!strcmp(pszName,"college")) {
					WRITEC_U8S("College",pszValue=json_as_string(jnItem));
					WRITEC_TS("Past3",TranslateT("Graduation College"));
					WRITEC_U8S("Past3Text",pszValue);
				}
				else if (!strcmp(pszName,"reg_time")) WRITEC_D("reg_time",(DWORD)json_as_float(jnItem));
				else if (!strcmp(pszName,"constel")) {
					LPWSTR pszSX[]={
						L"?",
						TranslateT("Aquarius"),
						TranslateT("Pisces"),
						TranslateT("Aries"),
						TranslateT("Taurus"),
						TranslateT("Gemini"),
						TranslateT("Cancer"),
						TranslateT("Leo"),
						TranslateT("Virgo"),
						TranslateT("Libra"),
						TranslateT("Scorpio"),
						TranslateT("Sagittarius"),
						TranslateT("Capricorn"),
					};
					WRITEC_B("Zodiac",nTemp=json_as_int(jnItem));
					WRITEC_TS("Past2",TranslateT("Zodiac"));
					WRITEC_TS("Past2Text",pszSX[nTemp]);
				}
				else if (!strcmp(pszName,"blood")) {
					WRITEC_B("Blood",nTemp=json_as_int(jnItem));
					WRITEC_TS("Past0",TranslateT("Blood Type"));
					WRITEC_U8S("Past0Text",nTemp==0?"?":nTemp==1?"A":nTemp==2?"B":nTemp==3?"O":"AB");
				}
				else if (!strcmp(pszName,"homepage")) WRITEC_U8S("Homepage",pszValue=json_as_string(jnItem)); // OK
				else if (hContact!=NULL && !strcmp(pszName,"stat")) WRITEC_W("Status",Web2StatusToMIM(pszValue=json_as_string(jnItem)));
				else if (!strcmp(pszName,"vip_info")) WRITEC_W("vip_info",json_as_int(jnItem));
				else if (!strcmp(pszName,"country")) WRITEC_U8S("Country",pszValue=json_as_string(jnItem)); // OK
				else if (!strcmp(pszName,"city")) WRITEC_U8S("City",pszValue=json_as_string(jnItem)); // OK
				else if (!strcmp(pszName,"personal")) WRITEC_U8S("About",pszValue=json_as_string(jnItem));
				else if (!strcmp(pszName,"nick")) WRITEC_U8S("Nick",pszValue=json_as_string(jnItem)); // Ok
				else if (!strcmp(pszName,"shengxiao")) {
					LPWSTR pszSX[]={
						L"?",
						TranslateT("Rat"),
						TranslateT("Ox"),
						TranslateT("Tiger"),
						TranslateT("Hare"),
						TranslateT("Dragon"),
						TranslateT("Snake"),
						TranslateT("Horse"),
						TranslateT("Ram"),
						TranslateT("Monkey"),
						TranslateT("Rooster"),
						TranslateT("Dog"),
						TranslateT("Pig"),
					};
					WRITEC_B("Horoscope",nTemp=json_as_int(jnItem));
					WRITEC_TS("Past1",TranslateT("Chinese Zodiac"));
					WRITEC_TS("Past1Text",pszSX[nTemp]);
				}
				else if (!strcmp(pszName,"email")) { WRITEC_U8S("Email",pszValue=json_as_string(jnItem)); WRITEC_U8S("e-mail",pszValue); }
				else if (!strcmp(pszName,"client_type")) WRITEC_W("client_type",json_as_int(jnItem));
				else if (!strcmp(pszName,"province")) { WRITEC_U8S("Province",pszValue=json_as_string(jnItem)); WRITEC_U8S("State",pszValue); }
				else if (!strcmp(pszName,"gender")) WRITEC_B("Gender",*(pszValue=json_as_string(jnItem))=='m'?'M':*pszValue=='f'?'F':'?');
				else if (!strcmp(pszName,"mobile")) { WRITEC_U8S("Mobile",pszValue=json_as_string(jnItem)); WRITEC_U8S("Cellular",pszValue); }
				else QLog(__FUNCTION__"(): Ignored unknown entity %s",pszName);

				if (pszValue) json_free(pszValue);
				json_free(pszName);
			}
		}

		/*
		{
		   "face":0,
		   "birthday":{
			  "month":0,
			  "year":0,
			  "day":0
		   },
		   "occupation":"0",
		   "phone":"-",
		   "allow":1,
		   "college":"-",
		   "reg_time":1114844684,
		   "uin":431533706,
		   "constel":0,
		   "blood":0,
		   "homepage":"-",
		   "stat":10,
		   "vip_info":0,
		   "country":"",
		   "city":"",
		   "personal":"U+6DB4模鳴竭擱,U+59A6U+7E6B飲羶衄隱U+72DFU+FE5D",
		   "nick":"^_^2",
		   "shengxiao":0,
		   "email":"431533706@qq.com",
		   "client_type":41,
		   "province":"",
		   "gender":"unknown",
		   "mobile":""
		}
		*/

	} else {
		QLog(__FUNCTION__"(): retcode!=0!");
	}
}

void CProtocol::GetOnlineBuddies2(CWebQQ2* webqq2, LPSTR pszArgs, JSONNODE* jn) {
	if (json_as_int(json_get(jn,"retcode"))==0) {
		/*
		{
		   "retcode":0,
		   "result":[
			  {"uin":624209290, "status":"online", "client_type":1},
			  {"uin":2469015452, "status":"online", "client_type":1}
		   ]
		}
		*/
		JSONNODE* jnResult=json_get(jn,"result");
		JSONNODE* jnItem;
		DWORD dwUIN;
		HANDLE hContact;
		LPSTR pszTemp;

		for (int c=0; c<json_size(jnResult); c++) {
			jnItem=json_at(jnResult,c);
			dwUIN=(DWORD)json_as_float(json_get(jnItem,"uin"));

			if (hContact=FindContactWithPseudoUIN(dwUIN)) {
				WRITEC_W("Status",Web2StatusToMIM(pszTemp=json_as_string(json_get(jnItem,"status"))));
				json_free(pszTemp);
				WriteClientType(hContact,json_as_int(json_get(jnItem,"client_type")));
			} else {
				QLog(__FUNCTION__"(): Contact with pseudo UIN %u could not be found!",dwUIN);
			}
		}
	} else {
		QLog(__FUNCTION__"(): retcode!=0!");
	}
}

void CProtocol::GetGroupInfoExt2(CWebQQ2* webqq2, LPSTR pszArgs, JSONNODE* jn) {
	/*
{
   "retcode":0,
   "result":{
      "stats":[ // 35
         {"client_type":1, "uin":3272588127, "stat":30},
         {"client_type":1, "uin":1580006950, "stat":30},
         {"client_type":1, "uin":1869225485, "stat":70},
		 ...
      ],
      "minfo":[ // 307
         {"nick":"浅紫色的地狱", "province":"", "gender":"unknown", "uin":3255728089, "country":"", "city":""},
         {"nick":"心之镜", "province":"河南", "gender":"male", "uin":2989632540, "country":"中国", "city":"郑州"},
         {"nick":"就许一个愿，", "province":"湖北", "gender":"male", "uin":2806275235, "country":"中国", "city":"黄冈"},
		 ...
      ],
      "ginfo":{
         "face":0,
         "memo":"库拉啦 11月15日生日快乐~~~\n群规：http://tieba.baidu.com/f?kz=849244116\n\n",
         "class":317,
         "fingermemo":"http://tieba.baidu.com/f?kz=601567359\r\n需要看完上述群规才能入群，否则不通过验证",
         "code":1933612015,
         "createtime":1219576634,
         "flag":34603025,
         "level":1,
         "name":"叶色的闭锁空间",
         "gid":867899790,
         "owner":373816670,
         "members":[ // 307
            {"muin":3255728089, "mflag":0},
            {"muin":2989632540, "mflag":8},
            {"muin":2806275235, "mflag":0},
			...
         ],
         "option":2
      },
      "cards":[ // 163
         {"muin":4181579314, "card":"糟糕c"},
         {"muin":2070899654, "card":"虾"},
         {"muin":3709177032, "card":"追迹者-苹蝈蝈"},
		 ...
      ],
      "vipinfo":[ // 307
         {"vip_level":0, "u":4124135409, "is_vip":0},
         {"vip_level":0, "u":3969035175, "is_vip":0},
         {"vip_level":0, "u":2956222994, "is_vip":0},
		 ...
      ]
   }
}
*/
	if (json_as_int(json_get(jn,"retcode"))==0) {
		JSONNODE* jnResult=json_get(jn,"result");
		JSONNODE* jnSection;
		JSONNODE* jnSubSection;
		JSONNODE* jnItem;
		DWORD dwUIN;
		HANDLE hContact=NULL;
		LPSTR pszTemp;
		LPSTR pszSection;
		LPGROUP_MEMBER lpGM;
		map<DWORD,LPGROUP_MEMBER> members;

		for (int c=0; c<json_size(jnResult); c++) {
			jnSection=json_at(jnResult,c);
			pszSection=json_name(jnSection);

			if (!strcmp(pszSection,"stats")) {
		        // {"client_type":1, "uin":3272588127, "stat":30},
				for (int c2=0; c2<json_size(jnSection); c2++) {
					jnItem=json_at(jnSection,c2);
					lpGM=(LPGROUP_MEMBER)mir_alloc(sizeof(GROUP_MEMBER));
					memset(lpGM,0,sizeof(GROUP_MEMBER));
					lpGM->uin=(DWORD)json_as_float(json_get(jnItem,"uin"));
					lpGM->stat=json_as_int(json_get(jnItem,"stat"));
					lpGM->client_type=json_as_int(json_get(jnItem,"client_type"));
					members[lpGM->uin]=lpGM;
				}
			} else if (!strcmp(pszSection,"minfo")) {
				// {"nick":"浅紫色的地狱", "province":"", "gender":"unknown", "uin":3255728089, "country":"", "city":""},
				for (int c2=0; c2<json_size(jnSection); c2++) {
					jnItem=json_at(jnSection,c2);
					if (!(lpGM=members[dwUIN=(DWORD)json_as_float(json_get(jnItem,"uin"))])) {
						lpGM=(LPGROUP_MEMBER)mir_alloc(sizeof(GROUP_MEMBER));
						memset(lpGM,0,sizeof(GROUP_MEMBER));
						lpGM->uin=dwUIN;
						members[dwUIN]=lpGM;
					}

					lpGM->nick=json_as_string(json_get(jnItem,"nick"));
				}
			} else if (!strcmp(pszSection,"ginfo")) {
				if (hContact=FindContactWithPseudoUIN(dwUIN=(DWORD)json_as_float(json_get(jnSection,"gid")))) {
					// gid and code should be already correct
					WRITEC_U8S("Notice",pszTemp=json_as_string(json_get(jnSection,"memo")));
					DBWriteContactSettingUTF8String(hContact,"CList","StatusMsg",pszTemp);
					json_free(pszTemp);

					WRITEC_U8S("Description",pszTemp=json_as_string(json_get(jnSection,"fingermemo")));
					json_free(pszTemp);

					WRITEC_W("Flag",json_as_int(json_get(jnSection,"flag")));
					WRITEC_D("Creator",(DWORD)json_as_float(json_get(jnSection,"owner")));
					WRITEC_W("Level",json_as_int(json_get(jnSection,"level")));
					WRITEC_W("Face",json_as_int(json_get(jnSection,"face")));
					WRITEC_D("createtime",(DWORD)json_as_float(json_get(jnSection,"createtime")));
					WRITEC_W("option",json_as_int(json_get(jnSection,"option")));

					jnSubSection=json_get(jnSection,"members");
					for (int c2=0; c2<json_size(jnSubSection); c2++) {
						jnItem=json_at(jnSubSection,c2);
						if (lpGM=group_members[(DWORD)json_as_float(json_get(jnItem,"muin"))]) {
							lpGM->mflag=(DWORD)json_as_float(json_get(jnItem,"mflag"));
						}
					}
				} else {
					QLog(__FUNCTION__"(): Contact with pseudo UIN %u could not be found!",dwUIN);
				}
			} else if (!strcmp(pszSection,"cards")) {
				// {"muin":4181579314, "card":"糟糕c"},
				for (int c2=0; c2<json_size(jnSection); c2++) {
					jnItem=json_at(jnSection,c2);
					if (lpGM=group_members[(DWORD)json_as_float(json_get(jnItem,"muin"))]) {
						lpGM->card=json_as_string(json_get(jnItem,"card"));
					}
				}
			} else {
				QLog(__FUNCTION__"(): Ignored unknown section %s",pszSection);
			}

			json_free(pszSection);
		}

	} else {
		QLog(__FUNCTION__"(): retcode!=0!");
	}

}

// Poll2
void CProtocol::Poll2(CWebQQ2* webqq2, LPSTR pszArgs, JSONNODE* jn) {
	QLog("Poll2: args=%s",pszArgs);

	for (map<LPCSTR,CallbackFunc>::iterator iter=poll2_callbacks.begin(); iter!=poll2_callbacks.end(); iter++) {
		if (!strcmp(pszArgs,iter->first)) {
			(this->*(iter->second))(webqq2, pszArgs,jn);
			break;
		}
	}
}

void CProtocol::Poll2_message(CWebQQ2* webqq2, LPSTR pszArgs, JSONNODE* jn) {
	if (DWORD from_uin=json_as_float(json_get(jn,"from_uin"))) {
		if (HANDLE hContact=FindContactWithPseudoUIN(from_uin)) {
			DWORD msg_id=json_as_float(json_get(jn,"msg_id"));
			if (CheckDuplicatedMessage(msg_id)) return;

			DWORD send_time=json_as_float(json_get(jn,"time")); // qqid
			string str=Web2ParseMessage(json_get(jn,"content"),NULL,msg_id,from_uin);

			PROTORECVEVENT pre={PREF_UTF};
			CCSDATA ccs={hContact,PSR_MESSAGE,NULL,(LPARAM)&pre};

			pre.timestamp=send_time+600<READ_D2(NULL,"LoginTS")?send_time:(DWORD)time(NULL);
			pre.szMessage=(char*)str.c_str();

			CallService(MS_PROTO_CHAINRECV, 0, (LPARAM)&ccs);
		} else
			QLog(__FUNCTION__"(): ERROR: hContact==NULL!");
	} else
		QLog(__FUNCTION__"(): ERROR: from_uin==0!");
}

void CProtocol::Poll2_group_message(CWebQQ2* webqq2, LPSTR pszArgs, JSONNODE* jn) {
	if (DWORD from_uin=json_as_float(json_get(jn,"from_uin"))) {
		if (CheckDuplicatedMessage(json_as_float(json_get(jn,"msg_id2")))) return;

		if (HANDLE hContact=FindContactWithPseudoUIN(from_uin)) {
			DWORD group_code=json_as_float(json_get(jn,"group_code")); // extid
			if (!READC_B2("Updated")) {
				QLog(__FUNCTION__"(): Update qun information: %u, ext=%u",group_code,READC_D2(QQ_INFO_EXTID));
				if (m_webqq->web2_api_get_group_info_ext(READC_D2(QQ_INFO_EXTID))) WRITEC_B("Updated",1);
			}
			DWORD send_uin=json_as_float(json_get(jn,"send_uin")); // qqid
			DWORD send_time=json_as_float(json_get(jn,"time")); // qqid
			string str=Web2ParseMessage(json_get(jn,"content"),hContact,group_code,send_uin);

			PROTORECVEVENT pre={PREF_UTF};
			CCSDATA ccs={hContact,PSR_MESSAGE,NULL,(LPARAM)&pre};

			pre.timestamp=send_time+600<READ_D2(NULL,"LoginTS")?send_time:(DWORD)time(NULL);
			pre.szMessage=(char*)str.c_str();
			if (READC_B2(QQ_SILENTQUN)) pre.flags|=PREF_CREATEREAD;

			CallService(MS_PROTO_CHAINRECV, 0, (LPARAM)&ccs);
		} else
			QLog(__FUNCTION__"(): ERROR: hContact==NULL!");
	} else
		QLog(__FUNCTION__"(): ERROR: from_uin==0!");
}

#define REGCB(x) callbacks[#x]=&CProtocol::x;
#define POLL2_REGCB(x) poll2_callbacks[#x]=&CProtocol::Poll2_##x;
void CProtocol::registerCallbacks() {
	//callbacks["GetFriendInfo2"]=&CProtocol::GetFriendInfo2;
	REGCB(GetFriendInfo2);
	REGCB(GetGroupListMask2);
	REGCB(GetUserFriends2);
	REGCB(GetOnlineBuddies2);
	REGCB(Poll2);

	REGCB(GetGroupInfoExt2);
	/*
	Poll2 commands:
	message, shake_message, sess_message, group_message, kick_message, file_message, system_message,
	filesrv_transfer, tips, sys_g_msg, av_request, discu_message, push_offfile, notify_offfile, input_notify
	*/
	POLL2_REGCB(message);
}

void CProtocol::WebQQ2_Callback(CWebQQ2* webqq2, LPCSTR pcszCommand, LPSTR pszArgs, JSONNODE* jn) {
	// pszArgs can be number if pszArgs[0]==1
	// i.e. if (*pszArgs==1) val=*(LPDWORD)pcszCommand & 0x0fffffff
	// TODO: Any problem?

	QLog("WebQQ2CB: cmd=%s args=%s",pcszCommand,pszArgs);

	for (map<LPCSTR,CallbackFunc>::iterator iter=callbacks.begin(); iter!=callbacks.end(); iter++) {
		if (!strcmp(pcszCommand,iter->first)) {
			(this->*(iter->second))(webqq2, pszArgs,jn);
			break;
		}
	}
}

// Stubs
HANDLE   CProtocol::ChangeInfo( int iInfoType, void* pInfoData ){ return 0; }

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

