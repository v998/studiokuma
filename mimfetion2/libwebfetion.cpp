#include "StdAfx.h"
#pragma comment(lib,"wininet")
#pragma comment(lib,"Advapi32")

#define BUFFERSIZE 1048576
#define OUTBUFFERSIZE 1048576

LPCSTR CLibWebFetion::g_referer_webqq="https://webim.feixin.10086.cn/";
LPCSTR CLibWebFetion::g_domain_qq="https://webim.feixin.10086.cn";
LPCSTR CLibWebFetion::g_server_webim="webim.feixin.10086.cn";

CLibWebFetion::CLibWebFetion(LPSTR pszID, LPSTR pszPassword, LPVOID pvObject, WEBQQ_CALLBACK_HUB wch, HANDLE hNetlib):
	m_id(strdup(pszID)),
	m_password(strdup(pszPassword)), 
	m_userobject(pvObject), 
	m_wch(wch),
	m_status(WEBQQ_STATUS_OFFLINE),
	m_sequence(0),
	m_buffer(NULL),
	m_proxyhost(NULL),
	m_basepath(NULL),
	m_hInetRequest(NULL),
	m_processstatus(0),
	m_proxyuser(NULL),
	m_proxypass(NULL),
	m_stop(false),
	m_web2_psessionid(NULL),
	m_initialstatus(WEBIM_STATUS_ONLINE),
	m_hNetlib(hNetlib){
	memset(m_storage,0,10*sizeof(LPSTR));
	// memset(m_web2_storage,0,10*sizeof(JSONNODE*));
}

CLibWebFetion::~CLibWebFetion() {
	Stop();
	SetStatus(WEBQQ_STATUS_OFFLINE);

	/*
	for (map<DWORD,LPWEBQQ_OUT_PACKET>::iterator iter=m_outpackets.begin(); iter!=m_outpackets.end(); iter++) {
		LocalFree(iter->second->cmd);
		LocalFree(iter->second);
	}
	*/

	if (m_id) free(m_id);
	if (m_password) free(m_password);
	if (m_proxyhost) free(m_proxyhost);
	if (m_proxypass) free(m_proxypass);
	if (m_basepath) free(m_basepath);
	if (m_web2_psessionid) free(m_web2_psessionid);
	if (m_buffer) LocalFree(m_buffer);

	for (int c=0; c<10; c++) {
		if (m_storage[c]) LocalFree(m_storage[c]);
	}

	Log(__FUNCTION__"(): Instance Destruction");
}

void CLibWebFetion::CleanUp() {
	// TODO: Really needed?
}

void CLibWebFetion::Start() {
	DWORD dwThread;
	CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)_ThreadProc,this,0,&dwThread);
}

void CLibWebFetion::Stop() {
	if (m_hInet) {
		HINTERNET hInet=m_hInet;
		m_stop=true;
		if (m_status>=WEBQQ_STATUS_ONLINE) {
			WebIM_SetPresence(-1);
			SetStatus(WEBQQ_STATUS_OFFLINE);
		}
		m_hInet=NULL;
		InternetCloseHandle(hInet);
	}
}

DWORD WINAPI CLibWebFetion::_ThreadProc(CLibWebFetion* lpParameter) {
	return lpParameter->ThreadProc();
}

void CLibWebFetion::ThreadLoop4b() {
	bool breakloop=false;

	m_sequence=1;

	WebIM_GetPersonalInfo();
	WebIM_GetContactList();

	SetStatus(WEBQQ_STATUS_ONLINE);

loopstart:
	__try {
		while (m_status!=WEBQQ_STATUS_ERROR && m_stop==false && (breakloop=web2_channel_poll()));
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		Log("CLibWebFetion Crashed at process phase %d, trying to resume operation...",m_processstatus);
		SendToHub(WEBQQ_CALLBACK_CRASH,NULL,(LPVOID)m_processstatus);

		goto loopstart;
	}
}

DWORD CLibWebFetion::ThreadProc() {
	if (!PrepareParams()) return 0;

	if (m_status!=WEBQQ_STATUS_ERROR) {
		if (!Negotiate()) return 0;
	}
	if (m_status!=WEBQQ_STATUS_ERROR) {
		Log(__FUNCTION__"(): Initial login complete");
		ThreadLoop4b();
	}

	return 0;
};

void CLibWebFetion::SetStatus(WEBQQSTATUSENUM newstatus) {
	LPARAM dwStatus=MAKELPARAM(m_status,newstatus);
	m_status=newstatus;
	SendToHub(WEBQQ_CALLBACK_CHANGESTATUS,NULL,&dwStatus);
}

void CLibWebFetion::SendToHub(DWORD dwCommand, LPSTR pszArgs, LPVOID pvCustom) {
	if (m_wch) {
		if ((dwCommand & 0xfffff000)==0xffff0000) m_processstatus=11;
		m_wch(m_userobject,dwCommand,pszArgs,pvCustom);
	}
}

void CLibWebFetion::SetProxy(LPSTR pszHost, LPSTR pszUser, LPSTR pszPass) {
	if (m_proxyhost) free(m_proxyhost);
	if (m_proxyuser) free(m_proxyuser);
	if (m_proxypass) free(m_proxypass);
	m_proxyhost=strdup(pszHost);
	if (pszUser && *pszUser) m_proxyuser=strdup(pszUser);
	if (pszPass && *pszPass) m_proxypass=strdup(pszPass);
}

LPSTR CLibWebFetion::GetCookie(LPCSTR pszName) {
	LPSTR pszCookie=m_storage[WEBQQ_STORAGE_COOKIE];
	while (*pszCookie) {
		if (!stricmp(pszCookie,pszName)) {
			return pszCookie+strlen(pszCookie)+1;
		} else {
			pszCookie+=strlen(pszCookie);
			if (pszCookie[1]==0)
				pszCookie+=2;
			else
				pszCookie+=strlen(pszCookie+1)+3;
		}
	}

	return NULL;
}

void CLibWebFetion::Log(char *fmt,...) {
	static CHAR szLog[1024];
	va_list vl;

	va_start(vl, fmt);
	
	szLog[_vsnprintf(szLog, sizeof(szLog)-1, fmt, vl)]=0;
	SendToHub(WEBQQ_CALLBACK_DEBUGMESSAGE,szLog);

	va_end(vl);
}

LPSTR CLibWebFetion::GetHTMLDocument(LPCSTR pszUrl, LPCSTR pszReferer, LPDWORD pdwLength, BOOL fWeb2Assign) {
	LPSTR pszBuffer;

	*pdwLength=0;
	HINTERNET hInet=m_hInet;

	LPSTR pszServer=(LPSTR)strstr(pszUrl,"//")+2;
	LPSTR pszUri=strchr(pszServer,'/');
	*pszUri=0;

	HINTERNET hInetConnect=InternetConnectA(hInet,pszServer,INTERNET_DEFAULT_HTTP_PORT,NULL,NULL,INTERNET_SERVICE_HTTP,0,NULL);
	*pszUri='/';

	if (!hInetConnect) {
		Log(__FUNCTION__"(): InternetConnectA() failed: %d, hInet=%p",GetLastError(),hInet);
		*pdwLength=(DWORD)-1;
		return false;
	}

	HINTERNET hInetRequest=HttpOpenRequestA(hInetConnect,"GET",pszUri,NULL,pszReferer,NULL,INTERNET_FLAG_PRAGMA_NOCACHE|INTERNET_FLAG_RELOAD,NULL);
	if (!hInetRequest) {
		DWORD err=GetLastError();
		Log(__FUNCTION__"(): HttpOpenRequestA() failed: %d",err);
		InternetCloseHandle(hInetConnect);
		InternetCloseHandle(hInet);
		*pdwLength=(DWORD)-1;
		SetLastError(err);
		return false;
	}

	DWORD dwRead=70000; // poll is 60 secs timeout
	InternetSetOption(hInetRequest,INTERNET_OPTION_RECEIVE_TIMEOUT,&dwRead,sizeof(DWORD));

	if (!(HttpSendRequestA(hInetRequest,NULL,0,NULL,0))) {
		DWORD err=GetLastError();
		Log(__FUNCTION__"(): HttpSendRequestA() failed, reason=%d",err);
		InternetCloseHandle(hInetRequest);
		InternetCloseHandle(hInetConnect);
		*pdwLength=(DWORD)-1;
		SetLastError(err);
		return false;
	}


	dwRead=0;
	DWORD dwWritten=sizeof(DWORD);
	
	HttpQueryInfo(hInetRequest/*hUrl*/,HTTP_QUERY_CONTENT_LENGTH|HTTP_QUERY_FLAG_NUMBER,pdwLength,&dwWritten,&dwRead);

	pszBuffer=NULL;

	if (strlen(pszUrl)>200 && (pszBuffer=(LPSTR)strchr(pszUrl,'?'))) *pszBuffer=0;

	if (strlen(pszUrl)<200) 
		Log(__FUNCTION__"() url=%s size=%d",pszUrl,*pdwLength);
	else
		Log(__FUNCTION__"() size=%d",*pdwLength);

	if (pszBuffer) *pszBuffer='?';

	if (!*pdwLength) *pdwLength=BUFFERSIZE;

	pszBuffer=(LPSTR)LocalAlloc(LMEM_FIXED,*pdwLength+1);
	LPSTR ppszBuffer=pszBuffer;

	while (InternetReadFile(hInetRequest/*hUrl*/,ppszBuffer,*pdwLength,&dwRead) && dwRead>0) {
		ppszBuffer+=dwRead;
		dwRead=0;
	}
	*ppszBuffer=0;
	*pdwLength=ppszBuffer-pszBuffer;

	InternetCloseHandle(hInetRequest/*hUrl*/);
	InternetCloseHandle(hInetConnect);

	return pszBuffer;
}

LPCSTR CLibWebFetion::GetReferer(WEBQQREFERERENUM type) {
	switch (type) {
		case WEBQQ_REFERER_WEBQQ:
			return g_referer_webqq;
		default:
			return NULL;
	}
}

bool CLibWebFetion::PrepareParams() {
	SetStatus(WEBQQ_STATUS_PREPARE);

	if (!(m_hInet=InternetOpenA("Mozilla/5.0 (Windows; U; Windows NT 5.1; en-US; rv:1.9.1.3) Gecko/20090824 Firefox/3.5.3 GTB5",m_proxyhost?INTERNET_OPEN_TYPE_PROXY:INTERNET_OPEN_TYPE_DIRECT,m_proxyhost,NULL,0))) {
		Log(__FUNCTION__"(): Failed initializing WinINet (InternetOpen)! Err=%d\n",GetLastError());
		SetStatus(WEBQQ_STATUS_ERROR);
		return FALSE;
	}

	if (m_proxyuser) {
		InternetSetOption(m_hInet,INTERNET_OPTION_USERNAME, m_proxyuser,  (DWORD)strlen(m_proxyuser)+1);  
		if (m_proxypass) {
			InternetSetOption(m_hInet,INTERNET_OPTION_PASSWORD, m_proxypass,  (DWORD)strlen(m_proxypass)+1);  
			free(m_proxypass);
			m_proxypass=NULL;
		}

		free(m_proxyuser);
		m_proxyuser=NULL;
	}

	if (!m_buffer) m_buffer=(LPSTR)LocalAlloc(LMEM_FIXED,BUFFERSIZE); // Cannot remove because RefreshCookie() needs it

	return true;
}

void CLibWebFetion::SetInitialStatus(int status) {
	m_initialstatus=status;
}

bool CLibWebFetion::Negotiate() {
	CHAR szCode[5]={0};
	char szTemp[MAX_PATH];

	SetStatus(WEBQQ_STATUS_NEGOTIATE);

	// http://webim.feixin.10086.cn/WebIM/GetPicCode.aspx?Type=ccpsession

	srand(GetTickCount());
	sprintf(szTemp,"http://webim.feixin.10086.cn/WebIM/GetPicCode.aspx?Type=ccpsession&%f",(double)rand()/(double)RAND_MAX);

	SendToHub(WEBQQ_CALLBACK_NEEDVERIFY,szTemp,szCode);
	if (*szCode)
		return Login(szCode);
	else {
		SetStatus(WEBQQ_STATUS_ERROR);
	}
	return false;
}

bool CLibWebFetion::Login(LPSTR pszCode) {
	DWORD dwSize;

	SetStatus(WEBQQ_STATUS_LOGIN);

	/*
https://webim.feixin.10086.cn/WebIM/Login.aspx	POST	https://webim.feixin.10086.cn/
Content-Type:application/x-www-form-urlencoded
UserName:1234567890
Pwd:xxxxxx
Ccp:duay
OnlineStatus:400
	*/

	sprintf(m_buffer,"UserName=%s&Pwd=%s&Ccp=%s&OnlineStatus=%d",m_id,m_password,pszCode,m_initialstatus);
	if (LPSTR pszData=PostHTMLDocument("https://webim.feixin.10086.cn","/WebIM/Login.aspx",g_referer_webqq,m_buffer,&dwSize)) {
		JSONNODE* jn=json_parse(pszData);
		LPSTR pszReason=NULL;

		switch (json_as_int(json_get(jn,"rc"))) {
			case 200:
				// Login Successful
				RefreshCookie();
				m_web2_psessionid=strdup(GetCookie("webim_sessionid"));
				InternetSetCookieA("https://webim.feixin.10086.cn",NULL,"webim_sessionid=");
				sprintf(m_postssid,"ssid=%s",m_web2_psessionid);
				break;
			case 312:
				pszReason="312: Wrong Verification Code";
				break;
			case 404:
				pszReason="404: User not found";
				break;
			case 321:
				pszReason="321: Password not match";
				break;
		}

		LocalFree(pszData);

		if (pszReason) {
			SendToHub(WEBQQ_CALLBACK_LOGINFAIL,pszReason);
			SetStatus(WEBQQ_STATUS_ERROR);
		} else
			return true;
	}

	return false;
}

void CLibWebFetion::RefreshCookie() {
	DWORD dwSize=BUFFERSIZE;
	int len;
	InternetGetCookieA(g_domain_qq,NULL,m_buffer,&dwSize);

	if (m_storage[WEBQQ_STORAGE_COOKIE]) LocalFree(m_storage[WEBQQ_STORAGE_COOKIE]);
	len=(int)strlen(m_buffer);
	m_storage[WEBQQ_STORAGE_COOKIE]=(LPSTR)LocalAlloc(LMEM_FIXED, len+3);
	strcpy(m_storage[WEBQQ_STORAGE_COOKIE],m_buffer);
	m_storage[WEBQQ_STORAGE_COOKIE][len]=0;
	m_storage[WEBQQ_STORAGE_COOKIE][len+1]=0;
	m_storage[WEBQQ_STORAGE_COOKIE][len+2]=0;

	LPSTR pszCookie=m_storage[WEBQQ_STORAGE_COOKIE];
	LPSTR pszCookie2=strchr(pszCookie,';');

	while (pszCookie=strchr(pszCookie,'=')) {
		if (pszCookie2!=NULL && pszCookie2<pszCookie) {
			*pszCookie2=0;
			pszCookie2[1]=0;
			pszCookie=pszCookie2+2;
			pszCookie2=strchr(pszCookie,';');
		} else {
			*pszCookie=0;
			if (pszCookie=strchr(pszCookie+1,';')) {
				*pszCookie=0;
				pszCookie[1]=0;
				pszCookie+=2;
				pszCookie2=strchr(pszCookie,';');
			} else
				break;
		}
	}
}

LPSTR CLibWebFetion::GetStorage(int index) {
	return m_storage[index];
}

void CLibWebFetion::SetBasePath(LPCSTR pszPath) {
	m_basepath=strdup(pszPath);
}

/** MIMQQ4b **/
LPSTR CLibWebFetion::PostHTMLDocument(LPCSTR pszServer, LPCSTR pszUri, LPCSTR pszReferer, LPCSTR pszPostBody, LPDWORD pdwLength) {
	DWORD dwRead=BUFFERSIZE;
	LPSTR pszBuffer;

	Log("%s > %s",pszUri,pszPostBody);

	HINTERNET hInetConnect;
	if (strncmp(pszServer,"https://",8))
		hInetConnect=InternetConnectA(m_hInet,pszServer,INTERNET_DEFAULT_HTTP_PORT,NULL,NULL,INTERNET_SERVICE_HTTP,0,NULL);
	else
		hInetConnect=InternetConnectA(m_hInet,pszServer+8,INTERNET_DEFAULT_HTTPS_PORT,NULL,NULL,INTERNET_SERVICE_HTTP,0,NULL);

	if (!hInetConnect) {
		Log(__FUNCTION__"(): InternetConnectA() failed: %d",GetLastError());
		*pdwLength=(DWORD)-1;
		return false;
	}

	HINTERNET hInetRequest=HttpOpenRequestA(hInetConnect,"POST",pszUri,NULL,pszReferer,NULL,INTERNET_FLAG_PRAGMA_NOCACHE|INTERNET_FLAG_RELOAD|(strncmp(pszServer,"https://",8)?0:(INTERNET_FLAG_SECURE|INTERNET_FLAG_IGNORE_CERT_CN_INVALID|INTERNET_FLAG_IGNORE_CERT_DATE_INVALID)),NULL);
	if (!hInetRequest) {
		DWORD err=GetLastError();
		Log(__FUNCTION__"(): HttpOpenRequestA() failed: %d", err);
		*pdwLength=(DWORD)-1;
		InternetCloseHandle(hInetConnect);
		SetLastError(err);
		return false;
	}

	if (!(HttpSendRequestA(hInetRequest,"Content-Type: application/x-www-form-urlencoded; charset=UTF-8\r\nX-Requested-With: XMLHttpRequest",-1,(LPVOID)pszPostBody,(DWORD)strlen(pszPostBody)))) {
		DWORD err=GetLastError();
		Log(__FUNCTION__"(): HttpSendRequestA() failed, reason=%d",err);
		InternetCloseHandle(hInetRequest);
		InternetCloseHandle(hInetConnect);
		Log(__FUNCTION__"(): Terminate connection");
		*pdwLength=(DWORD)-1;
		return false;
	}

	dwRead=0;
	DWORD dwWritten=sizeof(DWORD);

	if (!HttpQueryInfo(hInetRequest,HTTP_QUERY_CONTENT_LENGTH|HTTP_QUERY_FLAG_NUMBER,pdwLength,&dwWritten,&dwRead)) {
		Log(__FUNCTION__"(): Warning - HttpQueryInfo failed(%d), *pdwLength set to default!",GetLastError());
		*pdwLength=BUFFERSIZE;
	}
	Log(__FUNCTION__"() size=%d",*pdwLength);

	if (!*pdwLength) *pdwLength=BUFFERSIZE;

	pszBuffer=(LPSTR)LocalAlloc(LMEM_FIXED,*pdwLength+1);
	LPSTR ppszBuffer=pszBuffer;

	while (InternetReadFile(hInetRequest,ppszBuffer,*pdwLength,&dwRead) && dwRead>0) {
		ppszBuffer+=dwRead;
		dwRead=0;
	}
	*ppszBuffer=0;
	*pdwLength=ppszBuffer-pszBuffer;

	InternetCloseHandle(hInetRequest);
	InternetCloseHandle(hInetConnect);

	return pszBuffer;
}

bool CLibWebFetion::web2_channel_poll() {
	LPSTR pszResponse;
	DWORD dwSize;
	bool retried=false;
	char szQuery[384];
	
	while (true) {
		sprintf(szQuery,"/WebIM/GetConnect.aspx?Version=%d",m_sequence++);

		if ((pszResponse=PostHTMLDocument(g_server_webim,szQuery,g_referer_webqq,m_postssid,&dwSize))==NULL || dwSize==0xffffffff) {
			if (GetLastError()==12002) return true; // timeout is now normal
			if (m_stop) {
				Log(__FUNCTION__"() Stop signal detected");
				SetStatus(WEBQQ_STATUS_ERROR);
				return false;
			}

			if (retried) {
				SetStatus(WEBQQ_STATUS_ERROR);
				return false;
			} else {
				Log(__FUNCTION__"() GetLastError()=%d Try again",GetLastError());
				retried=true;
			}
		} else if (*pszResponse) {
			if (*pszResponse=='{') {
				JSONNODE* jn=json_parse(pszResponse);
				int result=json_as_int(json_get(jn,"rc"));

				switch (result) {
					case 200:
						{
							bool ret=true;
							JSONNODE* jnResult=json_get(jn,"rv");
							JSONNODE* jnItem;
							int c=0;
							int nodecount=json_size(jnResult);
							int polltype;
							char szPollType[32]="GetConnect:";

							// TODO: For debug only
							Log("%s",pszResponse);

							for (int c=0; c<nodecount; c++) {
								jnItem=json_at(jnResult,c);
								// name maybe non-quoted, which fails to json name parsing
								// polltype=json_as_int(json_get(jnItem,"DataType"));
								polltype=json_as_int(json_at(jnItem,0));
								Log(__FUNCTION__"(): Poll Type=%d",polltype);

								itoa(polltype,szPollType+11,10);
								// SendToHub(WEBQQ_CALLBACK_WEB2,szPollType,json_get(jnItem,"Data"));
								SendToHub(WEBQQ_CALLBACK_WEB2,szPollType,json_at(jnItem,1));
							}

							return ret;
						}
						break;
					case 302:
						Log(__FUNCTION__"() Empty response (302)");
						break;
					case 304:
						Log(__FUNCTION__"() Event wait timeout (304-OK)");
						break;
					default:
						Log(__FUNCTION__"() Unknown error (%d)",result);
						if (true /*!web2_channel_login()*/) {
							SetStatus(WEBQQ_STATUS_ERROR);
							return false;
						} else
							return true;
				}

				json_delete(jn);
				LocalFree(pszResponse);
				return true;
			} else {
				Log(__FUNCTION__"() Non JSON reply, possibly messed up, try again");
			}
		}
	}
	return false;
}

bool CLibWebFetion::web2_check_result(LPSTR pszCommand, LPSTR pszResult) {
	int retcode=-1;

	if (*pszResult!='{') {
		Log(__FUNCTION__"(): JSON Invalid for %s",pszCommand);
		// SendToHub(WEBQQ_CALLBACK_WEB2_ERROR,pszCommand,NULL);
	} else if (JSONNODE* jn=json_parse(pszResult)) {
		retcode=json_as_int(json_get(jn,"rc"));
		if (retcode==200) {
			SendToHub(WEBQQ_CALLBACK_WEB2,pszCommand,json_size(jn)==1?NULL:json_get(jn,"rv"));
		} else {
			char szMsg[MAX_PATH];
			sprintf(szMsg,"%d",retcode);
			Log(__FUNCTION__"(): Command:%s, Return code %s",pszCommand,szMsg);
			SendToHub(WEBQQ_CALLBACK_WEB2_ERROR,pszCommand,szMsg);
		}
		json_delete(jn);
	} else {
		Log(__FUNCTION__"(): JSON Parse Failed for %s",pszCommand);
		SendToHub(WEBQQ_CALLBACK_WEB2_ERROR,pszCommand,pszResult);
	}

	LocalFree(pszResult);

	return retcode==200;
}

LPSTR CLibWebFetion::UrlEncode(LPSTR pszDst, LPCSTR pszSrc) {
	LPSTR pszBuffer=pszDst;
	LPCSTR pszBody=pszSrc;

	while (*pszBody) {
		if (*pszBody>='a' && *pszBody<='z' || *pszBody>='A' && *pszBody<='Z' || *pszBody>='0' && *pszBody<='9')
			*pszBuffer++=*pszBody;
		else if (*pszBody==' ')
			*pszBuffer++='+';
		else
			pszBuffer+=sprintf(pszBuffer,"%%%02X",(unsigned char)*pszBody);
			// *pszBuffer++=*pszBody;

		pszBody++;
	}

	*pszBuffer=0;

	return pszBuffer;
}

bool CLibWebFetion::WebIM_Generic(LPCSTR pszCommand,LPCSTR pszPost,JSONNODE** jnResponse) {
	LPSTR pszTemp;
	DWORD dwSize;
	static char szQuery[1024];
	LPSTR pszPost2=NULL;

	sprintf(szQuery,"/WebIM/%s.aspx?Version=%d",pszCommand,m_sequence++);

	if (pszPost!=NULL) {
		pszPost2=(LPSTR)LocalAlloc(LMEM_FIXED,strlen(pszPost)+strlen(m_postssid)+2);
		sprintf(pszPost2,"%s&%s",m_postssid,pszPost);
	}

	if ((pszTemp=PostHTMLDocument(g_server_webim, szQuery, g_referer_webqq,pszPost2?pszPost2:m_postssid,&dwSize))!=NULL && dwSize!=0xffffffff) {
		Log("%s",pszTemp);
		if (pszPost2) LocalFree(pszPost2);

		if (jnResponse==NULL)
			return web2_check_result((LPSTR)pszCommand,pszTemp);
		else {
			*jnResponse=json_parse(pszTemp);
			LocalFree(pszTemp);
			return json_type(jnResponse)!=JSON_NULL;
		}
	}

	if (pszPost2) LocalFree(pszPost2);
	return false;
}

bool CLibWebFetion::WebIM_GetPersonalInfo() {
	return WebIM_Generic("GetPersonalInfo");
}

bool CLibWebFetion::WebIM_GetContactList() {
	return WebIM_Generic("GetContactList");
}

bool CLibWebFetion::WebIM_SetPresence(int presence, LPSTR pszCustom) {
	if (presence==-1) {
		return WebIM_Generic("logout");
	} else {
		LPSTR pszBuffer;
		LPSTR pszPOSTBuffer=pszBuffer=(LPSTR)LocalAlloc(LMEM_FIXED,(strlen(pszCustom)+1)*3+32);

		pszBuffer+=sprintf(pszBuffer,"Presence=%d&Custom=",presence);
		pszBuffer=UrlEncode(pszBuffer,pszCustom);

		bool ret=WebIM_Generic("SetPresence",pszPOSTBuffer);
		LocalFree(pszPOSTBuffer);
		return ret;
	}
}

bool CLibWebFetion::WebIM_SendMsg(DWORD dwUid, LPSTR pszMsg, bool fSms) {
	LPSTR pszBuffer;
	LPSTR pszPOSTBuffer=pszBuffer=(LPSTR)LocalAlloc(LMEM_FIXED,(strlen(pszMsg)+1)*3+32);

	pszBuffer+=sprintf(pszBuffer,"To=%u&msg=",dwUid);
	pszBuffer=UrlEncode(pszBuffer,pszMsg);
	sprintf(pszBuffer,"&IsSendSms=%d",fSms?1:0);

	bool ret=WebIM_Generic("SendMsg",pszPOSTBuffer);
	LocalFree(pszPOSTBuffer);
	return ret;
}

bool CLibWebFetion::WebIM_SetImpresa(LPSTR pszImpresa) {
	LPSTR pszBuffer;
	LPSTR pszPOSTBuffer=pszBuffer=(LPSTR)LocalAlloc(LMEM_FIXED,(strlen(pszImpresa)+1)*3+32);

	pszBuffer+=sprintf(pszBuffer,"Impresa=");
	pszBuffer=UrlEncode(pszBuffer,pszImpresa);

	bool ret=WebIM_Generic("SetPresence",pszPOSTBuffer);
	LocalFree(pszPOSTBuffer);
	return ret;
}

bool CLibWebFetion::WebIM_HandleAddBuddy(DWORD dwUid, bool fAgree, LPSTR pszLocalName, int groupIndex) {
	char szPost[MAX_PATH];
	LPSTR pszPost=szPost;
	pszPost+=sprintf(pszPost,"BuddyId=%u&Result=%d&BuddyList=%d&LocalName=",dwUid,fAgree?1:0,groupIndex);
	UrlEncode(pszPost,pszLocalName);

	return WebIM_Generic("HandleAddBuddy",szPost);
}

bool CLibWebFetion::WebIM_GetPortrait(DWORD dwUid, int size, LPSTR pszCrc) {
	char szUrl[MAX_PATH];
	LPSTR pszTemp;
	DWORD dwSize;
	sprintf(szUrl,"http://webim.feixin.10086.cn/WebIM/GetPortrait.aspx?did=%u&Size=%d&Crc=%s&mid=%u",dwUid,size,pszCrc,dwUid);

	if ((pszTemp=GetHTMLDocument(szUrl,g_referer_webqq,&dwSize))!=NULL && dwSize!=0xffffffff) {
		sprintf(szUrl,"GetPortrait:%u",dwUid);
					
		SendToHub(WEBQQ_CALLBACK_WEB2,szUrl,pszTemp);
		LocalFree(pszTemp);
		return true;
	}

	return false;
}

int CLibWebFetion::WebIM_AddBuddy(LPCSTR pszUidOrPhone, LPCSTR pszDesc, LPCSTR pszLocalName, int groupIndex) {
	char szCode[5]={0};
	char szPost[MAX_PATH*2];

	sprintf(szPost,"http://webim.feixin.10086.cn/WebIM/GetPicCode.aspx?Type=addbuddy_ccpsession&%f",(double)rand()/(double)RAND_MAX);

	SendToHub(WEBQQ_CALLBACK_NEEDVERIFY,szPost,szCode);

	if (*szCode) {
		LPSTR pszPost=szPost;
		JSONNODE* jnResponse;

		RefreshCookie();

		pszPost+=sprintf(pszPost,"AddType=%d&UserName=%s&Desc=",strlen(pszUidOrPhone)==11?1:0,pszUidOrPhone);
		pszPost=UrlEncode(pszPost,pszDesc);
		pszPost+=sprintf(pszPost,"&LocalName=");
		pszPost=UrlEncode(pszPost,pszLocalName);
		pszPost+=sprintf(pszPost,"&Ccp=%s&CcpId=%s&BuddyLists=%d&PhraseId=0&SubscribeFlag=0",szCode,GetCookie("addbuddy_ccpsession"),groupIndex);

		if (WebIM_Generic("AddBuddy",szPost,&jnResponse)) {
			// 200=OK, 312=VerifyFail, 404=UserNotFound, 521=BuddyExists
			int ret=json_as_int(json_get(jnResponse,"rc"));
			json_delete(jnResponse);
			return ret;
		}
	}

	return 0;
}

int CLibWebFetion::FetchUserHead(DWORD myid, DWORD qqid, LPSTR crc, LPSTR saveto) {
	// Return 0=Fail, 1=BMP, 2=GIF, 3=JPG/Unknown
	// http://webim.feixin.10086.cn/WebIM/GetPortrait.aspx?did=860538411&Size=3&Crc=2110977292&mid=739420892
	char szUrl[MAX_PATH];
	DWORD dwSize;
	LPSTR pszFile;
	
	sprintf(szUrl,"http://webim.feixin.10086.cn/WebIM/GetPortrait.aspx?did=%u&Size=3&Crc=%s&mid=%u",qqid,crc,myid);
	pszFile=GetHTMLDocument(szUrl,g_referer_webqq,&dwSize);

	if (dwSize!=(DWORD)-1 && dwSize>0 && pszFile!=NULL) {
		int format=memcmp(pszFile,"BM",2)?memcmp(pszFile,"GIF",3)?3:2:1;

		strcpy(strrchr(saveto,'.')+1,format==1?"bmp":format==2?"gif":"jpg");

		HANDLE hFile=CreateFileA(saveto,GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,CREATE_ALWAYS,0,NULL);
		if (hFile!=INVALID_HANDLE_VALUE) {
			DWORD dwWritten;
			WriteFile(hFile,pszFile,dwSize,&dwWritten,NULL);
			CloseHandle(hFile);
			LocalFree(pszFile);
			return format;
		}
		LocalFree(pszFile);
	} else
		Log(__FUNCTION__"(): Invalid response data");

	return 0;
}

bool CLibWebFetion::ParseResponse4b(JSONNODE* jnResult) {
	bool ret=true;
	JSONNODE* jnItem;
	int c=0;
	int nodecount=json_size(jnResult);
	LPSTR pszPollType;

	for (int c=0; c<nodecount; c++) {
		jnItem=json_at(jnResult,c);
		pszPollType=json_as_string(json_get(jnItem,"poll_type"));
		Log(__FUNCTION__"(): Poll Type=%s",pszPollType);
		json_free(pszPollType);
	}

	return ret;
}

/** Test **/
void CLibWebFetion::Test() {
}
