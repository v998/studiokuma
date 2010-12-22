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
					case CLibWebFetion::WEBQQ_STATUS_OFFLINE:
						// This is only called when m_webqq is destructing
						QLog("WEBQQ_CALLBACK_CHANGESTATUS: WEBQQ_STATUS_OFFLINE");

						if (m_iStatus!=ID_STATUS_OFFLINE) {
							if (!Miranda_Terminated()) {
								BroadcastStatus(ID_STATUS_OFFLINE);
								SetContactsOffline();
							} else
								m_iStatus=ID_STATUS_OFFLINE;

							if (m_webqq) {
								// CLibWebFetion* webqq=m_webqq;
								m_webqq=NULL;
								// delete webqq;
							}
						}
						break;
					case CLibWebFetion::WEBQQ_STATUS_ERROR:
						QLog("WEBQQ_CALLBACK_CHANGESTATUS: WEBQQ_STATUS_ERROR");

						if (m_iStatus!=ID_STATUS_OFFLINE) {
							if (!Miranda_Terminated()) {
								SetContactsOffline();
								BroadcastStatus(ID_STATUS_OFFLINE);
							} else
								m_iStatus=ID_STATUS_OFFLINE;

							if (m_webqq) {
								CLibWebFetion* webqq=m_webqq;
								m_webqq=NULL;
								delete webqq;
							}
						}
						break;
					case CLibWebFetion::WEBQQ_STATUS_ONLINE:
					case CLibWebFetion::WEBQQ_STATUS_INVISIBLE:
						if (m_iStatus==ID_STATUS_CONNECTING) {
							WRITE_D(NULL,"LoginTS",time(NULL));
							if (m_iDesiredStatus==ID_STATUS_AWAY) {
								SetStatus(m_iDesiredStatus);
								break;
							}
						}
						BroadcastStatus(m_iDesiredStatus);
						break;
					case CLibWebFetion::WEBQQ_STATUS_AWAY:
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
		if (!strcmp(szCommand,"GetPersonalInfo"))
			HandleWebIMPersonalInfo(jnResult);
		else if (!strcmp(szCommand,"GetContactList"))
			HandleWebIMContactList(jnResult);
		else if (!strncmp(szCommand,"GetConnect:",11))
			HandleWebIMConnect(strtoul(strchr(szCommand,':')+1,NULL,10),jnResult);
		else
			QLog(__FUNCTION__"(): Unhandled command %s, fSuccess=%s",szCommand,fSuccess?"True":"False");
		return;
	}

	QLog(__FUNCTION__"(): Unhandled command %s, fSuccess=%s",szCommand,fSuccess?"True":"False");
}

#if 0
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
	/*
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
	*/
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
	/*
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
					WRITEC_B("Zodiac",json_as_int(jnItem));
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
	*/
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
	/*
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
		*/
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

	/*
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
		*/
}

void CProtocol::HandleWeb2QQLevel(JSONNODE* jnResult) {
	// {"retcode":0,"result":{"tuin":431533686,"hours":30279,"days":2063,"level":43,"remainDays":49}}
	/*
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
		*/
}

void CProtocol::HandleWeb2SystemMessage(JSONNODE* jnValue) {
#if 0
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
#endif
}
#endif // Web2

void CProtocol::UpdateContactInfo(HANDLE hContact, JSONNODE* jnInfo) {
	// WARNING: hContact can be NULL to update owner info, so checking of hContact left to calling function!

	int nItems=json_size(jnInfo);
	LPSTR pszName;
	JSONNODE* jnItem;
	LPSTR pszTemp;

	for (int c=0; c<nItems; c++) {
		jnItem=json_at(jnInfo,c);
		pszName=json_name(jnItem);
		pszTemp=NULL;

		// personal "bd":"1900-01-01","bdv":0,"bt":null,"ca":"","car":null,"cas":"1","crc":"0","creds":[{"c":"CRAOAABZMZnQTT2aj8IVIDTy+c3x5xcJphCiG6E2EIIerM93nU+DmYqVS5BpPw0XLBj7n3XW9fY\/9PP+2ExgacC2\/2AlnJma5VWgglhZm9xD21QKfg==","dm":"m161.com.cn"}],"ebs":2,"em":"starkwong@hotmail.com","gd":1,"gp":"identity=0;phone=0;email=1;birthday=1;business=0;presence=1;contact=1;location=3;buddy=2;ivr=2;buddy-ex=0;show=1;","i":"","lv":"0","lvs":"9","mn":"","n":"","nn":"^_^","oc":null,"pe":null,"prof":null,"sid":489952672,"sms":"365.0:0:0","sv":"9","uid":739420892,"ur":"香港.[zhCN].香港.","uri":"sip:489952672@fetion.com.cn;p=16101","v":"344478620","we":null
		// one possible "bl":"0","bss":1,"ln":"","rs":0,"uid":860538411,"uri":"sip:331809751@fetion.com.cn;p=16101","v":"0"
		// "bl":"0","ct":0,"is":"*","isBk":0,"ln":"","p":"identity=0;","rs":1,"uid":860538411,"uri":"sip:331809751@fetion.com.cn;p=16101"

		if (!strcmp(pszName,"pb")) { if (hContact) WRITEC_W("Status",MapStatus(atoi(pszTemp=json_as_string(jnItem)))); }
		else if (!strcmp(pszName,"bd")) {
			pszTemp=json_as_string(jnItem);
			WRITEC_W("BirthYear",atoi(pszTemp));
			WRITEC_B("BirthMonth",atoi(pszTemp+5));
			WRITEC_B("BirthDay",atoi(pszTemp+8));
		}
		else if (!strcmp(pszName,"mn")) { WRITEC_U8S("Mobile",pszTemp=json_as_string(jnItem)); }
		else if (!strcmp(pszName,"nn")) { WRITEC_U8S("Nick",pszTemp=json_as_string(jnItem)); }
		else if (!strcmp(pszName,"i")) { WRITEC_U8S("Impresa",pszTemp=json_as_string(jnItem)); DBWriteContactSettingUTF8String(hContact,"CList","StatusMsg",pszTemp); }
		else if (!strcmp(pszName,"sms")) { WRITEC_U8S("SMSPolicy",pszTemp=json_as_string(jnItem)); }
		else if (!strcmp(pszName,"sid")) { WRITEC_U8S("Sid",pszTemp=json_as_string(jnItem)); }
		else if (!strcmp(pszName,"crc")) {
			PROTO_AVATAR_INFORMATION pai={sizeof(pai)};
			pai.hContact=hContact;
			WRITEC_U8S("Crc",pszTemp=json_as_string(jnItem)); 
			CallContactService(hContact,PS_GETAVATARINFO,0,(LPARAM)&pai);
		}
		else if (!strcmp(pszName,"em")) { WRITEC_U8S("Email",pszTemp=json_as_string(jnItem)); }
		else if (!strcmp(pszName,"uri")) { WRITEC_U8S("Uri",pszTemp=json_as_string(jnItem)); }
		else if (!strcmp(pszName,"isBk")) { WRITEC_B("IsBlack",json_as_int(jnItem)); }
		else if (!strcmp(pszName,"ln")) { pszTemp=json_as_string(jnItem); if (*pszTemp) DBWriteContactSettingUTF8String(hContact,"CList","MyHandle",pszTemp); else DBDeleteContactSetting(hContact,"CList","MyHandle"); }
		else if (!strcmp(pszName,"rs")) { WRITEC_B("Relation",json_as_int(jnItem)); } // 0=Unconfirmed 1=Buddy 2=Declined 3=Stranger
		else
			QLog(__FUNCTION__"(): Ignored unknown entity %s",pszName);

		if (pszTemp) json_free(pszTemp);
	}
}

void CProtocol::HandleWebIMPersonalInfo(JSONNODE* jnValue) {
	DWORD uid=json_as_float(json_get(jnValue,"uid"));
	HANDLE hContact=NULL;

	UpdateContactInfo(NULL,jnValue);
	WRITEC_D(UNIQUEIDSETTING,uid);

	hContact=AddOrFindContact(uid);
	UpdateContactInfo(hContact,jnValue);
	WRITEC_W("Status",ID_STATUS_ONLINE);
	WRITEC_B("IsMe",1);
}

void CProtocol::HandleWebIMContactList(JSONNODE* jnValue) {
	JSONNODE* jnCategory;
	JSONNODE* jnNode;
	HANDLE hContact;
	int nItems;
	DWORD qqid;
	int id;
	LPSTR pszTemp;
	LPWSTR pszGroup;

	if (READ_B2(NULL,"GroupFetched")==0) {
		if (jnCategory=json_get(jnValue,"bl")) {
			WRITE_B(NULL,"GroupFetched",1);
			nItems=json_size(jnCategory);
			QLog(__FUNCTION__"(): Categories count=%d",nItems);

			for (int c=0; c<nItems; c++) {
				jnNode=json_at(jnCategory,c);
				if (pszTemp=json_as_string(json_get(jnNode,"n"))) {
					if ((id=FindGroupByName(pszTemp))==-1) {
						pszGroup=mir_utf8decodeW(pszTemp);
						id=CallService(MS_CLIST_GROUPCREATE,0,(LPARAM)pszGroup);
						mir_free(pszGroup);
					}
					m_groups[json_as_int(json_get(jnNode,"id"))]=id;
					json_free(pszTemp);
				} else {
					QLog(__FUNCTION__"(): ERROR: n==NULL!");
				}
			}
		}
	} else
		QLog(__FUNCTION__"(): Ignored group information");

	if (jnCategory=json_get(jnValue,"bds")) {
		// {"uin":4016762,"categories":0},
		nItems=json_size(jnCategory);
		QLog(__FUNCTION__"(): Friends count=%d",nItems);

		for (int c=0; c<nItems; c++) {
			jnNode=json_at(jnCategory,c);
			if (qqid=json_as_float(json_get(jnNode,"uid"))) {
				if (hContact=AddOrFindContact(qqid)) {
					// bl possibly in string
					if (/*READC_W2(QQ_STATUS)==0 &&*/ m_groups[qqid]!=0 && (qqid=json_as_int(json_get(jnNode,"bl")))>0) {
						// This contact is new, move group
						CallService(MS_CLIST_CONTACTCHANGEGROUP,(WPARAM)hContact,(LPARAM)m_groups[qqid]);
					}

					UpdateContactInfo(hContact,jnNode);
				} else {
					QLog(__FUNCTION__"(): ERROR: Failed to add/find contact with qqid==%u, possibly DB Damage!",qqid);
				}
			} else {
				QLog(__FUNCTION__"(): ERROR: qqid==0!");
			}
		}
	}

	hContact=NULL;
	WRITEC_B("UHDownloadReady",1);
}

void CProtocol::HandleWebIMConnect(int type, JSONNODE* jnResult) {
	switch (type) {
		case WEBIM_GETCONNECT_ITEM_CONTACT_STATUS:
			if (HANDLE hContact=FindContact(json_as_float(json_get(jnResult,"uid")))) {
				UpdateContactInfo(hContact,jnResult);
			} else
				QLog(__FUNCTION__"(%d): ERR - hContact==NULL!",type);
			break;
		case WEBIM_GETCONNECT_ITEM_CONTACT_MESSAGE:
			{
				int msgType=json_as_int(json_get(jnResult,"msgType"));

				if (msgType==2) {
					if (HANDLE hContact=FindContact(json_as_float(json_get(jnResult,"fromUid")))) {
						LPSTR pszMsg=json_as_string(json_get(jnResult,"msg"));

						PROTORECVEVENT pre={PREF_UTF};
						CCSDATA ccs={hContact,PSR_MESSAGE,NULL,(LPARAM)&pre};

						pre.timestamp=(DWORD)time(NULL);
						pre.szMessage=pszMsg;

						CallService(MS_PROTO_CHAINRECV, 0, (LPARAM)&ccs);
					} else
						QLog(__FUNCTION__"(%d): ERROR: hContact==NULL!",type);
				} else if (msgType==3 || msgType==4) {
					QLog(__FUNCTION__"(%d): msgType=%d not handled",msgType);
				}
			}
			break;
		case WEBIM_GETCONNECT_ITEM_LOGOUT:
			{
				int exitCode=json_as_int(json_get(jnResult,"ec"));
				if (exitCode==900) {
					ShowNotification(TranslateT("You have been kicked offline due to login from another location."), NIIF_ERROR);
				}
				// 902~905
				SetStatus(ID_STATUS_OFFLINE);
			}
			break;
		case WEBIM_GETCONNECT_ITEM_ADD_REQUEST:
			{
				DWORD dwUIN=json_as_float(json_get(jnResult,"uid"));
				HANDLE hContact=FindContact(dwUIN);
				LPSTR pszTemp;

				if (!hContact) {
					hContact=AddOrFindContact(dwUIN,true,false);
					WRITEC_U8S("Uri",pszTemp=json_as_string(json_get(jnResult,"uri")));
					json_free(pszTemp);
				}

				CCSDATA ccs;
				PROTORECVEVENT pre;
				char* msg=json_as_string(json_get(jnResult,"desc"));
				char* szBlob;
				char* pCurBlob;

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

				json_free(msg);
			}
			break;
		case WEBIM_GETCONNECT_ITEM_ADD_RESULT:
			if (json_as_int(json_get(jnResult,"ba"))==1) {
				DWORD dwUIN=json_as_float(json_get(jnResult,"uid"));
				if (HANDLE hContact=AddOrFindContact(dwUIN)) {
					WCHAR szTemp[MAX_PATH];
					swprintf(szTemp,json_as_int(json_get(jnResult,"rs"))==2?TranslateT("%u rejected your authorization request!"):TranslateT("%u approved your authorization request!"),dwUIN);
					ShowNotification(szTemp,NIIF_INFO);
				}
			}
			break;
	}
}
