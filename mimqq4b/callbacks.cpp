#include "StdAfx.h"

void CProtocol::_CallbackHub(LPVOID pvObject, DWORD dwCommand, LPSTR pszArgs, LPVOID pvCustom) {
	((CProtocol*)pvObject)->CallbackHub(dwCommand,pszArgs,pvCustom);
}

void CProtocol::CallbackHub(DWORD dwCommand, LPSTR pszArgs, LPVOID pvCustom) {
	switch (dwCommand) {
		case WEBQQ_CALLBACK_CHANGESTATUS:
			{
				DWORD dwStatus=*(LPDWORD)pvCustom;
				switch (HIWORD(dwStatus)) {
					case CLibWebQQ::WEBQQ_STATUS_OFFLINE:
						// This is only called when m_webqq is destructing
						QLog("WEBQQ_CALLBACK_CHANGESTATUS: WEBQQ_STATUS_OFFLINE");

						if (m_iStatus!=ID_STATUS_OFFLINE) {
							if (!Miranda_Terminated()) {
								BroadcastStatus(ID_STATUS_OFFLINE);
								if (g_httpServer) g_httpServer->UnregisterQunImages(this);
								SetContactsOffline();
							} else
								m_iStatus=ID_STATUS_OFFLINE;

							if (m_webqq) {
								// CLibWebQQ* webqq=m_webqq;
								m_webqq=NULL;
								// delete webqq;
							}
						}
						break;
					case CLibWebQQ::WEBQQ_STATUS_ERROR:
						QLog("WEBQQ_CALLBACK_CHANGESTATUS: WEBQQ_STATUS_ERROR");

						if (m_iStatus!=ID_STATUS_OFFLINE) {
							if (!Miranda_Terminated()) {
								if (g_httpServer) g_httpServer->UnregisterQunImages(this);
								SetContactsOffline();
								BroadcastStatus(ID_STATUS_OFFLINE);
							} else
								m_iStatus=ID_STATUS_OFFLINE;

							if (m_webqq) {
								CLibWebQQ* webqq=m_webqq;
								m_webqq=NULL;
								delete webqq;
							}
						}
						break;
					case CLibWebQQ::WEBQQ_STATUS_ONLINE:
					case CLibWebQQ::WEBQQ_STATUS_INVISIBLE:
						if (m_iStatus==ID_STATUS_CONNECTING) {
							WRITE_D(NULL,"LoginTS",time(NULL));
							if (m_iDesiredStatus==ID_STATUS_AWAY) {
								SetStatus(m_iDesiredStatus);
								break;
							}
						}
						BroadcastStatus(m_iDesiredStatus);
						break;
					case CLibWebQQ::WEBQQ_STATUS_AWAY:
						BroadcastStatus(ID_STATUS_AWAY);
						break;
				}
			}
			break;
		case WEBQQ_CALLBACK_LOGINFAIL:
			{
				LPSTR pszCur=NULL, pszNext=strstr(pszArgs,"','");
				while (pszNext) {
					pszCur=pszNext+3;
					pszNext=strstr(pszCur,"','");
				}
				ProtoBroadcastAck(m_szModuleName,NULL,ACKTYPE_LOGIN,ACKRESULT_FAILED,NULL,LOGINERR_WRONGPASSWORD);
				if (pszCur) {
					*strrchr(pszCur,'\'')=0;
					LPWSTR pszMsg=mir_utf8decodeW(pszCur);
					MessageBox(NULL,pszMsg,m_tszUserName,MB_ICONERROR|MB_SYSTEMMODAL);
					mir_free(pszMsg);
				} else {
					MessageBox(NULL,TranslateT("Incorrect Login Information or Verify Code."),m_tszUserName,MB_ICONERROR);
				}

			}
			break;
		case WEBQQ_CALLBACK_NEEDVERIFY:
			strcpy((LPSTR)pvCustom,CCodeVerifyWindow(g_hInstance,NULL,pszArgs,m_webqq).GetCode());
			break;
		case WEBQQ_CALLBACK_DEBUGMESSAGE:
			QLog("%s",pszArgs);
			break;
		case WEBQQ_CALLBACK_CRASH:
			{
				WCHAR szTemp[MAX_PATH];
				MIRANDASYSTRAYNOTIFY msn={sizeof(msn),"MIMQQ4"};
				swprintf(szTemp,TranslateT("Warning: MIMQQ4 caused an unhandled exception, process phase is %d. MIMQQ4 is trying to resume operation.\nIf this doesn't work, please restart Miranda IM."),(DWORD)pvCustom);
				msn.tszInfo=szTemp;
				msn.tszInfoTitle=L"MIMQQ4";
				msn.dwInfoFlags=NIIF_ERROR|NIIF_INTERN_UNICODE;

				CallService(MS_CLIST_SYSTRAY_NOTIFY,0,(LPARAM)&msn);
			}
			break;
		case WEBQQ_CALLBACK_QUNIMGUPLOAD:
			HandleQunImgUploadStatus((LPWEBQQ_QUNUPLOAD_STATUS)pvCustom);
			break;
		case WEBQQ_CALLBACK_WEB2P2PIMGUPLOAD:
			HandleWeb2P2PImgUploadStatus((LPWEBQQ_QUNUPLOAD_STATUS)pvCustom);
			break;
		case WEBQQ_CALLBACK_WEB2:
			HandleWeb2Result(dwCommand==WEBQQ_CALLBACK_WEB2,pszArgs,(JSONNODE*)pvCustom);
			break;
		case WEBQQ_CALLBACK_WEB2_ERROR:
			if (*(LPCSTR)pvCustom=='{'){
				char szDumpFile[MAX_PATH];
				GetModuleFileNameA(NULL,szDumpFile,MAX_PATH);
				*strrchr(szDumpFile,'\\')=0;
				CreateDirectoryA(strcat(szDumpFile,"\\mimqq4dump"),NULL);
				strcat(szDumpFile,"\\");
				strcat(szDumpFile,strchr(pszArgs,'/')?strrchr(pszArgs,'/')+1:pszArgs);
				strcat(szDumpFile,".txt");

				HANDLE hFile=CreateFileA(szDumpFile,GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,CREATE_ALWAYS,0,NULL);
				DWORD dwWritten;
				LPCSTR pszResult=(LPCSTR)pvCustom;
				WriteFile(hFile,pszResult,(DWORD)strlen(pszResult),&dwWritten,NULL);
				CloseHandle(hFile);

				ShowNotification(TranslateT("Note: A packet reply was failed to parse. Check MIM\\mimqq4dump directory and submit the files to developer."),NIIF_WARNING);
			} /*else if (m_searchuin && !strcmp(pszArgs,"/api/get_group_info_ext")) {
				ProtoBroadcastAck(m_szModuleName, NULL, ACKTYPE_SEARCH, ACKRESULT_SUCCESS, (HANDLE)m_searchuin, 0);
				m_searchuin=0;
			} else if (m_searchuin && !strcmp(pszArgs,"/api/get_single_info")) {
				// m_webqq->web2_api_get_group_info_ext(m_searchuin);
				SearchQunByExtID();
			}*/
			break;
		default:
			if ((dwCommand&0xfffff000)==0xfffff000) {
				QLog("Callback command=%x",dwCommand);

				/*
				if (m_sentMessages.size()) {
					map<DWORD,HANDLE>::iterator iter=m_sentMessages.begin();
					ProtoBroadcastAck(m_szModuleName, iter->second, ACKTYPE_MESSAGE, ACKRESULT_SUCCESS, (HANDLE)iter->first, 0);
					m_sentMessages.erase(iter);
				}
				*/
			} else if ((dwCommand&0xfffff000)==0xffff0000) {
#if 0 // Web1
				// Custom callback
				switch (dwCommand & 0xfff) {
					case WEBQQ_CMD_GET_GROUP_INFO:
						HandleGroupInfo((LPWEBQQ_GROUPINFO)pvCustom);
						break;
					case WEBQQ_CMD_GET_LIST_INFO:
						HandleListInfo((LPWEBQQ_LISTINFO)pvCustom);
						break;
					case WEBQQ_CMD_GET_NICK_INFO:
						HandleNickInfo((LPWEBQQ_NICKINFO)pvCustom);
						break;
					case WEBQQ_CMD_CLASS_DATA:
						HandleClassInfo((LPWEBQQ_CLASSDATA)pvCustom);
						break;
					case WEBQQ_CMD_GET_MESSAGE:
						HandleMessage((LPWEBQQ_MESSAGE)pvCustom);
						break;
					case WEBQQ_CMD_CONTACT_STATUS:
						HandleContactStatus((LPWEBQQ_CONTACT_STATUS)pvCustom);
						break;
					case WEBQQ_CMD_SET_STATUS_RESULT:
						BroadcastStatus(m_iDesiredStatus);
						break;
					case WEBQQ_CMD_GET_USER_INFO:
						HandleUserInfo(pszArgs);
						break;
					case WEBQQ_CMD_GET_CLASS_MEMBER_NICKS:
						HandleClassMemberNicks((LPWEBQQ_NICKINFO)pvCustom);
						break;
					case WEBQQ_CMD_GET_SIGNATURE_INFO:
						HandleSignatures((LPWEBQQ_LONGNAMEINFO)pvCustom);
						break;
					case WEBQQ_CMD_GET_REMARK_INFO:
						HandleRemarkInfo((LPWEBQQ_REMARKINFO)pvCustom);
						break;
					case WEBQQ_CMD_GET_HEAD_INFO:
						HandleHeadInfo(pszArgs);
						break;
					case WEBQQ_CMD_SYSTEM_MESSAGE:
						HandleSystemMessage(pszArgs,(LPSTR)pvCustom);
						break;
						/*
					case WEBQQ_CMD_SEND_C2C_MESSAGE_RESULT:
						map<DWORD,HANDLE>::iterator iter=m_sentMessages.find(*(LPDWORD)pvCustom);
						if (iter!=m_sentMessages.end()) {
							ProtoBroadcastAck(m_szModuleName, iter->second, ACKTYPE_MESSAGE, ACKRESULT_SUCCESS, (HANDLE)iter->first, 0);
							m_sentMessages.erase(iter);
						}
						break;
						*/
				}
#endif
			}
			break;
	}
}

void CProtocol::HandleWeb2Result(bool fSuccess, LPSTR szCommand, JSONNODE* jnResult) {
	if (fSuccess) {
		if (!strcmp(szCommand,"/channel/change_status")) {
			BroadcastStatus(m_iDesiredStatus);
		} else if (!strncmp(szCommand,"/api/get_group_name_list_mask",29))
			HandleWeb2GroupNameListMask(jnResult);
		else if (!strncmp(szCommand,"/api/get_online_buddies",23))
			HandleWeb2OnlineBuddies(jnResult);
		else if (!strncmp(szCommand,"/api/get_single_long_nick",25))
			HandleWeb2SingleLongNick(jnResult);
		else if (!strncmp(szCommand,"/api/get_friend_info",20))
			HandleWeb2FriendInfo(strtoul(strchr(szCommand,':')+1,NULL,10),jnResult);
		else if (!strncmp(szCommand,"/api/get_user_friends",21))
			HandleWeb2UserFriends(jnResult);
		else if (!strncmp(szCommand,"/channel/get_online_buddies",27))
			HandleWeb2OnlineBuddies(jnResult);
		else if (!strncmp(szCommand,"/api/get_group_info_ext",23))
			HandleWeb2GroupInfoExt(jnResult);
		else if (!strncmp(szCommand,"/api/get_qq_level",17))
			HandleWeb2QQLevel(jnResult);
		else if (!strcmp(szCommand,"buddies_status_change"))
			HandleWeb2BuddiesStatusChange(jnResult);
		else if (!strcmp(szCommand,"group_message"))
			HandleWeb2GroupMessage(jnResult);
		else if (!strcmp(szCommand,"message"))
			HandleWeb2Message(jnResult);
		else if (!strcmp(szCommand,"system_message"))
			HandleWeb2SystemMessage(jnResult);
		else if (!strcmp(szCommand,"kick_message")) {
			ShowNotification(TranslateT("You have been kicked offline due to login from another location."), NIIF_ERROR);
			SetStatus(ID_STATUS_OFFLINE);
		} else
			QLog(__FUNCTION__"(): Unhandled command %s, fSuccess=%s",szCommand,fSuccess?"True":"False");
		return;
	} else {

	}
	QLog(__FUNCTION__"(): Unhandled command %s, fSuccess=%s",szCommand,fSuccess?"True":"False");
}

void CProtocol::HandleWeb2GroupNameListMask(JSONNODE* jnResult) {
	/*
	"result":{
		"gnamelist":[
			{"gid":203532792,"code":1532792,"flag":1049617,"name":"\u51C9\u5BAB\u6625\u65E5\u7684\u65E7\u5C01\u7EDD"},
			{"gid":204571213,"code":2571213,"flag":17826817,"name":"Miranda IM"},
			{"gid":2104564026,"code":24564026,"flag":17825793,"name":"\u6E2C\u8A66\u7FA4"},
			{"gid":2127766740,"code":47766740,"flag":50331665,"name":"\u53F6\u8272\u7684\u95ED\u9501\u7A7A\u95F4"},
			{"gid":2138914413,"code":58914413,"flag":1064961,"name":"\u6E2C\u8A66\u7FA42"}
		],
		"gmasklist":[
			{"gid":1000,"mask":1}
		]
	}
	*/
	JSONNODE* jnGNL=json_get(jnResult,"gnamelist");
	JSONNODE* jnNode;
	HANDLE hContact;
	int nGNL=json_size(jnGNL);
	DWORD intid;
	LPSTR pszTemp;
	QLog(__FUNCTION__"(): Group count=%d",nGNL);

	for (int c=0; c<nGNL; c++) {
		jnNode=json_at(jnGNL,c);
		if (intid=json_as_float(json_get(jnNode,"gid"))) {
			if (hContact=AddOrFindContact(intid)) {
				WRITEC_D(QQ_INFO_EXTID,json_as_float(json_get(jnNode,"code")));
				WRITEC_B("IsQun",1);
				WRITEC_B("Composed",0);
				WRITEC_B("Updated",0);
				WRITEC_D("Flag",json_as_float(json_get(jnNode,"flag")));
				WRITEC_U8S("Nick",pszTemp=json_as_string(json_get(jnNode,"name")));
				json_free(pszTemp);
				WRITEC_W(QQ_STATUS,READC_B2(QQ_SILENTQUN)?ID_STATUS_DND:ID_STATUS_ONLINE);
			} else {
				QLog(__FUNCTION__"(): Exception: Unable to add/find contact with intid=%d, possibly DB damage!",intid);
			}
		} else {
			QLog(__FUNCTION__"(): Exception: Skipped qun with intid==0!");
		}
	}
}

void CProtocol::HandleWeb2UserFriends(JSONNODE* jnResult) {
	/*
	"result":{
		"categories":[
			{"index":1,"name":"\u65B0\u589E\u7FA4\u7D44"},
			{"index":2,"name":"\u6D4B\u8BD5\u5206\u7EC4"}
		],
		"friends":[
			{"uin":4016762,"categories":0},
			{"uin":8382290,"categories":0},
			...
			{"uin":705566155,"categories":1}
		],
		"info":[
			{"uin":4016762,"nick":"\u5FC3\u4E4B\u955C","face":0,"flag":582},
			{"uin":18116063,"nick":"\u5C0F\u8DEF","face":303,"flag":8389186},
			...
			{"uin":705566155,"nick":"\u9B54\u6CD5\u5C11\u5973\u9006\u5C9B","face":0,"flag":512}
		],
		"marknames":[
			{"uin":315948499,"markname":"My Notice 2"},
			{"uin":431533686,"markname":"My Notice 1"},
			{"uin":2138914413,"markname":"Notice3_Group"}
		]
	}
	*/
	JSONNODE* jnCategory;
	JSONNODE* jnNode;
	HANDLE hContact;
	int nItems;
	DWORD qqid;
	int id;
	LPSTR pszTemp;
	LPWSTR pszGroup;

	if (READ_B2(NULL,"GroupFetched")==0) {
		if (jnCategory=json_get(jnResult,"categories")) {
			// {"index":1,"name":"\u65B0\u589E\u7FA4\u7D44"},
			WRITE_B(NULL,"GroupFetched",1);
			nItems=json_size(jnCategory);
			QLog(__FUNCTION__"(): Categories count=%d",nItems);

			for (int c=0; c<nItems; c++) {
				jnNode=json_at(jnCategory,c);
				if (pszTemp=json_as_string(json_get(jnNode,"name"))) {
					if ((id=FindGroupByName(pszTemp))==-1) {
						pszGroup=mir_utf8decodeW(pszTemp);
						id=CallService(MS_CLIST_GROUPCREATE,0,(LPARAM)pszGroup);
						mir_free(pszGroup);
					}
					m_groups[json_as_int(json_get(jnNode,"index"))]=id;
					json_free(pszTemp);
				} else {
					QLog(__FUNCTION__"(): ERROR: name==NULL!");
				}
			}
		}

		if (jnCategory=json_get(jnResult,"friends")) {
			// {"uin":4016762,"categories":0},
			nItems=json_size(jnCategory);
			QLog(__FUNCTION__"(): Friends count=%d",nItems);

			for (int c=0; c<nItems; c++) {
				jnNode=json_at(jnCategory,c);
				if (qqid=json_as_float(json_get(jnNode,"uin"))) {
					if (hContact=AddOrFindContact(qqid)) {
						if (/*READC_W2(QQ_STATUS)==0 &&*/ m_groups[qqid]!=0 && (qqid=json_as_int(json_get(jnNode,"categories")))>0) {
							// This contact is new, move group
							CallService(MS_CLIST_CONTACTCHANGEGROUP,(WPARAM)hContact,(LPARAM)m_groups[qqid]);
						}
					} else {
						QLog(__FUNCTION__"(): ERROR: Failed to add/find contact with qqid==%u, possibly DB Damage!",qqid);
					}
				} else {
					QLog(__FUNCTION__"(): ERROR: qqid==0!");
				}
			}
		}
	} else
		QLog(__FUNCTION__"(): Ignored group information");

	if (jnCategory=json_get(jnResult,"info")) {
		// {"uin":4016762,"nick":"\u5FC3\u4E4B\u955C","face":0,"flag":582},
		nItems=json_size(jnCategory);
		QLog(__FUNCTION__"(): Info count=%d",nItems);

		for (int c=0; c<nItems; c++) {
			jnNode=json_at(jnCategory,c);
			if (qqid=json_as_float(json_get(jnNode,"uin"))) {
				if (hContact=AddOrFindContact(qqid)) {
					WRITEC_U8S("Nick",pszTemp=json_as_string(json_get(jnNode,"nick")));
					json_free(pszTemp);
					WRITEC_W("Face",json_as_int(json_get(jnNode,"face")));
					WRITEC_D("Flag",json_as_int(json_get(jnNode,"flag")));
				} else {
					QLog(__FUNCTION__"(): ERROR: Failed to add/find contact with qqid==%u!",qqid);
				}
			} else {
				QLog(__FUNCTION__"(): ERROR: qqid==0!");
			}
		}
	}

	if (jnCategory=json_get(jnResult,"marknames")) {
		// {"uin":315948499,"markname":"My Notice 2"},
		nItems=json_size(jnCategory);
		QLog(__FUNCTION__"(): Marknames count=%d",nItems);

		for (int c=0; c<nItems; c++) {
			jnNode=json_at(jnCategory,c);
			if (qqid=json_as_float(json_get(jnNode,"uin"))) {
				if (hContact=FindContact(qqid)) {
					DBWriteContactSettingUTF8String(hContact,"CList","MyHandle",pszTemp=json_as_string(json_get(jnNode,"markname")));
					json_free(pszTemp);
				} else {
					QLog(__FUNCTION__"(): ERROR: Failed to add/find contact with qqid==%u!",qqid);
				}
			} else {
				QLog(__FUNCTION__"(): ERROR: qqid==0!");
			}
		}
	}

	hContact=NULL;
	WRITEC_B("UHDownloadReady",1);
}

void CProtocol::HandleWeb2SingleLongNick(JSONNODE* jnResult) {
	// {"retcode":0,"result":[]}
	// {"retcode":0,"result":[{"uin":431533706,"lnick":"\u662F\uFF0C\u6211\u5728\u9019\u88E1\u3002"}]}
	int nItems=json_size(jnResult);
	JSONNODE* jnNode;
	DWORD qqid;
	HANDLE hContact;
	LPSTR pszTemp;

	QLog(__FUNCTION__"(): lnick count=%d",nItems);

	for (int c=0; c<nItems; c++) {
		jnNode=json_at(jnResult,c);
		if (qqid=json_as_float(json_get(jnNode,"uin"))) {
			if ((hContact=FindContact(qqid))!=NULL || qqid==m_webqq->GetQQID()) {
				WRITEC_U8S("PersonalSignature", pszTemp=json_as_string(json_get(jnNode,"lnick")));
				if (hContact) {
					if (qqid==m_webqq->GetQQID()) WRITE_U8S(NULL,"PersonalSignature",pszTemp);
					DBWriteContactSettingUTF8String(hContact,"CList","StatusMsg",pszTemp);
				}
				json_free(pszTemp);
			} else {
				QLog(__FUNCTION__"(): ERROR: Failed to find contact with qqid==%u!",qqid);
			}
		} else {
			QLog(__FUNCTION__"(): ERROR: qqid==0!");
		}
	}
}

void CProtocol::HandleWeb2OnlineBuddies(JSONNODE* jnResult) {
	/*
	"result":[
		{"uin":315948499,"status":"busy","client_type":1},
		{"uin":431533686,"status":"online","client_type":1}
	]
	*/

	JSONNODE* jnNode;
	int nItems=json_size(jnResult);
	HANDLE hContact;
	DWORD qqid;
	LPSTR pszTemp;

	QLog(__FUNCTION__"(): Online buddies=%d",nItems);

	for (int c=0; c<nItems; c++) {
		jnNode=json_at(jnResult,c);
		if (qqid=json_as_float(json_get(jnNode,"uin"))) {
			if (hContact=FindContact(qqid)) {
				WRITEC_W("Status",Web2StatusToMIM(pszTemp=json_as_string(json_get(jnNode,"status"))));
				json_free(pszTemp);
				// WRITEC_B("ClientType",json_as_int(json_get(jnNode,"client_type")));
				WriteClientType(hContact,json_as_int(json_get(jnNode,"client_type")));
			} else {
				QLog(__FUNCTION__"(): Failed to find contact with qqid=%u!",qqid);
			}
		} else {
			QLog(__FUNCTION__"(): ERROR: qqid==0!");
		}
	}

	hContact=NULL;
	if (READC_B2("UHDownloadReady")) {
		WRITEC_B("UHDownloadReady",0);
		GetAllAvatars();
	}
}

void CProtocol::HandleWeb2FriendInfo(DWORD uin, JSONNODE* jnResult) {
	/*
	{
		"stat":"offline",
		"nick":"^_^2",
		"country":"",
		"province":"",
		"city":"",
		"gender":"unknown",
		"face":0,
		"birthday":{"year":0,"month":0,"day":0},
		"allow":1,
		"blood":0,
		"shengxiao":0,
		"constel":0,
		"phone":"-",
		"mobile":"-",
		"email":"431533706@qq.com",
		"occupation":"0",
		"college":"-",
		"homepage":"-",
		"personal":"\u6DB4\u6A21\u9CF4\u7AED\u64F1,\u59A6\u7E6B\u98F2\u7FB6\u8844\u96B1\u72DF\uFE5D"
	}
	*/
	int nItems=json_size(jnResult);
	int nTemp;
	// WCHAR szTemp[MAX_PATH];
	JSONNODE* jnItem;
	LPSTR pszName;
	LPSTR pszValue;

	QLog(__FUNCTION__"(): Single Info Item Count=%d",nItems);

	HANDLE hContact=FindContact(uin);

	if (!hContact && uin!=m_webqq->GetQQID()) {
		QLog(__FUNCTION__"() Contact %u not in list! Ignoring",uin);
	} else {
		for (int d=0; d<2; d++) {
			for (int c=0; c<nItems; c++) {
				jnItem=json_at(jnResult,c);
				pszName=json_name(jnItem);
				pszValue=NULL;

				if (!strcmp(pszName,"nick")) WRITEC_U8S("Nick",pszValue=json_as_string(jnItem)); // Ok
				else if (!strcmp(pszName,"country")) WRITEC_U8S("Country",pszValue=json_as_string(jnItem)); // OK
				else if (!strcmp(pszName,"province")) { WRITEC_U8S("Province",pszValue=json_as_string(jnItem)); WRITEC_U8S("State",pszValue); }
				else if (!strcmp(pszName,"city")) WRITEC_U8S("City",pszValue=json_as_string(jnItem)); // OK
				else if (!strcmp(pszName,"gender")) WRITEC_B("Gender",*(pszValue=json_as_string(jnItem))=='m'?'M':*pszValue=='f'?'F':'?');
				else if (!strcmp(pszName,"face")) WRITEC_W("Face",json_as_int(jnItem));
				else if (!strcmp(pszName,"birthday")) {
					// "birthday":{"year":0,"month":0,"day":0},
					WRITEC_W("BirthYear",json_as_int(json_get(jnItem,"year")));
					WRITEC_B("BirthMonth",json_as_int(json_get(jnItem,"month")));
					WRITEC_B("BirthDay",json_as_int(json_get(jnItem,"day")));
				}
				else if (!strcmp(pszName,"allow")) WRITEC_B("AuthType",json_as_int(jnItem));
				else if (!strcmp(pszName,"blood")) {
					WRITEC_B("Blood",nTemp=json_as_int(jnItem));
					WRITEC_TS("Past0",TranslateT("Blood Type"));
					WRITEC_U8S("Past0Text",nTemp==0?"?":nTemp==1?"A":nTemp==2?"B":nTemp==3?"O":"AB");
				}
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
				else if (!strcmp(pszName,"phone")) { WRITEC_U8S("Telephone",pszValue=json_as_string(jnItem)); WRITEC_U8S("Phone",pszValue); }
				else if (!strcmp(pszName,"mobile")) { WRITEC_U8S("Mobile",pszValue=json_as_string(jnItem)); WRITEC_U8S("Cellular",pszValue); }
				else if (!strcmp(pszName,"email")) { WRITEC_U8S("Email",pszValue=json_as_string(jnItem)); WRITEC_U8S("e-mail",pszValue); }
				else if (!strcmp(pszName,"occupation")) { WRITEC_U8S("Occupation2",pszValue=json_as_string(jnItem)); WRITEC_U8S("CompanyPosition",pszValue=json_as_string(jnItem)); }
				else if (!strcmp(pszName,"college")) {
					WRITEC_U8S("College",pszValue=json_as_string(jnItem));
					WRITEC_TS("Past3",TranslateT("Graduation College"));
					WRITEC_U8S("Past3Text",pszValue);
				}
				else if (!strcmp(pszName,"homepage")) WRITEC_U8S("Homepage",pszValue=json_as_string(jnItem)); // OK
				else if (!strcmp(pszName,"personal")) WRITEC_U8S("About",pszValue=json_as_string(jnItem));
				else if (hContact!=NULL && !strcmp(pszName,"stat")) WRITEC_W("Status",Web2StatusToMIM(pszValue=json_as_string(jnItem)));
				else QLog(__FUNCTION__"(): Ignored unknown entity %s",pszName);

				if (pszValue) json_free(pszValue);
				json_free(pszName);
			}
			if (hContact) {
				ProtoBroadcastAck(m_szModuleName,hContact,ACKTYPE_GETINFO,ACKRESULT_SUCCESS,(HANDLE)1,0);
			}
			if (!hContact || uin!=m_webqq->GetQQID()) 
				break;
			else
				hContact=NULL;
		}
	}
}

void CProtocol::HandleWeb2GroupInfoExt(JSONNODE* jnResult) {
	/*
	"result":{
		"ginfo":{
			"gid":204571213,
			"code":2571213,
			"flag":17826817,
			"owner":753633,
			"name":"Miranda IM",
			"level":0,
			"face":1,
			"memo":"www.miranda-im.org\r\n\nwww.studiokuma.com/s9y\r\n\nhi.baidu.com/software10\r\n\ncnmim.d.bname.us",
			"fingermemo":"\u672C\u7FA4\u4E0D\u89E3\u7B54\u5176\u5B83\u4EFB\u4F55\u6253\u5305MIM\u95EE\u9898 \u52A0\u5165\u987B\u6709\u8F6F\u4EF6\u4F7F\u7528\u57FA\u7840\u53CADIY\u7CBE\u795E \u8FD8\u6709\u672C\u7FA4\u957F\u671F\u6210\u5458\u63A8\u8350",
			"members":[
				{"muin":82383,"mflag":4},
				{"muin":753633,"mflag":12}
			]
		},
		"minfo":[
			{"uin":82383,"nick":"\u54B8\u9C7C"},
			{"uin":753633,"nick":"J7N/yhh"}
		],
		"stats":[
			{"uin":82383,"stat":10},
			{"uin":753633,"stat":20}
		],
		"cards":[
			{"muin":61738051,"card":"\u5929\u5D0E\u306E\u53F8"},
			{"muin":22661725,"card":"\u571F\u62E8\u9F20/cat"}
		]
	}
	*/
	JSONNODE* jnCategory=json_get(jnResult,"ginfo");
	JSONNODE* jnItem;
	int nItems=json_size(jnCategory);
	DWORD uin=json_as_float(json_get(jnCategory,"gid"));
	DWORD gid=json_as_float(json_get(jnCategory,"code"));
	HANDLE hContact=NULL;
	LPSTR pszTemp;
	char szUID[16];
	char szName[MAX_PATH];
	LPSTR pszName;

	if (uin) {
		if (hContact=FindContact(uin)) {
			WRITEC_B("Updated",1);
			WRITEC_D("ExternalID",gid);
			WRITEC_W("Flag",json_as_int(json_get(jnCategory,"flag")));
			WRITEC_D("Creator",json_as_int(json_get(jnCategory,"owner")));
			WRITEC_W("Level",json_as_int(json_get(jnCategory,"level")));
			WRITEC_W("Face",json_as_int(json_get(jnCategory,"face")));
			sprintf(szName,pszName=mir_utf8encodeW(TranslateT("(QQ Qun) %s")),pszTemp=json_as_string(json_get(jnCategory,"name")));

			WRITEC_U8S("Nick",szName);
			mir_free(pszName);
			json_free(pszTemp);
			WRITEC_U8S("Notice",pszTemp=json_as_string(json_get(jnCategory,"memo")));
			DBWriteContactSettingUTF8String(hContact,"CList","StatusMsg",pszTemp);
			json_free(pszTemp);
			WRITEC_U8S("Description",pszTemp=json_as_string(json_get(jnCategory,"fingermemo")));
			json_free(pszTemp);

			jnCategory=json_get(jnResult,"cards");
			map<int,bool> cardsmap;
			nItems=json_size(jnCategory);
			QLog(__FUNCTION__"(): Qun %d has %d cards",uin,nItems);

			for (int c=0; c<nItems; c++) {
				// WARNING! Some nodes can be empty
				jnItem=json_at(jnCategory,c);
				if (json_size(jnItem)>0) {
					itoa(uin=json_as_int(json_get(jnItem,"muin")),szUID,10);
					WRITEC_U8S(szUID,pszTemp=json_as_string(json_get(jnItem,"card")));
					json_free(pszTemp);
					cardsmap[uin]=true;
				}
			}

			jnCategory=json_get(jnResult,"minfo");
			nItems=json_size(jnCategory);
			WRITEC_W("MemberCount",nItems);
			QLog(__FUNCTION__"(): Qun %d has %d members",uin,nItems);

			for (int c=0; c<nItems; c++) {
				jnItem=json_at(jnCategory,c);
				uin=json_as_int(json_get(jnItem,"uin"));
				if (!cardsmap[uin]) {
					itoa(uin,szUID,10);
					WRITEC_U8S(szUID,pszTemp=json_as_string(json_get(jnItem,"nick")));
					json_free(pszTemp);
				}
			}
		} else
			QLog(__FUNCTION__"(): ERROR: Failed to find qun contact %u",uin);

		/*
		if (m_searchuin==gid) {
			LPSTR pszValue;

			// Received search result
			QLog(__FUNCTION__"(): Received search result");
			m_searchuin=0;
			jnCategory=json_get(jnResult,"ginfo");

			PROTOSEARCHRESULT psr={sizeof(psr)};
			TCHAR uid[16];
			TCHAR szGID[16];
			_ultot(json_as_float(json_get(jnCategory,"gid")),uid,10);
			psr.id=uid;

			_ultot(gid,szGID,10);
			psr.nick=szGID;


			jnItem=json_get(jnCategory,"name");
			psr.lastName=mir_utf8decodeW(pszValue=json_as_string(jnItem));
			json_free(pszValue);

			jnItem=json_get(jnCategory,"memo");
			psr.firstName=mir_utf8decodeW(pszValue=json_as_string(jnItem));
			json_free(pszValue);

			jnItem=json_get(jnCategory,"fingermemo");
			psr.email=mir_utf8decodeW(pszValue=json_as_string(jnItem));
			json_free(pszValue);

			psr.flags=PSR_UNICODE;

			ProtoBroadcastAck(m_szModuleName, NULL, ACKTYPE_SEARCH, ACKRESULT_DATA, (HANDLE)gid, (LPARAM)&psr);
			mir_free(psr.firstName);
			mir_free(psr.lastName);
			// mir_free(psr.nick);
			mir_free(psr.email);

			ProtoBroadcastAck(m_szModuleName, NULL, ACKTYPE_SEARCH, ACKRESULT_SUCCESS, (HANDLE)gid, 0);
		}
		*/
	} else {
		QLog(__FUNCTION__"(): ERROR: gid==0!");
	}

}

void CProtocol::HandleWeb2BuddiesStatusChange(JSONNODE* jnValue) {
	// "value":{"uin":277862728,"status":"online","client_type":1}
	if (int uin=json_as_int(json_get(jnValue,"uin"))) {
		if (HANDLE hContact=FindContact(uin)) {
			LPSTR pszTemp;
			WRITEC_W("Status",Web2StatusToMIM(pszTemp=json_as_string(json_get(jnValue,"status"))));
			json_free(pszTemp);
			//WRITEC_B("ClientType",json_as_int(json_get(jnValue,"client_type")));
			WriteClientType(hContact,json_as_int(json_get(jnValue,"client_type")));
		} else
			QLog(__FUNCTION__"(): ERROR: hContact==NULL!");
	} else
		QLog(__FUNCTION__"(): ERROR: uin==0!");
}

void CProtocol::HandleWeb2GroupMessage(JSONNODE* jnValue) {
	/*
	"value":{
		"msg_id":3105,
		"from_uin":2138914413,
		"to_uin":85379868,
		"msg_id2":548752,
		"msg_type":43,
		"reply_ip":2887223212,
		"group_code":58914413,
		"send_uin":431533686,
		"seq":94,
		"time":1284998869,
		"info_seq":58914413,
		"content":[
			[
				"font",{
					"size":9,
					"color":"000000",
					"style":[0,0,0],
					"name":"\u5B8B\u4F53"
				}
			],[
				"cface",{
					"name":"E9D8263BAEE04E06D7BA65153F54F15C.jpg",
					"file_id":3873906531,
					"key":"kYcHT8iFXBGKgETy",
					"server":"124.115.1.114:443"
				}
			],
			"9\u6708\u5E95\u90FD\u6709\u54EA\u4E9B\u5B8C\u7ED3\u7684\u52A8\u753B\uFF1F  ",
			["face",73]
		]
	}
	*/
	if (DWORD from_uin=json_as_float(json_get(jnValue,"from_uin"))) {
		if (CheckDuplicatedMessage(json_as_float(json_get(jnValue,"msg_id2")))) return;

		if (HANDLE hContact=FindContact(from_uin)) {
			DWORD group_code=json_as_float(json_get(jnValue,"group_code")); // extid
			if (!READC_B2("Updated")) {
				QLog(__FUNCTION__"(): Update qun information: %u, ext=%u",group_code,READC_D2(QQ_INFO_EXTID));
				if (m_webqq->web2_api_get_group_info_ext(READC_D2(QQ_INFO_EXTID))) WRITEC_B("Updated",1);
			}
			DWORD send_uin=json_as_float(json_get(jnValue,"send_uin")); // qqid
			DWORD send_time=json_as_float(json_get(jnValue,"time")); // qqid
			string str=Web2ParseMessage(json_get(jnValue,"content"),hContact,group_code,send_uin);

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

void CProtocol::HandleWeb2Message(JSONNODE* jnValue) {
	/*
	"value":{
		"msg_id":13910,
		"from_uin":85379868,
		"to_uin":431533706,
		"msg_id2":305521,
		"msg_type":9,
		"reply_ip":2887452734,
		"time":1285235428,
		"content":[
			["font",{"size":8,"color":"000000","style":[0,0,0],"name":"Tahoma"}],
			["cface","A661B29654962A3F9744E94F951AA5FA.jpg",""]
		],
		"raw_content":"\u001532A661B29654962A3F9744E94F951AA5FA.jpgA"
	}
	*/

	if (DWORD from_uin=json_as_float(json_get(jnValue,"from_uin"))) {
		if (HANDLE hContact=FindContact(from_uin)) {
			DWORD msg_id=json_as_float(json_get(jnValue,"msg_id"));
			if (CheckDuplicatedMessage(msg_id)) return;

			DWORD send_time=json_as_float(json_get(jnValue,"time")); // qqid
			string str=Web2ParseMessage(json_get(jnValue,"content"),NULL,msg_id,from_uin);

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

void CProtocol::HandleWeb2QQLevel(JSONNODE* jnResult) {
	// {"retcode":0,"result":{"tuin":431533686,"hours":30279,"days":2063,"level":43,"remainDays":49}}
	DWORD dwUIN=json_as_float(json_get(jnResult,"tuin"));
	if (HANDLE hContact=FindContact(dwUIN)) {
		WCHAR szTemp[MAX_PATH];
		int level;
		int days, remain;
		WRITEC_D("ActiveHours",json_as_float(json_get(jnResult,"hours")));
		WRITEC_D("ActiveDays",days=json_as_float(json_get(jnResult,"days")));
		WRITEC_W("Level",level=json_as_int(json_get(jnResult,"level")));
		WRITEC_W("RemainDays",remain=json_as_int(json_get(jnResult,"remainDays")));

		WRITEC_TS("Interest0Cat",TranslateT("Level"));
		swprintf(szTemp,L"%d (%d Sun %d Moon %d Star)",level,level/16,(level%16)/4,level%4);
		WRITEC_TS("Interest0Text",szTemp);

		WRITEC_TS("Interest1Cat",TranslateT("Active Days"));
		swprintf(szTemp,L"%d (%d Hours to next level)",days,remain);
		WRITEC_TS("Interest1Text",szTemp);
	} else
		QLog(__FUNCTION__"(): ERROR: dwUIN==0!");
}

void CProtocol::HandleWeb2SystemMessage(JSONNODE* jnValue) {
	LPSTR pszType=json_as_string(json_get(jnValue,"type"));

	if (!strcmp(pszType,"verify_pass") || !strcmp(pszType,"verify_pass_add")) {
		// {"retcode":0,"result":[{"poll_type":"system_message","value":{"seq":37525,"type":"verify_pass","from_uin":431533686,"stat":20}}]}
		DWORD dwUIN=json_as_float(json_get(jnValue,"from_uin"));
		HANDLE hContact=AddOrFindContact(dwUIN);
		WCHAR szTemp[MAX_PATH];
		swprintf(szTemp,TranslateT("%u approved your authorization request!"),dwUIN);
		ShowNotification(szTemp,NIIF_INFO);
		m_webqq->web2_api_get_friend_info(dwUIN);
	} else if (!strcmp(pszType,"verify_rejected")) {
		DWORD dwUIN=json_as_float(json_get(jnValue,"from_uin"));
		LPSTR pszMsg=json_as_string(json_get(jnValue,"msg"));
		LPWSTR pwszMsg=mir_utf8decodeW(pszMsg);
		WCHAR szTemp[MAX_PATH];
		swprintf(szTemp,TranslateT("%u rejected your authorization request. reason: %s"),dwUIN,pwszMsg);
		mir_free(pwszMsg);
		json_free(pszMsg);
		ShowNotification(szTemp,NIIF_WARNING);
	} else if (!strcmp(pszType,"verify_required")) {
		DWORD dwUIN=json_as_float(json_get(jnValue,"from_uin"));

		CCSDATA ccs;
		PROTORECVEVENT pre;
		HANDLE hContact=FindContact(dwUIN);
		char* msg=json_as_string(json_get(jnValue,"msg"));
		char* szBlob;
		char* pCurBlob;

		if (!hContact) { // The buddy is not in my list, get information on buddy
			hContact=AddOrFindContact(dwUIN,true,false);
			m_webqq->web2_api_get_friend_info(dwUIN);
		}
		//util_log(0,"%s(): QQID=%d, msg=%s",__FUNCTION__,qqid,szMsg);

		ccs.szProtoService=PSR_AUTH;
		ccs.hContact=hContact;
		ccs.wParam=0;
		ccs.lParam=(LPARAM)&pre;
		pre.flags=PREF_UTF;
		pre.timestamp=(DWORD)time(NULL);
		pre.lParam=sizeof(DWORD)+4+sizeof(HANDLE)+strlen(msg)+5;

		/*blob is: uin(DWORD), hcontact(HANDLE), nick(ASCIIZ), first(ASCIIZ), last(ASCIIZ), email(ASCIIZ), reason(ASCIIZ)*/
		// Leak
		pCurBlob=szBlob=(char *)mir_alloc(pre.lParam);
		memcpy(pCurBlob,&dwUIN,sizeof(DWORD)); pCurBlob+=sizeof(DWORD);
		memcpy(pCurBlob,&hContact,sizeof(HANDLE)); pCurBlob+=sizeof(HANDLE);
		strcpy((char *)pCurBlob," "); pCurBlob+=2;
		strcpy((char *)pCurBlob," "); pCurBlob+=2;
		strcpy((char *)pCurBlob," "); pCurBlob+=2;
		strcpy((char *)pCurBlob," "); pCurBlob+=2;
		//strcpy((char *)pCurBlob,szMsg);
		strcpy((char *)pCurBlob,msg);
		pre.szMessage=(char *)szBlob;

		CallService(MS_PROTO_CHAINRECV,0,(LPARAM)&ccs);
	} else {
		QLog(__FUNCTION__"(): Type %s not handled!",pszType);
	}

	json_free(pszType);
}

#if 0 // Web1
void CProtocol::HandleGroupInfo(LPWEBQQ_GROUPINFO lpGI) {
	int id;
	LPWSTR pszGroup;

	if (READ_B2(NULL,"GroupFetched")==0) {
		WRITE_B(NULL,"GroupFetched",1);
		while (lpGI) {
			if ((id=FindGroupByName(lpGI->name))==-1) {
				pszGroup=mir_utf8decodeW(lpGI->name);
				id=CallService(MS_CLIST_GROUPCREATE,0,(LPARAM)pszGroup);
				mir_free(pszGroup);
			}
			m_groups[lpGI->index]=id;
			lpGI=lpGI->next;
		}
	}
}

void CProtocol::HandleListInfo(LPWEBQQ_LISTINFO lpLI) {
	HANDLE hContact;
	DWORD dwQQList[20];
	int dwIndex=0;

	while (lpLI) {
		if (hContact=AddOrFindContact(lpLI->qqid)) {
			if (!lpLI->isqun && READC_W2(QQ_STATUS)==0) {
				// This contact is new, move group
				CallService(MS_CLIST_CONTACTCHANGEGROUP,(WPARAM)hContact,(LPARAM)m_groups[lpLI->group]);
			}
			WRITEC_B("IsQun",lpLI->isqun?1:0);
			/*
			if (lpLI->isqun) {
				WRITEC_B("QunInit",0);
				WRITEC_B("QunInitBase",0);
				m_qunsToInit++;
			}
			*/

			if (!lpLI->isqun/* && lpLI->status!=CLibWebQQ::WEBQQ_PROTOCOL_STATUS_OFFLINE*/) {
				WRITEC_W(QQ_STATUS,MapStatus(lpLI->status));
				WRITEC_W("TermStatus",lpLI->termstatus);
				dwQQList[dwIndex++]=lpLI->qqid;
				if (dwIndex>=20) {
					m_webqq->GetLongNames(dwIndex,dwQQList);
					dwIndex=0;
				}
			} else {
				WRITEC_B("Composed",0);
				WRITEC_B("Updated",0);
				WRITEC_W(QQ_STATUS,READC_B2(QQ_SILENTQUN)?ID_STATUS_DND:ID_STATUS_ONLINE);
			}
		}
		lpLI=lpLI->next;
	}

	if (dwIndex>0) m_webqq->GetLongNames(dwIndex,dwQQList);
}

void CProtocol::HandleNickInfo(LPWEBQQ_NICKINFO lpNI) {
	// NOTE: WebQQ Sometimes returns this packet before contact list! We must handle it also, otherwise the contacts will not have names!
	HANDLE hContact;

	while (lpNI) {
		if (hContact=AddOrFindContact(lpNI->qqid)) {
			WRITEC_W("Age",lpNI->age);
			WRITEC_W("Face",lpNI->face);
			WRITEC_W("Gender",lpNI->male?'M':'F');
			WRITEC_U8S("Nick",m_webqq->DecodeText(lpNI->name));
			lpNI->viplevel;
		}
		lpNI=lpNI->next;
	}
}

void CProtocol::HandleClassInfo(LPWEBQQ_CLASSDATA lpCD) {
	switch (lpCD->subcommand) {
		case WEBQQ_CLASS_SUBCOMMAND_CLASSINFO:
			{
				LPWEBQQ_CLASSINFO lpCI=(LPWEBQQ_CLASSINFO)lpCD;
				if (HANDLE hContact=FindContact(lpCI->intid)) {
					LPWSTR pszName;
					WCHAR szName[MAX_PATH];
					/*
					DWORD dwMembers[200];
					map<DWORD,DWORD[200]>::iterator iter=m_qunMemberInit.find(lpCI->intid);

					if (iter!=m_qunMemberInit.end())
						memcpy(dwMembers,&(iter->second),sizeof(DWORD)*200);
					else
						memset(dwMembers,0,sizeof(DWORD)*200);
					*/

					if (lpCI->haveinfo) {
						WRITEC_D(QQ_INFO_EXTID,lpCI->extid);
						swprintf(szName,TranslateT("(QQ Qun) %s"),pszName=mir_utf8decodeW(m_webqq->DecodeText(lpCI->name)));
						WRITEC_TS("Nick",szName);
						mir_free(pszName);

						// Leave to web2 because of super qun
						/*
						WRITEC_D("Creator",lpCI->creator);
						WRITEC_U8S("Description",m_webqq->DecodeText(lpCI->desc));
						WRITEC_U8S("Notice",m_webqq->DecodeText(lpCI->notice));
						DBWriteContactSettingUTF8String(hContact,"CList","StatusMsg",lpCI->notice); // already decoded
						WRITEC_D("Properties",lpCI->prop);
						*/
						WRITEC_W("Status",ID_STATUS_ONLINE);
					}
					/*
					int c=0;
					while (dwMembers[c]) c++;

					for (LPWEBQQ_CLASSMEMBER lpCM=lpCI->members; lpCM->qqid; lpCM++) {
						dwMembers[c++]=lpCM->qqid;
					}
					*/
					/*
					for (LPWEBQQ_CLASSMEMBER lpCM=lpCI->members; lpCM->qqid; lpCM++) {
						if (m_qunMemberNames.find(lpCM->qqid)==m_qunMemberNames.end())
							m_qunMemberNames[lpCM->qqid]="";
						
					}
					*/

					// Defered to web2
					/*
					if (!lpCI->havenext) {
						m_webqq->GetClassMembersRemarkInfo(lpCI->intid);
					}
					*/
				}
			}
			break;
		case WEBQQ_CLASS_SUBCOMMAND_REMARKINFO:
			{
				LPWEBQQ_CLASS_REMARKS lpCR=(LPWEBQQ_CLASS_REMARKS)lpCD;
				if (HANDLE hContact=FindContact(lpCR->qunid)) {
					for (LPWEBQQ_REMARK lpR=lpCR->remarks; lpR->qqid; lpR++) {
						WRITEC_U8S(lpR->qqid_str,m_webqq->DecodeText(lpR->name));
					}

					/*
					if (!lpCR->havenext) {
						m_qunsInited++;
						if (m_qunsInited==m_qunsToInit) {
							DWORD dwQQ[24]={0};

							QLog("Members to get name=%d",m_qunMemberNames.size());

							m_qunMembersInited=m_qunMemberNames.begin();
							for (int c=0; c<24; c++) {
								if (m_qunMembersInited==m_qunMemberNames.end()) break;

								dwQQ[c]=m_qunMembersInited->first;

								m_qunMembersInited++;
							}

							if (*dwQQ) m_webqq->GetNickInfo(dwQQ);
						} else {
							QLog("Total Quns=%d, Inited=%d",m_qunsToInit,m_qunsInited);
						}
					}
					*/
				} else
					QLog("Error: qun %u not in database!",lpCR->qunid);
			}
			break;
			/*
		case WEBQQ_CLASS_SUBCOMMAND_CLASSMESSAGERESULT:
			{
				map<DWORD,HANDLE>::iterator iter=m_sentMessages.find(lpCD->dwStub);
				if (iter!=m_sentMessages.end()) {
					ProtoBroadcastAck(m_szModuleName, iter->second, ACKTYPE_MESSAGE, ACKRESULT_SUCCESS, (HANDLE)iter->first, 0);
					m_sentMessages.erase(iter);
				}
			}
			break;
			*/
	}
}

void CProtocol::HandleMessage(LPWEBQQ_MESSAGE lpM) {
	if (lpM->type==WEBQQ_MESSAGE_TYPE_FORCE_DISCONNECT) {
		ShowNotification(TranslateT("You were logged out from QQ network due to duplicated login"),NIIF_ERROR);
		ProtoBroadcastAck(m_szModuleName,NULL,ACKTYPE_LOGIN,ACKRESULT_FAILED,NULL,LOGINERR_OTHERLOCATION);
		return;
	}

	HANDLE hContact = FindContact(lpM->sender);

	if (CheckDuplicatedMessage(lpM->sequence)) return;

	if (!hContact) {
		hContact=AddOrFindContact(lpM->sender);
		WRITEC_W("Status",ID_STATUS_ONLINE);

		if (lpM->type==WEBQQ_MESSAGE_TYPE_CLASS) WRITEC_B("IsQun",1);
	}

	PROTORECVEVENT pre={PREF_UTF};
	CCSDATA ccs={hContact,PSR_MESSAGE,NULL,(LPARAM)&pre};

	if (lpM->type==WEBQQ_MESSAGE_TYPE_CLASS && lpM->requestType==WEBQQ_MESSAGE_REQUEST_TEXT) {
		DBVARIANT dbv={0};
		pre.timestamp=(DWORD)lpM->timestamp+600<READ_D2(NULL,"LoginTS")?(DWORD)lpM->timestamp:(DWORD)time(NULL);

		if (!READC_B2("Updated")) {
			QLog(__FUNCTION__"(): Update qun information: %u",READC_D2(QQ_INFO_EXTID));
			if (m_webqq->web2_api_get_group_info_ext(READC_D2(QQ_INFO_EXTID))) WRITEC_B("Updated",1);
		}

		if (READC_U8S2(lpM->classSender_str,&dbv) || !*dbv.pszVal) {
			if (dbv.pszVal) {
				DBFreeVariant(&dbv);
				dbv.pszVal=0;
			}

			// Handled by web2 above
			/*
			map<DWORD,string>::iterator iter=m_qunMemberNames.find(lpM->classSender);
			if (iter!=m_qunMemberNames.end()) {
				if (iter->second.length()>0) {
					WRITEC_U8S(lpM->classSender_str,iter->second.c_str());
					READC_U8S2(lpM->classSender_str,&dbv);
				}
			} else {
				DWORD dwQQID[24]={lpM->classSender};
				m_qunMemberNames[lpM->classSender]="";
				m_webqq->GetNickInfo(dwQQID);
			}
			*/
		}

		int len=15/*number*/+2/*crlf*/+21/*biu*/+22/*color*/+16/*size*/+10/*safety*/+CountSmileys(lpM->text,TRUE)*10/*smileys*/+CountQunPics(lpM->text)*88/*QunImg*/+(int)strlen(lpM->text);
		if (dbv.pszVal) len+=(int)strlen(dbv.pszVal);
		LPSTR pszMsg=(LPSTR)mir_alloc(len);
		LPSTR ppszMsg=pszMsg;

		if (dbv.pszVal)
			ppszMsg+=sprintf(ppszMsg,"%s (%s):\r\n",dbv.pszVal,lpM->classSender_str);
		else
			ppszMsg+=sprintf(ppszMsg,"%s:\r\n",lpM->classSender_str);

		if (lpM->hasFormat) {
			HDC hdc=GetDC(NULL);

			// Translate pixel to pt
			int lpsy=GetDeviceCaps(hdc, LOGPIXELSY);
			ReleaseDC(NULL,hdc);

			ppszMsg+=sprintf(ppszMsg,"%s%s%s[color=%06x][size=%d]",lpM->bold?"[b]":"",lpM->italic?"[i]":"",lpM->underline?"[u]":"",lpM->color,(lpM->size>0?lpM->size:9)*lpsy/72);
		}

		ppszMsg+=sprintf(ppszMsg,lpM->text);

		if (lpM->hasFormat) {
			ppszMsg+=sprintf(ppszMsg,"[/size][/color]%s%s%s",lpM->underline?"[/u]":"",lpM->italic?"[/i]":"",lpM->bold?"[/b]":"");
		}

		DecodeSmileys(pszMsg);
		ProcessQunPics(pszMsg,READC_D2("ExternalID"),lpM->timestamp,TRUE);

		pre.szMessage=pszMsg;

		if (READC_B2(QQ_SILENTQUN)) pre.flags|=PREF_CREATEREAD;

		CallService(MS_PROTO_CHAINRECV, 0, (LPARAM)&ccs);
		mir_free(pszMsg);
		if (dbv.pszVal) DBFreeVariant(&dbv);
	} else if (lpM->type==WEBQQ_MESSAGE_TYPE_CONTACT1 && lpM->requestType==WEBQQ_MESSAGE_REQUEST_TEXT) {
		int len=21/*biu*/+22/*color*/+16/*size*/+10/*safety*/+CountSmileys(lpM->text,TRUE)*10/*smileys*/+CountQunPics(lpM->text)*331/*P2PImg*/+(int)strlen(lpM->text);
		pre.timestamp=(DWORD)lpM->timestamp+600<READ_D2(NULL,"LoginTS")?(DWORD)lpM->timestamp:(DWORD)time(NULL);

		LPSTR pszMsg=(LPSTR)mir_alloc(len);
		LPSTR ppszMsg=pszMsg;

		if (lpM->hasFormat) {
			HDC hdc=GetDC(NULL);

			// Translate pixel to pt
			int lpsy=GetDeviceCaps(hdc, LOGPIXELSY);
			ReleaseDC(NULL,hdc);

			ppszMsg+=sprintf(ppszMsg,"%s%s%s[color=%06x][size=%d]",lpM->bold?"[b]":"",lpM->italic?"[i]":"",lpM->underline?"[u]":"",lpM->color,(lpM->size>0?lpM->size:9)*lpsy/72);
		}

		ppszMsg+=sprintf(ppszMsg,lpM->text);

		if (lpM->hasFormat) {
			ppszMsg+=sprintf(ppszMsg,"[/size][/color]%s%s%s",lpM->underline?"[/u]":"",lpM->italic?"[/i]":"",lpM->bold?"[/b]":"");
		}

		DecodeSmileys(pszMsg);
		ProcessQunPics(pszMsg,lpM->sender,lpM->timestamp,FALSE);

		pre.szMessage=pszMsg;
		CallService(MS_PROTO_CHAINRECV, 0, (LPARAM)&ccs);
		mir_free(pszMsg);
	} else if ((lpM->type==WEBQQ_MESSAGE_TYPE_CONTACT1 || lpM->type==WEBQQ_MESSAGE_TYPE_CONTACT2) && lpM->requestType==WEBQQ_MESSAGE_REQUEST_FILE_APPROVE && *lpM->fileType=='c') {
		g_httpServer->HandleP2PImage(m_webqq->GetQQID(),lpM);
	}
}

void CProtocol::HandleContactStatus(LPWEBQQ_CONTACT_STATUS lpCS) {
	if (HANDLE hContact = FindContact(lpCS->qqid)) {
		WRITEC_W("Status",MapStatus(lpCS->status));
		WRITEC_W("TermStatus",lpCS->terminationStat);
		m_webqq->GetLongNames(1,&lpCS->qqid);
	}
}

#define UITYPE_WORD "\x01"
#define UITYPE_DWORD "\x02"
#define UITYPE_STRING "\x03"
#define UITYPE_SEX "\x04"
#define UITYPE_BYTE "\x05"

void CProtocol::HandleUserInfo(LPSTR pszArgs) {
	DWORD qqid=strtoul(pszArgs,NULL,10);
	HANDLE hContact;

	if ((hContact = AddOrFindContact(qqid,true,true))!=NULL || qqid==m_webqq->GetQQID()) {
		// TODO: This may be moved to global scope for details upload, which is the same format
		LPCSTR pszWriteNames[]={
			UITYPE_DWORD UNIQUEIDSETTING, // 0
			UITYPE_STRING "Nick",
			UITYPE_STRING "Country",
			UITYPE_STRING "Province",
			UITYPE_STRING "ZIP",
			UITYPE_STRING "Address",
			UITYPE_STRING "Telephone",
			UITYPE_WORD "Age",
			UITYPE_SEX "Gender",
			UITYPE_STRING "FirstName",
			UITYPE_STRING "Email", // 10
			UITYPE_STRING "PagerSN",
			UITYPE_STRING "PagerNum",
			UITYPE_STRING "PagerSP",
			UITYPE_WORD "PagerBaseNum",
			UITYPE_BYTE "PagerType",
			UITYPE_BYTE "Occupation",
			UITYPE_STRING "Homepage",
			UITYPE_BYTE "AuthType",
			UITYPE_STRING "IcqNo",
			UITYPE_STRING "IcqPwd", // 20
			UITYPE_WORD "Face",
			UITYPE_STRING "Mobile",
			UITYPE_BYTE "MobileType",
			UITYPE_STRING "About",
			UITYPE_STRING "City",
			UITYPE_STRING "SecretEmail",
			UITYPE_STRING "IDCard",
			UITYPE_BYTE "GSMType",
			UITYPE_BYTE "OpenHP",
			UITYPE_BYTE "OpenContact", // 30
			UITYPE_STRING "College",
			UITYPE_BYTE "Horoscope",
			UITYPE_BYTE "Zodiac",
			UITYPE_BYTE "Blood", 
			UITYPE_DWORD "QQLevel",
			UITYPE_STRING "Unknown6", // 36
			NULL
		};

		LPSTR pszEnd=m_webqq->GetArgument(pszArgs,CLibWebQQ::WEBQQ_USER_DETAIL_ENDOFTABLE);
		int blocksize=pszEnd-pszArgs+1;
		LPSTR pszLocal=(LPSTR)mir_alloc(blocksize);
		LPSTR pszCurrent=pszLocal;
		LPSTR pszNext;
		HANDLE hContact2=hContact;
		memcpy(pszLocal,pszArgs,blocksize);

		for (int c=0; c==0 || (c==1 && qqid==m_webqq->GetQQID()); c++) {
			pszCurrent=pszLocal;
			for (LPCSTR* pszName=pszWriteNames; *pszName; *pszName++) {
				pszNext=pszCurrent+strlen(pszCurrent)+1;
				switch (**pszName) {
					case 0x01 /*UITYPE_WORD*/:
						WRITEC_W(*pszName+1,atoi(pszCurrent));
						break;
					case 0x02 /*UITYPE_DWORD*/:
						WRITEC_D(*pszName+1,strtoul(pszCurrent,NULL,10));
						break;
					case 0x05 /*UITYPE_BYTE*/:
						WRITEC_B(*pszName+1,atoi(pszCurrent));
						break;
					case 0x04 /*UITYPE_SEX*/:
						WRITEC_B(*pszName+1,strcmp(pszCurrent,"男")?strcmp(pszCurrent,"女")?'?':'F':'M');
						break;
					case 0x03 /*UITYPE_STRING*/:
						WRITEC_U8S(*pszName+1,m_webqq->DecodeText(pszCurrent));
						break;
					default:
						QLog("Error: Undefined data type 0x%02x for user detail '%s'",*(LPBYTE)**pszName,*pszName+1);
						break;
				}

				pszCurrent=pszNext;
			}
			if (hContact==NULL) break;
			hContact=NULL;
		}
		mir_free(pszLocal);

		ProtoBroadcastAck(m_szModuleName, hContact2, ACKTYPE_GETINFO, ACKRESULT_SUCCESS, (HANDLE)1, 0);
	}
}

void CProtocol::HandleClassMemberNicks(LPWEBQQ_NICKINFO lpNI) {
	// This function no longer used
	/*
	while (lpNI) {
		m_qunMemberNames[lpNI->qqid]=lpNI->name;
		lpNI=lpNI->next;
	}
	*/

	/*
	DWORD dwQQ[24]={0};

	for (int c=0; c<24; c++) {
		if (m_qunMembersInited==m_qunMemberNames.end()) break;

		dwQQ[c]=m_qunMembersInited->first;

		m_qunMembersInited++;
	}

	if (*dwQQ) m_webqq->GetNickInfo(dwQQ);
	*/
}

void CProtocol::HandleSignatures(LPWEBQQ_LONGNAMEINFO lpLNI) {
	HANDLE hContact;
	while (lpLNI) {
		if (hContact=FindContact(lpLNI->index)) {
			WRITEC_U8S("Signature",m_webqq->DecodeText(lpLNI->name));
			DBWriteContactSettingUTF8String(hContact,"CList","StatusMsg",lpLNI->name);
		}
		if (lpLNI->index==m_webqq->GetQQID()) {
			hContact=NULL;
			WRITEC_U8S("Signature",m_webqq->DecodeText(lpLNI->name));
			DBWriteContactSettingUTF8String(hContact,"CList","StatusMsg",lpLNI->name);
		}
		lpLNI=lpLNI->next;
	}
}

void CProtocol::HandleRemarkInfo(LPWEBQQ_REMARKINFO lpRI) {
	HANDLE hContact;
	DBVARIANT dbv;

	while (lpRI) {
		if (hContact=FindContact(lpRI->index)) {
			if (*lpRI->name) {
				if (DBGetContactSettingUTF8String(hContact,"CList","MyHandle",&dbv))
					DBWriteContactSettingUTF8String(hContact,"CList","MyHandle",m_webqq->DecodeText(lpRI->name));
				else
					DBFreeVariant(&dbv);
			} else
				DBDeleteContactSetting(hContact,"CList","MyHandle");
		}
		lpRI=lpRI->next;
	}
}

void CProtocol::HandleHeadInfo(LPSTR pszArgs) {
	/*
	WCHAR szPath[MAX_PATH];
	char szFile[MAX_PATH];
	HANDLE hContact=NULL;
	FoldersGetCustomPathW(m_folders[FOLDER_AVATARS],szPath,MAX_PATH,L"QQ");

	sprintf(szFile,"%S\\%u.jpg",szPath,m_webqq->GetQQID());
	WRITEC_U8S("PendingAvatar",szFile);
	CreateThreadObj(&CProtocol::FetchAvatar,NULL);
	*/
	GetAllAvatars();
}

void CProtocol::HandleSystemMessage(LPSTR pszType, LPSTR pszArgs) {
	if (!strcmp(pszType,"3")) {
		DWORD dwUIN;
		HANDLE hContact=AddOrFindContact(dwUIN=strtoul(pszArgs,NULL,10));
		WCHAR szTemp[MAX_PATH];
		swprintf(szTemp,TranslateT("%s approved your authorization request!"),pszArgs);
		ShowNotification(szTemp,NIIF_INFO);
		m_webqq->web2_api_get_single_info(dwUIN);
	} else
		QLog(__FUNCTION__"(): Unknown message type %s",pszType);
}
#endif // Web1

bool CProtocol::CheckDuplicatedMessage(DWORD seq) {
	for (int c=0; c<MIMQQ4_MSG_CACHE_SIZE; c++) {
		if (m_msgseq[c]==seq) {
			QLog(__FUNCTION__"(): Ignored duplicate message #%u",seq);
			return true;
		}
	}

	m_msgseq[m_msgseqCurrent++]=seq;
	if (m_msgseqCurrent>=MIMQQ4_MSG_CACHE_SIZE) m_msgseqCurrent=0;
	return false;
}

void CProtocol::HandleWeb2P2PImgUploadStatus(LPWEBQQ_QUNUPLOAD_STATUS lpQS) {
	switch (lpQS->status) {
		case 0:
			QLog("WEB2_P2PIMGUPLOAD: QQID=%u Total Size=%u",lpQS->qqid,lpQS->number);
			break;
		case 1:
			QLog("WEB2_P2PIMGUPLOAD: QQID=%u Transferred=%u",lpQS->qqid,lpQS->number);
			break;
		case 2:
			QLog("WEB2_P2PIMGUPLOAD: QQID=%u Waiting Response",lpQS->qqid);
			break;
		case 3:
			QLog("WEB2_P2PIMGUPLOAD: QQID=%u File Name=%s",lpQS->qqid,lpQS->string);
			if (HANDLE hContact=FindContact(lpQS->qqid)) {
				char szMsg[512];
				
				sprintf(szMsg,"[img]http://127.0.0.1:%u/web2p2pimg/%s",g_httpServer->GetPort(),lpQS->string);
				g_httpServer->RegisterQunImage(strstr(szMsg,"/web2p2pimg"),this);
				strcat(szMsg,"[/img]");
				CallService(MS_MSG_SENDMESSAGE,(WPARAM)hContact,(LPARAM)szMsg);
			}
			break;
		case 4:
			{
				QLog("WEBQQ_CALLBACK_QUNIMGUPLOAD: QQID=%u Fail Response=%s",lpQS->qqid,lpQS->string);
				POPUPDATAW pd={FindContact(lpQS->qqid)};
				LPWSTR pszMsg=mir_utf8decodeW(lpQS->string);
				pd.lchIcon=LoadIcon(g_hInstance,MAKEINTRESOURCE(IDI_ICON1));
				wcscpy(pd.lptzContactName,m_tszUserName);
				wcscpy(pd.lptzText,pszMsg);
				PUAddPopUpW(&pd);
				mir_free(pszMsg);
			}
			break;
		default:
			QLog("WEBQQ_CALLBACK_QUNIMGUPLOAD: QQID=%u ???",lpQS->qqid);
			break;
	}
}

void CProtocol::HandleQunImgUploadStatus(LPWEBQQ_QUNUPLOAD_STATUS lpQS) {
	switch (lpQS->status) {
		case 0:
			QLog("WEBQQ_CALLBACK_QUNIMGUPLOAD: QQID=%u Total Size=%u",lpQS->qqid,lpQS->number);
			break;
		case 1:
			QLog("WEBQQ_CALLBACK_QUNIMGUPLOAD: QQID=%u Transferred=%u",lpQS->qqid,lpQS->number);
			break;
		case 2:
			QLog("WEBQQ_CALLBACK_QUNIMGUPLOAD: QQID=%u Waiting Response",lpQS->qqid);
			break;
		case 3:
			QLog("WEBQQ_CALLBACK_QUNIMGUPLOAD: QQID=%u File Name=%s",lpQS->qqid,lpQS->string);
			if (HANDLE hContact=FindContact(lpQS->qqid)) {
				char szMsg[MAX_PATH];
				
				sprintf(szMsg,"[img]http://127.0.0.1:%u/cgi-bin/webqq_app/?cmd=2&bd=%s",g_httpServer->GetPort(),lpQS->string);
				g_httpServer->RegisterQunImage(strstr(szMsg,"/cgi-bin"),this);
				strcat(szMsg,"[/img]");
				CallService(MS_MSG_SENDMESSAGE,(WPARAM)hContact,(LPARAM)szMsg);
			}
			break;
		case 4:
			{
				QLog("WEBQQ_CALLBACK_QUNIMGUPLOAD: QQID=%u Fail Response=%s",lpQS->qqid,lpQS->string);
				POPUPDATAW pd={FindContact(lpQS->qqid)};
				LPWSTR pszMsg=mir_utf8decodeW(lpQS->string);
				pd.lchIcon=LoadIcon(g_hInstance,MAKEINTRESOURCE(IDI_ICON1));
				wcscpy(pd.lptzContactName,m_tszUserName);
				wcscpy(pd.lptzText,pszMsg);
				PUAddPopUpW(&pd);
				mir_free(pszMsg);
			}
			break;
		default:
			QLog("WEBQQ_CALLBACK_QUNIMGUPLOAD: QQID=%u ???",lpQS->qqid);
			break;
	}
}
