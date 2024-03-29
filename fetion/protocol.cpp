#include "stdafx.h"

#define WRITEINFO_U8S(x) if (impresa = ezxml_attr(personal,x)) WRITEC_U8S(x,impresa)
#define WRITEINFO_S(x) if (impresa = ezxml_attr(personal,x)) WRITEC_S(x,impresa)
#define WRITEINFO_W(x) if ((impresa = ezxml_attr(personal,x))!=NULL && *impresa) WRITEC_W(x,atoi(impresa))
#define WRITEINFO_B(x) if ((impresa = ezxml_attr(personal,x))!=NULL && *impresa) WRITEC_B(x,(BYTE)atoi(impresa))
#define WRITEINFO_D(x) if ((impresa = ezxml_attr(personal,x))!=NULL && *impresa) WRITEC_D(x,atol(impresa))

void CNetwork::fetion_login() {
	DBVARIANT dbv;
	HANDLE hContact=NULL;

	m_tg = 0; //temp group chat id
	//m_registerexpire = 400;
	m_impresa=NULL;
	m_GetContactFlag = 0;

	m_ssic=m_uri=m_username=m_mobileno=m_password=m_impresa=/*m_SysCfgServer=m_status=*/m_regcallid=NULL;
	m_tg=m_cseq=m_registerexpire=m_registerstatus=0;
	m_reregister=0;
	m_notdisconnect=false;
	ZeroMemory(&m_registrar,sizeof(m_registrar));
	ZeroMemory(&m_proxy,sizeof(m_proxy));
	m_transactions.clear();
	m_tempgroup.clear();

	util_log(__FUNCTION__);

	if (READC_S2("LoginID",&dbv)) {
		ShowNotification(TranslateT("You did not enter your username or mobile phone number in options."),NIIF_ERROR);
		ProtoBroadcastAck(m_szModuleName,NULL,ACKTYPE_LOGIN,ACKRESULT_FAILED,NULL,LOGINERR_BADUSERID);
		BroadcastStatus(ID_STATUS_OFFLINE);
		return;
	} else {
		if (IsCMccNo(dbv.pszVal)) {
			m_mobileno=mir_strdup(dbv.pszVal);
			m_username=NULL;
		} else {
			m_username=mir_strdup(dbv.pszVal);
			m_mobileno=NULL;
		}
		DBFreeVariant(&dbv);
	}

	//	m_servername = mir_strdup(userserver[1]);
	//m_SysCfgServer = mir_strdup("nav.fetion.com.cn");
	BroadcastStatus(ID_STATUS_CONNECTING);

	//Try to get systemconfig
	ForkThread((ThreadFunc)&CNetwork::RetriveSysCfg);
}

void __cdecl CNetwork::RetriveSysCfg(LPVOID) {
	char szBody[384];
	HANDLE hContact=NULL;

	sprintf(szBody,"<config><user %s=\"%s\" /><client type=\"PC\" version=\"" FETION_VERSION "\" platform=\"W5.1\" /><servers version=\"%d\" /><service-no version=\"%d\" /><parameters version=\"%d\" /><hints version=\"%d\" /><http-applications version=\"%d\" /><client-config version=\"%d\" /><services version=\"%d\" /></config>\r\n\r\n",m_mobileno?"mobile-no":"sid",m_mobileno?m_mobileno:m_username,READC_W2("version_servers"),READC_W2("version_service-no"),READC_W2("version_parameters"),READC_W2("version_hints"),READC_W2("version_http-applications"),READC_W2("version_client-config"),READC_W2("version_services"));

	NETLIBHTTPHEADER nlhh[]={{"User-Agent",FETION_USER_AGENT}};
	NETLIBHTTPREQUEST nlhr={
		sizeof(nlhr),
		REQUEST_POST,
		NLHRF_GENERATEHOST|NLHRF_REMOVEHOST,
		"http://nav.fetion.com.cn:80/nav/getsystemconfig.aspx",
		nlhh,
		1,
		szBody,
		strlen(szBody)
	};

	util_log(__FUNCTION__ "(): Retrieve system config");
	if (NETLIBHTTPREQUEST* nlhrr=(NETLIBHTTPREQUEST*)CallService(MS_NETLIB_HTTPTRANSACTION,(WPARAM)g_hNetlibUser,(LPARAM)&nlhr)) {
		if (nlhrr->resultCode==200) {
			util_log(nlhrr->pData);
			ezxml_t f1=ezxml_parse_str(nlhrr->pData,nlhrr->dataLength);
			ezxml_t item;
			ezxml_t ezServer;

#define WRITESERVER(x) if (item = ezxml_child(ezServer,x)) {\
	WRITEC_S(x,item->txt);\
	util_log("\t%s=%s",x,item->txt);\
} else util_log("\t%s not available",x)

			if (ezServer=ezxml_child(f1,"servers")) {
				WRITEC_W("version_servers",atoi(ezxml_attr(ezServer,"version")));
				WRITESERVER("sipc-proxy");
				WRITESERVER("ssi-app-sign-in-v2");
				WRITESERVER("get-uri");
			}

			if (ezServer=ezxml_child(f1,"service-no")) WRITEC_W("version_service-no",atoi(ezxml_attr(ezServer,"version")));
			if (ezServer=ezxml_child(f1,"parameters")) WRITEC_W("version_parameters",atoi(ezxml_attr(ezServer,"version")));
			if (ezServer=ezxml_child(f1,"hints")) WRITEC_W("version_hints",atoi(ezxml_attr(ezServer,"version")));

			if (ezServer=ezxml_child(f1,"http-applications")) {
				WRITEC_W("version_http-applications",atoi(ezxml_attr(ezServer,"version")));
				WRITESERVER("get-portrait");
				WRITESERVER("set-portrait");
				WRITESERVER("get-group-portrait");
				WRITESERVER("set-group-portrait");
			}

			if (ezServer=ezxml_child(f1,"client-config")) WRITEC_W("version_client-config",atoi(ezxml_attr(ezServer,"version")));

			ezxml_free(f1);

			LoginToSsiPortal(NULL);

		} else {
			util_log(__FUNCTION__ "(): Bad HTTP status code (%d)",nlhrr->resultCode);
		}
		CallService(MS_NETLIB_FREEHTTPREQUESTSTRUCT,(WPARAM)nlhrr,0);
	} else {
		util_log(__FUNCTION__ "(): MS_NETLIB_HTTPTRANSACTION failed");
	}
}

void __cdecl CNetwork::LoginToSsiPortal(LPVOID) {
	SSLAgent mAgent(this);
	DBVARIANT dbvServer, dbvPassword;
	LPSTR szURL=(LPSTR)mir_alloc(MAX_PATH);
	unsigned int status=0;
	LPSTR szBody;
	HANDLE hContact=NULL;
	
	READC_S2("ssi-app-sign-in-v2",&dbvServer);

	if (READC_S2("Password",&dbvPassword)) {
		DBFreeVariant(&dbvServer);
		ShowNotification(TranslateT("You did not enter your password in options."),NIIF_ERROR);
		ProtoBroadcastAck(m_szModuleName,NULL,ACKTYPE_LOGIN,ACKRESULT_FAILED,NULL,LOGINERR_WRONGPASSWORD);
		BroadcastStatus(ID_STATUS_OFFLINE);
		mir_free(szURL);
		return;
	}

	CallService(MS_DB_CRYPT_DECODESTRING,strlen(dbvPassword.pszVal),(LPARAM)dbvPassword.pszVal);
	m_password=mir_strdup(dbvPassword.pszVal);
	LPSTR pszPasswordDigest=fetion_ssi_calcuate_digest(m_password);

	strcpy(szURL,dbvServer.pszVal);

	if (m_mobileno!=NULL) {
		mir_snprintf(szURL+strlen(szURL),MAX_PATH-strlen(szURL),"?mobileno=%s&domain=fetion.com.cn&digest=%s",m_mobileno,pszPasswordDigest);
		// mir_snprintf(szURL+strlen(szURL),MAX_PATH-strlen(szURL),"?mobileno=%s&domains=fetion.com.cn%3bm161.com.cn%3bwww.ikuwa.cn",m_mobileno,dbvPassword.pszVal);
	} else
		// mir_snprintf(szURL+strlen(szURL),MAX_PATH-strlen(szURL),"?sid=%s&domain=fetion.com.cn&pwd=%s",m_username,m_password);
		mir_snprintf(szURL+strlen(szURL),MAX_PATH-strlen(szURL),"?sid=%s&domain=fetion.com.cn&digest=%s",m_username,pszPasswordDigest);
		// mir_snprintf(szURL+strlen(szURL),MAX_PATH-strlen(szURL),"?sid=%s&domains=fetion.com.cn%3bm161.com.cn%3bwww.ikuwa.cn",m_username,dbvPassword.pszVal);

	mir_free(pszPasswordDigest);
	DBFreeVariant(&dbvPassword);

	util_log(__FUNCTION__ "(): Logging into SSI Portal using server: %s",dbvServer.pszVal);
	DBFreeVariant(&dbvServer);

	LPSTR result=mAgent.getSslResult(&szURL,"","",status,szBody);
	/*
	if (status==401 || status==421) {
		mir_free(result);
		result=mAgent.getSslResult(&szURL,"","",status,szBody);
	}
	*/
	if (status!=200 && status!=100) {
		util_log("\tLogin Failed");
		ProtoBroadcastAck(m_szModuleName,NULL,ACKTYPE_LOGIN,ACKRESULT_FAILED,NULL,LOGINERR_WRONGPASSWORD);
		BroadcastStatus(ID_STATUS_OFFLINE);
		if (status==404)
			MessageBox(NULL,TranslateT("Fetion service not available for your UIN/Phone number."),NULL,MB_ICONERROR|MB_SYSTEMMODAL);
		else if (status==401)
			MessageBox(NULL,TranslateT("Incorrect UIN/Phone number or password."),NULL,MB_ICONERROR|MB_SYSTEMMODAL);
		else if (status==405)
			MessageBox(NULL,TranslateT("New version of client required to use with this uid."),NULL,MB_ICONERROR|MB_SYSTEMMODAL);
		else if (status==422)
			MessageBox(NULL,TranslateT("Your UIN requires graphic verification, however MIMFetion doesn't support it yet.\nPlease login with official client first. If this error stills shows, change your password to become more complex."),NULL,MB_ICONERROR|MB_SYSTEMMODAL);
		else if (status==421)
			MessageBox(NULL,TranslateT("The password for your UIN is too simple. The server rejected your login."),NULL,MB_ICONERROR|MB_SYSTEMMODAL);
		else
			MessageBox(NULL,TranslateT("Unknown login error occured, please try again later."),NULL,MB_ICONERROR|MB_SYSTEMMODAL);
	} else {
		LPSTR pszTemp=strstr(result,"Cookie: ssic=");
		if (pszTemp) {
			*strchr(pszTemp,';')=0;
			m_ssic=mir_strdup(pszTemp+13);
			util_log("\tCookie=%s",m_ssic);

			pszTemp=strstr(pszTemp+strlen(pszTemp)+1,"<?xml");

			ezxml_t f1=ezxml_parse_str(pszTemp,strlen(pszTemp));
			ezxml_t node=ezxml_child(f1,"user");
			LPCSTR uri;
			if (uri=ezxml_attr(node,"uri")) {
				WRITEC_S("uri",uri);
				m_uri=mir_strdup(uri);
			}

			if (!m_username) {
				m_username=mir_strdup(uri+4);
				*strchr(m_username,'@')=0;
				WRITEC_D(UNIQUEIDSETTING,atol(m_username));
			} else if (!m_mobileno)
				m_mobileno=mir_strdup(ezxml_attr(node,"mobile-no"));
			
			ezxml_free(f1);

			READC_S2("sipc-proxy",&dbvServer);
			*strchr(dbvServer.pszVal,':')=0;
			setServer(false,dbvServer.pszVal,atol(dbvServer.pszVal+strlen(dbvServer.pszVal)+1));
			DBFreeVariant(&dbvServer);
			connect();
		} else
			util_log("\tCookie not available!");
	}
	mir_free(result);
	mir_free(szURL);
}

void CNetwork::do_register_exp(int expire) {
	LPSTR pszBody=(LPSTR)mir_alloc(300);
	LPSTR pszHdr=NULL;
	HANDLE hContact=NULL;

	m_reregister = time(NULL) + expire - 100;
	//sprintf(pszBody," <args><device type=\"PC\" version=\"%d\" client-version=\"" FETION_VERSION "\" /><caps value=\"fetion-im;im-session;temp-group\" /><events value=\"contact;permission;system-message\" /><user-info attributes=\"all\" /><presence><basic value=\"%d\" desc=\"\" /></presence></args>",READC_W2("version_custom-config"),ConvertStatus(m_iDesiredStatus,false));
	sprintf(pszBody," <args><device type=\"PC\" version=\"%d\" client-version=\"" FETION_VERSION "\" /><caps value=\"simple-im;im-session;temp-group;personal-group\" /><events value=\"contact;permission;system-message;personal-group\" /><user-info attributes=\"all\" /><presence><basic value=\"%d\" desc=\"\" /></presence></args>",READC_W2("version_custom-config"),ConvertStatus(m_iDesiredStatus,false));
	if(m_registerstatus == FETION_REGISTER_RETRY)
	{
		// pszHdr=g_strdup_printf("A: Digest response=\"%s\",cnonce=\"%s\"\r\n", m_registrar.digest_session_key, m_registrar.cnonce);
		pszHdr=g_strdup_printf("A: Digest algorithm=\"SHA1-sess\",response=\"%s\",cnonce=\"%s\",salt=\"" SALT "\"\r\n", m_registrar.digest_session_key, m_registrar.cnonce);
		//pszHdr=g_strdup_printf("A: Digest algorithm=\"MD5-sess\",response=\"%s\",cnonce=\"%s\"\r\n", m_registrar.digest_session_key, m_registrar.cnonce);
	}
	else if(m_registerstatus == FETION_REGISTER_COMPLETE)
	{
		if(expire==0) strcpy(pszHdr,"X: 0\r\n");
		mir_free(pszBody);
		pszBody=NULL;
	}
	else 
	{
		m_registerstatus = FETION_REGISTER_SENT;
		pszHdr=NULL;
	}


	send_sip_request("R", "", "", pszHdr, pszBody, NULL, &CNetwork::process_register_response);
	if(pszBody!=NULL) mir_free(pszBody);
	if(pszHdr!=NULL) mir_free(pszHdr);
}

void CNetwork::do_register() {
	do_register_exp(400);
}

void CNetwork::send_sip_request(LPCSTR method, LPCSTR url, LPCSTR to, LPCSTR addheaders, LPCSTR body, LPCSTR callid2, TransCallback tc) {
	LPCSTR callid = callid2 ? callid2 : gencallid();
	LPCSTR addh = "";
	LPSTR pszOutstr=(LPSTR)mir_alloc(65535);

	if(!strcmp(method, "R"))
	{
		if(m_regcallid)
		{
			mir_free((LPVOID)callid);
			callid = mir_strdup(m_regcallid);
		}
		else m_regcallid = mir_strdup(callid);
	}

	if(addheaders) addh = addheaders;


	sprintf(pszOutstr,"%s fetion.com.cn SIP-C/2.0\r\n"
		"F: %s\r\n"
		"I: %s\r\n"
		"Q: %d %s\r\n"
		"%s%s",
		method,
		m_username,
		callid,
		++m_cseq,
		method,
		to,
		addh);
	if(body)
		sprintf(pszOutstr+strlen(pszOutstr),"L: %d\r\n\r\n%s",strlen(body),body);
	else
		sprintf(pszOutstr+strlen(pszOutstr),"\r\n\r\n");


	if (!callid2) mir_free((LPVOID)callid);

	/* add to ongoing transactions */

	transactions_add_buf(pszOutstr, tc);

	sendout_pkt(pszOutstr);

	mir_free(pszOutstr);
}

void CNetwork::sendout_pkt(LPCSTR buf)
{
	time_t currtime = time(NULL);
	int writelen = strlen(buf);
	//int ret;

	//purple_debug(PURPLE_DEBUG_MISC, "fetion", "\n\nsending - %s\n######\n%s\n######\n\n", ctime(&currtime), buf);

	this->send(buf,writelen);
}

void CNetwork::transactions_remove(struct transaction *trans)
{
	if(trans->msg) sipmsg_free(trans->msg);
	m_transactions.remove(trans);
	mir_free(trans);
}

void CNetwork::transactions_add_buf(LPCSTR buf, TransCallback callback)
{
	struct transaction *trans = (struct transaction*)mir_alloc(sizeof(struct transaction));
	ZeroMemory(trans,sizeof(struct transaction));
	trans->time = time(NULL);
	trans->msg = sipmsg_parse_msg(buf);
	trans->cseq = sipmsg_find_header(trans->msg, "Q");
	trans->callback = callback;
	m_transactions.push_back(trans);
}
void CNetwork::transactions_free_all()
{
	while (m_transactions.size())
	{
		transactions_remove(m_transactions.front());
	}
}


struct transaction *CNetwork::transactions_find(struct sipmsg *msg)
{
	struct transaction *trans;
	LPCSTR cseq = sipmsg_find_header(msg, "Q");

	if (cseq)
	{
		for (list<transaction*>::iterator iter=m_transactions.begin(); iter!=m_transactions.end(); iter++) {
			trans=*iter;
			if (!strcmp(trans->cseq,cseq))
				return trans;
		}
	} else {
		util_log(__FUNCTION__ "(): Received message contains no CSeq header.");
	}

	return NULL;
}

bool __cdecl CNetwork::process_register_response(struct sipmsg *msg, struct transaction *tc)
{
	LPCSTR tmp;
	LPCSTR szExpire;
	util_log("\tin process register response response: %d", msg->response);
	switch (msg->response)
	{
	case 200:
		if(m_registerstatus < FETION_REGISTER_COMPLETE)
		{
			/* get buddies from blist */
			HANDLE hContact=NULL;
			ezxml_t f1=ezxml_parse_str(msg->body,msg->bodylen);
			ezxml_t client=ezxml_child(f1,"client");
			WRITEC_D("ip",inet_addr(ezxml_attr(client,"public-ip")));
			client=ezxml_child(client,"custom-config");
			WRITEC_W("version_custom-config",atoi(ezxml_attr(client,"version")));
			BroadcastStatus(m_iDesiredStatus);

			client=ezxml_child(f1,"user-info");
			if (atoi(ezxml_attr(client,"personal-info-version"))!=READC_W2("version_personal-info")) GetPersonalInfo();
			if (atoi(ezxml_attr(client,"contact-version"))!=READC_W2("version_contact"))
				GetContactList();
			else
				fetion_subscribe_exp(NULL);

			ezxml_free(f1);

			char szTemp[MAX_PATH];
			sprintf(szTemp,"<args><group-list version=\"%d\" attributes=\"name;identity\" /></args>",READC_W2("version_group-list"));
			send_sip_request("S","","","N: PGGetGroupList\r\n",szTemp,NULL,&CNetwork::PGGetGroupList_cb);
		}
		m_registerstatus = FETION_REGISTER_COMPLETE;
		szExpire = sipmsg_find_header(msg,"X");
		if(szExpire!=NULL)
			m_registerexpire = atoi(szExpire);
		util_log("\tRegister: [%s]",szExpire);
		break;
	case 401:
		if(m_registerstatus != FETION_REGISTER_RETRY)
		{
			util_log("\tREGISTER retries %d", m_registrar.retries);
			if(m_registrar.retries > FETION_REGISTER_RETRY_MAX)
			{
				ShowNotification(TranslateT("Incorrect Password"),NIIF_ERROR);
				BroadcastStatus(ID_STATUS_OFFLINE);
				ProtoBroadcastAck(m_szModuleName,NULL,ACKTYPE_LOGIN,ACKRESULT_FAILED,NULL,LOGINERR_WRONGPASSWORD);
				return TRUE;
			}
			tmp = sipmsg_find_header(msg, "W");
			util_log("\tbefore fill_auth: %s",tmp);
			fill_auth(this,tmp, &m_registrar);
			m_registerstatus = FETION_REGISTER_RETRY;
			do_register();
		}
		break;
	default:
		if (m_registerstatus != FETION_REGISTER_RETRY)
		{
			util_log("\tUnrecognized return code for REGISTER.");
			if (m_registrar.retries > FETION_REGISTER_RETRY_MAX)
			{
				ShowNotification(TranslateT("Unknown Server Response"),NIIF_ERROR);
				BroadcastStatus(ID_STATUS_OFFLINE);

				return TRUE;
			}
			m_registerstatus = FETION_REGISTER_RETRY;
			do_register();
		}
		break;
	}
	return TRUE;
}

bool CNetwork::GetContactList()
{
	LPSTR body, hdr;

	if(m_GetContactFlag == 1)
	{
		//purple_timeout_remove(sip->GetContactTimeOut);
		//sip->GetContactTimeOut = NULL;
		return TRUE;
	}
	hdr = mir_strdup("N: GetContactList\r\n");
	body = mir_strdup("<args><contacts><buddy-lists /><buddies attributes=\"all\" /><mobile-buddies attributes=\"all\" /><chat-friends /><blacklist /></contacts></args>");
	send_sip_request("S","","",hdr,body,NULL,&CNetwork::GetContactList_cb);

	mir_free(body);
	mir_free(hdr);

	return TRUE;
}

bool CNetwork::GetContactList_cb(struct sipmsg *msg, struct transaction *tc) 
{
	ezxml_t f1;
	ezxml_t xnBuddies;
	ezxml_t xn;
	map<int,int> groupMap;
	HANDLE hContact;

	util_log("in process GetContactList response response: %d", msg->response);
	switch (msg->response)
	{
	case 200:
		f1=ezxml_parse_str(msg->body,msg->bodylen);
		if (xn=ezxml_child(f1,"contacts")->child->child) {
			// <buddy-list />
			CLIST_INTERFACE* ci=(CLIST_INTERFACE*)CallService(MS_CLIST_RETRIEVE_INTERFACE,0,0);
			int newgroup;
			LPWSTR pszGroupU;

			do {
				pszGroupU=mir_utf8decodeW(ezxml_attr(xn,"name"));
				newgroup=util_group_name_exists(pszGroupU,-1);
				if (newgroup==-1) {
					newgroup=CallService(MS_CLIST_GROUPCREATE, 0, 0);
					ci->pfnRenameGroup(newgroup,pszGroupU);
				} else
					newgroup++;
				mir_free(pszGroupU);
				groupMap[atoi(ezxml_attr(xn,"id"))]=newgroup;
			} while (xn=xn->next);
		}
	
		if (xnBuddies=ezxml_child(ezxml_child(f1,"contacts"),"buddies")) {
			int status=ID_STATUS_ONTHEPHONE;
			LPCSTR impresa;

			do {
				if (!strcmp(xnBuddies->name,"mobile-buddies")) 
					status=ID_STATUS_OFFLINE;
				else if (!strcmp(xnBuddies->name,"chat-friends"))
					continue;
				else if (!strcmp(xnBuddies->name,"blacklist"))
					status=ID_STATUS_DND;
				else
					status=ID_STATUS_ONTHEPHONE;

				xn=xnBuddies->child;
				while (xn) {
					LPCSTR pszURI=ezxml_attr(xn,"uri");
					LPCSTR pszName=ezxml_attr(xn,"local-name");
					LPCSTR pszGroup=ezxml_attr(xn,"buddy-lists");
					int id=(*pszURI=='t'?atol(pszURI+6):atol(pszURI+4));
					hContact=FindContact(id);
					if (!hContact) {
						hContact=AddContact(id,false,false);
						WRITEC_W("Status",status);
						WRITEC_S("uri",pszURI);

						if(pszName && *pszName) {
							WRITEC_U8S("Nick",pszName);
						} else {
							LPSTR pszNick=mir_strdup(pszURI+4);
							if (status==ID_STATUS_ONTHEPHONE) *strrchr(pszNick,';')=0;
							WRITEC_U8S("Nick",pszNick);
							mir_free(pszNick);
						}
					} else {
						DBDeleteContactSetting(hContact,"CList","Hidden");
						pszGroup=NULL;
					}

					ezxml_t personal=xn;
					WRITEINFO_S("user-id");
					WRITEINFO_B("relation-status");
					WRITEINFO_B("online-notify");
					WRITEINFO_D("feike-read-version");
					WRITEC_B("Updated",1);

					if (pszGroup && *pszGroup) {
						CallService(MS_CLIST_CONTACTCHANGEGROUP, (WPARAM)hContact, (LPARAM)groupMap[atoi(pszGroup)]);
					}
					xn=xn->next;
				}
			} while (xnBuddies=xnBuddies->sibling);
		}

		hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, (WPARAM)NULL, (LPARAM)NULL);
		while (hContact) {
			if (!lstrcmpA(m_szModuleName,(char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact,(LPARAM)NULL)) && 
				READC_B2("IsGroup")==0 && READC_B2("Updated")==0 && READC_B2("IsMe")==0) {
					HANDLE hThisContact=hContact;
					hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, (LPARAM)NULL);
					CallService(MS_DB_CONTACT_DELETE,(WPARAM)hThisContact,0);
					continue;
			}
			hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, (LPARAM)NULL);
		}

		ezxml_free(f1);

		fetion_subscribe_exp(NULL);

		break;
	default:
		GetContactList();
		break;
	}

	return TRUE;
}

void CNetwork::process_input_message(struct sipmsg *msg)
{
	bool found = FALSE;
	if(msg->response == 0)
	{ /* request */
		if(!strcmp(msg->method, "M"))
		{
			process_incoming_message(msg);
			found = TRUE;
		} else if(!strcmp(msg->method, "BN"))
		{
			process_incoming_BN(msg);
			found = TRUE;

		} else if(!strcmp(msg->method, "I"))
		{	
			process_incoming_invite(msg);
			found = TRUE;

		} else if(!strcmp(msg->method, "A"))
		{

		} else if(!strcmp(msg->method, "N"))
		{
			process_incoming_notify(msg);
			found = TRUE;
		} else if(!strcmp(msg->method,"B"))
		{
			if (LPCSTR pTmp=sipmsg_find_header(msg,"F")) {
				if (HANDLE hContact=FindContact(atol(pTmp+4))) {
					if (ServiceExists(MS_POPUP_ADDPOPUPW)) {
						POPUPDATAW ppd={0};
						DBVARIANT dbv;
						if (READC_TS2("Nick",&dbv))
							swprintf(ppd.lpwzContactName,L"%S",pTmp);
						else {
							_tcscpy(ppd.lpwzContactName,dbv.ptszVal);
							DBFreeVariant(&dbv);
						}
						_tcscpy(ppd.lpwzText,TranslateT("Contact left channel"));
						ppd.lchIcon=(HICON)LoadImage(g_hInstance, MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0);
						CallService(MS_POPUP_ADDPOPUPW,(WPARAM)&ppd,0);
					}
				}
			}

			send_sip_response(msg,200,"OK",NULL);
			found = TRUE;

		} else if(!strcmp(msg->method,"O"))
		{
			send_sip_response(msg,200,"OK",NULL);
			found = TRUE;

		} else {
			util_log("not implemented:\n%s",msg->body);
		}
	} else { /* response */
		struct transaction *trans = transactions_find(msg);
		if(trans)
		{
			if(msg->response == 407)
			{
				LPSTR resend, auth;
				LPCSTR ptmp;

				if(m_proxy.retries > 3) return;
				m_proxy.retries++;
				/* do proxy authentication */

				ptmp = sipmsg_find_header(msg, "Proxy-Authenticate");

				fill_auth(this,ptmp, &m_proxy);
				auth = auth_header(this,&m_proxy, trans->msg->method, trans->msg->target);
				sipmsg_remove_header(trans->msg, "Proxy-Authorization");
				sipmsg_add_header(trans->msg, "Proxy-Authorization", auth);
				mir_free(auth);
				resend = sipmsg_to_string(trans->msg);
				/* resend request */
				sendout_pkt(resend);
				mir_free(resend);
			} else if(msg->response == 522){

				if(!strcmp(trans->msg->method,"S"))	
				{
					util_log("AddBuddy:522");
					if(trans->callback) {
						typedef bool (__cdecl *pCallbackFunc)(void *owner, struct sipmsg *, struct transaction *);
						pCallbackFunc pCF=(pCallbackFunc)*(void**)&trans->callback;
						pCF(this,msg,trans);
					}
				}
				found = TRUE;
			} else {
				if(msg->response == 100)
				{
					/* ignore provisional response */
					util_log("got trying response");
				} else {
					m_proxy.retries = 0;
					if(!strcmp(trans->msg->method, "R"))
					{

						/* This is encountered when a REGISTER request was ...
						*/
						if(msg->response == 401)
						{
							/* denied until further authentication was provided. */
							/*
							LPSTR resend, auth;
							LPCSTR ptmp;
							*/

							if(m_registrar.retries > FETION_REGISTER_RETRY_MAX) return;
							m_registrar.retries++;
							//WWWAuthenticate = "W"
#if 0
							ptmp = sipmsg_find_header(msg, "W");

							fill_auth(this,ptmp, &m_registrar);
							auth = auth_header(this,&m_registrar, trans->msg->method, trans->msg->target);
							sipmsg_add_header(trans->msg, "A", auth);
							mir_free(auth);
							resend = sipmsg_to_string(trans->msg);
							/* resend request */
							sendout_pkt(resend);
							mir_free(resend);
#endif
						}
						else if (msg->response != 200)
						{
							/* denied for some other reason! */
							m_registrar.retries++;
						}
						else 
						{
							LPCSTR callid;

							callid = sipmsg_find_header(msg,"I");
							m_registrar.retries = 0;
							m_regcallid = mir_strdup(callid);

						}
					} else if(!strcmp(trans->msg->method, "M")) {
						if (LPCSTR pTmp=sipmsg_find_header(msg,"T")) {
							if (HANDLE hContact=FindContact((*pTmp=='t'||pTmp[4]=='P')?atol(pTmp+6):atol(pTmp+4))) {
								delayReport_t* dr=(delayReport_t*)mir_alloc(sizeof(delayReport_t));
								dr->hContact=hContact;
								dr->ackType=ACKTYPE_MESSAGE;
								dr->ackResult=ACKRESULT_SUCCESS;
								dr->aux=1;
								dr->aux2=NULL;
								ForkThread((ThreadFunc)&CNetwork::delayReport,dr);
							}
						}
					} else {
						if(msg->response == 401)
						{
							/* This is encountered when a generic (MESSAGE, NOTIFY, etc)
							* was denied until further authorization is provided.
							*/
						} else {
							/* Reset any count of retries that may have
							* accumulated in the above branch.
							*/
							m_registrar.retries = 0;

						}
					}
					if(trans->callback)
					{
						/* call the callback to process response*/
						//(trans->*callback)(msg, trans);
						typedef bool (__cdecl *pCallbackFunc)(void *owner, struct sipmsg *, struct transaction *);
						pCallbackFunc pCF=(pCallbackFunc)*(void**)&trans->callback;
						pCF(this,msg,trans);
					}
					transactions_remove(trans);
				}
			}
			found = TRUE;
		} else {
			util_log("received response to unknown transaction");
		}
	}
	if(!found)
	{
		/*if (m_iStatus==ID_STATUS_CONNECTING && *msg->method=='R' && msg->response==200) {
			HANDLE hContact=NULL;
			ezxml_t f1=ezxml_parse_str(msg->body,msg->bodylen);
			ezxml_t client=ezxml_child(f1,"client");
			WRITEC_D("ip",inet_addr(ezxml_attr(client,"public-ip")));
			client=ezxml_child(client,"custom-config");
			WRITEC_W("version_custom-config",atoi(ezxml_attr(client,"version")));
			BroadcastStatus(m_iDesiredStatus);

			client=ezxml_child(f1,"user-info");
			if (atoi(ezxml_attr(client,"personal-info-version"))!=READC_W2("version_personal-info")) GetPersonalInfo();
			if (atoi(ezxml_attr(client,"contact-version"))!=READC_W2("version_contact"))
				GetContactList();
			else
				fetion_subscribe_exp(NULL);

			ezxml_free(f1);

			char szTemp[MAX_PATH];
			sprintf(szTemp,"<args><group-list version=\"%d\" attributes=\"name;identity\" /></args>",READC_W2("version_group-list"));
			send_sip_request("S","","","N: PGGetGroupList\r\n",szTemp,NULL,&CNetwork::PGGetGroupList_cb);
		} else if (!found)*/
			util_log("received a unknown sip message with method %s and response %d\n", msg->method, msg->response);
	}
}

LPSTR CNetwork::ProcessHTMLMessage(LPSTR pszMsg, ezxml_t xml) {
	if (xml->name[1]==0) {
		pszMsg+=sprintf(pszMsg,"[%c]",(*xml->name)+0x20);
	} else if (!strcmp(xml->name,"Font")) {
		LPCSTR pszAttr;
		if (pszAttr=ezxml_attr(xml,"Color")) {
			pszMsg+=sprintf(pszMsg,"[color=%06x]",atoi(pszAttr) & 0xffffff);
		}
		if (pszAttr=ezxml_attr(xml,"Size")) {
			pszMsg+=sprintf(pszMsg,"[size=%s]",pszAttr);
		}
	}

	if (*xml->txt)
		pszMsg+=sprintf(pszMsg,xml->txt);
	else {
		// has children
		ezxml_t xmlChild=xml->child;
		while (xmlChild) {
			pszMsg=ProcessHTMLMessage(pszMsg,xmlChild);
			xmlChild=xmlChild->next;
		}
	}

	if (xml->name[1]==0) {
		pszMsg+=sprintf(pszMsg,"[/%c]",(*xml->name)+0x20);
	} else if (!strcmp(xml->name,"Font")) {
		LPCSTR pszAttr;
		if (pszAttr=ezxml_attr(xml,"Size")) {
			pszMsg+=sprintf(pszMsg,"[/size]");
		}
		if (pszAttr=ezxml_attr(xml,"Color")) {
			pszMsg+=sprintf(pszMsg,"[/color]");
		}
	}

	return pszMsg;
}

void CNetwork::process_incoming_message(struct sipmsg *msg)
{
	LPCSTR from;
	struct group_chat *g_chat=NULL;
	LPCSTR contenttype;
	bool found = FALSE;

	from = sipmsg_find_header(msg,"F");
	if(!from) return;

	util_log("\tgot message from %s: %s", from, msg->body);

	contenttype = sipmsg_find_header(msg, "C");
	if(!contenttype || !strncmp(contenttype, "text/plain", 10) || !strncmp(contenttype, "text/html-fragment", 18))
	{
		if(strncmp(from,"sip:TG",6)==0)
		{
			g_chat = m_tempgroup[atol(from)];
			if (!g_chat) return;
			from = sipmsg_find_header(msg,"SO");
			if (!from) return;
			//serv_got_chat_in(g_chat->chatid,from,0,msg->body,time(NULL));
			util_log("\tserv_got_chat_in not implemented!\n");
		}
		else {
			PROTORECVEVENT pre;
			CCSDATA ccs;
			pre.flags=PREF_UTF;
			if (!strncmp(contenttype, "text/html-fragment", 18)) {
				LPSTR pszMsg=(LPSTR)mir_alloc(msg->bodylen+12);
				strcpy(pszMsg,"<msg>");
				strncpy(pszMsg+5,msg->body,msg->bodylen);
				strcpy(pszMsg+msg->bodylen+5,"</msg>");

				ezxml_t f1=ezxml_parse_str(pszMsg,msg->bodylen+11);
				if (LPCSTR from2=sipmsg_find_header(msg,"SO")) {
					// Qun Message
					HANDLE hContact=AddContact(atol(from+6),true,false);
					DBVARIANT dbv;
					pre.szMessage=(LPSTR)mir_alloc(msg->bodylen+MAX_PATH);
					if (READC_U8S2(from2,&dbv)) {
						strcpy(pre.szMessage,from2);
					} else {
						strcpy(pre.szMessage,dbv.pszVal);
						sprintf(pre.szMessage+strlen(pre.szMessage),"(%d)",atol(from2+4));
						DBFreeVariant(&dbv);
					}
					strcat(pre.szMessage,": ");
					//strcat(pre.szMessage,f1->txt);
					ProcessHTMLMessage(pre.szMessage+strlen(pre.szMessage),f1);
				} else {
					pre.szMessage=(LPSTR)mir_alloc(msg->bodylen);
					ProcessHTMLMessage(pre.szMessage,f1);
				}
				ezxml_free(f1);
				mir_free(pszMsg);
			} else {
				pre.szMessage=mir_strdup(msg->body);
			}

			pre.timestamp=time(NULL);
			pre.lParam=0;
			ccs.hContact=AddContact((*from=='t'||from[4]=='P')?atol(from+6):atol(from+4),true,false);
			ccs.szProtoService = PSR_MESSAGE;
			ccs.wParam = 0;
			ccs.lParam = ( LPARAM )&pre;
			CallService(MS_PROTO_CHAINRECV, 0, ( LPARAM )&ccs );

			//serv_got_im(from, msg->body, 0, time(NULL));
		}
		sipmsg_remove_header(msg,"C");
		sipmsg_remove_header(msg,"D");
		sipmsg_remove_header(msg,"K");
		sipmsg_remove_header(msg,"XI");
		send_sip_response(msg, 200, "OK", NULL);
		found = TRUE;
	}

	if(!found)
	{
		util_log("\tgot unknown mime-type");

		contenttype = sipmsg_find_header(msg, "N");
		if(contenttype==NULL || strncmp(contenttype,"system-message",14)!=0)
			send_sip_response(msg, 415, "Unsupported media type", NULL);
	}
}

void CNetwork::send_sip_response(struct sipmsg *msg, int code, LPCSTR text, LPCSTR body)
{
	GSList *tmp = msg->headers;
	LPSTR name;
	LPSTR value;
	LPSTR pszOutstr = (LPSTR)mir_alloc(1024);

	/* When sending the acknowlegements and errors, the content length from the original
	message is still here, but there is no body; we need to make sure we're sending the
	correct content length */
	sipmsg_remove_header(msg, "L");
	if(body)
	{
		char len[12];
		sprintf(len, "%d"  , strlen(body));
		sipmsg_add_header(msg, "L", len);
	}

	sprintf(pszOutstr, "SIP-C/2.0 %d %s\r\n", code, text);
	while(tmp)
	{
		name = ((struct siphdrelement*) (tmp->data))->name;
		value = ((struct siphdrelement*) (tmp->data))->value;

		sprintf(pszOutstr+strlen(pszOutstr), "%s: %s\r\n", name, value);
		tmp = g_slist_next(tmp);
	}
	sprintf(pszOutstr+strlen(pszOutstr), "\r\n%s", body ? body : "");
	sendout_pkt(pszOutstr);
	mir_free(pszOutstr);
}

void CNetwork::fetion_subscribe_exp(HANDLE hContact) {
	LPSTR body=(LPSTR)mir_alloc(10240), hdr;
	DBVARIANT dbv;

	strcpy(body,"<args><subscription><contacts>");
	hdr = g_strdup_printf("N: presence\r\n");

	if (!hContact) {
		hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, (WPARAM)NULL, (LPARAM)NULL);
		while (hContact) {
			if (!lstrcmpA(m_szModuleName,(char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact,(LPARAM)NULL))) {
				if (!READC_B2("IsMe") && !READC_S2("uri",&dbv)) {
					if (*dbv.pszVal=='s' && dbv.pszVal[4]!='P') {
						strcat(body,"<contact uri=\"");
						util_log("fetion:sub name=[%s]",dbv.pszVal);
						strcat(body,dbv.pszVal);	
						strcat(body,"\" />");
					}
					DBFreeVariant(&dbv);
				}
			}

			hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, (LPARAM)NULL);
		}
	} else {
		if (!READC_S2("uri",&dbv)) {
			strcat(body,"<contact uri=\"");
			util_log("fetion:sub name=[%s]",dbv.pszVal);
			strcat(body,dbv.pszVal);	
			strcat(body,"\" />");
			DBFreeVariant(&dbv);
		}
	}

	strcat(body,"</contacts>");
	strcat(body,"<presence><basic attributes=\"all\" /><personal attributes=\"all\" /><extended types=\"sms;location;listening;ring-back-tone\" /></presence></subscription><subscription><contacts><contact uri=\"");
	strcat(body,m_uri);
	strcat(body,"\" /></contacts><presence><extended types=\"sms;location;listening;ring-back-tone\" /></presence></subscription></args>");


	util_log("fetion:sub name=[%s]",body);

	send_sip_request("SUB", "", "",hdr, body,NULL, &CNetwork::process_subscribe_response);
	mir_free(body);
	mir_free(hdr);
}

bool __cdecl CNetwork::process_subscribe_response(struct sipmsg *msg, struct transaction *tc) {
	util_log("process subscribe response[%s]",msg->body);

	return TRUE;
}

void CNetwork::process_incoming_notify(struct sipmsg *msg) {
	LPSTR from;
	LPCSTR fromhdr;
	LPSTR basicstatus_data;
	ezxml_t f1;
	ezxml_t basicstatus = NULL, tuple, status;
	HANDLE hContact;
	bool isonline = FALSE;
	LPCSTR sshdr = NULL;

	fromhdr = sipmsg_find_header(msg, "F");
	from = parse_from(fromhdr);
	if(!from) return;

	hContact=FindContact(*from=='t'?atol(from+6):atol(from+4));
	if (!hContact)
	{
		mir_free(from);
		util_log("\tCould not find the buddy.");
		return;
	}

#if 0 // What use?
	if (b->dialog && !dialog_match(b->dialog, msg))
	{
		/* We only accept notifies from people that
		* we already have a dialog with.
		*/
		purple_debug_info("fetion","No corresponding dialog for notify--discard\n");
		g_free(from);
		return;
	}
#endif

	f1=ezxml_parse_str(msg->body, msg->bodylen);

	if(!f1)
	{
		util_log("\tprocess_incoming_notify: no parseable pidf");
		sshdr = sipmsg_find_header(msg, "Subscription-State");
		if (sshdr)
		{
			int i = 0;
			LPSTR* ssparts = g_strsplit(sshdr, ":", 0);
			while (ssparts[i])
			{
				g_strchug(ssparts[i]);
				if (!strncmp(ssparts[i], "terminated",10))
				{
					util_log("\tSubscription expired!");
					DELC("ourtag");
					DELC("theirtag");
					DELC("callid");

					WRITEC_W("Status",ID_STATUS_OFFLINE);
					break;
				}
				i++;
			}
			g_strfreev(ssparts);
		}
		send_sip_response(msg, 200, "OK", NULL);
		mir_free(from);
		return;
	}

	if ((tuple = ezxml_child(f1, "tuple")))
		if ((status = ezxml_child(f1, "status")))
			basicstatus = ezxml_child(f1, "basic");

	if(!basicstatus)
	{
		util_log("\tprocess_incoming_notify: no basic found");
		ezxml_free(f1);
		mir_free(from);
		return;
	}

	basicstatus_data = basicstatus->txt;

	if(!basicstatus_data)
	{
		util_log("\tprocess_incoming_notify: no basic data found");
		ezxml_free(f1);
		mir_free(from);
		return;
	}

	if(strstr(basicstatus_data, "open"))
		isonline = TRUE;


	if(isonline) 
		WRITEC_W("Status",ID_STATUS_ONLINE);
	else 
		WRITEC_W("Status",ID_STATUS_OFFLINE);

	ezxml_free(f1);
	mir_free(from);
	mir_free(basicstatus_data);

	send_sip_response(msg, 200, "OK", NULL);
}

void CNetwork::process_incoming_BN(struct sipmsg *msg) {
	LPCSTR new_crc;
	LPCSTR nickname, basicstatus, event_type, uri, impresa;
	HANDLE hContact=NULL;
	LPCSTR from;
	//LPSTR cur, alias, nickbuf;
	ezxml_t root, event_node, item;
	ezxml_t basic, personal;

	root = ezxml_parse_str(msg->body, msg->bodylen);
	if (!root) return;
	util_log("\tin BN[%s]",msg->body);	
	event_node = ezxml_child(root, "event");
	if (!event_node) return;
	event_type = ezxml_attr(event_node,"type");
	if(strncmp(event_type,"PresenceChanged",15)==0)
	{
		for(item=ezxml_child(event_node,"presence");item;item=item->next)
		{
			uri = ezxml_attr(item,"uri");
			basic = ezxml_child(item,"basic");
			if(basic !=NULL)
			{
				basicstatus = ezxml_attr(basic,"value");
				if (!basicstatus) basicstatus="0";

				hContact=FindContact(atol(uri+4));
				if(hContact==NULL)
				{
					LPSTR pszName=mir_strdup(uri+4);
					hContact=AddContact(atol(uri+4),false,false);
					WRITEC_S("uri",uri);
					*strrchr(pszName,';')=0;
					WRITEC_S("Nick",pszName);
				}

				int mimstatus=ConvertStatus(atoi(basicstatus),true);
				if (mimstatus==-1) 
					mimstatus=ID_STATUS_ONLINE;
				else if (mimstatus==ID_STATUS_INVISIBLE)
					mimstatus=ID_STATUS_ONTHEPHONE;

				WRITEC_W("Status",mimstatus);
			}

			personal = ezxml_child(item,"personal");
			if(personal==NULL)
				continue;
			nickname = ezxml_attr(personal,"nickname");
			impresa = ezxml_attr(personal,"impresa");
			new_crc = ezxml_attr(personal,"portrait-crc");

			hContact=FindContact(atol(uri+4));
			if (!hContact) return;
			if(nickname!=NULL && *nickname)
			{
				WRITEC_U8S("Nick",nickname);
			}
			if(impresa!=NULL && *impresa!='\0')
				DBWriteContactSettingUTF8String(hContact, "CList", "StatusMsg", impresa);

			if(new_crc!=NULL && strcmp(new_crc,"0")!=0)
				CheckPortrait(uri,new_crc);
		}

	}
	else if(strncmp(event_type,"UserEntered",11)==0)
	{
		// This thing should be handled by chat plugin
		from = sipmsg_find_header(msg,"F");

		if(from!=NULL && strncmp(from,"sip:TG",6)==0)
		{
			hContact=FindContact(atol(from+6));
			if (!hContact) return;
		}
		for(item=ezxml_child(event_node,"member");item;item=item->next)
		{
			uri = ezxml_attr(item,"uri");
			/*
			b = purple_find_buddy(sip->account, uri);
			if( b == NULL)
				purple_conv_chat_add_user(PURPLE_CONV_CHAT(g_chat->conv),uri,NULL, PURPLE_CBFLAGS_NONE, TRUE);
			else
				purple_conv_chat_add_user(PURPLE_CONV_CHAT(g_chat->conv),purple_buddy_get_alias(b),NULL, PURPLE_CBFLAGS_NONE, TRUE);
				*/
		}

	}
	else if(strncmp(event_type,"UserLeft",11)==0)
	{
		// Handled by chat plugin
		from = sipmsg_find_header(msg,"F");
		/*
		if(from!=NULL && strncmp(from,"sip:TG",6)==0)
		{
			g_chat = g_hash_table_lookup(sip->tempgroup,from);
			g_return_if_fail(g_chat!=NULL);
		}
		for(item=ezxml_child(event_node,"member");item;item=xmlnode_get_next_twin(item))
		{
			uri = ezxml_attr(item,"uri");
			b = purple_find_buddy(sip->account, uri);
			if( b == NULL)
				purple_conv_chat_remove_user(PURPLE_CONV_CHAT(g_chat->conv),uri,NULL);
			else
				purple_conv_chat_remove_user(PURPLE_CONV_CHAT(g_chat->conv),purple_buddy_get_alias(b),NULL);
		}
		*/
	}
	else if (!strcmp(event_type,"AddBuddyApplication")) {
		//DBEVENTINFO dbei;
		item=ezxml_child(event_node,"application");
		LPCSTR uri=ezxml_attr(item,"uri");
		int uin=atol(uri+(*uri=='s'?4:6));
		HANDLE hContact=FindContact(uin);
		if (!hContact) {
			int hGroup=util_group_name_exists(L"我的好友",0);
			hContact=AddContact(uin,true,false);
			if (hGroup!=-1) CallService(MS_CLIST_CONTACTCHANGEGROUP,(WPARAM)hContact,hGroup+1);
		}
		util_log("\tReceived authorization request");
		// Show that guy
		DBDeleteContactSetting(hContact,"CList","Hidden");
		WRITEC_S("uri",uri);

		PROTORECVEVENT pre={0,(DWORD)time(NULL)};
		CCSDATA ccs={hContact,PSR_AUTH,0,(LPARAM)&pre};
		LPCSTR desc=ezxml_attr(item,"desc");
		if (!desc) desc="";

		pre.lParam=sizeof(DWORD)*2+3+strlen(desc)+strlen(uri)+2;

		PBYTE pCurBlob=(PBYTE)alloca(pre.lParam);
		pre.szMessage=(LPSTR)pCurBlob;

		*(PDWORD)pCurBlob=uin; pCurBlob+=sizeof(DWORD);
		*(PDWORD)pCurBlob=(DWORD)hContact; pCurBlob+=sizeof(DWORD);
		strcpy((LPSTR)pCurBlob, desc); pCurBlob+=strlen(desc)+1;
		*pCurBlob = '\0'; pCurBlob++;	   //firstName
		*pCurBlob = '\0'; pCurBlob++;	   //lastName
		strcpy((LPSTR)pCurBlob, uri); pCurBlob += strlen(uri)+1;
		*pCurBlob = '\0';         	   //reason

		CallService(MS_PROTO_CHAINRECV, 0, (LPARAM)&ccs);
	}
	else if (!strcmp(event_type,"ApproveResult")) {
		if (ezxml_t f1=ezxml_parse_str(msg->body,msg->bodylen)) {
			if (ezxml_t f2=ezxml_get(f1,"event",0,"authorize",-1)) {
				WCHAR szTemp[MAX_PATH];
				LPCSTR uri=ezxml_attr(f2,"approve-uri");
				int id=atoi(uri+6);
				bool success=false;

				if (*ezxml_attr(f2,"approve-result")=='2') {
					if (HANDLE hContact=AddContact(id,false,false)) {
						DBDeleteContactSetting(hContact,"CList","Hidden");
						DBDeleteContactSetting(hContact,"CList","NotOnList");
						swprintf(szTemp,TranslateT("You have been approved to join group %d!"),id);
						success=true;
					}
				}

				if (!success) {
					swprintf(szTemp,TranslateT("You have been rejected to join group %d."),id);
				}

				ShowNotification(szTemp,success?NIIF_INFO:NIIF_ERROR);
			}
			ezxml_free(f1);
		}

	}
	else if (!strcmp(event_type,"ServiceResult")) {
		item=ezxml_child(ezxml_child(event_node,"results"),"contacts");
		basic=ezxml_child(item,"contact");
		LPCSTR uri=ezxml_attr(basic,"uri");
		HANDLE hContact=FindContact(atoi(uri+4));
		if (hContact) {
			if (personal=ezxml_child(item,"personal")) {
				LPCSTR impresa;
				WRITEINFO_S("lunar-animal");
				WRITEINFO_S("horoscope");
				WRITEINFO_S("profile");
				WRITEINFO_S("blood-type");
				WRITEINFO_S("occupation");
				WRITEINFO_S("hobby");
				if (impresa=ezxml_attr(personal,"personal-email")) WRITEC_S("e-mail",impresa);
				if (impresa=ezxml_attr(personal,"work-email")) WRITEC_S("e-mail2",impresa);
				if (impresa=ezxml_attr(personal,"other-email")) WRITEC_S("e-mail3",impresa);
			}

			delayReport_t* dr=(delayReport_t*)mir_alloc(sizeof(delayReport_t));
			dr->hContact=hContact;
			dr->ackType=ACKTYPE_GETINFO;
			dr->ackResult=ACKRESULT_SUCCESS;
			dr->aux=1;
			dr->aux2=(LPVOID)0;
			ForkThread((ThreadFunc)&CNetwork::delayReport,dr);
		}
	}

	ezxml_free(root);
}

// Portrait means avatar!
void CNetwork::CheckPortrait(LPCSTR who, LPCSTR crc) {
	LPCSTR old_crc = NULL;
	HANDLE hContact = FindContact(atol(who+(who[4]=='P'?6:4)));
	DBVARIANT dbv;
	if (!hContact && strcmp(who,m_uri)) return;

	if (!READC_S2("portrait-crc",&dbv)) {
		if (!strcmp(dbv.pszVal,crc)) {
			DBFreeVariant(&dbv);
			return;
		}
		DBFreeVariant(&dbv);
	}

	GetPortrait(hContact,who,crc,NULL);	

}

void CNetwork::GetPortrait(HANDLE hContact, LPCSTR sip, LPCSTR crc, LPCSTR host) {
	LPSTR server_ip;
	if (!hContact) return;
	
	if(host!=NULL)
	{
		server_ip = mir_strdup(host);
	}
	else {
		DBVARIANT dbv;
		if (sip[4]=='P' && DBGetContactSettingString(NULL,m_szModuleName,"get-group-portrait",&dbv))
			return;
		else if (sip[4]!='P' && DBGetContactSettingString(NULL,m_szModuleName,"get-portrait",&dbv))
			return;
			
		server_ip = mir_strdup(dbv.pszVal+7);
		*strchr(server_ip,'/')=0;
		DBFreeVariant(&dbv);
	}

	util_log("GetPortrait:buddy[%s]\n",sip);

#if 0
	head = g_strdup_printf("GET /hds/getportrait.aspx?%sUri=%s"
		"&Size=%s&c=%s HTTP/1.1\r\n"
		"User-Agent: IIC2.0/PC 3.1.0480\r\n"
		"Accept: image/pjpeg;image/jpeg;image/bmp;image/x-windows-bmp;image/png;image/gif\r\n"
		"Host: %s\r\n\r\n",
		(who->host?"transfer=1&":""),who->name,"96",ssic,server_ip
		);
#endif

	char szUrl[MAX_PATH];

	NETLIBHTTPHEADER nlhh[]={
		{"User-Agent",FETION_USER_AGENT},
		{"Accept","image/pjpeg;image/jpeg;image/bmp;image/x-windows-bmp;image/png;image/gif"}
	};
	NETLIBHTTPREQUEST nlhr={
		sizeof(nlhr),
		REQUEST_GET,
		NLHRF_GENERATEHOST|NLHRF_REMOVEHOST|NLHRF_HTTP11,
		szUrl,
		nlhh,
		2,
		NULL,
		0
	};

	LPSTR pszEncodeSSIC=(LPSTR)CallService(MS_NETLIB_URLENCODE,0,(LPARAM)m_ssic);
	bool loop=true;

	if (sip[4]=='P')
		sprintf(szUrl,"http://%s/hds/getgroupportrait.aspx?%sUri=%s&Size=%d&c=%s",server_ip,host?"transfer=1&":"",sip,96,pszEncodeSSIC);
	else
		sprintf(szUrl,"http://%s/hds/getportrait.aspx?%sUri=%s&Size=%d&c=%s",server_ip,host?"transfer=1&":"",sip,96,pszEncodeSSIC);
	HeapFree(GetProcessHeap(),0,pszEncodeSSIC);

	while (loop) {
		loop=false;
		if (NETLIBHTTPREQUEST* nlhrr=(NETLIBHTTPREQUEST*)CallService(MS_NETLIB_HTTPTRANSACTION,(WPARAM)g_hNetlibUser,(LPARAM)&nlhr)) {
			if (nlhrr->resultCode==200) {
				CHAR szFilename[MAX_PATH];
				CallService(MS_DB_GETPROFILEPATH,MAX_PATH,(LPARAM)szFilename);
				strcat(szFilename,"\\");
				strcat(szFilename,m_szModuleName);
				CreateDirectoryA(szFilename,NULL);
				strcat(szFilename,"\\");
				strcat(szFilename,sip+4);
				strcpy(strrchr(szFilename,'@'),".jpg");
				util_log("\tSave to %s",szFilename);

				HANDLE hFile=CreateFileA(szFilename,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,0,NULL);
				if (hFile==INVALID_HANDLE_VALUE) {
					util_log("\tFailed to open file for writing");
				} else {
					DWORD dwWritten;
					WriteFile(hFile,nlhrr->pData,nlhrr->dataLength,&dwWritten,NULL);
					CloseHandle(hFile);
					WRITEC_S("portrait-crc",crc);

					PROTO_AVATAR_INFORMATION pai={sizeof(pai)};
					strcpy(pai.filename,szFilename);
					pai.format=PA_FORMAT_JPEG;
					pai.hContact=hContact;
					ProtoBroadcastAck(m_szModuleName, (HANDLE)hContact, ACKTYPE_AVATAR, ACKRESULT_SUCCESS, (HANDLE)&pai, (LPARAM)0);
				}
			} else if (nlhrr->resultCode==302) {
				util_log("\tRedirection found");
				for (int c=0; c<nlhrr->headersCount; c++) {
					if (!strcmp(nlhrr->headers[c].szName,"Location")) {
						strcpy(szUrl,nlhrr->headers[c].szValue);
						util_log("\tRedirect to %s",szUrl);
						loop=true;
						break;
					}
				}
				if (!loop) {
					util_log("\tFailed to find redirection url!");
				}
			} else {
				util_log(__FUNCTION__ "(): Bad HTTP status code (%d)",nlhrr->resultCode);
			}
			CallService(MS_NETLIB_FREEHTTPREQUESTSTRUCT,(WPARAM)nlhrr,0);
		} else {
			util_log(__FUNCTION__ "(): MS_NETLIB_HTTPTRANSACTION failed");
		}
	}

	mir_free(server_ip);

}

void CNetwork::process_incoming_invite(struct sipmsg *msg) {
	LPCSTR to, callid;	
	LPSTR body;
	LPCSTR my_ip;
	int my_port;
	//struct group_chat *g_chat;
	//struct fetion_buddy * buddy =NULL;
	HANDLE hContact=NULL;

	my_ip = inet_ntoa(m_myip); // purple_network_get_my_ip(sip->fd);
	my_port = m_myport; // purple_network_get_port_from_fd(sip->fd);
	util_log("\tInvite:[%s:%d]\n",my_ip,my_port);
	body = g_strdup_printf("v=0\r\n"
		"o=-0 0 IN %s:%d\r\n"
		"s=session\r\n"
		"c=IN IP4 %s:%d\r\n"
		"t=0 0\r\n"
		"m=message %d sip %s\r\n",
		my_ip,my_port,my_ip,my_port,my_port,m_uri);

	util_log("\tInvite:answer[%s]\n",body);
	send_sip_response(msg,200,"OK",body);

	callid = sipmsg_find_header(msg,"I");
	to = sipmsg_find_header(msg,"F");
	if(strncmp(to,"sip:TG",6)!=0)
	{
		HANDLE hContact=FindContact(atol(to+4));
		if (!hContact) {
			hContact=AddContact(atol(to+4),true,false);
			WRITEC_S("uri",to);
		}
		WRITEC_S("callid",callid);
		
		if (ServiceExists(MS_POPUP_ADDPOPUPW)) {
			POPUPDATAW ppd={0};
			DBVARIANT dbv;
			if (READC_TS2("Nick",&dbv))
				swprintf(ppd.lpwzContactName,L"%S",to);
			else {
				_tcscpy(ppd.lpwzContactName,dbv.ptszVal);
				DBFreeVariant(&dbv);
			}
			_tcscpy(ppd.lpwzText,TranslateT("Chat session established by contact request"));
			ppd.lchIcon=(HICON)LoadImage(g_hInstance, MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0);
			CallService(MS_POPUP_ADDPOPUPW,(WPARAM)&ppd,0);
		}
	}
	else
	{
		util_log("\tUnhandled! Group conversion!");

		/*
		g_chat = g_new0(struct group_chat,1);
		g_chat->chatid = sip->tg++;
		g_chat->callid = g_strdup(callid);	
		g_chat->groupname = g_strdup(to);
		g_hash_table_insert(sip->tempgroup, g_chat->groupname, g_chat);
		sip->tempgroup_id = g_list_append(sip->tempgroup_id, g_chat);

		g_chat->conv = serv_got_joined_chat(sip->gc,g_chat->chatid,"Fetion Chat");
		purple_conv_chat_add_user(PURPLE_CONV_CHAT(g_chat->conv),purple_account_get_alias(sip->account),NULL, PURPLE_CBFLAGS_NONE, TRUE);
		*/
	}
	mir_free(body);
}

void CNetwork::fetion_keep_alive() {
	time_t curtime = time(NULL);
	/* register again if first registration expires */
	if(m_reregister < curtime)
	{
		do_register();
	}

	return;
}

void CNetwork::fetion_send_message(LPCSTR to, LPCSTR msg, LPCSTR type, const bool sms) {
	LPSTR hdr;
	LPSTR fullto;
	int  self_flag,sms_flag;

	self_flag = 0;
	sms_flag = 0;
	HANDLE hContact=FindContact(atol(to+((*to=='t'||to[4]=='P')?6:4)));
	LPSTR callid=NULL;
	/* Impossible in MIM
	if(buddy==NULL)
	{
		buddy = g_new0(struct fetion_buddy, 1);
		buddy->name = g_strdup(to);
		g_hash_table_insert(sip->buddies, buddy->name, buddy);
	}
	*/
	if(!sms)
	{
		if(strcmp(m_uri,to)!=0)
		{
			DBVARIANT dbv;
			if (READC_S2("callid",&dbv)) {
				callid=gencallid(); 
				WRITEC_S("callid",callid);
				DBFreeVariant(&dbv);
				//	if(purple_presence_is_online(presence))
				if (READC_W2("Status")!=ID_STATUS_ONTHEPHONE)
					SendInvite(to,callid);
			} else {
				callid=mir_strdup(dbv.pszVal);
				DBFreeVariant(&dbv);
			}
			//if(purple_presence_is_online(presence))
			/*
			if (READC_W2("Status")!=ID_STATUS_ONTHEPHONE)
				sms_flag = 0;
			else
				sms_flag = 1;
				*/

		}
		else
			self_flag = 1;
	}
	else
		sms_flag=1;



	if((sms_flag == 0) &&(self_flag!=1) &&(strncmp("sip:", to, 4)==0))
		fullto = g_strdup_printf("T: %s\r\n", to);
	else 
		fullto = g_strdup_printf("T: %s\r\nN: SendSMS\r\n", to);


	util_log("fetion:sending ","to:[%s] msg:[%s]\n",to,msg);
	if(type)
		hdr = g_strdup_printf("C: %s\r\n", type);
	else 
		hdr = mir_strdup("C: text/plain\r\n");

	if (READC_W2("Status")==ID_STATUS_ONTHEPHONE) {
		LPSTR oldhdr=hdr;
		hdr=g_strdup_printf("%sK: SaveHistory\r\n",hdr);
		mir_free(oldhdr);
		sms_flag=1;
	}

	if (sms_flag==1 && MultiByteToWideChar(CP_UTF8,0,msg,-1,NULL,0)>180)
		MessageBox(NULL,TranslateT("Offline Fetion Message/SMS Message is limited to 180 wide characters."),NULL,MB_ICONERROR | MB_SYSTEMMODAL);
	else
		send_sip_request("M", NULL, fullto, hdr, msg, callid, NULL);

	mir_free(hdr);
	mir_free(fullto);
	mir_free(callid);
}

void CNetwork::SendInvite(LPCSTR who, LPCSTR callid) {
	LPSTR body, hdr, fullto;
	LPCSTR my_ip;
	int my_port;
	HANDLE hContact;
	if(strncmp("sip:", who, 4)==0)
		fullto = g_strdup_printf("T: %s\r\n", who);
	else 
		return;
	hContact=FindContact(atol(who+(who[4]=='P'?6:4)));
	if (!hContact) return;

	my_ip = inet_ntoa(m_myip); // purple_network_get_my_ip(sip->fd);
	my_port = m_myport; // purple_network_get_port_from_fd(sip->fd);

	util_log("\tSendInvite:[%s:%d]",my_ip,my_port);
	hdr = g_strdup_printf("K: text/html-fragment\r\n"
		"K: multiparty\r\n");	
	body = g_strdup_printf("v=0\r\n"
		"o=-0 0 IN %s:%d\r\n"
		"s=session\r\n"
		"c=IN IP4 %s:%d\r\n"
		"t=0 0\r\n"
		"m=message %d sip %s\r\n",
		my_ip,my_port,my_ip,my_port,my_port,m_uri);

	util_log("SendInvite:[%s]",body);

	if (who[4]!='P' && ServiceExists(MS_POPUP_ADDPOPUPW)) {
		POPUPDATAW ppd={0};
		DBVARIANT dbv;
		if (READC_TS2("Nick",&dbv))
			swprintf(ppd.lpwzContactName,L"%S",who);
		else {
			_tcscpy(ppd.lpwzContactName,dbv.ptszVal);
			DBFreeVariant(&dbv);
		}
		_tcscpy(ppd.lpwzText,TranslateT("Chat session established by my request"));
		ppd.lchIcon=(HICON)LoadImage(g_hInstance, MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0);
		CallService(MS_POPUP_ADDPOPUPW,(WPARAM)&ppd,0);
	}
	
	send_sip_request("I","",fullto,hdr,body,callid,(TransCallback) &CNetwork::SendInvite_cb);
	mir_free(fullto);
	mir_free(hdr);
	mir_free(body);
}

void __cdecl CNetwork::SendInvite_cb(struct sipmsg *msg, struct transaction *tc) {
	LPCSTR to;
	LPSTR fullto;

	to = sipmsg_find_header(msg,"T");	
	fullto = g_strdup_printf("T: %s\r\n",to);

	util_log("\tSendACK:\n");
	send_sip_request("A","",fullto,NULL,NULL,NULL,NULL);

	mir_free(fullto);
}

bool CNetwork::GetPersonalInfo() {
	LPSTR body, hdr;
	hdr = mir_strdup("N: GetPersonalInfo\r\n");	
	body = mir_strdup("<args><personal attributes=\"all\" /><services version=\"\" attributes=\"all\" /><config version=\"0\" attributes=\"all\" /><mobile-device attributes=\"all\" /></args>");

	send_sip_request("S", "","", hdr, body, NULL,&CNetwork::GetPersonalInfo_cb);

	mir_free(body);
	mir_free(hdr);
	return TRUE;
}

bool __cdecl CNetwork::GetPersonalInfo_cb(struct sipmsg *msg, struct transaction *tc) {
	ezxml_t root, personal;
	LPCSTR nickname;
	LPCSTR impresa;
	HANDLE hContact=NULL;
	util_log("in process GetPersonalInfo response response: %d", msg->response);
	switch (msg->response)
	{
	case 200:
		root = ezxml_parse_str(msg->body,msg->bodylen);
		if (!root) return FALSE;
		personal = ezxml_child(root,"personal");
		if (!personal) return FALSE;
		WRITEC_W("version_personal",atoi(ezxml_attr(personal,"version")));
		nickname = ezxml_attr(personal,"nickname");
		if((nickname!=NULL) && (*nickname!='\0')) {
			WRITEC_U8S("Nick",nickname);
		} else {
			LPSTR pszTemp=mir_strdup(m_uri+4);
			*strchr(pszTemp,'@')=0;
			WRITEC_U8S("Nick",pszTemp);
			mir_free(pszTemp);
		}
		impresa = ezxml_attr(personal,"impresa");
		if((impresa!=NULL) && (*impresa!='\0')) {
			DBWriteContactSettingUTF8String(NULL,"CList","StatusMsg",impresa);
		} else
			DBDeleteContactSetting(NULL,"CList","StatusMsg");
		
		WRITEINFO_U8S("name");
		if (impresa = ezxml_attr(personal,"gender")) WRITEC_B("gender",(BYTE)*impresa);
		if ((impresa = ezxml_attr(personal,"birthday-valid"))!=NULL && *impresa=='1') WRITEINFO_S("birth-date");
		WRITEINFO_S("nation");
		WRITEINFO_S("province");
		WRITEINFO_W("city");
		WRITEINFO_B("lunar-animal");
		WRITEINFO_B("horoscope");
		WRITEINFO_B("blood-type");
		WRITEINFO_U8S("occupation");
		WRITEINFO_U8S("hobby");
		if (impresa = ezxml_attr(personal,"portrait-crc")) {
			CheckPortrait(m_uri,impresa);
		}
		WRITEINFO_U8S("job-title");
		WRITEINFO_U8S("home-phone");
		WRITEINFO_U8S("work-phone");
		WRITEINFO_U8S("other-phone");
		WRITEINFO_U8S("personal-email");
		WRITEINFO_U8S("work-email");
		WRITEINFO_U8S("other-email");
		WRITEINFO_B("primary-email");
		WRITEINFO_U8S("company");
		WRITEINFO_U8S("company-email");
		WRITEINFO_B("ivr-enabled");
		WRITEINFO_B("match-enabled");
		
		ezxml_free(root);

		hContact=FindContact(atoi(m_uri+4));
		if (!hContact) {
			hContact=AddContact(atoi(m_uri+4),false,false);
			WRITEC_TS("Nick",TranslateT("<My Fetion Account>"));
		}
		WRITEC_B("IsMe",1);
		WRITEC_B("ForceSMS",1);
		WRITEC_S("uri",m_uri);
		WRITEC_W("Status",ID_STATUS_ONTHEPHONE);

		break;
	default:
		GetPersonalInfo();

		break;
	}
	return TRUE;
}

#if 0
void CNetwork::test() {
	char szXml[1024];
	const char* myMsg="<msg><Font Face='宋体' Color='-65536' Size='10.5'>You</Font><Font Face='宋体' Color='-8355712' Size='10.5'><B>Are</B></Font><Font Face='宋体' Color='-8355712' Size='10.5'>Really</Font><Font Face='宋体' Color='-65536' Size='10.5'><B>Really</B></Font><Font Face='宋体' Color='-16776961' Size='10.5'>Boring</Font></msg>";
	char szTest[1024];

	strcpy(szXml,"<msg>");
	strncpy(szXml+5,myMsg,strlen(myMsg));
	strcpy(szXml+strlen(myMsg)+5,"</msg>");

	ezxml_t xml=ezxml_parse_str(szXml,strlen(szXml));
	ProcessHTMLMessage(szTest,xml);
	ezxml_free(xml);
	/*
	LPSTR pszSrc=(LPSTR)mir_alloc(3072);
	FILE* fp=fopen("r:\\out.txt","rb");

	int size=fread(pszSrc,1,3072,fp);
	fclose(fp);

	LPSTR cur = strstr(pszSrc, "\r\n\r\n");
	cur += 2;
	cur[0] = '\0';
	struct sipmsg *msg=sipmsg_parse_header(pszSrc);
	cur[0] = '\r';
	cur += 2;

	msg->body=cur;
	msg->bodylen=size-(cur-pszSrc);
	GetContactList_cb(msg,NULL);
	mir_free(pszSrc);
	util_log(psz);
	mir_free(psz);
	*/
}
#endif

void __cdecl CNetwork::SetPresence_cb(struct sipmsg *msg, struct transaction *tc) {
	if (LPCSTR pszStatus=strstr(msg->body,"<basic value=")) {
		BroadcastStatus(ConvertStatus(atoi(pszStatus+14),true));
	}
}

bool __cdecl CNetwork::AddBuddy_cb(struct sipmsg *msg, struct transaction *tc) {
	if (msg->response==521) {
		// Data Exist
		GetContactList();
	} else if(msg->response != 522)	{
		HANDLE hContact;
		ezxml_t root, item;
		LPCSTR uri, name;
		int uin;

		root = ezxml_parse_str(msg->body, msg->bodylen);
		//if (!(item = ezxml_child(ezxml_child(ezxml_child(root,"contacts"),"buddies"),"buddy"))) return FALSE;
		if (!(item=ezxml_get(root,"contacts",0,"buddies",0,"buddy",-1))) return FALSE;

		uri = ezxml_attr(item, "uri");
		uin=atoi(uri+(*uri=='s'?4:6));
		name = ezxml_attr(item, "local-name");

		hContact=AddContact(uin,false,false);
		DBDeleteContactSetting(hContact,"CList","NotOnList");
		DBDeleteContactSetting(hContact,"CList","Hidden");
		WRITEC_S("uri",uri);
		if (name && *name)
			WRITEC_U8S("Nick",name);
		else {
			LPSTR pszNick=mir_strdup(uri+(*uri=='s'?4:6));
			*strrchr(pszNick,';')=0;
			WRITEC_U8S("Nick",pszNick);
			mir_free(pszNick);
		}

		ezxml_free(root);

		fetion_subscribe_exp(hContact);		
	}
	else
	{
		util_log("AddBuddy_cb:Try to Add as MobileBuddy");
		AddMobileBuddy(msg,tc);
	}

	return TRUE;
}

void CNetwork::AddMobileBuddy(struct sipmsg *msg ,struct transaction *tc) {
	ezxml_t root, son, item;
	LPSTR body;
	LPCSTR uri, name, group_id;
	LPSTR buddy_name;
	struct sipmsg *old=NULL;
	LPCSTR real_name;
	DBVARIANT dbv;

	DBGetContactSettingUTF8String(NULL,m_szModuleName,"Nick",&dbv);
	real_name = dbv.pszVal;

	if (!(old=tc->msg)) return;
	util_log("\tAddMobileBuddy:oldmsg[%s]",old->body);
	root = ezxml_parse_str(old->body, old->bodylen);
	item = ezxml_get(root,"contacts",0,"buddies",0,"buddy",-1);
	if (!item) return;

	uri = ezxml_attr(item, "uri");
	name = ezxml_attr(item, "local-name");
	group_id = ezxml_attr(item, "buddy-lists");
	buddy_name = g_strdup_printf("%s", uri);

	ezxml_free(root);

	root = ezxml_new("args");
	son = ezxml_add_child(root,"contacts",0);
	son = ezxml_add_child(son,"mobile-buddies",0);
	item = ezxml_add_child(son,"mobile-buddy",0);

	ezxml_set_attr(item,"expose-mobile-no","1");
	ezxml_set_attr(item,"expose-name","1");
	ezxml_set_attr(item,"invite","1");

	ezxml_set_attr(item,"uri",buddy_name);
	ezxml_set_attr(item,"buddy-lists","1");
	//ezxml_set_attr(item,"desc",sip->mobileno);
	ezxml_set_attr(item,"desc",real_name);

	body = ezxml_toxml(root,0);
	util_log("\tadd_buddy:body=[%s]",body);

	send_sip_request("S","","","N: AddMobileBuddy\r\n",body,NULL,(TransCallback) &CNetwork::AddMobileBuddy_cb);

	mir_free(buddy_name);
	ezxml_free(root);
	free(body);


}

void __cdecl CNetwork::AddMobileBuddy_cb(struct sipmsg *msg, struct transaction *tc) {
	return ;
}

bool __cdecl CNetwork::PGApplyGroup_cb( struct sipmsg *msg, struct transaction *tc ) {
	if (msg->response==200) {
		ShowNotification(TranslateT("Authorization request sent successfully. You will be notified of result when group administrators replied."),NIIF_INFO);
	} else {
		WCHAR szTemp[MAX_PATH];
		swprintf(szTemp,TranslateT("Failed sending authorization request. Please try again later.\n\nServer replied error number %d."),msg->response);
		ShowNotification(szTemp,NIIF_ERROR);
	}
	return 1;
}

bool __cdecl CNetwork::PGGetGroupList_cb( struct sipmsg *msg, struct transaction *tc ) {
	if (msg->response==200) {
		// <results><group-list  version ="6" ><group  uri="sip:PGxxxx@fetion.com.cn;p=xxxx"  identity="3"/></group-list></results>
		if (ezxml_t f1=ezxml_parse_str(msg->body,msg->bodylen)) {
			HANDLE hContact=NULL;
			ezxml_t grouplist=ezxml_child(f1,"group-list");
			WRITEC_W("version_group-list",atoi(ezxml_attr(grouplist,"version")));
			if (ezxml_t f2=ezxml_child(grouplist,"group")) {
				do {
					LPCSTR uri=ezxml_attr(f2,"uri");
					int id=atoi(uri+6);
					HANDLE hContact=FindContact(id);
					if (!hContact) {
						AddContact(id,FALSE,FALSE);
						if (!hContact) {
							int hGroup=util_group_name_exists(L"我的好友",0);
							hContact=AddContact(id,true,false);
							if (hGroup!=-1) CallService(MS_CLIST_CONTACTCHANGEGROUP,(WPARAM)hContact,hGroup+1);
						}
						WRITEC_S("uri",uri);
					}

					DBDeleteContactSetting(hContact,"CList","Hidden");
					DBDeleteContactSetting(hContact,"CList","NotOnList");
					WRITEC_B("IsGroup",1);
					WRITEC_B("Updated",1);
				} while (f2=f2->next);

				hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, (WPARAM)NULL, (LPARAM)NULL);
				while (hContact) {
					if (!lstrcmpA(m_szModuleName,(char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact,(LPARAM)NULL)) && 
						READC_B2("IsGroup")==1 && READC_B2("Updated")==0) {
							HANDLE hThisContact=hContact;
							hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, (LPARAM)NULL);
							CallService(MS_DB_CONTACT_DELETE,(WPARAM)hThisContact,0);
							continue;
					}
					hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, (LPARAM)NULL);
				}
			}
			ezxml_free(f1);
		}
		ProcessGroups();
	}
	return 1;
}

bool __cdecl CNetwork::PGGetGroupInfo_cb( struct sipmsg *msg, struct transaction *tc ) {
	// <results><groups><group status-code="200" attributes-version="1" uri="sip:PG4843690@fetion.com.cn;p=4609" name="test qun" category="271" introduce="test qun, don&apos;t add" bulletin="" portrait-crc="-478240578" searchable="1" join-approved-type="3" current-member-count="2" limit-member-count="150" set-group-portrait-hds="http://221.176.31.5/hds/setgroupportrait.aspx" get-group-portrait-hds="http://221.176.31.5/hds/getgroupportrait.aspx" create-time="Fri, 01 Aug 2008 15:30:00 GMT" group-activity=""/></groups></results>
	if (msg->response==200) {
		if (ezxml_t f1=ezxml_parse_str(msg->body,msg->bodylen)) {
			ezxml_t f2=ezxml_get(f1,"groups",0,"group",-1);

			while (f2) {
				LPCSTR uri=ezxml_attr(f2,"uri");
				if (HANDLE hContact=FindContact(atoi(uri+6))) {
					LPCSTR impresa;
					ezxml_t personal=f2;
					WRITEINFO_S("status-code");
					WRITEINFO_S("uri");
					WRITEINFO_U8S("name");
					if (impresa) WRITEC_U8S("Nick",impresa);
					WRITEINFO_W("category");
					WRITEINFO_U8S("introduce");
					WRITEINFO_U8S("bulletin");
					if (impresa = ezxml_attr(personal,"portrait-crc")) {
						CheckPortrait(uri,impresa);
					}
					WRITEINFO_B("searchable");
					WRITEINFO_B("join-approved-type");
					WRITEINFO_W("current-member-count");
					WRITEINFO_W("limit-member-count");
					WRITEINFO_S("create-time");
					WRITEINFO_S("group-activity");
				}
				f2=f2->next;
			}
			ezxml_free(f1);
		}
	}
	return true;
} 

void CNetwork::ProcessGroups() {
	HANDLE hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, (WPARAM)NULL, (LPARAM)NULL);
	DBVARIANT dbv;
	LPSTR pszCallid;
	LPSTR uri;
	char szBody[MAX_PATH];
	ezxml_t root=NULL;
	ezxml_t root2=NULL;
	ezxml_t root3=NULL;
	ezxml_t groups;
	ezxml_t groups2;
	ezxml_t groups3;
	ezxml_t group=NULL;
	ezxml_t group2=NULL;
	ezxml_t group3=NULL;
	list<LPSTR> uriList;

	while (hContact) {
		if (!lstrcmpA(m_szModuleName,(char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact,(LPARAM)NULL)) && READC_B2("IsGroup")!=0) {
			if (DBGetContactSettingByte(hContact,"CList","NotOnList",0)==0 && !READC_S2("uri",&dbv)) {
				if (dbv.pszVal[4]=='P') {
					uri=strdup(dbv.pszVal);
					pszCallid=gencallid();
					WRITEC_S("callid",pszCallid);
					SendInvite(dbv.pszVal,pszCallid);
					mir_free(pszCallid);

					if (!root) {
						root=ezxml_new("args");
						root2=ezxml_new("args");
						root3=ezxml_new("args");
						groups=ezxml_add_child(root,"groups",0);
						groups2=ezxml_add_child(ezxml_add_child(root2,"subscription",0),"groups",0);
						groups3=ezxml_add_child(root3,"groups",0);
						ezxml_set_attr(groups,"attributes","all");
						ezxml_set_attr(groups3,"attributes","member-uri;member-nickname;member-iicnickname;member-identity");
					}
					group=ezxml_add_child(groups,"group",group?group->off+1:0);
					//itoa(READC_W2("attributes-version"),szVersion,10);
					ezxml_set_attr(group,"uri",uri);
					ezxml_set_attr(group,"attributes-version","0"/*szVersion*/);

					group2=ezxml_add_child(groups2,"group",group2?group2->off+1:0);
					ezxml_set_attr(group2,"uri",uri);

					group3=ezxml_add_child(groups3,"group",group3?group3->off+1:0);
					ezxml_set_attr(group3,"uri",uri);
					itoa(READC_W2("members-major-version"),szBody,10);
					uriList.push_back(strdup(szBody));
					ezxml_set_attr(group3,"members-major-version",uriList.back());
					itoa(READC_W2("members-minor-version"),szBody,10);
					uriList.push_back(strdup(szBody));
					ezxml_set_attr(group3,"members-minor-version",uriList.back());

					sprintf(szBody,"<args><personal-properties attributes=\"all\"><personal-property group-uri=\"%s\" /></personal-properties></args>",uri);
					send_sip_request("S","","","N: PGGetPersonalInfo\r\n",szBody,NULL,NULL);
					WRITEC_W("Status",ID_STATUS_ONLINE);
					uriList.push_back(uri);
				}
				DBFreeVariant(&dbv);
			}
		}
		hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, (LPARAM)NULL);
	}

	if (root) {
		LPSTR xml=ezxml_toxml(root,0);
		send_sip_request("S","","","N: PGGetGroupInfo\r\n",xml,NULL,&CNetwork::PGGetGroupInfo_cb);
		free(xml);
		ezxml_free(root);

		xml=ezxml_toxml(root2,0);
		send_sip_request("SUB","","","N: PGPresence\r\n",xml,NULL,&CNetwork::process_subscribe_response);
		free(xml);
		ezxml_free(root2);

		xml=ezxml_toxml(root3,0);
		send_sip_request("S","","","N: PGGetGroupMembers\r\n",xml,NULL,&CNetwork::PGGetGroupMembers_cb);
		free(xml);
		ezxml_free(root3);

		while (uriList.size()) {
			free(uriList.front());
			uriList.pop_front();
		}
	}
}

bool __cdecl CNetwork::PGGetGroupMembers_cb( struct sipmsg *msg, struct transaction *tc ) {
	// <results><groups><group uri="sip:PG4843690@fetion.com.cn;p=4609" status-code="200" member-list-major-version="15"  member-list-minor-version="1" ><member  uri="sip:732022644@fetion.com.cn;p=4609" nickname="" iicnickname="MyNick" identity="1" /><member  uri="sip:765888526@fetion.com.cn;p=4609" nickname="" iicnickname="" identity="3" /></group></groups></results>
	if (msg->response==200) {
		if (ezxml_t f1=ezxml_parse_str(msg->body,msg->bodylen)) {
			ezxml_t f2=ezxml_get(f1,"groups",0,"group",-1);
			ezxml_t f3;

			while (f2) {
				LPCSTR uri=ezxml_attr(f2,"uri");
				if (HANDLE hContact=FindContact(atoi(uri+6))) {
					LPCSTR impresa;
					LPCSTR memberuri;
					ezxml_t personal=f2;
					WRITEINFO_W("member-list-major-version");
					WRITEINFO_W("member-list-minor-version");

					f3=ezxml_child(f2,"member");

					while (f3) {
						memberuri=ezxml_attr(f3,"uri");
						if ((impresa=ezxml_attr(f3,"nickname"))!=NULL && *impresa)
							WRITEC_U8S(memberuri,impresa);
						else if ((impresa=ezxml_attr(f3,"iicnickname"))!=NULL && *impresa)
							WRITEC_U8S(memberuri,impresa);

						f3=f3->next;
					}
				}
				f2=f2->next;
			}
			ezxml_free(f1);
		}
	}
	return true;
} 

void CNetwork::WriteLocationInfo(HANDLE hContact) {
	DBVARIANT dbv;
	if (!READC_U8S2("state_raw",&dbv)) {
		if (*dbv.pszVal) {
			//if (ezxml_t xml=ezxml_parse_file("plugins\\Imps.Client.Resource.AdminArea.Province.xml")) {
			if (FILE* fp=fopen("plugins\\Imps.Client.Resource.AdminArea.Province.xml","r")) {
				ezxml_t xml=ezxml_parse_fp(fp);

				for (ezxml_t child=xml->child; child; child=child->next) {
					if (!strcmp(ezxml_attr(child,"id"),dbv.pszVal)) {
						WRITEC_U8S("State",child->txt);
						break;
					}
				}
				ezxml_free(xml);
				fclose(fp);

				DBVARIANT dbv2;
				if (!READC_U8S2("city_raw",&dbv2)) {
					//if (xml=ezxml_parse_file("plugins\\Imps.Client.Resource.AdminArea.City.xml")) {
					if (fp=fopen("plugins\\Imps.Client.Resource.AdminArea.City.xml","r")) {
						xml=ezxml_parse_fp(fp);
						for (ezxml_t child=xml->child; child; child=child->next) {
							if (!strcmp(ezxml_attr(child,"id"),dbv.pszVal)) {
								for (ezxml_t child2=child->child; child2; child2=child2->next) {
									if (!strcmp(ezxml_attr(child2,"id"),dbv2.pszVal)) {
										WRITEC_U8S("City",child2->txt);
										break;
									}
								}
								break;
							}
						}
						ezxml_free(xml);
						fclose(fp);
					}
					DBFreeVariant(&dbv2);
				}
			}
		}
		DBFreeVariant(&dbv);
	}
}

bool __cdecl CNetwork::GetBuddyInfo_cb(struct sipmsg *msg, struct transaction *tc) {
	ezxml_t root, personal;
	LPCSTR uri;
	LPCSTR impresa;
	HANDLE hContact;

	util_log("GetBuddyInfo_cb[%s]\n",msg->body);
	root = ezxml_parse_str(msg->body, msg->bodylen);
	if (!(personal=ezxml_get(root,"contacts",0,"contact",-1))) {
		ezxml_free(root);
		return false;
	}

	uri = ezxml_attr(personal, "uri");
	if (!(personal = ezxml_child(personal,"personal"))) {
		ezxml_free(root);
		return false;
	}
	if (!(hContact=FindContact(atoi(uri+4)))) {
		ezxml_free(root);
		return false;
	}
	if (impresa=ezxml_attr(personal,"nickname")) WRITEC_U8S("Nick",impresa);
	WRITEINFO_S("version");
	WRITEINFO_S("user-id");
	if (impresa = ezxml_attr(personal,"gender")) WRITEC_B("gender",(BYTE)*impresa);
	// WRITEINFO_S("province");
	if (impresa = ezxml_attr(personal, "province")) WRITEC_U8S("state_raw",impresa);
	if (impresa = ezxml_attr(personal, "city")) WRITEC_U8S("city_raw",impresa);
	WriteLocationInfo(hContact);

	if (impresa = ezxml_attr(personal, "nation")) WRITEC_U8S("Country",impresa);

	if (impresa = ezxml_attr(personal, "mobile-no")) WRITEC_U8S("Phone",impresa);

	WRITEINFO_B("ivr-enabled");
	WRITEINFO_B("provisioning");
	WRITEINFO_B("birthday-valid");
	impresa = ezxml_attr(personal, "birth-date");
	if (impresa && *impresa) {
		LPSTR impresa2=(LPSTR)impresa;
		impresa2[4]=impresa2[7]=0;
		WRITEC_W("BirthYear",atoi(impresa));
		WRITEC_W("BirthMonth",atoi(impresa+5));
		WRITEC_W("BirthDay",atoi(impresa+8));
		impresa2[4]=impresa2[7]='-';
	}

	delayReport_t* dr=(delayReport_t*)mir_alloc(sizeof(delayReport_t));
	dr->hContact=hContact;
	dr->ackType=ACKTYPE_GETINFO;
	dr->ackResult=ACKRESULT_SUCCESS;
	dr->aux=1;
	dr->aux2=(LPVOID)0;
	ForkThread((ThreadFunc)&CNetwork::delayReport,dr);

	util_log("get info \n");

	return true;
}

void CNetwork::GetBuddyInfo(LPCSTR who) {
	//int xml_len;
	ezxml_t root,son,item;
	LPSTR body;


	root = ezxml_new("args");
	son = ezxml_add_child(root,"contacts",0);
	ezxml_set_attr(son,"attributes","all");
	ezxml_set_attr(son,"extended-attributes","score-level");
	item = ezxml_add_child(son,"contact",0);

	ezxml_set_attr(item,"uri",who);

	body = ezxml_toxml(root,0);
	util_log("GetBuddyInfo:body=[%s]",body);

	send_sip_request("S","","","N: GetContactsInfo\r\n",body,NULL,&CNetwork::GetBuddyInfo_cb);

	ezxml_free(root);
	free(body);
}

void CNetwork::SetImpresa(LPCSTR u8msg) {
	ezxml_t root,son;
	LPSTR body;

	root = ezxml_new("args");
	son = ezxml_add_child(root,"personal",0);
	ezxml_set_attr(son,"impresa",u8msg);

	body = ezxml_toxml(root,0);
	util_log("GetImpresa:body=[%s]",body);

	send_sip_request("S","","","N: SetPersonalInfo\r\n",body,NULL,NULL);

	ezxml_free(root);
	free(body);
}