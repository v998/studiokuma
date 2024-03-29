#include "StdAfx.h"
#pragma comment(lib,"wininet")
#pragma comment(lib,"Advapi32")

#define BUFFERSIZE 1048576
#define OUTBUFFERSIZE 1048576

LPCSTR CLibWebQQ::g_referer_webqq="http://webqq.qq.com/";
LPCSTR CLibWebQQ::g_referer_webproxy="http://web-proxy2.qq.com/";
LPCSTR CLibWebQQ::g_domain_qq="http://qq.com";
LPCSTR CLibWebQQ::g_referer_main="http://webqq.qq.com/main.shtml?direct__2";
LPCSTR CLibWebQQ::g_referer_web2="http://web2.qq.com/";
LPCSTR CLibWebQQ::g_referer_web2proxy="http://s.web2.qq.com/proxy.html?v=20101025002";
LPCSTR CLibWebQQ::g_server_web2_connection="s.web2.qq.com";

CLibWebQQ::CLibWebQQ(DWORD dwQQID, LPSTR pszPassword, LPVOID pvObject, WEBQQ_CALLBACK_HUB wch, HANDLE hNetlib):
	m_hEventHTML(CreateEvent(NULL,FALSE,TRUE,NULL)), 
	m_hEventCONN(CreateEvent(NULL,FALSE,TRUE,NULL)), 
	m_qqid(dwQQID), 
	m_password(strdup(pszPassword)), 
	m_userobject(pvObject), 
	m_wch(wch),
	m_status(WEBQQ_STATUS_OFFLINE),
	m_sequence(0),
	m_uv(0),
	m_buffer(NULL),
	m_outbuffer(NULL),
	m_r_cookie(0),
	m_proxyhost(NULL),
	m_appid(NULL),
	m_basepath(NULL),
	cs_0x26_timeout(0),
	cs_0x3e_next_pos(0),
	m_hInetRequest(NULL),
	m_processstatus(0),
	m_loginhide(FALSE),
	m_proxyuser(NULL),
	m_proxypass(NULL),
	m_web2_vfwebqq(NULL),
	m_web2_psessionid(NULL),
	m_useweb2(FALSE),
	m_web2_nextqunkey(0),
	m_stop(false),
	m_hNetlib(hNetlib) {
	memset(m_storage,0,10*sizeof(LPSTR));
	memset(m_web2_storage,0,10*sizeof(JSONNODE*));
	itoa(rand()%100,m_web2_clientid,10);
	ultoa(time(NULL)%1000000,m_web2_clientid+strlen(m_web2_clientid),10);
}

CLibWebQQ::~CLibWebQQ() {
	Stop();
	SetStatus(WEBQQ_STATUS_OFFLINE);

	CloseHandle(m_hEventHTML);
	CloseHandle(m_hEventCONN);

	for (map<DWORD,LPWEBQQ_OUT_PACKET>::iterator iter=m_outpackets.begin(); iter!=m_outpackets.end(); iter++) {
		LocalFree(iter->second->cmd);
		LocalFree(iter->second);
	}

	if (m_web2_vfwebqq) json_free(m_web2_vfwebqq);
	if (m_web2_psessionid) json_free(m_web2_psessionid);
	if (m_password) free(m_password);
	if (m_proxyhost) free(m_proxyhost);
	if (m_proxypass) free(m_proxypass);
	if (m_basepath) free(m_basepath);
	if (m_buffer) LocalFree(m_buffer);
	if (m_outbuffer) LocalFree(m_outbuffer);
	if (m_appid) free(m_appid);

	for (int c=0; c<10; c++) {
		if (m_storage[c]) LocalFree(m_storage[c]);
		if (m_web2_storage[c]) json_delete(m_web2_storage[c]);
	}

	Log(__FUNCTION__"(): Instance Destruction");
}

void CLibWebQQ::CleanUp() {
	// TODO: Really needed?
}

void CLibWebQQ::Start() {
	DWORD dwThread;
	CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)_ThreadProc,this,0,&dwThread);
}

void CLibWebQQ::Stop() {
	if (m_hInet) {
		HINTERNET hInet=m_hInet;
		m_stop=true;
		if (m_useweb2 && m_status>=WEBQQ_STATUS_ONLINE) web2_channel_change_status("offline");
		m_hInet=NULL;
		InternetCloseHandle(hInet);
	}
}

void CLibWebQQ::SetLoginHide(BOOL val) {
	m_loginhide=val; 
}

DWORD WINAPI CLibWebQQ::_ThreadProc(CLibWebQQ* lpParameter) {
	return lpParameter->ThreadProc();
}

void CLibWebQQ::ThreadLoop4b() {
	bool breakloop=false;

	/*
	msg_id:
    o = (new Date()).getTime(), o = (o - o % 1000) / 1000;
    o = o % 10000 * 10000;
    var n = function () {
        k++;
        return o + k;
    };
	*/

	m_sequence=time(NULL) % 10000 * 10000;

	if (!web2_channel_login()) {
		SetStatus(WEBQQ_STATUS_ERROR);
		return;
	}

loopstart:
	__try {
		while (m_status!=WEBQQ_STATUS_ERROR && m_stop==false && (breakloop=web2_channel_poll()));
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		Log("CLibWebQQ(4b) Crashed at process phase %d, trying to resume operation...",m_processstatus);
		SendToHub(WEBQQ_CALLBACK_CRASH,NULL,(LPVOID)m_processstatus);

		if (!m_storage[WEBQQ_STORAGE_PARAMS]) {
			SetStatus(WEBQQ_STATUS_ERROR);
		}

		goto loopstart;
	}

	// if (!breakloop)	LocalFree(m_outbuffer);
}

DWORD CLibWebQQ::ThreadProc() {
	if (!ProbeAppID()) return 0;
	if (m_status!=WEBQQ_STATUS_ERROR) {
		if (!PrepareParams()) return 0;
	}
	if (m_status!=WEBQQ_STATUS_ERROR) {
		if (!Negotiate()) return 0;
	}
	if (m_status!=WEBQQ_STATUS_ERROR) {
		if (!m_useweb2) {
			m_outbuffer=m_outbuffer_current=(LPSTR)LocalAlloc(LMEM_FIXED,OUTBUFFERSIZE);
			*m_outbuffer=0;
			sprintf(m_buffer,"o_cookie=%u",m_qqid);
			InternetSetCookieA(g_domain_qq,NULL,m_buffer);
		}

		RefreshCookie();

		Log(__FUNCTION__"(): Initial login complete, use engine=%s",m_useweb2?"MIMQQ4b(web2)":"MIMQQ4a(webqq)");
		if (m_useweb2)
			ThreadLoop4b();
	}

	return 0;
};

void CLibWebQQ::SetStatus(WEBQQSTATUSENUM newstatus) {
	LPARAM dwStatus=MAKELPARAM(m_status,newstatus);
	m_status=newstatus;
	SendToHub(WEBQQ_CALLBACK_CHANGESTATUS,NULL,&dwStatus);
}

void CLibWebQQ::SendToHub(DWORD dwCommand, LPSTR pszArgs, LPVOID pvCustom) {
	if (m_wch) {
		if ((dwCommand & 0xfffff000)==0xffff0000) m_processstatus=11;
		m_wch(m_userobject,dwCommand,pszArgs,pvCustom);
	}
}

void CLibWebQQ::SetProxy(LPSTR pszHost, LPSTR pszUser, LPSTR pszPass) {
	if (m_proxyhost) free(m_proxyhost);
	if (m_proxyuser) free(m_proxyuser);
	if (m_proxypass) free(m_proxypass);
	m_proxyhost=strdup(pszHost);
	if (pszUser && *pszUser) m_proxyuser=strdup(pszUser);
	if (pszPass && *pszPass) m_proxypass=strdup(pszPass);
}

LPSTR CLibWebQQ::GetArgument(LPSTR pszArgs, int n) {
	for (int c=0; c<n; c++) {
		pszArgs+=strlen(pszArgs)+1;
	}
	return pszArgs;
}

LPSTR CLibWebQQ::GetCookie(LPCSTR pszName) {
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

void CLibWebQQ::Log(char *fmt,...) {
	static CHAR szLog[1024];
	va_list vl;

	va_start(vl, fmt);
	
	szLog[_vsnprintf(szLog, sizeof(szLog)-1, fmt, vl)]=0;
	SendToHub(WEBQQ_CALLBACK_DEBUGMESSAGE,szLog);

	va_end(vl);
}

DWORD CLibWebQQ::GetRND2() {
	srand(GetTickCount());
	SYSTEMTIME st;
	GetSystemTime(&st);
	return ((DWORD)floor(((double)rand()/(double)RAND_MAX)*(double)UINT_MAX+0.5f)*st.wMilliseconds)%10000000000;
}

DWORD CLibWebQQ::GetUV() {
	return m_uv?m_uv:GetRND2();
}

DWORD CLibWebQQ::GetRND() {
	srand(GetTickCount());
	return (DWORD)floor(((double)rand()/(double)RAND_MAX)*(double)100000+0.5f);
}

LPCSTR CLibWebQQ::GetNRND() {
	static char szNRND[20]={0};
	
	if (!*szNRND) {
		SYSTEMTIME st;
		GetSystemTime(&st);
		sprintf(szNRND,"F%d%d%d%d%u", st.wYear%100,st.wMonth,st.wDay,st.wMilliseconds,GetRND());
	}
	return szNRND;
}

LPCSTR CLibWebQQ::GetSSID() {
	static char szSSID[20]={0};

	if (!*szSSID) {
		sprintf(szSSID,"s%u",GetRND2());
	}
	return szSSID;
}

void CLibWebQQ::SetUseWeb2(bool val) {
	m_useweb2=val;
}

LPSTR CLibWebQQ::GetHTMLDocument(LPCSTR pszUrl, LPCSTR pszReferer, LPDWORD pdwLength, BOOL fWeb2Assign) {
	// char szFileName[MAX_PATH];
	LPSTR pszBuffer;

	*pdwLength=0;
	HINTERNET hInet=m_hInet;

	/*
	if (!(hInet=InternetOpenA("Mozilla/5.0 (Windows; U; Windows NT 5.1; en-US; rv:1.9.1.3) Gecko/20090824 Firefox/3.5.3 GTB5",m_proxyhost?INTERNET_OPEN_TYPE_PROXY:INTERNET_OPEN_TYPE_DIRECT,m_proxyhost,NULL,0))) {
		Log(__FUNCTION__"(): Failed initializing WinINet (InternetOpen)! Err=%d\n",GetLastError());
		*pdwLength=(DWORD)-1;
		return FALSE;
	}
	if (fWeb2Assign) this->m_hInet=hInet;

	if (m_proxyuser) {
		InternetSetOption(hInet,INTERNET_OPTION_USERNAME, m_proxyuser,  (DWORD)strlen(m_proxyuser)+1);  
		if (m_proxypass) {
			InternetSetOption(hInet,INTERNET_OPTION_PASSWORD, m_proxypass,  (DWORD)strlen(m_proxypass)+1);  
		}
	}
	*/

	LPSTR pszServer=(LPSTR)strstr(pszUrl,"//")+2;
	LPSTR pszUri=strchr(pszServer,'/');
	*pszUri=0;

	HINTERNET hInetConnect=InternetConnectA(hInet,pszServer,INTERNET_DEFAULT_HTTP_PORT,NULL,NULL,INTERNET_SERVICE_HTTP,0,NULL);
	*pszUri='/';

	if (!hInetConnect) {
		Log(__FUNCTION__"(): InternetConnectA() failed: %d, hInet=%p",GetLastError(),hInet);
		*pdwLength=(DWORD)-1;
		// InternetCloseHandle(hInet);
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
		// InternetCloseHandle(hInet);
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
	// InternetCloseHandle(hInet);

	return pszBuffer;
}

LPCSTR CLibWebQQ::GetReferer(WEBQQREFERERENUM type) {
	switch (type) {
		case WEBQQ_REFERER_WEBQQ:
			return g_referer_webqq;
		case WEBQQ_REFERER_WEBPROXY:
			return g_referer_webproxy;
		case WEBQQ_REFERER_PTLOGIN:
			return m_referer_ptlogin;
		case WEBQQ_REFERER_MAIN:
			return g_referer_main;
		case WEBQQ_REFERER_WEB2:
			return g_referer_web2;
		case WEBQQ_REFERER_WEB2PROXY:
			return g_referer_web2proxy;
		default:
			return NULL;
	}
}

DWORD CLibWebQQ::AppendQuery(DWORD (*func)(CLibWebQQ*,LPSTR,LPSTR), LPSTR pszArgs) {
	// static DWORD m_sequence=0;
	DWORD ret;
	DWORD fn=func(NULL,NULL,NULL);
	DWORD ack=func(this,NULL,NULL);
	DWORD seq=0;
	LPSTR pszStart;
	bool firstpacket=(*m_outbuffer==0);
	WaitForSingleObject(m_hEventCONN,INFINITE);

	pszStart=m_outbuffer_current=m_outbuffer_current+strlen(m_outbuffer_current);

	if (fn==0x17) {
		m_outbuffer_current+=sprintf(m_outbuffer_current,"%s%u;%02x;",*m_outbuffer?"\x1d":"",m_qqid,fn);
	} else {
		m_outbuffer_current+=sprintf(m_outbuffer_current,fn>0xff?"%s%u;%04x;%u;%s;":"%s%u;%02x;%u;%s;",*m_outbuffer?"\x1d":"",m_qqid,fn,seq=m_sequence,m_storage[WEBQQ_STORAGE_PARAMS]?GetArgument(m_storage[WEBQQ_STORAGE_PARAMS],SC0X22_WEB_SESSION):"00000000");
		if (m_storage[WEBQQ_STORAGE_PARAMS]) m_sequence++;
	}
	m_outbuffer_current+=(ret=func(this,m_outbuffer_current,pszArgs));
	if (ret) {
		strcpy(m_outbuffer_current,";");
		m_outbuffer_current++;
	}
	/*
	if (ack) {
		LPWEBQQ_OUT_PACKET lpOP=m_outpackets[seq];
		if (lpOP) {
			LocalFree(lpOP->cmd);
			LocalFree(lpOP);
		}

		if (!firstpacket) pszStart++;
		lpOP=(LPWEBQQ_OUT_PACKET)LocalAlloc(LMEM_FIXED,sizeof(WEBQQ_OUT_PACKET));
		lpOP->type=fn;
		lpOP->created=GetTickCount();
		lpOP->retried=0;
		lpOP->cmd=(LPSTR)LocalAlloc(LMEM_FIXED,strlen(pszStart)+1);
		memcpy(lpOP->cmd,pszStart,strlen(pszStart)+1);
		m_outpackets[seq]=lpOP;
	}
	*/
	SetEvent(m_hEventCONN);
	return seq;
}

void CLibWebQQ::GetPasswordHash(LPCSTR pszVerifyCode, LPSTR pszOut) {
	char szTemp[50];
	char szTemp2[16];
	DWORD dwSize;

	LPSTR ppszTemp;

	HCRYPTPROV hCP;
	HCRYPTHASH hCH;
	CryptAcquireContextA(&hCP,NULL,MS_DEF_PROV_A,PROV_RSA_FULL,CRYPT_VERIFYCONTEXT);

	// #1
	CryptCreateHash(hCP,CALG_MD5,NULL,0,&hCH);
	CryptHashData(hCH,(LPBYTE)m_password,(DWORD)strlen(m_password),0);
	dwSize=MAX_PATH;
	CryptGetHashParam(hCH,HP_HASHVAL,(LPBYTE)szTemp2,&dwSize,0);
	CryptDestroyHash(hCH);

	// #2
	CryptCreateHash(hCP,CALG_MD5,NULL,0,&hCH);
	CryptHashData(hCH,(LPBYTE)szTemp2,16,0);
	dwSize=MAX_PATH;
	CryptGetHashParam(hCH,HP_HASHVAL,(LPBYTE)szTemp,&dwSize,0);
	CryptDestroyHash(hCH);

	// #3
	CryptCreateHash(hCP,CALG_MD5,NULL,0,&hCH);
	CryptHashData(hCH,(LPBYTE)szTemp,16,0);
	dwSize=MAX_PATH;
	CryptGetHashParam(hCH,HP_HASHVAL,(LPBYTE)szTemp2,&dwSize,0);
	CryptDestroyHash(hCH);

	// #4
	CryptCreateHash(hCP,CALG_MD5,NULL,0,&hCH);
	ppszTemp=szTemp;
	for (LPSTR ppszTemp2=szTemp2; ppszTemp2-szTemp2<16; ppszTemp2++) {
		ppszTemp+=sprintf(ppszTemp,"%02X",*(LPBYTE)ppszTemp2);
	}

	strcpy(szTemp+32,pszVerifyCode);
	CryptHashData(hCH,(LPBYTE)szTemp,32+strlen(pszVerifyCode),0);
	dwSize=MAX_PATH;
	CryptGetHashParam(hCH,HP_HASHVAL,(LPBYTE)szTemp2,&dwSize,0);
	CryptDestroyHash(hCH);

	CryptReleaseContext(hCP,0);

	ppszTemp=pszOut;
	for (LPSTR ppszTemp2=szTemp2; ppszTemp2-szTemp2<16; ppszTemp2++) {
		ppszTemp+=sprintf(ppszTemp,"%02X",*(LPBYTE)ppszTemp2);
	}
}

bool CLibWebQQ::ProbeAppID() {
	SetStatus(WEBQQ_STATUS_PROBE);

	/*if (!m_useweb2)*/ {
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
	}
	/*
	DWORD dwSize;
	LPSTR pszData=GetHTMLDocument("http://webqq.qq.com/",NULL,&dwSize);
	if (!pszData) {
		SetStatus(WEBQQ_STATUS_ERROR);
		return FALSE;
	}

	LPSTR ppszData=strstr(pszData,"&appid=");
	if (ppszData) {
		*strchr(ppszData+1,'&')=0;
		m_appid=strdup(ppszData+7);
		Log(__FUNCTION__"(): APPID=%s",m_appid);
	} else {
		Log(__FUNCTION__"(): Failed getting APPID!");
		LocalFree(pszData);
		SetStatus(WEBQQ_STATUS_ERROR);
		return FALSE;
	}

	LocalFree(pszData);
	*/
	m_appid=strdup("1002101");

	sprintf(m_referer_ptlogin,"http://ui.ptlogin2.qq.com/cgi-bin/login?style=4&appid=%s&enable_qlogin=0&no_verifyimg=1&s_url=http://webqq.qq.com/main.shtml?direct__2&f_url=loginerroralert",m_appid);
	return TRUE;
}

bool CLibWebQQ::PrepareParams() {
	// LPSTR pszData;
	// DWORD dwSize;
	char szTemp[30];

	SetStatus(WEBQQ_STATUS_PREPARE);
	if (!m_buffer) m_buffer=(LPSTR)LocalAlloc(LMEM_FIXED,BUFFERSIZE); // Cannot remove because RefreshCookie() needs it

/*
GET /collect?pj=1990&dm=webqq.qq.com&url=/&arg=&rdm=&rurl=&rarg=&icache=-&uv=1527858512&nu=1&ol=0&loc=http%3A//webqq.qq.com/&column=&subject=&nrnd=F106981946296&rnd=7546 HTTP/1.1
Host: trace.qq.com:80
Referer: http://webqq.qq.com/
Cookie: pgv_pvid=1527858512; pgv_flv=9.0 r100; pgv_info=ssid=s7498192482; pgv_r_cookie=106981946296
*/
	sprintf(szTemp,"pgv_pvid=%u",GetUV());
	InternetSetCookieA(g_domain_qq,NULL,szTemp);
	InternetSetCookieA(g_domain_qq,NULL,"pgv_flv=9.0 r100");

	sprintf(szTemp,"pgv_info=ssid=%s",GetSSID());
	InternetSetCookieA(g_domain_qq,NULL,szTemp);

	sprintf(szTemp,"pgv_r_cookie=%s",GetNRND()+1);
	InternetSetCookieA(g_domain_qq,NULL,szTemp);

	/*
	sprintf(m_buffer,"http://trace.qq.com:80/collect?pj=1990&dm=webqq.qq.com&url=/&arg=&rdm=&rurl=&rarg=&icache=-&uv=%u&nu=1&ol=0&loc=%s&column=&subject=&nrnd=%s&rnd=%u",GetUV(),"http%3A//webqq.qq.com/",GetNRND(),GetRND());
	if (pszData=GetHTMLDocument(m_buffer,g_referer_webqq,&dwSize)) LocalFree(pszData);
	if (dwSize==(DWORD)-1) {
		SetStatus(WEBQQ_STATUS_ERROR);
		return false;
	}
	*/

/*
GET /pingd?dm=webqq.qq.com&url=/&tt=WebQQ%20%u2013%20%u7F51%u9875%u76F4%u63A5%u804AQQ%uFF0CQQ%u65E0%u6240%u4E0D%u5728&rdm=-&rurl=-&pvid=-&scr=1024x768&scl=24-bit&lang=en&java=0&cc=undefined&pf=Linux%20i686&tz=-8&flash=9.0%20r100&ct=-&vs=3.2&column=&arg=&rarg=&ext=4&reserved1=&hurlcn=F106981946296&rand=74062 HTTP/1.1
Host: pingfore.qq.com
Referer: http://webqq.qq.com/
Cookie: pgv_pvid=1527858512; pgv_flv=9.0 r100; pgv_info=ssid=s7498192482; pgv_r_cookie=106981946296
*/
	/*
	sprintf(m_buffer,"http://pingfore.qq.com/pingd?dm=webqq.qq.com&%s&hurlcn=%s&rand=%u","url=/&tt=WebQQ%20%u2013%20%u7F51%u9875%u76F4%u63A5%u804AQQ%uFF0CQQ%u65E0%u6240%u4E0D%u5728&rdm=-&rurl=-&pvid=-&scr=1024x768&scl=24-bit&lang=en&java=0&cc=undefined&pf=Linux%20i686&tz=-8&flash=9.0%20r100&ct=-&vs=3.2&column=&arg=&rarg=&ext=4&reserved1=",GetNRND(),GetRND());
	if (pszData=GetHTMLDocument(m_buffer,g_referer_webqq,&dwSize)) LocalFree(pszData);
	if (dwSize==(DWORD)-1) {
		SetStatus(WEBQQ_STATUS_ERROR);
		return false;
	}
	*/

/*
GET /cr?id=5&d=datapt=v1.2|scripts::http%3A%2F%2Fxui.ptlogin2.%22|http%3A%2F%2Fui.ptlogin2.qq.com%2Fcgi-bin%2Flogin%3Fstyle%3D4%26appid%3D1002101%26enable_qlogin%3D0%26no_verifyimg%3D1%26s_url%3Dhttp%3A%2F%2Fwebqq.qq.com%2Fmain.shtml%3Fdirect__2%26f_url%3Dloginerroralert HTTP/1.1
Host: cr.sec.qq.com
Referer: http://ui.ptlogin2.qq.com/cgi-bin/login?style=4&appid=1002101&enable_qlogin=0&no_verifyimg=1&s_url=http://webqq.qq.com/main.shtml?direct__2&f_url=loginerroralert
Cookie: pgv_pvid=1527858512; pgv_flv=9.0 r100; pgv_info=ssid=s7498192482; pgv_r_cookie=106981946296
*/
	/*
	sprintf(m_buffer,"http://cr.sec.qq.com/cr?id=5&d=datapt=v1.2|scripts::%s%s%s","http%3A%2F%2Fxui.ptlogin2.%22|http%3A%2F%2Fui.ptlogin2.qq.com%2Fcgi-bin%2Flogin%3Fstyle%3D4%26appid%3D",m_appid,"%26enable_qlogin%3D0%26no_verifyimg%3D1%26s_url%3Dhttp%3A%2F%2Fwebqq.qq.com%2Fmain.shtml%3Fdirect__2%26f_url%3Dloginerroralert");
	if (pszData=GetHTMLDocument(m_buffer,m_referer_ptlogin,&dwSize)) LocalFree(pszData);
	if (dwSize==(DWORD)-1) {
		SetStatus(WEBQQ_STATUS_ERROR);
		return false;
	}
	*/

/*
GET /cr?id=5&d=datapp=v1.2|scripts::http%3A%2F%2F58.60.13.192%2Fcgi-bin%2Fwebqq_stat%3Fo%3D%22|scripts::http%3A%2F%2F58.60.13.192%2Fcgi-bin%2Fwebqq_stat%3Fo%3D%22|scripts::http%3A%2F%2F58.60.13.192%2Fcgi-bin%2Fwebqq%2Fwebqq_report%3Fid%3D%22|scripts::http%3A%2F%2F58.60.13.192%2Fcgi-bin%2Fwebqq%2Fwebqq_report%3Fid%3D%22|http%3A%2F%2Fwebqq.qq.com%2F HTTP/1.1
Host: cr.sec.qq.com
Referer: http://ui.ptlogin2.qq.com/cgi-bin/login?style=4&appid=1002101&enable_qlogin=0&no_verifyimg=1&s_url=http://webqq.qq.com/main.shtml?direct__2&f_url=loginerroralert
Cookie: pgv_pvid=1527858512; pgv_flv=9.0 r100; pgv_info=ssid=s7498192482; pgv_r_cookie=106981946296
*/
	/*
	if (pszData=GetHTMLDocument("http://cr.sec.qq.com/cr?id=5&d=datapp=v1.2|scripts::http%3A%2F%2F58.60.13.192%2Fcgi-bin%2Fwebqq_stat%3Fo%3D%22|scripts::http%3A%2F%2F58.60.13.192%2Fcgi-bin%2Fwebqq_stat%3Fo%3D%22|scripts::http%3A%2F%2F58.60.13.192%2Fcgi-bin%2Fwebqq%2Fwebqq_report%3Fid%3D%22|scripts::http%3A%2F%2F58.60.13.192%2Fcgi-bin%2Fwebqq%2Fwebqq_report%3Fid%3D%22|http%3A%2F%2Fwebqq.qq.com%2F",m_referer_ptlogin,&dwSize)) LocalFree(pszData);
	if (dwSize==(DWORD)-1) {
		SetStatus(WEBQQ_STATUS_ERROR);
		return false;
	}
	*/

	return true;
}

bool CLibWebQQ::Negotiate() {
	LPSTR pszData;
	DWORD dwSize;
	CHAR szCode[16]={0};
	char szTemp[MAX_PATH];

	SetStatus(WEBQQ_STATUS_NEGOTIATE);
/*
GET /check?uin=431533706&appid=1002101&r=0.1788768728924257 HTTP/1.1
Host: ptlogin2.qq.com
Referer: http://ui.ptlogin2.qq.com/cgi-bin/login?style=4&appid=1002101&enable_qlogin=0&no_verifyimg=1&s_url=http://webqq.qq.com/main.shtml?direct__2&f_url=loginerroralert
Cookie: pgv_pvid=1527858512; pgv_flv=9.0 r100; pgv_info=ssid=s7498192482; pgv_r_cookie=106981946296; ptvfsession=38b11f2837efd6e0a3cead6ab1dbaea1315345b6668ab9e9d7bf7116f139652d4a8e28c44636f8bee26bfaa1dbcf9002

ptui_checkVC('1','');
*/
	srand(GetTickCount());
	// sprintf(szTemp,"http://ptlogin2.qq.com/check?uin=%u&appid=1002101&r=%f",m_qqid,(double)rand()/(double)RAND_MAX);
	sprintf(szTemp,"http://ptlogin2.qq.com/check?uin=%u&appid=1003903&r=%f",m_qqid,(double)rand()/(double)RAND_MAX);
	if (!(pszData=GetHTMLDocument(szTemp,m_referer_ptlogin,&dwSize))) {
		SetStatus(WEBQQ_STATUS_ERROR);
		return false;;
	}

	if (strstr(pszData,"ptui_checkVC('0'")) {
		LPSTR pszCode=strstr(pszData,"','")+3;
		*strchr(pszCode,'\'')=0;
		strcpy(szCode,pszCode);
		LocalFree(pszData);
		return Login(szCode);
	} else if (strstr(pszData,"ptui_checkVC('1'")) {
		// ptui_checkVC('1','b422d0878a7017b6ed554ee6fe8d166a52b500e9e7b5e250');
		char szPath[MAX_PATH]={0};
		// DWORD dwWritten;
		LPSTR pszVC_TYPE=strstr(pszData,"','")+3;

		*strchr(pszVC_TYPE,'\'')=0;

		/*
		if (m_basepath) {
			strcat(strcpy(szPath,m_basepath),"\\");
		}
		sprintf(szPath+strlen(szPath),"verycode-%u.jpg",m_qqid);
		*/
/*
GET /getimage?aid=1002101&r=0.754857305282543 HTTP/1.1
Host: ptlogin2.qq.com
Referer: http://ui.ptlogin2.qq.com/cgi-bin/login?style=4&appid=1002101&enable_qlogin=0&no_verifyimg=1&s_url=http://webqq.qq.com/main.shtml?direct__2&f_url=loginerroralert
Cookie: pgv_pvid=1527858512; pgv_flv=9.0 r100; pgv_info=ssid=s7498192482; pgv_r_cookie=106981946296; ptvfsession=38b11f2837efd6e0a3cead6ab1dbaea1315345b6668ab9e9d7bf7116f139652d4a8e28c44636f8bee26bfaa1dbcf9002
*/
// http://captcha.qq.com/getimage?aid=1002101&r=0.8544500968419015&uin=431533706&vc_type=b422d0878a7017b6ed554ee6fe8d166a52b500e9e7b5e250

		srand(GetTickCount());
		// sprintf(m_buffer,"http://ptlogin2.qq.com/getimage?aid=%s&r=%f",m_appid,(double)rand()/(double)RAND_MAX);
		sprintf(szTemp,"http://captcha.qq.com/getimage?aid=1002101&r=%f&uin=%u&vc_type=%s",(double)rand()/(double)RAND_MAX,m_qqid,pszVC_TYPE);
		LocalFree(pszData); // Free here because pszVC_TYPE

		SendToHub(WEBQQ_CALLBACK_NEEDVERIFY,szTemp,szCode);
		if (*szCode)
			return Login(strupr(szCode));
		else {
			SetStatus(WEBQQ_STATUS_ERROR);
		}
		/*
		if (!(pszData=GetHTMLDocument(szTemp,m_referer_ptlogin,&dwSize))) {
			SetStatus(WEBQQ_STATUS_ERROR);
			return false;
		}

		HANDLE hFile=CreateFileA(szPath,GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,CREATE_ALWAYS,0,NULL);
		if (hFile==INVALID_HANDLE_VALUE) {
			Log(__FUNCTION__"(): Error writing %s!",szPath);
			LocalFree(pszData);
			SetStatus(WEBQQ_STATUS_ERROR);
		} else {
			WriteFile(hFile,pszData,dwSize,&dwWritten,NULL);
			CloseHandle(hFile);
			SendToHub(WEBQQ_CALLBACK_NEEDVERIFY,szPath,szCode);
			LocalFree(pszData);
			if (*szCode)
				return Login(strupr(szCode));
			else {
				SetStatus(WEBQQ_STATUS_ERROR);
			}
		}
		*/

	}
	return false;
}

bool CLibWebQQ::Login(LPSTR pszCode) {
	char szHash[33];
	DWORD dwSize;

	SetStatus(WEBQQ_STATUS_LOGIN);

	GetPasswordHash(pszCode,szHash);

	// GET /login?u=431533706&p=346FE6613D7C77599CAA8F572AC155FF&verifycode=SDEG&remember_uin=1&aid=1002101&u1=http%3A%2F%2Fwebqq.qq.com%2Fmain.shtml%3Fdirect__2&h=1&ptredirect=1&ptlang=2052&from_ui=1&pttype=1&dumy=&fp=loginerroralert HTTP/1.1
	// GET /login?u=431533706&p=0257C25F949A61084FE69B6BC7178318&verifycode=!7LT&remember_uin=1&aid=1002101&u1=http%3A%2F%2Fweb2.qq.com%2Floginproxy.html%3Frun%3Deqq%26strong%3Dtrue&h=1&ptredirect=0&ptlang=2052&from_ui=1&pttype=1&dumy=&fp=loginerroralert	referer=ui.ptlogin2
	// GET /login?u=431533706&p=E9765C268D7E93343A681C646C37E5C5&verifycode=RMZQ&remember_uin=1&aid=1002101&u1=http%3A%2F%2Fweb2.qq.com%2Floginproxy.html%3Fstrong%3Dtrue&h=1&ptredirect=0&ptlang=2052&from_ui=1&pttype=1&dumy=&fp=loginerroralert
	sprintf(m_buffer,"http://ptlogin2.qq.com/login?u=%u&p=%s&verifycode=%s&remember_uin=1&aid=1002101&u1=%s&h=1&ptredirect=1%s&ptlang=2052&from_ui=1&pttype=1&dumy=&fp=loginerroralert",m_qqid,szHash,pszCode,"http%3A%2F%2Fweb2.qq.com%2Floginproxy.html%3Frun%3Deqq%26strong%3Dtrue",m_loginhide?"&webqq_type=1":"");
	if (LPSTR pszData=GetHTMLDocument(m_buffer,m_referer_ptlogin,&dwSize)) {
		if (!strstr(pszData,"ptuiCB('0','0','")) {
			Log("%s",pszData);
			SendToHub(WEBQQ_CALLBACK_LOGINFAIL,pszData);
			SetStatus(WEBQQ_STATUS_ERROR);
			LocalFree(pszData);
			return false;
		}
	} else {
		SetStatus(WEBQQ_STATUS_ERROR);
		return false;
	}
	return true;
}

void CLibWebQQ::RefreshCookie() {
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

#if 0 // Web1
bool CLibWebQQ::SendQuery() {
	BOOL fRetry=false;
	LPSTR pszLocalBuffer=NULL;

	while (true) {
		HINTERNET hInetConnect=InternetConnectA(m_hInet,"web-proxy2.qq.com",INTERNET_DEFAULT_HTTP_PORT,NULL,NULL,INTERNET_SERVICE_HTTP,0,NULL);
		m_processstatus=1;
		if (!hInetConnect) {
			SetStatus(WEBQQ_STATUS_ERROR);
			return false;
		}

		m_processstatus=2;
		m_hInetRequest=HttpOpenRequestA(hInetConnect,"POST","/conn_s",NULL,g_referer_webproxy,NULL,/*INTERNET_FLAG_NO_CACHE_WRITE|*/INTERNET_FLAG_PRAGMA_NOCACHE|INTERNET_FLAG_RELOAD,NULL);
		if (!m_hInetRequest) {
			Log(__FUNCTION__"(): HttpOpenRequestA() failed");
			if (true /*!fRetry*/) {
				Log(__FUNCTION__"(): Retry connection");
				fRetry=TRUE;
				continue;
			} else {
				Log(__FUNCTION__"(): Terminate connection");
				SetStatus(WEBQQ_STATUS_ERROR);
				return false;
			}
		}

		m_processstatus=3;
		if (!pszLocalBuffer) {
			pszLocalBuffer=(LPSTR)LocalAlloc(LMEM_FIXED,strlen(m_outbuffer)+1);
			strcpy(pszLocalBuffer,m_outbuffer);
			*m_outbuffer=0;
			m_outbuffer_current=m_outbuffer;
		}
		Log("> %s",pszLocalBuffer);

		m_processstatus=4;
		DWORD dwRead=30000;
		InternetSetOption(m_hInetRequest,INTERNET_OPTION_RECEIVE_TIMEOUT,&dwRead,sizeof(DWORD));

		if (!(HttpSendRequestA(m_hInetRequest,"X-Requested-From: webqq_client",-1,pszLocalBuffer,(DWORD)strlen(pszLocalBuffer)))) {
			DWORD err=GetLastError();
			Log(__FUNCTION__"(): HttpSendRequestA() failed, reason=%d",err);
			InternetCloseHandle(m_hInetRequest);
			m_hInetRequest=NULL;
			InternetCloseHandle(hInetConnect);
			if (err==12002/* && !fRetry*/) {
				// 12002=timeout
				Log(__FUNCTION__"(): Timeout: Retry connection");
				fRetry=TRUE;
				continue;
			} else if (err==12152 && !fRetry) {
				Log(__FUNCTION__"(): WSE12512: Retry connection");
				fRetry=TRUE;
				continue;
			} else if (*m_outbuffer) {
				Log(__FUNCTION__"(): m_outbuffer is not empty, let it send :)");
				*m_buffer=0;
				return true;
			} else {
				Log(__FUNCTION__"(): Terminate connection");
				SetStatus(WEBQQ_STATUS_ERROR);
				if (pszLocalBuffer) LocalFree(pszLocalBuffer);
				return false;
			}
		}

		dwRead=0;
		LPSTR pszBuffer=m_buffer;

		m_processstatus=5;
		while (InternetReadFile(m_hInetRequest,pszBuffer,BUFFERSIZE-1,&dwRead)==TRUE && dwRead>0) {
			pszBuffer+=dwRead;
			dwRead=0;
		}
		*pszBuffer=0;
		Log("< (%d) %s",pszBuffer-m_buffer,m_buffer);

		m_processstatus=6;
		InternetCloseHandle(m_hInetRequest);
		m_hInetRequest=NULL;
		InternetCloseHandle(hInetConnect);
		if (pszLocalBuffer) LocalFree(pszLocalBuffer);

		return true;
	}
}

bool CLibWebQQ::ParseResponse() {
	RECEIVEPACKETINFO rpi;
	CHAR szSplitSignature[16]=";\x1d";
	LPSTR pszNext, pszNext2;
	LPSTR pszCurrent=m_buffer;
	int turn=0;
	bool breakloop=false;
	m_processstatus=7;
	ultoa(m_qqid,szSplitSignature+2,10);

	/*
	*m_outbuffer=0;
	m_outbuffer_current=m_outbuffer;
	*/

	if (*m_buffer) {
		do {
			m_processstatus=8;
			if (pszNext=strstr(pszCurrent,szSplitSignature)) {
				pszNext+=2;
				pszNext[-1]=0;
			}

			Log("<- %s",pszCurrent);
			turn=0;
			
			m_processstatus=9;

			while (*pszCurrent) {
				if (pszNext2=strchr(pszCurrent,';')) 
					*pszNext2=0;
				else
					Log("Warning: Incomplete commands detected. Parsing may crash!");

				if (turn==0)
					rpi.qqid=strtoul(pszCurrent,NULL,10);
				else if (turn==1)
					rpi.cmd=strtoul(pszCurrent,NULL,16);
				else if (turn==2)
					rpi.seq=strtoul(pszCurrent,NULL,10);
				else if (turn==3)
					rpi.args=pszCurrent;

				turn++;
				pszCurrent+=strlen(pszCurrent)+1;
				if (!pszNext2) break;
			}

			pszCurrent=pszNext;

			if (rpi.qqid!=m_qqid) {
				Log(__FUNCTION__"(): ERROR: QQID Mismatch! (%u != %u) Packet Dropped.\n",rpi.qqid,m_qqid);
			} else {
				map<DWORD,LPWEBQQ_OUT_PACKET>::iterator iter=m_outpackets.find(rpi.seq);
				m_processstatus=13;
				//if (LPWEBQQ_OUT_PACKET lpOP=m_outpackets[rpi.seq]) {
				if (iter!=m_outpackets.end()) {
					LPWEBQQ_OUT_PACKET lpOP=iter->second;
					if (lpOP->type!=rpi.cmd) {
						Log(__FUNCTION__"(): WARNING: Packet with same seq but different cmd detected! Ignored OP removal.\n");
					} else {
						LocalFree(lpOP->cmd);
						LocalFree(lpOP);
						m_outpackets.erase(rpi.seq);
					}
				}

				m_processstatus=10;

				switch (rpi.cmd) {
					case 0x22: sc0x22_onSuccLoginInfo(rpi.args); break;
					case WEBQQ_CMD_GET_GROUP_INFO: sc0x3c_onSuccGroupInfo(rpi.args); break;
					case WEBQQ_CMD_GET_LIST_INFO: sc0x58_onSuccListInfo(rpi.args); break;
					case WEBQQ_CMD_GET_NICK_INFO: sc0x26_onSuccNickInfo(rpi.args); break;
					case WEBQQ_CMD_GET_REMARK_INFO: sc0x3e_onSuccRemarkInfo(rpi.args); break;
					case WEBQQ_CMD_GET_HEAD_INFO: sc0x65_onSuccHeadInfo(rpi.args); break;
					case WEBQQ_CMD_GET_CLASS_SIG_INFO: sc0x1d_onSuccGetQunSigInfo(rpi.args); break;
					case WEBQQ_CMD_CLASS_DATA: sc0x30_onClassData(rpi.args, &rpi); break;
					case WEBQQ_CMD_GET_MESSAGE: breakloop=sc0x17_onIMMessage(rpi.seq,rpi.args); break;
					case WEBQQ_CMD_GET_LEVEL_INFO: sc0x5c_onSuccLevelInfo(rpi.args); break;
					case 0x67: sc0x67_onSuccLongNickInfo(rpi.args); break;
					case WEBQQ_CMD_GET_USER_INFO: sc0x06_onSuccUserInfo(rpi.args); break;
					case WEBQQ_CMD_CONTACT_STATUS: sc0x81_onContactStatus(rpi.args); break;
					case WEBQQ_CMD_GET_CLASS_MEMBER_NICKS: sc0x0126_onSuccMemberNickInfo(rpi.args); break;
					case WEBQQ_CMD_RESET_LOGIN: sc0x01_onResetLogin(rpi.args); break;
					case WEBQQ_CMD_SYSTEM_MESSAGE: sc0x80_onSystemMessage(rpi.args); break;

					case WEBQQ_CMD_SEND_C2C_MESSAGE_RESULT:
						Log("WEBQQ_CMD_SEND_C2C_MESSAGE_RESULT");
					case WEBQQ_CMD_SET_STATUS_RESULT:
						if (!strcmp(rpi.args,"0")) {
							SendToHub(0xffff0000+rpi.cmd,rpi.args,&rpi.seq);
						}
						break;
				}
				
				m_processstatus=12;
				SendToHub(0xfffff000+rpi.cmd,rpi.args);
			}
		} while (!breakloop&&pszNext);
	} else if (!m_storage[WEBQQ_STORAGE_PARAMS]) {
		Log(__FUNCTION__"(): Empty CS0x22 reply, send again");
		AppendQuery(cs0x22_getLoginInfo);
		return false;
	} else {
		// Check packet
		LPWEBQQ_OUT_PACKET lpOP;

		for (map<DWORD,LPWEBQQ_OUT_PACKET>::iterator iter=m_outpackets.begin(); iter!=m_outpackets.end();) {
			lpOP=iter->second;
			if (lpOP) {
				if (GetTickCount()-lpOP->created>=5000) {
					if (lpOP->retried>=4) {
						Log(__FUNCTION__"(): Timed out sending 0x%02x packet %u, dropped.",lpOP->type,iter->first);
						LocalFree(lpOP->cmd);
						LocalFree(lpOP);
						iter=m_outpackets.erase(iter);
					} else {
						lpOP->retried++;
						Log(__FUNCTION__"(): Retry sending 0x%02x packet %u for %d time.",lpOP->type,iter->first,lpOP->retried);
						if (*m_outbuffer) m_outbuffer_current+=strlen(strcpy(m_outbuffer_current,"\x1d"));
						m_outbuffer_current+=strlen(strcpy(m_outbuffer_current,lpOP->cmd));
						iter++;
					}
				} else
					iter++;
			} else {
				Log(__FUNCTION__"(): ASSERT NULL outpacket seq %u",iter->first);
				iter=m_outpackets.erase(iter);
			}
		}
	}

	if (!breakloop) {
		if (!m_storage[WEBQQ_STORAGE_PARAMS]) {
			SetStatus(WEBQQ_STATUS_ERROR);
		} else {
			if (cs_0x1d_next_time>0 && GetTickCount()>cs_0x1d_next_time) {
				AppendQuery(cs0x1d_getQunSigInfo);
			}
			AppendQuery(cs0x00_keepAlive);
		}
	}

	return breakloop;
}

LPSTR CLibWebQQ::DecodeText(LPSTR pszText) {
	LPSTR pszRet=pszText;
	char szTemp[3];
	szTemp[2]=0;

	while (pszText=strchr(pszText,'%')) {
		strncpy(szTemp,pszText+1,2);
		*pszText=(char)strtoul(szTemp,NULL,16);
		// if (*pszText==13) *pszText=10;
		pszText++;
		memmove(pszText,pszText+2,strlen(pszText+1));
	}

	return pszRet;
}

void CLibWebQQ::SetOnlineStatus(WEBQQPROTOCOLSTATUSENUM newstatus) {
	char szTemp[16];
	AppendQuery(cs0x0d_setStatus,itoa(newstatus,szTemp,10));
	AttemptSendQueue();
}

void CLibWebQQ::GetClassMembersRemarkInfo(DWORD qunid) {
	char szTemp[32];
	sprintf(szTemp,"%u;0;0",qunid);
	AppendQuery(cs0x30_getMemberRemarkInfo,szTemp);
}

void CLibWebQQ::GetNickInfo(LPDWORD qqid) {
	char szTemp[320];
	int c=0;
	LPSTR pszTemp=szTemp+20;

	while (qqid[c]) {
		pszTemp+=strlen(ultoa(qqid[c],pszTemp,10));
		*pszTemp++=';';
		c++;
		if (c>=24) break;
	}
	pszTemp[-1]=0;

	itoa(c,szTemp,10);
	strcat(szTemp,";");
	memmove(szTemp+strlen(szTemp),szTemp+20,strlen(szTemp+20)+1);
	AppendQuery(cs0x0126_getMemberNickInfo,szTemp);
}

LPSTR CLibWebQQ::EncodeText(LPCSTR pszSrc, LPSTR pszDst) {
	while (*pszSrc) {
		if (*pszSrc==' ' || *pszSrc=='%' || *pszSrc=='\\' || *pszSrc=='\'' || *pszSrc=='/' || *pszSrc==' ') {
			*pszDst++='%';
			pszDst+=sprintf(pszDst,"%02X",(int)*(LPBYTE)pszSrc);
		} else
			*pszDst++=*pszSrc;

		pszSrc++;
	}
	*pszDst=0;
	
	return pszDst;
}

DWORD CLibWebQQ::SendContactMessage(DWORD qqid, LPCSTR message, int fontsize, LPSTR font, DWORD color, BOOL bold, BOOL italic, BOOL underline) {
	LPSTR pszBuffer=(LPSTR)LocalAlloc(LMEM_FIXED,10+17+strlen(font)*3+strlen(message)*3);
	LPSTR ppszBuffer=pszBuffer;
	DWORD seq;

	ppszBuffer+=strlen(ultoa(qqid,ppszBuffer,10));
	ppszBuffer+=sprintf(ppszBuffer,";0b;%d;",(strchr(message,0x15)!=NULL && strchr(message,0x1f)!=NULL)?252:0);
	
	ppszBuffer=EncodeText(message,ppszBuffer);
	*ppszBuffer++=';';

	BYTE attr=fontsize&31;
	if (bold) attr|=32;
	if (italic) attr|=64;
	if (underline) attr|=128;
	ppszBuffer+=sprintf(ppszBuffer,"%02x",(int)attr);
	ppszBuffer+=sprintf(ppszBuffer,"%06x",color);
	ppszBuffer+=sprintf(ppszBuffer,"10");
	ppszBuffer=EncodeText(font,ppszBuffer);
	seq=AppendQuery(cs0x16_sendMessage,pszBuffer);
	LocalFree(pszBuffer);
	AttemptSendQueue();
	return seq;
}
#endif // Web1

DWORD CLibWebQQ::SendContactMessage(DWORD qqid, WORD face, bool hasImage, JSONNODE* jnContent) {
	JSONNODE* jn=json_new(JSON_NODE);
	DWORD dwSize;
	LPSTR pszData;
	JSONNODE* jn2;
	DWORD dwRet=0;
	json_push_back(jn,json_new_f("to",qqid));
	json_push_back(jn,json_new_i("face",face));

	/*
	if (hasImage) {
		JSONNODE* jnResult=NULL;

		if (m_web2_nextqunkey==0 || time(NULL)<=m_web2_nextqunkey) {
			char szQuery[100]; // Typically only ~75 bytes
			sprintf(szQuery,"http://web2-b.qq.com/channel/get_gface_sig?clientid=%s&t=%u%u",m_web2_clientid,(DWORD)time(NULL),GetTickCount()%100);

			pszData=GetHTMLDocument(szQuery,g_referer_web2,&dwSize);
			if (pszData!=NULL && dwSize!=0xffffffff && *pszData=='{') {
				jn2=json_parse(pszData);

				if (json_as_int(json_get(jn2,"retcode"))!=0) {
					Log(__FUNCTION__"(): Get Qun key failed: bad retcode");
					jnContent=NULL;
				} else {
					jnResult=json_get(jn2,"result");
					if (json_as_int(json_get(jn2,"reply"))!=0) {
						Log(__FUNCTION__"(): Get Qun key failed: bad reply");
						jnContent=NULL;
					} else {
						if (m_web2_storage[WEBQQ_WEB2_STORAGE_QUNKEY]) json_delete(m_web2_storage[WEBQQ_WEB2_STORAGE_QUNKEY]);
						m_web2_storage[WEBQQ_WEB2_STORAGE_QUNKEY]=jn2;
					}
				}

				LocalFree(pszData);
			}
		} else
			jnResult=json_get(m_web2_storage[WEBQQ_WEB2_STORAGE_QUNKEY],"result");

		if (jnResult) {
			LPSTR pszTemp;
			json_push_back(jn,json_new_f("group_code",extid));
			json_push_back(jn,json_new_a("key",pszTemp=json_as_string(json_get(jnResult,"gface_key"))));
			json_free(pszTemp);
			json_push_back(jn,json_new_a("sig",pszTemp=json_as_string(json_get(jnResult,"gface_sig"))));
			json_free(pszTemp);
		}
	}
	*/

	if (jnContent) {
		LPSTR szContent=json_write(jnContent);
		json_push_back(jn,json_new_a("content",szContent));
		json_push_back(jn,json_new_f("msg_id",++m_sequence));
		json_push_back(jn,json_new_f("clientid",strtoul(m_web2_clientid,NULL,10)));
		json_push_back(jn,json_new_a("psessionid",m_web2_psessionid));

		LPSTR szJSON=json_write(jn);
		LPSTR pszContent=(LPSTR)LocalAlloc(LMEM_FIXED,strlen(szJSON)+3);

		strcat(strcpy(pszContent,"r="),szJSON);
		
		if ((pszData=PostHTMLDocument(g_server_web2_connection,"/channel/send_msg",GetReferer(WEBQQ_REFERER_WEB2PROXY),pszContent,&dwSize))!=NULL && dwSize!=0xffffffff && *pszData=='{') {
			jn2=json_parse(pszData);
			if (json_as_int(json_get(jn2,"retcode"))==0) 
				dwRet=m_sequence;
			else
				Log(__FUNCTION__"(): Send Contact Msg Failed: %s",pszData);

			LocalFree(pszData);
			json_delete(jn2);
		} else
			Log(__FUNCTION__"(): Send Qun Contact Failed: Connection Failed(%d)",GetLastError());

		LocalFree(pszContent);
		json_free(szJSON);
		json_free(szContent);
	}

	json_delete(jn);
	// json_delete(jnContent); - This left to caller

	return dwRet;
}

DWORD CLibWebQQ::SendClassMessage(DWORD intid, DWORD extid, bool hasImage, JSONNODE* jnContent) {
	JSONNODE* jn=json_new(JSON_NODE);
	DWORD dwSize;
	LPSTR pszData;
	JSONNODE* jn2;
	DWORD dwRet=0;
	json_push_back(jn,json_new_f("group_uin",intid));

	if (hasImage) {
		JSONNODE* jnResult=NULL;

		if (m_web2_nextqunkey==0 || time(NULL)<=m_web2_nextqunkey) {
			char szQuery[384]; // Typically only ~75 bytes
			sprintf(szQuery,"http://web2-b.qq.com/channel/get_gface_sig?clientid=%s&psessionid=%s&t=%u%u",m_web2_clientid,m_web2_psessionid,(DWORD)time(NULL),GetTickCount()%100);

			pszData=GetHTMLDocument(szQuery,g_referer_web2,&dwSize);
			if (pszData!=NULL && dwSize!=0xffffffff && *pszData=='{') {
				jn2=json_parse(pszData);

				if (json_as_int(json_get(jn2,"retcode"))!=0) {
					Log(__FUNCTION__"(): Get Qun key failed: bad retcode");
					jnContent=NULL;
				} else {
					jnResult=json_get(jn2,"result");
					if (json_as_int(json_get(jnResult,"reply"))!=0) {
						Log(__FUNCTION__"(): Get Qun key failed: bad reply");
						jnContent=NULL;
					} else {
						if (m_web2_storage[WEBQQ_WEB2_STORAGE_QUNKEY]) json_delete(m_web2_storage[WEBQQ_WEB2_STORAGE_QUNKEY]);
						m_web2_storage[WEBQQ_WEB2_STORAGE_QUNKEY]=jn2;
					}
				}

				LocalFree(pszData);
			}
		} else
			jnResult=json_get(m_web2_storage[WEBQQ_WEB2_STORAGE_QUNKEY],"result");

		if (jnResult) {
			LPSTR pszTemp;
			json_push_back(jn,json_new_f("group_code",extid));
			json_push_back(jn,json_new_a("key",pszTemp=json_as_string(json_get(jnResult,"gface_key"))));
			json_free(pszTemp);
			json_push_back(jn,json_new_a("sig",pszTemp=json_as_string(json_get(jnResult,"gface_sig"))));
			json_free(pszTemp);
		}
	}

	if (jnContent) {
		LPSTR szContent=json_write(jnContent);
		json_push_back(jn,json_new_a("content",szContent));
		json_push_back(jn,json_new_f("msg_id",++m_sequence));
		json_push_back(jn,json_new_f("clientid",strtoul(m_web2_clientid,NULL,10)));
		json_push_back(jn,json_new_a("psessionid",m_web2_psessionid));

		LPSTR szJSON=json_write(jn);
		LPSTR pszContent=(LPSTR)LocalAlloc(LMEM_FIXED,strlen(szJSON)+3);

		strcat(strcpy(pszContent,"r="),szJSON);
		
		if ((pszData=PostHTMLDocument(g_server_web2_connection,"/channel/send_group_msg",GetReferer(WEBQQ_REFERER_WEB2PROXY),pszContent,&dwSize))!=NULL && dwSize!=0xffffffff && *pszData=='{') {
			jn2=json_parse(pszData);
			if (json_as_int(json_get(jn2,"retcode"))==0) 
				dwRet=m_sequence;
			else
				Log(__FUNCTION__"(): Send Qun Msg Failed: %s",pszData);

			LocalFree(pszData);
			json_delete(jn2);
		} else
			Log(__FUNCTION__"(): Send Qun Msg Failed: Connection Failed(%d)",GetLastError());

		LocalFree(pszContent);
		json_free(szJSON);
		json_free(szContent);
	}

	json_delete(jn);
	// json_delete(jnContent); - This left to caller

	return dwRet;
}

#if 0 // Web1
DWORD CLibWebQQ::SendClassMessage(DWORD qunid, LPCSTR message, int fontsize, LPSTR font, DWORD color, BOOL bold, BOOL italic, BOOL underline) {
	LPSTR pszBuffer=(LPSTR)LocalAlloc(LMEM_FIXED,10+17+strlen(font)*3+strlen(message)*3);
	LPSTR ppszBuffer=pszBuffer;
	DWORD seq;

	ppszBuffer+=sprintf(ppszBuffer,"%u;",qunid);
	
	ppszBuffer=EncodeText(message,ppszBuffer);
	*ppszBuffer++=';';

	BYTE attr=fontsize&31;
	if (bold) attr|=32;
	if (italic) attr|=64;
	if (underline) attr|=128;
	ppszBuffer+=sprintf(ppszBuffer,"%02x",(int)attr);
	ppszBuffer+=sprintf(ppszBuffer,"%06x",color);
	ppszBuffer+=sprintf(ppszBuffer,"10");
	ppszBuffer=EncodeText(font,ppszBuffer);
	seq=AppendQuery(cs0x30_sendClassMessage,pszBuffer);
	LocalFree(pszBuffer);
	AttemptSendQueue();
	return seq;
}

void CLibWebQQ::AddFriendPassive(DWORD qqid, LPSTR message) {
	LPSTR pszSuspect=message;
	char szCmd[100];

	while (pszSuspect=strchr(pszSuspect,';')) *pszSuspect='_'; // Dirty workaround

	sprintf(szCmd,"2;%u;1;0;%s",qqid,message);
	AppendQuery(cs0xa8_addFriendPassive,szCmd);
	AttemptSendQueue();
}

void CLibWebQQ::AttemptSendQueue() {
	if (m_hInetRequest) {
		Log(__FUNCTION__"(): Attempt to interrupt conn_s");
		InternetCloseHandle(m_hInetRequest);
	} else {
		Log(__FUNCTION__"(): conn_s is not waiting");
	}
}
#endif // Web1

int CLibWebQQ::FetchUserHead(DWORD qqid, WEBQQUSERHEADENUM uhtype, LPSTR saveto) {
	// Return 0=Fail, 1=BMP, 2=GIF, 3=JPG/Unknown
	// var face_server_domain = "http://face%id%.qun.qq.com";
	// k.img.src = face_server_domain.replace("%id%", j.uin % 10 + 1) + "/cgi/svr/face/getface?type=1&me=" + this.uin + "&uin=" + j.uin
	char szUrl[MAX_PATH];
	DWORD dwSize;
	LPSTR pszFile;
	
	sprintf(szUrl,"http://face%d.qun.qq.com/cgi/svr/face/getface?cache=1&type=%d&fid=0&uin=%u&vfwebqq=%s&t=%u%03d",qqid%10+1,uhtype,qqid,m_web2_vfwebqq,(DWORD)time(NULL),GetTickCount());
	pszFile=GetHTMLDocument(szUrl,g_referer_main,&dwSize);

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

#if 0 // Web1
void CLibWebQQ::GetLongNames(int count, LPDWORD qqs) {
	char szSend[MAX_PATH];
	LPSTR pszSend=szSend+sprintf(szSend,"%d;",count);
	for (int c=0; c<count; c++) {
		pszSend+=sprintf(pszSend,"%u;",qqs[c]);
	}
	pszSend[-1]=0;
	AppendQuery(cs0x67_getLongNickInfo,szSend);
}

void CLibWebQQ::SendP2PRetrieveRequest(DWORD qqid, LPCSTR type) {
	char szBuffer[32];
	LPSTR ppszBuffer=szBuffer;

	ppszBuffer+=strlen(ultoa(qqid,ppszBuffer,10));
	ppszBuffer+=sprintf(ppszBuffer,";81;%s",type);
	AppendQuery(cs0x16_sendMessage,szBuffer);
	AttemptSendQueue();
}
#endif // Web1

DWORD CLibWebQQ::ReserveSequence() {
	return m_sequence++;
}

LPSTR CLibWebQQ::GetStorage(int index) {
	return m_storage[index];
}

#define BOUNDARY "----WebKitFormBoundaryzx79ypAk2ot8p91p"

void CLibWebQQ::UploadQunImage(HANDLE hFile, LPSTR pszFilename, DWORD respondid) {
	DWORD filelen=GetFileSize(hFile,NULL);
	LPSTR pszBuffer=(LPSTR)LocalAlloc(LMEM_FIXED,filelen+1024);
	int len;
	LPSTR pszSuffix=strrchr(pszFilename,'.');
	WEBQQ_QUNUPLOAD_STATUS wqs={respondid};

	
	HINTERNET hConn=InternetConnectA(m_hInet,"web.qq.com",INTERNET_DEFAULT_HTTP_PORT,NULL,NULL,INTERNET_SERVICE_HTTP,0,NULL);
	HINTERNET hRequest=HttpOpenRequestA(hConn,"POST","/cgi-bin/cface_upload",NULL,NULL,NULL,INTERNET_FLAG_PRAGMA_NOCACHE|INTERNET_FLAG_NO_CACHE_WRITE|INTERNET_FLAG_RELOAD,NULL);
	/*
	HINTERNET hConn=InternetConnectA(m_hInet,"127.0.0.1",INTERNET_DEFAULT_HTTP_PORT,NULL,NULL,INTERNET_SERVICE_HTTP,0,NULL);
	HINTERNET hRequest=HttpOpenRequestA(hConn,"POST","/posttest.php",NULL,NULL,NULL,INTERNET_FLAG_PRAGMA_NOCACHE|INTERNET_FLAG_NO_CACHE_WRITE|INTERNET_FLAG_RELOAD,NULL);
	*/

	// HttpAddRequestHeadersA(hRequest,"Accept: application/xml,application/xhtml+xml,text/html;q=0.9,text/plain;q=0.8,image/png,*/*;q=0.5",-1,HTTP_ADDREQ_FLAG_REPLACE|HTTP_ADDREQ_FLAG_ADD);
	HttpAddRequestHeadersA(hRequest,"Content-Type: multipart/form-data; boundary=" BOUNDARY,-1,HTTP_ADDREQ_FLAG_REPLACE|HTTP_ADDREQ_FLAG_ADD);
	/*
	HttpAddRequestHeadersA(hRequest,"Origin: http://webqq.qq.com",-1,HTTP_ADDREQ_FLAG_REPLACE|HTTP_ADDREQ_FLAG_ADD);
	*/
	HttpAddRequestHeadersA(hRequest,"Referer: http://webqq.qq.com/main.shtml?direct__2",-1,HTTP_ADDREQ_FLAG_REPLACE|HTTP_ADDREQ_FLAG_ADD);
	// HttpAddRequestHeadersA(hRequest,"User-Agent: Mozilla/5.0 (Windows; U; Windows NT 6.1; en-US) AppleWebKit/533.4 (KHTML, like Gecko) Chrome/5.0.375.99 Safari/533.4",-1,HTTP_ADDREQ_FLAG_REPLACE|HTTP_ADDREQ_FLAG_ADD);


	len=sprintf(pszBuffer,"--" BOUNDARY "\r\nContent-Disposition: form-data; name=\"custom_face\"; filename=\"%s\"\r\nContent-Type: image/%s\r\nContent-Transfer-Encoding: binary\r\n\r\n",pszFilename,(pszSuffix!=NULL && !stricmp(pszSuffix,".gif"))?"gif":"jpeg");
	// Log("ASSERT: len=%d, pszBuffer=%s",len,pszBuffer);
	// HttpSendRequestA(hRequest,NULL,0,pszBuffer,len);

	DWORD dwRead;
	wqs.status=0;
	wqs.number=filelen;
	SendToHub(WEBQQ_CALLBACK_QUNIMGUPLOAD,NULL,&wqs);
	// len=0;

	wqs.status=1;

	/*
	while (ReadFile(hFile,pszBuffer,16384,&dwRead,NULL) && dwRead>0) {
		HttpSendRequestA(hRequest,NULL,0,pszBuffer,dwRead);
		len+=dwRead;
		wqs.number=len;
		SendToHub(WEBQQ_CALLBACK_QUNIMGUPLOAD,NULL,&wqs);
	}
	*/
	ReadFile(hFile,pszBuffer+len,filelen,&dwRead,NULL);
	CloseHandle(hFile);

	len+=filelen;

	len+=sprintf(pszBuffer+len,"\r\n--" BOUNDARY "\r\nContent-Disposition: form-data; name=\"from\"\r\n\r\ncontrol");
	if (m_useweb2)
		len+=sprintf(pszBuffer+len,"\r\n--" BOUNDARY "\r\nContent-Disposition: form-data; name=\"f\"\r\n\r\nEQQ.Model.ChatMsg.callbackSendPicGroup");
	else
		len+=sprintf(pszBuffer+len,"\r\n--" BOUNDARY "\r\nContent-Disposition: form-data; name=\"f\"\r\n\r\nWEBQQ.obj.QQClient.mainPanel._picSendCallback");
	len+=sprintf(pszBuffer+len,"\r\n--" BOUNDARY "\r\nContent-Disposition: form-data; name=\"fileid\"\r\n\r\n1");
	len+=sprintf(pszBuffer+len,"\r\n--" BOUNDARY "--");
	HttpSendRequestA(hRequest,NULL,0,pszBuffer,len);
	
	wqs.status=2;
	wqs.number=0;
	SendToHub(WEBQQ_CALLBACK_QUNIMGUPLOAD,NULL,&wqs);

	DWORD dwWritten=sizeof(DWORD);
	DWORD dwValue=0;
	
	HttpQueryInfo(hRequest,HTTP_QUERY_STATUS_CODE|HTTP_QUERY_FLAG_NUMBER,&dwValue,&dwWritten,&dwRead);
	Log(__FUNCTION__"() Status=%d",dwValue);

	InternetReadFile(hRequest,pszBuffer,16384,&dwRead);
	pszBuffer[dwRead]=0;
	InternetCloseHandle(hRequest);
	InternetCloseHandle(hConn);

	// <head><script type="text/javascript">document.domain='qq.com';parent.WEBQQ.obj.QQClient.mainPanel._picSendCallback({'ret':0,'msg':'219C9C18D0C5A676899CD8595507CFCE.gIf'});</script></head><body></body>
	LPSTR ppszBuffer=strstr(pszBuffer,"'ret':");
	if (ppszBuffer!=NULL) {
		if (ppszBuffer[6]=='0') {
			ppszBuffer=strstr(ppszBuffer,"'msg':'")+7;
			*strstr(ppszBuffer,"'}")=0;
			wqs.status=3;
			wqs.string=ppszBuffer;
		} else if (ppszBuffer[6]=='4') {
			// Most likely already uploaded
			ppszBuffer=strstr(ppszBuffer,"'msg':'")+7;
			*strstr(ppszBuffer," -")=0;
			wqs.status=3;
			wqs.string=ppszBuffer;
		}
	} else {
		wqs.status=4;
		wqs.string=pszBuffer;
	}
	SendToHub(WEBQQ_CALLBACK_QUNIMGUPLOAD,NULL,&wqs);
	LocalFree(pszBuffer);
}

void CLibWebQQ::SetBasePath(LPCSTR pszPath) {
	m_basepath=strdup(pszPath);
}

#if 0 // Web1
void CLibWebQQ::GetGroupInfo() {
	AppendQuery(cs0x3c_getGroupInfo);
}

/** CS **/

DWORD cs0x22_getLoginInfo(CLibWebQQ* protocol, LPSTR pszOutBuffer, LPSTR pszArgs) {
	if (!protocol) return 0x22;
	if (!pszOutBuffer) return 1; // Need ack
	return sprintf(pszOutBuffer,"%s;%s;%d",protocol->GetCookie("skey"),protocol->GetCookie("ptwebqq"),protocol->GetStorage(WEBQQ_STORAGE_PARAMS)==NULL?0:1);
}

DWORD cs0x3c_getGroupInfo(CLibWebQQ* protocol, LPSTR pszOutBuffer, LPSTR pszArgs) {
	if (!protocol) return 0x3c;
	if (!pszOutBuffer) return 1; // Need ack
	return (DWORD)strlen(strcpy(pszOutBuffer,"1"/* 2 is possible if !!(strtoul(pszArgs))!=0 */));
}

DWORD cs0x00_keepAlive(CLibWebQQ* protocol, LPSTR pszOutBuffer, LPSTR pszArgs) {
	return 0;
}

DWORD cs0x06_getUserInfo(CLibWebQQ* protocol, LPSTR pszOutBuffer, LPSTR pszArgs) {
	if (!protocol) return 0x06;
	if (!pszOutBuffer) return 1; // Need ack
	return sprintf(pszOutBuffer,"%u",protocol->GetQQID());
}

DWORD cs0x5c_getLevelInfo(CLibWebQQ* protocol, LPSTR pszOutBuffer, LPSTR pszArgs) {
	if (!protocol) return 0x5c;
	if (!pszOutBuffer) return 1; // Need ack
	return (DWORD)strlen(strcpy(pszOutBuffer,"88"));
}

DWORD cs0x67_getLongNickInfo(CLibWebQQ* protocol, LPSTR pszOutBuffer, LPSTR pszArgs) {
	// Args: count;id1;id2;...
	if (!protocol) return 0x67;
	if (!pszOutBuffer) return 1; // Need ack
	return sprintf(pszOutBuffer,"03;%s",pszArgs);
}

DWORD cs0x58_getListInfo(CLibWebQQ* protocol, LPSTR pszOutBuffer, LPSTR pszArgs) {
	// Args:  cs_0x58_next_uin
	if (!protocol) return 0x58;
	if (!pszOutBuffer) return 1; // Need ack
	return (DWORD)strlen(strcpy(pszOutBuffer,pszArgs));
}

DWORD cs0x26_getNickInfo(CLibWebQQ* protocol, LPSTR pszOutBuffer, LPSTR pszArgs) {
	// Args: cs_0x26_next_pos, cs_0x26_timeout++
	if (!protocol) return 0x26;
	if (!pszOutBuffer) return 1; // Need ack
	return (DWORD)strlen(strcpy(pszOutBuffer,pszArgs));
}

DWORD cs0x3e_getRemarkInfo(CLibWebQQ* protocol, LPSTR pszOutBuffer, LPSTR pszArgs) {
	// Args: cs_0x3e_next_pos
	if (!protocol) return 0x3e;
	if (!pszOutBuffer) return 1; // Need ack
	return sprintf(pszOutBuffer,"4;%s",pszArgs);
}

DWORD cs0x65_getHeadInfo(CLibWebQQ* protocol, LPSTR pszOutBuffer, LPSTR pszArgs) {
	if (!protocol) return 0x65;
	if (!pszOutBuffer) return 1; // Need ack
	return sprintf(pszOutBuffer,"02;%u",protocol->GetQQID());
}

DWORD cs0x1d_getQunSigInfo(CLibWebQQ* protocol, LPSTR pszOutBuffer, LPSTR pszArgs) {
	if (!protocol) return 0x1d;
	if (!pszOutBuffer) return 1; // Need ack
	return 0;
}

DWORD cs0x30_getClassInfo(CLibWebQQ* protocol, LPSTR pszOutBuffer, LPSTR pszArgs) {
	// Use when member info not get
	// Args: qunid;msg;font
	if (!protocol) return 0x30;
	if (!pszOutBuffer) return 1; // Need ack
	return sprintf(pszOutBuffer,"72;%s",pszArgs);
}

DWORD cs0x17_sendMessageAck(CLibWebQQ* protocol, LPSTR pszOutBuffer, LPSTR pszArgs) {
	// Use when member info not get
	// Args: msgseq;session;qunid;qqid;seq;type;tail
	if (!protocol) return 0x17;
	if (!pszOutBuffer) return 0; // No ack required
	return (DWORD)strlen(strcpy(pszOutBuffer,pszArgs));
}

DWORD cs0x30_getMemberRemarkInfo(CLibWebQQ* protocol, LPSTR pszOutBuffer, LPSTR pszArgs) {
	// Args: qunid;0;next_pos
	if (!protocol) return 0x30;
	if (!pszOutBuffer) return 1; // Need ack
	return sprintf(pszOutBuffer,"0f;%s",pszArgs);
}

DWORD cs0x30_sendClassMessage(CLibWebQQ* protocol, LPSTR pszOutBuffer, LPSTR pszArgs) {
	// Args: qunid;0;next_pos
	if (!protocol) return 0x30;
	if (!pszOutBuffer) return 0; // No ack required
	return sprintf(pszOutBuffer,"0a;%s",pszArgs);
}

DWORD cs0x0126_getMemberNickInfo(CLibWebQQ* protocol, LPSTR pszOutBuffer, LPSTR pszArgs) {
	// Args: count(<=24);qqid;qqid;...
	if (!protocol) return 0x0126;
	if (!pszOutBuffer) return 1; // Need ack
	return sprintf(pszOutBuffer,"0;%s",pszArgs);
}

DWORD cs0x0d_setStatus(CLibWebQQ* protocol, LPSTR pszOutBuffer, LPSTR pszArgs) {
	// Args: status
	if (!protocol) return 0x0d;
	if (!pszOutBuffer) return 1; // Need ack
	return (DWORD)strlen(strcpy(pszOutBuffer,pszArgs));
}

DWORD cs0x16_sendMessage(CLibWebQQ* protocol, LPSTR pszOutBuffer, LPSTR pszArgs) {
	// Args: uin;0b;face(0?);text;fontstyle
	// sendClassMsg for qun
	if (!protocol) return 0x16;
	if (!pszOutBuffer) return 0; // No ack required
	return (DWORD)strlen(strcpy(pszOutBuffer,pszArgs));
}

DWORD cs0xa8_addFriendPassive(CLibWebQQ* protocol, LPSTR pszOutBuffer, LPSTR pszArgs) {
	// Args: 2;uin;1;0;text
	if (!protocol) return 0xa8;
	if (!pszOutBuffer) return 0; // No ack required
	return (DWORD)strlen(strcpy(pszOutBuffer,pszArgs));
}

/** SC **/

void CLibWebQQ::sc0x22_onSuccLoginInfo(LPSTR pszArgs) {
	int len;

	switch (strtoul(pszArgs,NULL,10)) {
		case 0:
			len=GetArgument(pszArgs,8)-pszArgs+1;
			if (m_storage[WEBQQ_STORAGE_PARAMS]) LocalFree(m_storage[WEBQQ_STORAGE_PARAMS]);
			m_storage[WEBQQ_STORAGE_PARAMS]=(LPSTR)LocalAlloc(LMEM_FIXED,len);
			memcpy(m_storage[WEBQQ_STORAGE_PARAMS],pszArgs,len);
			InternetSetCookieA(g_domain_qq,NULL,"ptwebqq=");
			sprintf(m_buffer,"wqqs=%s",GetArgument(m_storage[WEBQQ_STORAGE_PARAMS],SC0X22_WQQS));
			InternetSetCookieA(g_domain_qq,NULL,m_buffer);

			if (m_status<WEBQQ_STATUS_ONLINE) {
				SetStatus(m_loginhide?WEBQQ_STATUS_INVISIBLE:WEBQQ_STATUS_ONLINE);
				AppendQuery(cs0x3c_getGroupInfo);
			}
			// this.getClassInfo(); - List is empty here, will send later
			break;
		default:
			{
				WEBQQ_LOGININFO li={strtoul(pszArgs,NULL,10),GetArgument(pszArgs,2)};
				SendToHub(0xffff0022,pszArgs,&li);
				SetStatus(WEBQQ_STATUS_ERROR);
			}
			break;
		/*
		case 2:
			// TODO: Connection Error, retry
			break;
		case 4:
			// TODO: Privilege Error
			break;
		case 5:
			// TODO: Password Error
			break;
		case 6:
			// TODO: GetArgument(pszArgs,2) is error cause
			break;
		default:
			// TODO: Unknown Error
			break;
		*/
	}
}

void CLibWebQQ::sc0x3c_onSuccGroupInfo(LPSTR pszArgs) {
	LPWEBQQ_GROUPINFO pGIHead=NULL, pGILast=NULL, pGICurrent=NULL;

	for (LPSTR ppszArgs=pszArgs+strlen(pszArgs)+1;*ppszArgs;ppszArgs+=strlen(ppszArgs)+1) {
		pGICurrent=(LPWEBQQ_GROUPINFO)LocalAlloc(LMEM_FIXED,sizeof(WEBQQ_GROUPINFO));
		if (pGIHead) {
			pGILast->next=pGICurrent;
			pGILast=pGICurrent;
		} else {
			pGIHead=pGILast=pGICurrent;
		}
		pGICurrent->next=NULL;

		pGICurrent->index=atoi(ppszArgs);
		ppszArgs+=strlen(ppszArgs)+1; // Skip index

		pGICurrent->name=ppszArgs;
	}

	SendToHub(0xffff003c,pszArgs,pGIHead);

	while (pGIHead) {
		pGICurrent=pGIHead->next;
		LocalFree(pGIHead);
		pGIHead=pGICurrent;
	}

	// _1000: Online
	// 1000: Strangers
	// 1001: Blacklist

	/*
	AppendQuery(cs0x06_getUserInfo);
	AppendQuery(cs0x5c_getLevelInfo);
	*/
	char szTemp[MAX_PATH];
	/*
	strcpy(szTemp,"1;");
	ultoa(m_qqid,szTemp+2,10);
	// AppendQuery(cs0x67_getLongNickInfo,szTemp);
	*/
	web2_api_get_single_info(m_qqid);
	web2_api_get_qq_level(m_qqid);
	web2_api_get_single_long_nick(m_qqid);

	AppendQuery(cs0x58_getListInfo,"0");

	sprintf(szTemp,"0;%u",cs_0x26_timeout++);
	AppendQuery(cs0x26_getNickInfo,szTemp);
	AppendQuery(cs0x3e_getRemarkInfo,"0");
	AppendQuery(cs0x65_getHeadInfo);
	AppendQuery(cs0x1d_getQunSigInfo);
}

void CLibWebQQ::sc0x58_onSuccListInfo(LPSTR pszArgs) {
	LPWEBQQ_LISTINFO pLIHead=NULL, pLILast=NULL, pLICurrent=NULL;
	int s;
	LPSTR pszTemp;
	char szTemp[32];

	if ((pszTemp=GetArgument(pszArgs,5))!=NULL && strtoul(pszTemp,NULL,10)<4)
		s=5;
	else
		s=4;

	pszTemp=pszArgs+strlen(pszArgs)+1;

	while (*pszTemp) {
		pLICurrent=(LPWEBQQ_LISTINFO)LocalAlloc(LMEM_FIXED,sizeof(WEBQQ_LISTINFO));
		if (pLIHead) {
			pLILast->next=pLICurrent;
			pLILast=pLICurrent;
		} else {
			pLIHead=pLILast=pLICurrent;
		}
		pLICurrent->next=NULL;

		pLICurrent->qqid=strtoul(pszTemp,NULL,10);
		pszTemp+=strlen(pszTemp)+1;

		pLICurrent->isqun=strtoul(pszTemp,NULL,10);
		pszTemp+=strlen(pszTemp)+1;

		pLICurrent->group=(strtoul(pszTemp,NULL,10)&60)>>2;
		pszTemp+=strlen(pszTemp)+1;

		pLICurrent->status=strtoul(pszTemp,NULL,10);
		pszTemp+=strlen(pszTemp)+1;

		if (strcmp(pszTemp," ") && strtoul(pszTemp,NULL,10)<4) {
			pLICurrent->termstatus=strtoul(pszTemp,NULL,10);
		} else
			pLICurrent->termstatus=0;

		if (s==5) pszTemp+=strlen(pszTemp)+1;

		if (pLICurrent->isqun) {
			// NOTE: The send order is reversed with webqq if this command needs to continue
			sprintf(szTemp,"%u;0",pLICurrent->qqid);
			AppendQuery(cs0x30_getClassInfo,szTemp);
		}
	}

	SendToHub(0xffff0058,pszArgs,pLIHead);

	while (pLIHead) {
		pLICurrent=pLIHead->next;
		LocalFree(pLIHead);
		pLIHead=pLICurrent;
	}

	if (strcmp(pszArgs,"0")) {
		AppendQuery(cs0x58_getListInfo,pszArgs);
	}
}

void CLibWebQQ::sc0x26_onSuccNickInfo(LPSTR pszArgs) {
	// pszArgs[0] is next call (0=no need)
	LPWEBQQ_NICKINFO pNIHead=NULL, pNILast=NULL, pNICurrent=NULL;

	LPSTR pszTemp=pszArgs+strlen(pszArgs)+1;

	while (*pszTemp) {
		pNICurrent=(LPWEBQQ_NICKINFO)LocalAlloc(LMEM_FIXED,sizeof(WEBQQ_NICKINFO));
		if (pNIHead) {
			pNILast->next=pNICurrent;
			pNILast=pNICurrent;
		} else {
			pNIHead=pNILast=pNICurrent;
		}
		pNICurrent->next=NULL;

		pNICurrent->qqid=strtoul(pszTemp,NULL,10);
		pszTemp+=strlen(pszTemp)+1;

		if ((pNICurrent->face=strtoul(pszTemp,NULL,10)) >= FACE_MAX_INDEX) pNICurrent->face=0;
		pszTemp+=strlen(pszTemp)+1;
		
		pNICurrent->age=strtoul(pszTemp,NULL,10);
		pszTemp+=strlen(pszTemp)+1;

		pNICurrent->male=strtoul(pszTemp,NULL,10)==0; // 1=F 0=M
		pszTemp+=strlen(pszTemp)+1;

		pNICurrent->name=pszTemp;
		pszTemp+=strlen(pszTemp)+1;

		pNICurrent->viplevel=strtoul(pszTemp,NULL,10);
		pszTemp+=strlen(pszTemp)+1;
	}

	SendToHub(0xffff0026,pszArgs,pNIHead);

	while (pNIHead) {
		pNICurrent=pNIHead->next;
		LocalFree(pNIHead);
		pNIHead=pNICurrent;
	}

	if (strcmp(pszArgs,"0")) {
		char szTemp[16];
		sprintf(szTemp,"%s;%u",pszArgs,cs_0x26_timeout++);
		AppendQuery(cs0x26_getNickInfo,szTemp);
	}
}

void CLibWebQQ::sc0x3e_onSuccRemarkInfo(LPSTR pszArgs) {
	switch (strtoul(pszArgs,NULL,10)) {
		case 0:
		case 4:
			{
				LPWEBQQ_REMARKINFO pRIHead=NULL, pRILast=NULL, pRICurrent=NULL;
				LPSTR pszTemp=GetArgument(pszArgs,2);

				while (*pszTemp) {
					pRICurrent=(LPWEBQQ_REMARKINFO)LocalAlloc(LMEM_FIXED,sizeof(WEBQQ_REMARKINFO));
					if (pRIHead) {
						pRILast->next=pRICurrent;
						pRILast=pRICurrent;
					} else {
						pRIHead=pRILast=pRICurrent;
					}
					pRICurrent->next=NULL;

					pRICurrent->index=strtoul(pszTemp,NULL,10);
					pszTemp+=strlen(pszTemp)+1;
					pszTemp+=strlen(pszTemp)+1;
					pRICurrent->name=pszTemp;
					pszTemp+=strlen(pszTemp)+1;
				}

				SendToHub(0xffff003e,pszArgs,pRIHead);

				while (pRIHead) {
					pRICurrent=pRIHead->next;
					LocalFree(pRIHead);
					pRIHead=pRICurrent;
				}

				if (strcmp(pszTemp=GetArgument(pszArgs,1),"0")) {
					char szTemp[16];
					itoa(++cs_0x3e_next_pos,szTemp,10);
					AppendQuery(cs0x3e_getRemarkInfo,szTemp);
				}
			}
			break;
		case 5:
		case 105:
			// TODO: Unknown
			/*
			var n = this._qqclient,
				t = r.params,
				q = -1;
			n.last_error = n.ERR_MSG_ENUM.ok;
			var p = r.info.uin,
				s = r.info.remark,
				o = n.bdylist.allUsers[p];
			if (o) {
				o.remark = String(s)
			}
			if (t[4] == 0) {
				q = 0
			}
			return q
			*/
			break;
		default:
			break;
	}
}

void CLibWebQQ::sc0x65_onSuccHeadInfo(LPSTR pszArgs) {
	// LPSTR richinfo=GetArgument(pszArgs,2);
	// TODO: s.mainPanel.mf_SetHead(s.uin, s.face)
	SendToHub(0xffff0065,pszArgs);
}

void CLibWebQQ::sc0x1d_onSuccGetQunSigInfo(LPSTR pszArgs) {
	// 431533706;1d;9;0;6Rr3iMUppeXcpfah;49be042020909aa0d7930769a2540ea4c3f54fed19437060226d6103286bc65686163a2e4d7e12aa0eeeb7de153eb9affefcd78e3e3457f5;16640;

	if (!strcmp(pszArgs,"0")) {
		int size=GetArgument(pszArgs,3)-pszArgs;
		LPSTR pszOld=m_storage[WEBQQ_STORAGE_QUNSIG];
		cs_0x1d_next_time=GetTickCount()+720000;
		if (pszOld) LocalFree(pszOld);
		pszOld=(LPSTR)LocalAlloc(LMEM_FIXED,size);
		m_storage[WEBQQ_STORAGE_QUNSIG]=(LPSTR)memcpy(pszOld,pszArgs,size);
	} else {
		cs_0x1d_next_time=GetTickCount()+60000;
	}
}

void CLibWebQQ::sc0x30_onClassData(LPSTR pszArgs, LPRECEIVEPACKETINFO lpRPI) {
	switch (strtoul(pszArgs,NULL,16)) {
		case WEBQQ_CLASS_SUBCOMMAND_CLASSINFO:
			{
				WEBQQ_CLASSINFO ci={0};
				ci.classdata.subcommand=WEBQQ_CLASS_SUBCOMMAND_CLASSINFO;

				DWORD x=0,next=0,s;

				pszArgs=pszArgs+strlen(pszArgs)+1; // 4

				ci.intid=strtoul(pszArgs,NULL,10);
				pszArgs=pszArgs+strlen(pszArgs)+1; // 5

				ci.extid=strtoul(pszArgs,NULL,10);
				pszArgs=pszArgs+strlen(pszArgs)+1; // 6

				if (!strcmp(pszArgs,"1")) {
					ci.haveinfo=true;

					pszArgs=pszArgs+strlen(pszArgs)+1;

					ci.prop=strtoul(pszArgs,NULL,10);
					pszArgs=pszArgs+strlen(pszArgs)+1;

					ci.creator=strtoul(pszArgs,NULL,10);
					pszArgs=pszArgs+strlen(pszArgs)+1;

					ci.name=pszArgs;
					pszArgs=pszArgs+strlen(pszArgs)+1;

					ci.notice=pszArgs;
					pszArgs=pszArgs+strlen(pszArgs)+1;

					ci.desc=pszArgs;

				} else {
					// Recurring results only has member list
					ci.haveinfo=false;
					ci.prop=0;
					ci.creator=0;
					ci.name=ci.notice=ci.desc="";
				}

				pszArgs=pszArgs+strlen(pszArgs)+1;

				if (ci.havenext=(strcmp(pszArgs,"1")==0)) {
					pszArgs=pszArgs+strlen(pszArgs)+1;
					next=strtoul(pszArgs,NULL,10);
				} else
					pszArgs=pszArgs+strlen(pszArgs)+1;

				//pszArgs=pszArgs+strlen(pszArgs)+1;
				LPSTR pszTemp;
				pszArgs=pszArgs+strlen(pszArgs)+1;

				if ((pszTemp=GetArgument(pszArgs,4))!=NULL && strtoul(pszTemp,NULL,10)<4) {
					s=5;
				} else
					s=4;

				LPWEBQQ_CLASSMEMBER pCM=ci.members;

				while (*pszArgs) {
					pCM->qqid=strtoul(pszArgs,NULL,10);
					pszArgs=pszArgs+strlen(pszArgs)+1;

					pCM->status=strtoul(pszArgs,NULL,10); // 10=online 20=offline 30=away
					pszArgs=pszArgs+strlen(pszArgs)+1;

					pszArgs=pszArgs+strlen(pszArgs)+1;

					pCM->n=strtoul(pszArgs,NULL,10);
					pszArgs=pszArgs+strlen(pszArgs)+1;

					if (s==4)
						pCM->D=3;
					else {
						pCM->D=strtoul(pszArgs,NULL,10);
						pszArgs=pszArgs+strlen(pszArgs)+1;
					}

					pCM++;
				}
				pCM->qqid=0;

				SendToHub(0xffff0030,pszArgs,&ci);

				if (next) {
					char szTemp[32];
					sprintf(szTemp,"%u;%u",ci.intid,next);
					AppendQuery(cs0x30_getClassInfo,szTemp);
				}
			}
			break;
		case WEBQQ_CLASS_SUBCOMMAND_REMARKINFO:
			pszArgs+=strlen(pszArgs)+1; // 4
			if (!strcmp(pszArgs,"0")) {
				WEBQQ_CLASS_REMARKS cr={0};
				LPSTR pszNext;
				LPWEBQQ_REMARK lpRN=cr.remarks;

				cr.classdata.subcommand=WEBQQ_CLASS_SUBCOMMAND_REMARKINFO;
				pszArgs+=strlen(pszArgs)+1; // 5

				cr.qunid=strtoul(pszArgs,NULL,10);
				pszArgs+=strlen(pszArgs)+1; // 6
				pszArgs+=strlen(pszArgs)+1; // 7

				pszNext=pszArgs;
				pszArgs+=strlen(pszArgs)+1; // 8

				while (*pszArgs) {
					lpRN->qqid_str=pszArgs;
					lpRN->qqid=strtoul(pszArgs,NULL,10);
					pszArgs+=strlen(pszArgs)+1;

					lpRN->name=pszArgs;
					pszArgs+=strlen(pszArgs)+1;

					lpRN++;
				}
				lpRN->qqid=0;

				cr.havenext=(strcmp(pszNext,"0")!=0);

				SendToHub(0xffff0030,pszArgs,&cr);

				if (cr.havenext) {
					char szTemp[32];
					sprintf(szTemp,"%u;0;%s",cr.qunid,pszNext);
					AppendQuery(cs0x30_getMemberRemarkInfo,szTemp);
				}/* else {
					// TODO: Should be handled in client because it keeps the member list
					mf_GetMemberNickInfo(k)
				}*/
			}
			break;
		case WEBQQ_CLASS_SUBCOMMAND_CLASSMESSAGERESULT:
			{
				WEBQQ_CLASSDATA cd;
				Log("WEBQQ_CLASS_SUBCOMMAND_CLASSMESSAGERESULT!");
				cd.subcommand=WEBQQ_CLASS_SUBCOMMAND_CLASSMESSAGERESULT;
				cd.dwStub=lpRPI->seq;
				SendToHub(0xffff0030,pszArgs,&cd);
			}
			break;
		case 0x73:
			// TODO
			break;
		default:
			break;
	}
}

void CLibWebQQ::AppendCS0x17Reply(LPSTR D, DWORD sender, DWORD seq, int type, LPSTR M) {
	char szTemp[MAX_PATH];
	sprintf(szTemp,"%s;%s;%u;%u;%u;%d;%s",D,GetArgument(m_storage[WEBQQ_STORAGE_PARAMS],SC0X22_WEB_SESSION),sender,m_qqid,seq,type,M);
	AppendQuery(cs0x17_sendMessageAck,szTemp);
}

bool CLibWebQQ::sc0x17_onIMMessage(DWORD seq, LPSTR pszArgs) {
	WEBQQ_MESSAGE wm={0};
	wm.sequence=seq;

	wm.D=pszArgs-2;
	while (wm.D[-1]) wm.D--;

	// [3]
	wm.sender=strtoul(pszArgs,NULL,10);
	pszArgs+=strlen(pszArgs)+1;// [4]

	wm.msgseq=strtoul(pszArgs,NULL,10);
	pszArgs+=strlen(pszArgs)+1;// [5]

	wm.type=strtoul(pszArgs,NULL,16);
	pszArgs+=strlen(pszArgs)+1;// [6]

	wm.M=pszArgs;
	while (*(wm.M+strlen(wm.M)+1)) wm.M+=strlen(wm.M)+1;

	if (wm.type==WEBQQ_MESSAGE_TYPE_CONTACT2) {
		/*
		if user is not a friend then reply CS17, may need to be handled in client code

		if (W.isFriend(S) == -1) {
			W.replyCS17(S, R, D, 1, M);
			return true;
		}
		*/
		wm.type=WEBQQ_MESSAGE_TYPE_CONTACT1;
	}
	if (wm.type==WEBQQ_MESSAGE_TYPE_ERROR) {
		// web-proxy lost connection
		SetStatus(WEBQQ_STATUS_ERROR);
		return true;
	}
	if (wm.type==WEBQQ_MESSAGE_TYPE_FORCE_DISCONNECT) {
		// Update evil network flag
		wm.isevil=atoi(pszArgs)==2;
		SendToHub(0xffff0017,pszArgs,&wm);
		SetStatus(WEBQQ_STATUS_ERROR);
		return true;
	}
	wm.timestamp=0;
	wm.requestType=strtoul(pszArgs,NULL,16);
	pszArgs+=strlen(pszArgs)+1; // [7]

	if (wm.requestType==WEBQQ_MESSAGE_REQUEST_TEXT) {
		wm.text=pszArgs;
		pszArgs+=strlen(pszArgs)+1; // [8]

		if (pszArgs[1]!=0) {
			char szFlags[3];
			strncpy(szFlags,pszArgs,2);
			szFlags[2]=0;

			DWORD ak=strtoul(szFlags,NULL,16);
			wm.hasFormat=true;
			wm.size=ak&31;
			wm.bold=ak&32;
			wm.italic=ak&64;
			wm.underline=ak&128;

			pszArgs[8]='_';
			wm.color=strtoul(pszArgs+2,NULL,16);
			pszArgs+=strlen(pszArgs)+1; // [9]
		} else
			wm.hasFormat=false;

		wm.hasMagicalEmotion=(wm.type==WEBQQ_MESSAGE_TYPE_MAGICAL_EMOTION);
	}

	// TODO: All placeholders?
	/*
	if (wm.ad==0x2b) {
		T = T.replace(/\x15[^\r\n\x1f]{1,}\x1f/g, function (al) {
			if (String(al).charAt(1) == "6") {
				var ak = "1_" + q[9] + "_" + q[13] + "_" + String(al).substr(2, String(al).length - 3) + "_" + S + "_" + new Date().getTime();
				return "<img id='" + ak + "' align='absmiddle' src='" + img_server_domain + "/images/img_loading.gif' />"
			} else {
				if (String(al).charAt(1) == "7") {
					return ""
				}
			}
		})
	} else {
		if ((wm.ad==0x09 || wm.ad==0x0a || wm.ad==0x1f) && wm.U==0x0b) {
					T = T.replace(/\x16[^\r\n\x1f]{1,80}\x1f/g, function (al) {
						if (String(al).charAt(2) == "\x1f") {
							return "<img align='absmiddle' src='" + img_server_domain + "/images/img_error.gif' />"
						} else {
							if (String(al).charAt(1) == "1") {
								var ak = String(al).substr(2, String(al).length - 3),
									an = ak.split("/"),
									am = an[0].split(".")[1];
								pic_id = "0_" + an[1] + "_" + am + "_" + new Date().getTime();
								W.applyOfflinePicAddr(S, an[1]);
								return "<img id='" + pic_id + "' align='absmiddle' src='" + img_server_domain + "/images/img_loading.gif' />"
							} else {
								if (String(al).charAt(1) == "2") {
									return ""
								}
							}
						}
					});
					var H = "";
					T = T.replace(/\x13\S{38}\x1f/, function (ak) {
						H = String(ak).substr(1, String(ak).length - 2);
						return ""
					});
					var C = 0;
					T = String(T).replace(/\x15[^\r\n\x1f]{1,}\x1f/g, function (am) {
						var ak = String(am).charAt(1);
						if (ak == "2") {
							var al = S + "_" + D + "_" + H + String(am).charAt(3) + ((String(am).charAt(2) == "0") ? ".jpg" : ".gif") + "_p_" + new Date().getTime();
							return "<img id='" + al + "' align='absmiddle' src='" + img_server_domain + "/images/img_loading.gif' />"
						} else {
							if (ak == "3") {
								C = 1;
								var al = S + "_" + D + "_" + String(am).substr(2, 36) + "_c_" + new Date().getTime();
								return "<img id='" + al + "' align='absmiddle' src='" + img_server_domain + "/images/img_loading.gif' />"
							} else {
								if (ak == "4") {
									return ""
								} else {
									if (ak == "8") {
										C = 3;
										return ""
									}
								}
							}
						}
					})
		}
	}
	*/

	if (((wm.type==WEBQQ_MESSAGE_TYPE_CONTACT1 || wm.type==WEBQQ_MESSAGE_TYPE_TEMP_SESSION) && wm.requestType==WEBQQ_MESSAGE_REQUEST_TEXT) || wm.type==WEBQQ_MESSAGE_TYPE_CONTACT2 || wm.type==WEBQQ_MESSAGE_TYPE_MAGICAL_EMOTION) {
		Log("cs0x17: Contact1orTempSession/Text");
		AppendCS0x17Reply(wm.D,wm.sender,wm.msgseq,1,wm.M);

		// TODO: Temp Session Creation: Handle at client?
		/*
		if (G && ad == "1f" && G.class_id > -1) {
			G.SessionTime = new Date().getTime();
			var Y = G.class_id,
				ag = S;
			if (G.SigC2CMsg == "") {
				W.getClassTempSession(Y, ag)
			}
			if (G.VerifySig == "") {
				W.getVerifyCodeSession(0, ag)
			}
		}
		*/

		// TODO: Make C2C image connection
		/*
		var x = N.match(/\x15\S{30,}\x1f/gi) || [];
		for (var Z = 0; Z < x.length; Z++) {
			var y = x[Z].substr(2, 36);
			var v = "C";
			W.applyLongConnId(S, v);
			break
		}
		*/
	} else if (wm.type==WEBQQ_MESSAGE_TYPE_CONTACT1 &&  wm.requestType==WEBQQ_MESSAGE_REQUEST_FILE_RECEIVE) {
		Log("cs0x17: Contact1/FileReceive(81)/%s",pszArgs);
		// pszArgs@7
		wm.fileType=pszArgs;
		wm.fileID=pszArgs+strlen(pszArgs)+1;
		AppendCS0x17Reply(wm.D,wm.sender,wm.msgseq,3,wm.M);

		if (strcmp(wm.fileType,"f")) {
			// C2C Image
			// W.agreeLongConnId(wm.sender, wm.fileType, wm.fileID)
			char szTemp[MAX_PATH];
			sprintf(szTemp,"%u;83;%s;%s",wm.sender,wm.fileType,wm.fileID);
			AppendQuery(cs0x16_sendMessage,szTemp);
			return false;
		} else {
			// TODO: C2C File Request
			wm.text=GetArgument(pszArgs,3); // file name

			// Agree: al._qqclient.agreeLongConnId(wm.sender, wm.fileType, wm.fileID);
			// Refuse: ar._qqclient.refuseLongConnId(wm.sender, wm.fileType, wm.fileID);
		}
		// Ref
		/*
		var E = q[7],
			u = q[8];
		if (E != "f") {
			W.agreeLongConnId(S, E, u)
		}
		if (E == "f") {
			var B = q[10];
			W.lc[u] = B;
			var L = true;
			for (var ab = 0; ab < G.tmp_file_ref.length; ab++) {
				if (G.tmp_file_ref[ab] == (S + "*" + u + "*" + B)) {
					L = false;
					break
				}
			}
			if (L == true) {
				G.tmp_file_ref.push(S + "*" + u + "*" + B);
				if (q[9] != "") {
					V.setTime(parseInt(q[9]) * 1000)
				}
				var z = S + "_" + u + "_" + B + "_agree",
					O = S + "_" + u + "_" + B + "_refuse";
				aj = V.format("yyyy-MM-dd hh:mm:ss");
				var I = W.bdylist.allUsers[String(S)].realname || W.bdylist.allUsers[String(S)].nick || S;
				T = '<span class="file_msg">对方给您发送了一个文件"' + String(B).forHtml() + '"</span><span class="filemsg_button"><a href="###" id="' + z + '">[同意]</a></span><span class="filemsg_button"><a href="###" id="' + O + '">[拒绝]</a></span>';
				m.on(z, "click", function (ak, al) {
					al._qqclient.agreeLongConnId(S, E, u);
					this.style.color = "gray";
					this.style.cursor = "default";
					this.style.textDecoration = "none";
					h.get(O).style.color = "gray";
					h.get(O).style.cursor = "default";
					h.get(O).style.textDecoration = "none";
					m.removeListener(z, "click");
					m.removeListener(O, "click");
					m.stopEvent(ak)
				}, this);
				m.on(O, "click", function (ax, ar) {
					ar._qqclient.refuseLongConnId(S, E, u);
					this.style.color = "gray";
					this.style.cursor = "default";
					this.style.textDecoration = "none";
					h.get(z).style.color = "gray";
					h.get(z).style.cursor = "default";
					h.get(z).style.textDecoration = "none";
					var am = (ar._qqclient.pm.cs_seq++) + 1000000;
					var ak = new Date();
					var au = ak.getFullYear(),
						al = ak.getMonth() + 1,
						aw = ak.getDate(),
						an = ak.getHours(),
						ao = ak.getMinutes(),
						aq = ak.getSeconds();
					var ay = (ak.getFullYear() + "-" + ((al > 9) ? al : "0" + al) + "-" + ((aw > 9) ? aw : "0" + aw) + "&nbsp;" + ((an > 9) ? an : "0" + an) + ":" + ((ao > 9) ? ao : "0" + ao) + ":" + ((aq > 9) ? aq : "0" + aq));
					var av = '<span class="filemsg_warning_icon"></span><span class="time">' + ay + "</span>";
					var az = av;
					var at = '<span class="file_msg">您已拒绝接收对方发送的文件"' + String(B).forHtml() + '"</span>';
					var ap = ar._qqclient.mainPanel._tabsManage._uin2container[S] || null;
					ap.toSendQQC2CMsg(S, am, az, at);
					m.removeListener(z, "click");
					m.removeListener(O, "click");
					m.stopEvent(ax)
				}, this);
				n = "对方给您发送了一个文件" + B;
				var s = '<span class="filemsg_warning_icon"></span><span class="time">' + aj + "</span>";
				var ah = W.bdylist.allUsers[String(S)].history.append(D, {
					style: "system-id",
					msg: s
				}, {
					style: "content",
					msg: T,
					font: A
				});
				if (!ah) {
					return true
				}
				if ((W.isMaskUserMsg() && !W.isActiveChatBox(o)) || !o || !o.isShowing()) {
					W.mainPanel.mf_addOffMsg(S, R, n, aj, 1)
				}
				if (!W.isMaskUserMsg()) {
					if (!o) {
						W.actChat(S, true)
					}
					W.mainPanel._tabsManage.mf_notify_recv(S);
					if (G.group_id != "1001") {
						if (!W.isMaskSound()) {
							W.playMsgSnd(1)
						}
						W.prompt_msg.change = "-" + String(I) + "来消息啦...";
						W.promptMsg()
					}
				}
				if (o) {
					var X = o._talkTabs;
					X.mf_updateHistory(S)
				}
			}
		}
		*/
	} else if ((wm.type==WEBQQ_MESSAGE_TYPE_CONTACT1||wm.type==WEBQQ_MESSAGE_TYPE_CONTACT2) && wm.requestType==WEBQQ_MESSAGE_REQUEST_FILE_APPROVE) {
		Log("cs0x17: Contact1or2/RequestFileApprove(83)/%s",pszArgs);
		// pszArgs@7
		wm.fileType=pszArgs;
		wm.fileID=pszArgs+strlen(pszArgs)+1;
		wm.timestamp=strtoul(wm.fileID+strlen(wm.fileID)+1,NULL,10);
		AppendCS0x17Reply(wm.D,wm.sender,wm.msgseq,3,wm.M);

		if (!strcmp(wm.fileType,"f")) {
			// TODO: Begin send file (Seems works at background?)
		} else {
			// TODO: Show user pic
			/*
			G.long_conn = new c(wm.fileType, wm.fileID, new Date());
			W.showUserPic(wm.sender, wm.fileType, wm.fileID)
			*/
		}
		// Ref
		/*
		var E = q[7],
			u = q[8];
		if (E == "f") {
			if (q[9] != "") {
				V.setTime(parseInt(q[9]) * 1000)
			}
			aj = V.format("yyyy-MM-dd hh:mm:ss");
			var I = W.bdylist.allUsers[String(S)].realname || W.bdylist.allUsers[String(S)].nick || S;
			var B = W.lc[u & 4095] || "文件";
			T = '<span class="file_msg">对方已同意接收"' + String(B).forHtml() + '"，开始传输文件</span>';
			n = "对方已同意接收" + B + "，开始传输文件";
			var s = '<span class="filemsg_warning_icon"></span><span class="time">' + aj + "</span>";
			var ah = W.bdylist.allUsers[String(S)].history.append(D, {
				style: "system-id",
				msg: s
			}, {
				style: "content",
				msg: T,
				font: A
			});
			if (!ah) {
				return true
			}
			if ((W.isMaskUserMsg() && !W.isActiveChatBox(o)) || !o || !o.isShowing()) {
				W.mainPanel.mf_addOffMsg(S, R, n, aj, 1)
			}
			if (!W.isMaskUserMsg()) {
				if (!o) {
					W.actChat(S, true)
				}
				W.mainPanel._tabsManage.mf_notify_recv(S);
				if (G.group_id != "1001") {
					if (!W.isMaskSound()) {
						W.playMsgSnd(1)
					}
				}
			}
			if (o) {
				var X = o._talkTabs;
				X.mf_updateHistory(S)
			}
		} else {
			G.long_conn = new c(E, u, new Date());
			W.showUserPic(S, E, u)
		}*/
	} else if ((wm.type==WEBQQ_MESSAGE_TYPE_CONTACT1||wm.type==WEBQQ_MESSAGE_TYPE_CONTACT2) && wm.requestType==WEBQQ_MESSAGE_REQUEST_FILE_REFUSE) {
		// pszArgs@7
		Log("cs0x17: Contact1or2/RequestFileRefuse(85)/%s",pszArgs);
		wm.fileType=pszArgs;
		wm.fileID=pszArgs+strlen(pszArgs)+1;
		wm.fileRefuseType=atoi(GetArgument(wm.fileID,2));
		AppendCS0x17Reply(wm.D,wm.sender,wm.msgseq,3,wm.M);

		if (!strcmp(wm.fileType,"f")) {
			if (wm.fileRefuseType==WEBQQ_MESSAGE_REFUSE_TYPE_CANCEL) {
				wm.fileRefuseContactCancelSend=(atoi(GetArgument(wm.fileID,1))==0);
			}
		}
		// Ref
		/*
		var E = q[7],
			u = q[8],
			aa = Number(q[9]),
			Q = Number(q[10]),
			s = "";
		if (q[11] != "") {
			V.setTime(parseInt(q[11]) * 1000)
		}
		aj = V.format("yyyy-MM-dd hh:mm:ss");
		if (E == "f") {
			var B = (W.lc[u & 4095]) || (W.lc[u]) || "文件";
			var I = W.bdylist.allUsers[String(S)].realname || W.bdylist.allUsers[String(S)].nick || S;
			if (Q == 1) {
				s = '<span class="filemsg_failure_icon"></span><span class="time">' + aj + "</span>";
				if (aa == 0) {
					T = '<span class="file_msg">对方取消发送文件"' + String(B).forHtml() + '"，文件传输中止</span>';
					n = "对方取消了发送文件" + B + "，文件传输中止"
				} else {
					T = '<span class="file_msg">对方取消了接收文件"' + String(B).forHtml() + '"，文件传输中止</span>';
					n = "对方取消了接收文件" + B + "，文件传输中止"
				}
			} else {
				if (Q == 2) {
					s = '<span class="filemsg_failure_icon"></span><span class="time">' + aj + "</span>";
					T = '<span class="file_msg">对方已拒绝或取消接收文件"' + String(B).forHtml() + '"，文件传输中止</span>';
					n = "对方已拒绝或取消接收文件" + B + "，文件传输中止"
				} else {
					if (Q == 3) {
						s = '<span class="filemsg_failure_icon"></span><span class="time">' + aj + "</span>";
						T = '<span class="file_msg">"' + String(B).forHtml() + '"传输失败，请重试</span>';
						n = B + "传输失败，请重试"
					}
				}
			}
			var ah = W.bdylist.allUsers[String(S)].history.append(D, {
				style: "system-id",
				msg: s
			}, {
				style: "content",
				msg: T,
				font: A
			});
			if (!ah) {
				return true
			}
			if ((W.isMaskUserMsg() && !W.isActiveChatBox(o)) || !o || !o.isShowing()) {
				W.mainPanel.mf_addOffMsg(S, R, n, aj, 1)
			}
			if (!W.isMaskUserMsg()) {
				if (!o) {
					W.actChat(S, true)
				}
				W.mainPanel._tabsManage.mf_notify_recv(S);
				if (G.group_id != "1001") {
					if (!W.isMaskSound()) {
						W.playMsgSnd(1)
					}
				}
			}
			if (o) {
				var X = o._talkTabs;
				X.mf_updateHistory(S)
			}
		}
		*/
	} else if ((wm.type==WEBQQ_MESSAGE_TYPE_CONTACT1 || wm.type==WEBQQ_MESSAGE_TYPE_CONTACT2) && wm.requestType==WEBQQ_MESSAGE_REQUEST_FILE_SUCCESS) {
		Log("cs0x17: Contact1or2/RequestFileSuccess(87)/%s",pszArgs);
		// pszArgs@7
		wm.fileType=pszArgs;
		wm.fileID=pszArgs+strlen(pszArgs)+1;
		wm.timestamp=strtoul(wm.fileID+strlen(wm.fileID)+1,NULL,10);
		AppendCS0x17Reply(wm.D,wm.sender,wm.msgseq,3,wm.M);

		if (strcmp(wm.fileType,"f")) {
			// TODO: Show user pic?
		} else {
			LPSTR p=GetArgument(wm.fileID,3);
			LPSTR f=p+strlen(p)+1;

			// TODO: W.getFileInfo(wm.sender, wm.fileID, p, f);
		}
		/*
		var E = q[7],
			u = q[8];
		if (q[9] != "0") {
			V.setTime(parseInt(q[9]) * 1000)
		}
		aj = V.format("yyyy-MM-dd hh:mm:ss");
		if (E != "f") {
			W.showUserPic(S, E, u)
		} else {
			var P = q[11],
				F = q[12],
				ac = W.getFileInfo(S, u, P, F);
			if (ac == null) {
				return true
			}
			var I = W.bdylist.allUsers[String(S)].realname || W.bdylist.allUsers[String(S)].nick || S;
			var s = '<span class="filemsg_warning_icon"></span><span class="time">' + aj + "</span>";
			T = '<span class="file_msg">您同意接收文件"' + String(ac[1]).forHtml() + '"</span><span class="filemsg_button"><a href="###" id="' + String(ac[0]).forHtml() + '">[点击获取]</a></span>';
			var J = 0;
			if ((J = String(ac[1]).indexOf(".WebQQ")) > -1 && (J = String(ac[1]).length - 6)) {
				T += '<br /><span class="important_msg">[安全提示]&nbsp;</span><span>此文件类型存在安全风险，为防止误点击运行恶意程序，已在原文件名称后添加了“.WebQQ”，如确认文件来源可靠，在接收文件后删除文件名中的“.WebQQ”后即可正常使用。</span><a href="http://im.qq.com/cgi-bin/safe/handbook?type=5&faq=1" target="_blank">查看详细帮助</a>'
			}
			m.on(ac[0], "click", function (ak, al) {
				window.open(ac[0], "filelog");
				this.style.color = "gray";
				this.style.cursor = "default";
				this.style.textDecoration = "none";
				m.removeListener(ac[0], "click");
				m.stopEvent(ak)
			}, this);
			n = "对方发送了一个文件" + ac[1] + "给您";
			var ah = W.bdylist.allUsers[String(S)].history.append(D, {
				style: "system-id",
				msg: s
			}, {
				style: "content",
				msg: T,
				font: A
			});
			if (!ah) {
				return true
			}
			if ((W.isMaskUserMsg() && !W.isActiveChatBox(o)) || !o || !o.isShowing()) {
				W.mainPanel.mf_addOffMsg(S, R, n, aj, 1)
			}
			if (!W.isMaskUserMsg()) {
				if (!o) {
					W.actChat(S, true)
				}
				W.mainPanel._tabsManage.mf_notify_recv(S);
				if (G.group_id != "1001") {
					if (!W.isMaskSound()) {
						W.playMsgSnd(1)
					}
				}
			}
			if (o) {
				var X = o._talkTabs;
				X.mf_updateHistory(S)
			}
		}
		*/
	}
	if (wm.type==WEBQQ_MESSAGE_TYPE_CLASS) {
		// pszArgs@8/9
		// Qun message
		AppendCS0x17Reply(wm.D,wm.sender,wm.msgseq,2,wm.M);

		if (!strcmp(pszArgs,"1")) {
			// Qun message
			pszArgs+=strlen(pszArgs)+1; // Skip a "1" value // 10
			wm.classExtID=strtoul(pszArgs,NULL,10);

			pszArgs+=strlen(pszArgs)+1; // 11
			wm.classSender_str=pszArgs;
			wm.classSender=strtoul(pszArgs,NULL,10);
			wm.timestamp=strtoul(GetArgument(pszArgs,3),NULL,10);
		}
	} else if (wm.timestamp==0)
		wm.timestamp=strtoul(pszArgs,NULL,10);

	if (wm.text) DecodeText(wm.text);
	SendToHub(0xffff0017,pszArgs,&wm);

	return false;
}

void CLibWebQQ::sc0x5c_onSuccLevelInfo(LPSTR pszArgs) {
	int level, vip_level, online_days, remain_days;
	DWORD qqid;

	// _cprintf("onSuccClassInfo(): ");

	switch (strtoul(pszArgs,NULL,16)) {
		case 0x88:
			pszArgs+=strlen(pszArgs)+1;
			if (!strcmp(pszArgs,"0")) {
				pszArgs+=strlen(pszArgs)+1;

				level=atoi(pszArgs);
				pszArgs+=strlen(pszArgs)+1;

				online_days=atoi(pszArgs);
				pszArgs+=strlen(pszArgs)+1;

				vip_level=atoi(pszArgs);
				pszArgs+=strlen(pszArgs)+1;

				remain_days=atoi(pszArgs);
				pszArgs+=strlen(pszArgs)+1;

				// _cprintf("level=%d online=%d remain=%d vip=%d\n",level,online_days,remain_days,vip_level);
			}
			break;
		case 0x89:
			// _cprintf("\n");
			pszArgs+=strlen(pszArgs)+1;
			if (!strcmp(pszArgs,"0")) {
				pszArgs+=strlen(pszArgs)+1;
				while (*pszArgs) {
					qqid=strtoul(pszArgs,0,10);
					pszArgs+=strlen(pszArgs)+1;

					level=atoi(pszArgs);
					pszArgs+=strlen(pszArgs)+1;

					vip_level=atoi(pszArgs);
					pszArgs+=strlen(pszArgs)+1;
					// _cprintf("- qqid=%u level=%d vip_level=%d\n",qqid,level,vip_level);
				}
			}
			break;
	}
}

void CLibWebQQ::sc0x67_onSuccLongNickInfo(LPSTR pszArgs) {
	switch (atoi(pszArgs)) {
		case 1:
			// ? !atoi(GetArgument(pszArgs,1)
			break;
		case 2:
			// ? return true;
			break;
		case 3:
			pszArgs+=strlen(pszArgs)+1;
			if (!strcmp(pszArgs,"0")) {
				LPWEBQQ_LONGNAMEINFO pLNIHead=NULL, pLNILast=NULL, pLNICurrent=NULL;
				pszArgs+=strlen(pszArgs)+1;
				pszArgs+=strlen(pszArgs)+1; // Skip unknown myid+1

				while (*pszArgs) {
					pLNICurrent=(LPWEBQQ_LONGNAMEINFO)LocalAlloc(LMEM_FIXED,sizeof(WEBQQ_LONGNAMEINFO));
					if (pLNIHead) {
						pLNILast->next=pLNICurrent;
						pLNILast=pLNICurrent;
					} else {
						pLNIHead=pLNILast=pLNICurrent;
					}
					pLNICurrent->next=NULL;

					pLNICurrent->index=strtoul(pszArgs,NULL,10);
					pszArgs+=strlen(pszArgs)+1;

					pLNICurrent->name=pszArgs;
					pszArgs+=strlen(pszArgs)+1;
					// _cprintf("- QQID=%u LongNick=%s\n",qqid,nick);
				}

				SendToHub(0xffff0067,pszArgs,pLNIHead);

				while (pLNIHead) {
					pLNICurrent=pLNIHead->next;
					LocalFree(pLNIHead);
					pLNIHead=pLNICurrent;
				}
			}
			break;
	}
}

void CLibWebQQ::sc0x06_onSuccUserInfo(LPSTR pszArgs) {
	LPSTR pszTemp=pszArgs;

	while (pszTemp=strchr(pszTemp,'\x1e')) {
		*pszTemp=0;
		pszTemp++;
	}

	// _cprintf("- qqid=%u nick=%s gender=%s face=%d vip=%d\n",qqid,nick,gender,face,vip);

	SendToHub(0xffff0006,pszArgs);
}

void CLibWebQQ::sc0x0126_onSuccMemberNickInfo(LPSTR pszArgs) {
	int failcount=atoi(pszArgs);
	LPWEBQQ_NICKINFO pNIHead=NULL, pNILast=NULL, pNICurrent=NULL;
	pszArgs+=strlen(pszArgs)+1; // 4

	while (*pszArgs) {
		pNICurrent=(LPWEBQQ_NICKINFO)LocalAlloc(LMEM_FIXED,sizeof(WEBQQ_NICKINFO));
		if (pNIHead) {
			pNILast->next=pNICurrent;
			pNILast=pNICurrent;
		} else {
			pNIHead=pNILast=pNICurrent;
		}
		pNICurrent->next=NULL;

		pNICurrent->qqid=strtoul(pszArgs,NULL,10);
		pszArgs+=strlen(pszArgs)+1;

		if ((pNICurrent->face=strtoul(pszArgs,NULL,10)) >= FACE_MAX_INDEX) pNICurrent->face=0;
		pszArgs+=strlen(pszArgs)+1;
		
		pNICurrent->name=pszArgs;
		pszArgs+=strlen(pszArgs)+1;

		pNICurrent->viplevel=strtoul(pszArgs,NULL,10);
		pszArgs+=strlen(pszArgs)+1;

		pNICurrent->age=0;
		pNICurrent->male=false;
	}

	SendToHub(0xffff0126,pszArgs,pNIHead);

	while (pNIHead) {
		pNICurrent=pNIHead->next;
		LocalFree(pNIHead);
		pNIHead=pNICurrent;
	}
}

void CLibWebQQ::sc0x81_onContactStatus(LPSTR pszArgs) {
	WEBQQ_CONTACT_STATUS cs;
	cs.qqid=strtoul(pszArgs,NULL,10);
	pszArgs+=strlen(pszArgs)+1; // 4

	cs.status=atoi(pszArgs);
	pszArgs+=strlen(pszArgs)+1; // 5

	cs.terminationStat=atoi(pszArgs);

	SendToHub(0xffff0081,pszArgs,&cs);
}

void CLibWebQQ::sc0x01_onResetLogin(LPSTR pszArgs) {
	switch (atoi(GetArgument(pszArgs,1))) {
		case 1:
		case 5:
		case 8:

		case 2:
		case 6:
		case 9:
			cs_0x26_timeout=0;
			cs_0x3e_next_pos=0;
			AppendQuery(cs0x22_getLoginInfo);
			break;
		case 3: // password_error
		case 4: // privilege_error
		case 7: // failed_conn_server
		default: // unknown_error
			SetStatus(WEBQQ_STATUS_ERROR);
			break;

	}
}

void CLibWebQQ::sc0x80_onSystemMessage(LPSTR pszArgs) {
	if (!strcmp(pszArgs,"3")) {
		// Only type 3 (Add successful) is support atm
		SendToHub(0xffff0080,pszArgs,GetArgument(pszArgs,1));
	}
}

/** End **/
#endif // Web1

/** MIMQQ4b **/
LPSTR CLibWebQQ::PostHTMLDocument(LPCSTR pszServer, LPCSTR pszUri, LPCSTR pszReferer, LPCSTR pszPostBody, LPDWORD pdwLength) {
	DWORD dwRead=BUFFERSIZE;
	LPSTR pszBuffer; //=m_buffer;
	bool passvar=false;
	bool vfpost=!strncmp(pszPostBody,"r={",3);
	WaitForSingleObject(m_hEventCONN,INFINITE);

	Log("%s > %s",pszUri,pszPostBody);
	// InternetCanonicalizeUrlA(pszPostBody,m_buffer,&dwRead,0);
	LPSTR pszPOSTBuffer=pszBuffer=(LPSTR)LocalAlloc(LMEM_FIXED,(strlen(pszPostBody)+1)*3);

	LPCSTR pszBody=pszPostBody;
	while (*pszBody) {
		if (*pszBody>='a' && *pszBody<='z' || *pszBody>='A' && *pszBody<='Z' || *pszBody>='0' && *pszBody<='9')
			*pszBuffer++=*pszBody;
		else if (*pszBody==' ')
			*pszBuffer++='+';
		else if (vfpost && *pszBody=='=' && !passvar) {
			passvar=true;
			*pszBuffer++=*pszBody;
		} else if (!vfpost && *pszBody=='&' || *pszBody=='=') {
			*pszBuffer++=*pszBody;
		} else
			pszBuffer+=sprintf(pszBuffer,"%%%02X",(unsigned char)*pszBody);

		pszBody++;
	}
	*pszBuffer=0;

	//HINTERNET m_hInet;

	/*
	if (!(m_hInet=InternetOpenA("Mozilla/5.0 (Windows; U; Windows NT 5.1; en-US; rv:1.9.1.3) Gecko/20090824 Firefox/3.5.3 GTB5",m_proxyhost?INTERNET_OPEN_TYPE_PROXY:INTERNET_OPEN_TYPE_DIRECT,m_proxyhost,NULL,0))) {
		Log(__FUNCTION__"(): Failed initializing WinINet (InternetOpen)! Err=%d\n",GetLastError());
		*pdwLength=(DWORD)-1;
		return FALSE;
	}

	if (m_proxyuser) {
		InternetSetOption(m_hInet,INTERNET_OPTION_USERNAME, m_proxyuser,  (DWORD)strlen(m_proxyuser)+1);  
		if (m_proxypass) {
			InternetSetOption(m_hInet,INTERNET_OPTION_PASSWORD, m_proxypass,  (DWORD)strlen(m_proxypass)+1);  
		}
	}
	*/

	HINTERNET hInetConnect=InternetConnectA(m_hInet,pszServer,INTERNET_DEFAULT_HTTP_PORT,NULL,NULL,INTERNET_SERVICE_HTTP,0,NULL);
	if (!hInetConnect) {
		Log(__FUNCTION__"(): InternetConnectA() failed: %d",GetLastError());
		SetEvent(m_hEventCONN);
		*pdwLength=(DWORD)-1;
		LocalFree(pszPOSTBuffer);
		// InternetCloseHandle(m_hInet);
		return false;
	}

	HINTERNET hInetRequest=HttpOpenRequestA(hInetConnect,"POST",pszUri,NULL,pszReferer,NULL,INTERNET_FLAG_PRAGMA_NOCACHE|INTERNET_FLAG_RELOAD,NULL);
	if (!hInetRequest) {
		DWORD err=GetLastError();
		Log(__FUNCTION__"(): HttpOpenRequestA() failed: %d", err);
		SetEvent(m_hEventCONN);
		*pdwLength=(DWORD)-1;
		LocalFree(pszPOSTBuffer);
		InternetCloseHandle(hInetConnect);
		SetLastError(err);
		// InternetCloseHandle(m_hInet);
		return false;
	}

	if (!(HttpSendRequestA(hInetRequest,"Content-Type: application/x-www-form-urlencoded; charset=UTF-8\r\nX-Requested-With: XMLHttpRequest",-1,(LPVOID)pszPOSTBuffer,(DWORD)strlen(pszPOSTBuffer)))) {
		DWORD err=GetLastError();
		Log(__FUNCTION__"(): HttpSendRequestA() failed, reason=%d",err);
		InternetCloseHandle(hInetRequest);
		// m_hInetRequest=NULL;
		InternetCloseHandle(hInetConnect);
		// InternetCloseHandle(m_hInet);
		Log(__FUNCTION__"(): Terminate connection");
		SetEvent(m_hEventCONN);
		*pdwLength=(DWORD)-1;
		LocalFree(pszPOSTBuffer);
		return false;
	}
	LocalFree(pszPOSTBuffer);

	dwRead=0;
	DWORD dwWritten=sizeof(DWORD);
	/*
	pszBuffer=m_buffer;

	while (InternetReadFile(hInetRequest,pszBuffer,BUFFERSIZE-1,&dwRead)==TRUE && dwRead>0) {
		pszBuffer+=dwRead;
		dwRead=0;
	}
	*pszBuffer=0;
	*pdwLength=pszBuffer-m_buffer;

	Log("%s < size=%d",pszUri,*pdwLength);
	LPSTR pszRet=(LPSTR)LocalAlloc(LMEM_FIXED,*pdwLength+1);
	memcpy(pszRet,m_buffer,*pdwLength+1);
	*/
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
	// InternetCloseHandle(m_hInet);
	
	SetEvent(m_hEventCONN);

	return pszBuffer/*pszRet*/;
}

void CLibWebQQ::Web2UploadP2PImage(HANDLE hFile, LPSTR pszFilename, DWORD respondid) {
	DWORD filelen=GetFileSize(hFile,NULL);
	LPSTR pszBuffer=(LPSTR)LocalAlloc(LMEM_FIXED,filelen+1536);
	int len;
	LPSTR pszSuffix=strrchr(pszFilename,'.');
	WEBQQ_QUNUPLOAD_STATUS wqs={respondid};

	sprintf(pszBuffer,"/ftn_access/upload_offline_pic?time=%u%03d",(DWORD)time(NULL),GetTickCount()%1000);
	
	HINTERNET hConn=InternetConnectA(m_hInet,"weboffline.ftn.qq.com",INTERNET_DEFAULT_HTTP_PORT,NULL,NULL,INTERNET_SERVICE_HTTP,0,NULL);
	HINTERNET hRequest=HttpOpenRequestA(hConn,"POST",pszBuffer,NULL,NULL,NULL,INTERNET_FLAG_PRAGMA_NOCACHE|INTERNET_FLAG_NO_CACHE_WRITE|INTERNET_FLAG_RELOAD,NULL);
	/*
	HINTERNET hConn=InternetConnectA(m_hInet,"127.0.0.1",INTERNET_DEFAULT_HTTP_PORT,NULL,NULL,INTERNET_SERVICE_HTTP,0,NULL);
	HINTERNET hRequest=HttpOpenRequestA(hConn,"POST","/posttest.php",NULL,NULL,NULL,INTERNET_FLAG_PRAGMA_NOCACHE|INTERNET_FLAG_NO_CACHE_WRITE|INTERNET_FLAG_RELOAD,NULL);
	*/

	// HttpAddRequestHeadersA(hRequest,"Accept: application/xml,application/xhtml+xml,text/html;q=0.9,text/plain;q=0.8,image/png,*/*;q=0.5",-1,HTTP_ADDREQ_FLAG_REPLACE|HTTP_ADDREQ_FLAG_ADD);
	HttpAddRequestHeadersA(hRequest,"Content-Type: multipart/form-data; boundary=" BOUNDARY,-1,HTTP_ADDREQ_FLAG_REPLACE|HTTP_ADDREQ_FLAG_ADD);
	HttpAddRequestHeadersA(hRequest,"Origin: http://web2.qq.com",-1,HTTP_ADDREQ_FLAG_REPLACE|HTTP_ADDREQ_FLAG_ADD);
	HttpAddRequestHeadersA(hRequest,"Referer: http://web2.qq.com/",-1,HTTP_ADDREQ_FLAG_REPLACE|HTTP_ADDREQ_FLAG_ADD);
	// HttpAddRequestHeadersA(hRequest,"User-Agent: Mozilla/5.0 (Windows; U; Windows NT 6.1; en-US) AppleWebKit/533.4 (KHTML, like Gecko) Chrome/5.0.375.99 Safari/533.4",-1,HTTP_ADDREQ_FLAG_REPLACE|HTTP_ADDREQ_FLAG_ADD);

	len=0;
	len+=sprintf(pszBuffer+len,    "--" BOUNDARY "\r\nContent-Disposition: form-data; name=\"callback\"\r\n\r\nparent.EQQ.Model.ChatMsg.callbackSendPic");
	len+=sprintf(pszBuffer+len,"\r\n--" BOUNDARY "\r\nContent-Disposition: form-data; name=\"locallangid\"\r\n\r\n2052");
	len+=sprintf(pszBuffer+len,"\r\n--" BOUNDARY "\r\nContent-Disposition: form-data; name=\"clientversion\"\r\n\r\n1409");
	len+=sprintf(pszBuffer+len,"\r\n--" BOUNDARY "\r\nContent-Disposition: form-data; name=\"uin\"\r\n\r\n%u",m_qqid);
	len+=sprintf(pszBuffer+len,"\r\n--" BOUNDARY "\r\nContent-Disposition: form-data; name=\"skey\"\r\n\r\n%s",GetCookie("skey"));
	len+=sprintf(pszBuffer+len,"\r\n--" BOUNDARY "\r\nContent-Disposition: form-data; name=\"appid\"\r\n\r\n1002101");
	len+=sprintf(pszBuffer+len,"\r\n--" BOUNDARY "\r\nContent-Disposition: form-data; name=\"peeruin\"\r\n\r\n593023668");
	len+=sprintf(pszBuffer+len,"\r\n--" BOUNDARY "\r\nContent-Disposition: form-data; name=\"file\"; filename=\"%s\"\r\nContent-Type: image/%s\r\n\r\n",pszFilename,(pszSuffix!=NULL && !stricmp(pszSuffix,".gif"))?"gif":"jpeg");
	// Log("ASSERT: len=%d, pszBuffer=%s",len,pszBuffer);
	// HttpSendRequestA(hRequest,NULL,0,pszBuffer,len);

	DWORD dwRead;
	wqs.status=0;
	wqs.number=filelen;
	SendToHub(WEBQQ_CALLBACK_QUNIMGUPLOAD,NULL,&wqs);
	// len=0;

	wqs.status=1;

	/*
	while (ReadFile(hFile,pszBuffer,16384,&dwRead,NULL) && dwRead>0) {
		HttpSendRequestA(hRequest,NULL,0,pszBuffer,dwRead);
		len+=dwRead;
		wqs.number=len;
		SendToHub(WEBQQ_CALLBACK_QUNIMGUPLOAD,NULL,&wqs);
	}
	*/
	ReadFile(hFile,pszBuffer+len,filelen,&dwRead,NULL);
	CloseHandle(hFile);

	len+=filelen;

	len+=sprintf(pszBuffer+len,"\r\n--" BOUNDARY "\r\nContent-Disposition: form-data; name=\"fileid\"\r\n\r\n1");
	len+=sprintf(pszBuffer+len,"\r\n--" BOUNDARY "--\r\n");
	HttpSendRequestA(hRequest,NULL,0,pszBuffer,len);
	
	wqs.status=2;
	wqs.number=0;
	SendToHub(WEBQQ_CALLBACK_QUNIMGUPLOAD,NULL,&wqs);

	DWORD dwWritten=sizeof(DWORD);
	DWORD dwValue=0;
	
	HttpQueryInfo(hRequest,HTTP_QUERY_STATUS_CODE|HTTP_QUERY_FLAG_NUMBER,&dwValue,&dwWritten,&dwRead);
	Log(__FUNCTION__"() Status=%d",dwValue);

	InternetReadFile(hRequest,pszBuffer,16384,&dwRead);
	pszBuffer[dwRead]=0;
	InternetCloseHandle(hRequest);
	InternetCloseHandle(hConn);

	JSONNODE* jn;
	LPSTR pszNode;
	LPSTR pszData;
	char szTemp[512];

	// <head><script type="text/javascript">document.domain='qq.com';parent.EQQ.Model.ChatMsg.callbackSendPic({"retcode":0, "result":"", "progress":100, "filesize":5013, "fileid":2, "filename":"%7B7BF33453-6B8E-BE04-5B46-E23CE852122F%7D[1].jpg", "filepath":"/b79dfec0-0412-4dd5-83a3-1bac4339b5f0"});</script></head><body></body>
	if (strstr(pszBuffer,"({\"retcode\"")) {
		*strstr(pszBuffer,");</")=0;
	
		jn=json_parse(strchr(pszBuffer,'{'));
		if (json_as_int(json_get(jn,"retcode"))==0) {
			sprintf(szTemp,"http://web2-b.qq.com/channel/apply_offline_pic_dl?f_uin=%u&file_path=%s&clientid=%s&psessionid=%s&t=%u%03d",m_qqid,pszNode=json_as_string(json_get(jn,"filepath")),m_web2_clientid,m_web2_psessionid,(DWORD)time(NULL),GetTickCount()%1000);
			json_free(pszNode);
			json_delete(jn);

			if ((pszData=GetHTMLDocument(szTemp,g_referer_web2,&dwWritten))!=NULL && dwWritten!=0xffffffff && dwWritten>0 && *pszData=='{') {
				JSONNODE* jn=json_parse(pszData);

				if (json_as_int(json_get(jn,"retcode"))==0) {
					JSONNODE* jnResult=json_get(jn,"result");
					LPSTR pszOutUrl=pszBuffer;
					LPSTR pszUrl=json_as_string(json_get(jnResult,"url"));
					Log(__FUNCTION__"(): ASSERT success=%d",json_as_int(json_get(jnResult,"success")));

					pszOutUrl+=strlen(strcpy(pszOutUrl,strstr(pszUrl,"://")+3));
					json_free(pszUrl);
					pszUrl=json_as_string(json_get(jnResult,"file_path"));
					pszOutUrl+=sprintf(pszOutUrl,"&file_path=%s&file_name=%s&file_size=%u",pszUrl,pszFilename,filelen);
					json_free(pszUrl);

					wqs.status=3;
					wqs.string=pszBuffer;
				} else {
					wqs.status=4;
					wqs.string="apply_offline_pic_dl JSON retcode error";
				}

				LocalFree(pszData);
			} else {
				wqs.status=4;
				wqs.string="apply_offline_pic_dl fetch error";
			}
		} else {
			wqs.status=4;
			wqs.string="upload_offline_pic JSON retcode error";
		}

		json_delete(jn);
	} else {
		wqs.status=4;
		wqs.string="upload_offline_pic reply error";
	}

	SendToHub(WEBQQ_CALLBACK_WEB2P2PIMGUPLOAD,NULL,&wqs);
	LocalFree(pszBuffer);
}

bool CLibWebQQ::web2_api_get_vfwebqq2() {
	LPSTR pszTemp;
	char szUrl[80];
	DWORD dwSize;

	sprintf(szUrl,"http://%s/api/get_vfwebqq2?t=%u%03d",g_server_web2_connection,(DWORD)time(NULL),GetTickCount());


	if ((pszTemp=GetHTMLDocument(szUrl,g_referer_web2proxy,&dwSize))!=NULL && dwSize!=0xffffffff) {
		// {"retcode":0,"result":["vfwebqq","4eb4c25480967da663fa5f47fcac9d8c724ab54f6540e4006d8eb828319b4c0cf8f3ce306c75cd78"]}
		JSONNODE* jn=json_parse(pszTemp);

		int retcode=json_as_int(json_get(jn,"retcode"));
		if (retcode==0) {
			bool relogin=m_web2_storage[WEBQQ_WEB2_STORAGE_LOGININFO]!=NULL;
			m_web2_logininfo=json_get(jn,"result");

			if (m_web2_storage[WEBQQ_WEB2_STORAGE_LOGININFO]) json_delete(m_web2_storage[WEBQQ_WEB2_STORAGE_LOGININFO]);
			m_web2_storage[WEBQQ_WEB2_STORAGE_LOGININFO]=jn;
			m_web2_vfwebqq=json_as_string(json_get(m_web2_logininfo,"vfwebqq"));

			if (!m_web2_vfwebqq || !*m_web2_vfwebqq) {
				Log("%s: Invalid vfwebqq!",__FUNCTION__);
				SendToHub(WEBQQ_CALLBACK_WEB2_ERROR,"vfwebqq",pszTemp);
			}
			
			if (!relogin) {
				SetStatus(m_loginhide?WEBQQ_STATUS_INVISIBLE:WEBQQ_STATUS_ONLINE);

				web2_api_get_friend_info(m_qqid);
				web2_api_get_qq_level(m_qqid);
				web2_api_get_single_long_nick(m_qqid);
				web2_api_get_group_name_list_mask();
				web2_api_get_user_friends();
				// web2_api_get_recent_contact();
				web2_channel_get_online_buddies();
			}
		} else {
			// Unknown login error
			WEBQQ_LOGININFO li={retcode,json_as_string(json_get(jn,"errmsg"))};
			SendToHub(0xffff0022,pszTemp,&li);
			SetStatus(WEBQQ_STATUS_ERROR);
			json_free(li.msg);
			json_delete(jn);
		}
		LocalFree(pszTemp);
	}
	return false;
}

bool CLibWebQQ::web2_channel_login() {
	LPSTR pszTemp;
	DWORD dwSize;
	char szQuery[512];

	if (m_web2_psessionid) 
		sprintf(szQuery,"r={\"status\":\"%s\",\"ptwebqq\":\"%s\",\"passwd_sig\":\"%s\",\"clientid\":\"%s\",\"psessonid\":\"%s\"}",m_loginhide?"hidden":"online",GetCookie("ptwebqq"),""/*DNA Not supported*/,m_web2_clientid,m_web2_psessionid);
	else
		sprintf(szQuery,"r={\"status\":\"%s\",\"ptwebqq\":\"%s\",\"passwd_sig\":\"%s\",\"clientid\":\"%s\",\"psessonid\":null}",m_loginhide?"hidden":"online",GetCookie("ptwebqq"),""/*DNA Not supported*/,m_web2_clientid);
	m_web2_logininfo=NULL;

	if ((pszTemp=PostHTMLDocument("d.web2.qq.com","/channel/login2","http://d.web2.qq.com/proxy.html?v=20101025002",szQuery,&dwSize))!=NULL && dwSize!=0xffffffff) {
		// {"retcode":0,"result":{"uin":431533706,"mode":"master","index":1053,"port":9000,"status":"online","vfwebqq":"3a111410084da65fa99c73d579803b94b7a8533a6e739e5641e7ddf66cc7c3e8e437c169e8b9728c"}}
		// {"retcode":0,"result":{"uin":85379868,"mode":"master","index":1056,"port":45411,"status":"online","vfwebqq":"c10ee8c5e05a274c37d1dac496a4a904cec41351ae36c80d3f69558d1d47398a822626e7e74d795b","psessionid":"8368046764001D636F6E6E7365727665725F77656271714031302E3133332E322E313434000020770000001203620516CB1C6D0000000A405561793973553355676D00000028C10EE8C5E05A274C37D1DAC496A4A904CEC41351AE36C80D3F69558D1D47398A822626E7E74D795B"}}
		
		JSONNODE* jn=json_parse(pszTemp);
		int retcode=json_as_int(json_get(jn,"retcode"));
		if (retcode==0) {
			bool relogin=m_web2_storage[WEBQQ_WEB2_STORAGE_LOGININFO]!=NULL;
			m_web2_logininfo=json_get(jn,"result");

			if (m_web2_storage[WEBQQ_WEB2_STORAGE_LOGININFO]) json_delete(m_web2_storage[WEBQQ_WEB2_STORAGE_LOGININFO]);
			m_web2_storage[WEBQQ_WEB2_STORAGE_LOGININFO]=jn;
			m_web2_vfwebqq=json_as_string(json_get(m_web2_logininfo,"vfwebqq"));
			m_web2_psessionid=json_as_string(json_get(m_web2_logininfo,"psessionid"));

			if (!m_web2_vfwebqq || !*m_web2_vfwebqq || !m_web2_psessionid || !*m_web2_psessionid) {
				Log("%s: Invalid vfwebqq/psessionid!",__FUNCTION__);
				SendToHub(WEBQQ_CALLBACK_WEB2_ERROR,"vfwebqq",pszTemp);
			}

			if (!relogin) {
				SetStatus(m_loginhide?WEBQQ_STATUS_INVISIBLE:WEBQQ_STATUS_ONLINE);

				web2_api_get_friend_info(m_qqid);
				web2_api_get_qq_level(m_qqid);
				web2_api_get_single_long_nick(m_qqid);
				web2_api_get_group_name_list_mask();
				web2_api_get_user_friends();
				// web2_api_get_recent_contact();
				web2_channel_get_online_buddies();
			}
		} else {
			// Unknown login error
			WEBQQ_LOGININFO li={retcode,json_as_string(json_get(jn,"errmsg"))};
			SendToHub(0xffff0022,pszTemp,&li);
			SetStatus(WEBQQ_STATUS_ERROR);
			json_free(li.msg);
			json_delete(jn);
		}
		LocalFree(pszTemp);
	}
	return m_web2_logininfo!=NULL;
}

bool CLibWebQQ::web2_channel_poll() {
	LPSTR pszResponse;
	DWORD dwSize;
	bool retried=false;
	char szQuery[512];

	sprintf(szQuery,"http://d.web2.qq.com/channel/poll2?clientid=%s&psessionid=%s&t=%u%03d&vfwebqq=%s",m_web2_clientid,m_web2_psessionid,(DWORD)time(NULL),GetTickCount()%1000,m_web2_vfwebqq);
	
	while (true) {
		if ((pszResponse=GetHTMLDocument(szQuery,"http://d.web2.qq.com/proxy.html?v=20101025002",&dwSize,true))==NULL || dwSize==0xffffffff) {
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
				int result=json_as_int(json_get(jn,"retcode"));

				switch (result) {
					case 0:
						// if (!ParseResponse4b(json_get(jn,"result"))) return false;
						{
							bool ret=true;
							JSONNODE* jnResult=json_get(jn,"result");
							JSONNODE* jnItem;
							int c=0;
							int nodecount=json_size(jnResult);
							LPSTR pszPollType;

							for (int c=0; c<nodecount; c++) {
								jnItem=json_at(jnResult,c);
								pszPollType=json_as_string(json_get(jnItem,"poll_type"));
								Log(__FUNCTION__"(): Poll Type=%s",pszPollType);
								SendToHub(WEBQQ_CALLBACK_WEB2,pszPollType,json_get(jnItem,"value"));
								json_free(pszPollType);
							}

							return ret;
						}
						break;
					case 102:
						if (result==102) Log(__FUNCTION__"() Event wait timeout (OK)");
						break;
					case 103:
						Log(__FUNCTION__"() Session timed out, login again");
						if (m_stop || !web2_channel_login()) {
							SetStatus(WEBQQ_STATUS_ERROR);
							return false;
						} else
							return true;
					case 100:
						Log(__FUNCTION__"() Server logged out");
						SetStatus(WEBQQ_STATUS_ERROR);
						return false;
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

bool CLibWebQQ::web2_channel_change_status(LPCSTR newstatus) {
	LPSTR pszResponse;
	DWORD dwSize;
	char szQuery[512];

	sprintf(szQuery,"http://d.web2.qq.com/channel/change_status2?newstatus=%s&clientid=%s&psessionid=%s&t=%u%03d&vfwebqq=%s",newstatus,m_web2_clientid,m_web2_psessionid,(DWORD)time(NULL),GetTickCount()%1000,m_web2_vfwebqq);

	if ((pszResponse=GetHTMLDocument(szQuery,"http://d.web2.qq.com/proxy.html?v=20101025002",&dwSize))==NULL || dwSize==0xffffffff) {
		return web2_check_result("/channel/change_status",pszResponse);
	}
	return false;
}

bool CLibWebQQ::web2_check_result(LPSTR pszCommand, LPSTR pszResult) {
	int retcode=-1;

	if (!pszResult || *pszResult!='{') {
		Log(__FUNCTION__"(): JSON Invalid for %s",pszCommand);
		Log("%s",pszResult);
		// SendToHub(WEBQQ_CALLBACK_WEB2_ERROR,pszCommand,NULL);
	} else if (JSONNODE* jn=json_parse(pszResult)) {
		retcode=json_as_int(json_get(jn,"retcode"));
		if (retcode==0) {
			SendToHub(WEBQQ_CALLBACK_WEB2,pszCommand,json_get(jn,"result"));
		} else {
			LPSTR szErrMsg=json_as_string(json_get(jn,"errmsg"));
			char szMsg[MAX_PATH];
			sprintf(szMsg,"%d:%s",retcode,szErrMsg);
			Log(__FUNCTION__"(): Command:%s, Return code %s",pszCommand,szMsg);
			SendToHub(WEBQQ_CALLBACK_WEB2_ERROR,pszCommand,szMsg);
			json_free(szErrMsg);
		}
		json_delete(jn);
	} else {
		Log(__FUNCTION__"(): JSON Parse Failed for %s",pszCommand);
		SendToHub(WEBQQ_CALLBACK_WEB2_ERROR,pszCommand,pszResult);
	}

	LocalFree(pszResult);

	return retcode==0;
}

bool CLibWebQQ::web2_api_get_friend_info(unsigned int tuin, JSONNODE** jnOutput) {
	LPSTR pszTemp;
	DWORD dwSize;
	char szQuery[MAX_PATH];

	sprintf(szQuery,"http://%s/api/get_friend_info2?tuin=%u&verifysession=&code=&vfwebqq=%s&t=%u%03d",g_server_web2_connection,tuin,m_web2_vfwebqq,(DWORD)time(NULL),GetTickCount()%1000);

	if ((pszTemp=GetHTMLDocument(szQuery, g_referer_web2,&dwSize))!=NULL && dwSize!=0xffffffff) {
		// {"retcode":0,"result":{"stat":"offline","nick":"^_^2","country":"","province":"","city":"","gender":"unknown","face":0,"birthday":{"year":0,"month":0,"day":0},"allow":1,"blood":0,"shengxiao":0,"constel":0,"phone":"-","mobile":"-","email":"431533706@qq.com","occupation":"0","college":"-","homepage":"-","personal":"\u6DB4\u6A21\u9CF4\u7AED\u64F1,\u59A6\u7E6B\u98F2\u7FB6\u8844\u96B1\u72DF\uFE5D"}}
		if (jnOutput) {
			*jnOutput=json_parse(pszTemp);
			LocalFree(pszTemp);
			return json_as_int(json_get(*jnOutput,"retcode"))==0;
		} else {
			sprintf(szQuery,"/api/get_friend_info:%u",tuin);
			return web2_check_result(szQuery,pszTemp);
		}
	}
	return false;
}

bool CLibWebQQ::web2_api_get_single_info(unsigned int tuin, JSONNODE** jnOutput) {
	LPSTR pszTemp;
	char szQuery[512];
	char szVeryCode[5];

	sprintf(szQuery,"http://ptlogin2.qq.com/getimage?aid=1003901&%u",(double)rand()/(double)RAND_MAX);

	SendToHub(WEBQQ_CALLBACK_NEEDVERIFY,szQuery,szVeryCode);
	if (*szVeryCode) {
		DWORD dwSize;

		RefreshCookie();

		sprintf(szQuery,"http://%s/api/get_single_info2?tuin=%u&verifysession=%s&code=%s&vfwebqq=%s&t=%u%03d",g_server_web2_connection,tuin,GetCookie("verifysession"),szVeryCode,m_web2_vfwebqq,(DWORD)time(NULL),GetTickCount()%1000);

		if ((pszTemp=GetHTMLDocument(szQuery, g_referer_web2,&dwSize))!=NULL && dwSize!=0xffffffff) {
			// {"retcode":0,"result":{"stat":"offline","nick":"^_^2","country":"","province":"","city":"","gender":"unknown","face":0,"birthday":{"year":0,"month":0,"day":0},"allow":1,"blood":0,"shengxiao":0,"constel":0,"phone":"-","mobile":"-","email":"431533706@qq.com","occupation":"0","college":"-","homepage":"-","personal":"\u6DB4\u6A21\u9CF4\u7AED\u64F1,\u59A6\u7E6B\u98F2\u7FB6\u8844\u96B1\u72DF\uFE5D"}}
			if (jnOutput) {
				*jnOutput=json_parse(pszTemp);
				LocalFree(pszTemp);
				return json_as_int(json_get(*jnOutput,"retcode"))==0;
			} else {
				sprintf(szQuery,"/api/get_single_info:%u",tuin);
				return web2_check_result(szQuery,pszTemp);
			}
		}
	}
	return false;
}

bool CLibWebQQ::web2_api_get_single_long_nick(unsigned int tuin) {
	LPSTR pszTemp;
	DWORD dwSize;
	char szQuery[MAX_PATH];

	sprintf(szQuery,"http://%s/api/get_single_long_nick2?tuin=%u&vfwebqq=%s&t=%u%03d",g_server_web2_connection,tuin,m_web2_vfwebqq,(DWORD)time(NULL),GetTickCount()%1000);

	if ((pszTemp=GetHTMLDocument(szQuery, g_referer_web2,&dwSize))!=NULL && dwSize!=0xffffffff) {
		// {"retcode":0,"result":[]}
		return web2_check_result("/api/get_single_long_nick",pszTemp);
	}
	return false;
}

bool CLibWebQQ::web2_vfwebqq_request(LPSTR uri) {
	if (!m_web2_vfwebqq) {
		m_web2_vfwebqq=json_as_string(json_get(m_web2_logininfo,"vfwebqq"));
	}
	if (m_web2_vfwebqq) {
		LPSTR pszTemp;
		DWORD dwSize;
		char szQuery[128];

		// r:{"vfwebqq":"3a111410084da65fa99c73d579803b94b7a8533a6e739e5641e7ddf66cc7c3e8e437c169e8b9728c"}
		sprintf(szQuery,"r={\"vfwebqq\":\"%s\"}",m_web2_vfwebqq);


		if ((pszTemp=PostHTMLDocument(g_server_web2_connection,uri,g_referer_web2proxy,szQuery,&dwSize))!=NULL && dwSize!=0xffffffff) {
			// {"retcode":0,"result":{"gnamelist":[{"gid":203532792,"code":1532792,"flag":1049617,"name":"\u51C9\u5BAB\u6625\u65E5\u7684\u65E7\u5C01\u7EDD"},{"gid":204571213,"code":2571213,"flag":17826817,"name":"Miranda IM"},{"gid":2104564026,"code":24564026,"flag":17825793,"name":"\u6E2C\u8A66\u7FA4"},{"gid":2127766740,"code":47766740,"flag":50331665,"name":"\u53F6\u8272\u7684\u95ED\u9501\u7A7A\u95F4"},{"gid":2138914413,"code":58914413,"flag":1064961,"name":"\u6E2C\u8A66\u7FA42"}],"gmasklist":[{"gid":1000,"mask":1}]}}
			Log("%s",pszTemp);
			return web2_check_result(uri,pszTemp);
		}
	} else {
		Log(__FUNCTION__"(): ERROR! m_webq_vfwebqq not defined!");
	}
	return false;
}

bool CLibWebQQ::web2_api_get_group_name_list_mask() {
	return web2_vfwebqq_request("/api/get_group_name_list_mask2");
}

bool CLibWebQQ::web2_api_get_user_friends() {
	return web2_vfwebqq_request("/api/get_user_friends2");
}


bool CLibWebQQ::web2_api_get_recent_contact() {
	return web2_vfwebqq_request("/api/get_recent_contact2");
}

bool CLibWebQQ::web2_channel_get_online_buddies() {
	LPSTR pszTemp;
	DWORD dwSize;
	char szQuery[512];

	sprintf(szQuery,"http://d.web2.qq.com/channel/get_online_buddies2?clientid=%s&psessionid=%s&t=%u%03d&vfwebqq=%s",m_web2_clientid,m_web2_psessionid,(DWORD)time(NULL),GetTickCount()%1000,m_web2_vfwebqq);

	if ((pszTemp=GetHTMLDocument(szQuery, g_referer_web2,&dwSize))!=NULL && dwSize!=0xffffffff) {
		// {"retcode":0,"result":[{"uin":315948499,"status":"busy","client_type":1},{"uin":431533686,"status":"online","client_type":1}]}
		return web2_check_result("/channel/get_online_buddies",pszTemp);
	}
	return false;
}

bool CLibWebQQ::web2_api_get_group_info_ext(unsigned int gcode) {
	LPSTR pszTemp;
	DWORD dwSize;
	char szQuery[MAX_PATH];

	sprintf(szQuery,"http://%s/api/get_group_info_ext2?gcode=%u&vfwebqq=%s&t=%u%03d",g_server_web2_connection,gcode,m_web2_vfwebqq,(DWORD)time(NULL),GetTickCount()%1000);

	if ((pszTemp=GetHTMLDocument(szQuery, g_referer_web2,&dwSize))!=NULL && dwSize!=0xffffffff) {
		// {"retcode":0,"result":{"ginfo":{"gid":204571213,"code":2571213,"flag":17826817,"owner":753633,"name":"Miranda IM","level":0,"face":1,"memo":"www.miranda-im.org\r\n\nwww.studiokuma.com/s9y\r\n\nhi.baidu.com/software10\r\n\ncnmim.d.bname.us","fingermemo":"\u672C\u7FA4\u4E0D\u89E3\u7B54\u5176\u5B83\u4EFB\u4F55\u6253\u5305MIM\u95EE\u9898 \u52A0\u5165\u987B\u6709\u8F6F\u4EF6\u4F7F\u7528\u57FA\u7840\u53CADIY\u7CBE\u795E \u8FD8\u6709\u672C\u7FA4\u957F\u671F\u6210\u5458\u63A8\u8350","members":[{"muin":82383,"mflag":4},{"muin":203376,"mflag":0},{"muin":416864,"mflag":0},{"muin":484197,"mflag":4},{"muin":753633,"mflag":12},{"muin":1044498,"mflag":4},{"muin":1576157,"mflag":4},{"muin":2299910,"mflag":0},{"muin":5978110,"mflag":12},{"muin":7281389,"mflag":0},{"muin":8318138,"mflag":0},{"muin":9124296,"mflag":0},{"muin":9437864,"mflag":0},{"muin":9449562,"mflag":12},{"muin":11065738,"mflag":4},{"muin":14061253,"mflag":0},{"muin":15485581,"mflag":0},{"muin":17876934,"mflag":0},{"muin":17883560,"mflag":12},{"muin":17951586,"mflag":0},{"muin":18002309,"mflag":0},{"muin":18214798,"mflag":4},{"muin":18408777,"mflag":0},{"muin":19424593,"mflag":0},{"muin":19609524,"mflag":0},{"muin":22661725,"mflag":0},{"muin":23849016,"mflag":0},{"muin":24006854,"mflag":4},{"muin":33228978,"mflag":4},{"muin":35999355,"mflag":0},{"muin":39928040,"mflag":8},{"muin":41194629,"mflag":5},{"muin":44157129,"mflag":0},{"muin":45435887,"mflag":8},{"muin":48151618,"mflag":0},{"muin":48767438,"mflag":0},{"muin":50022617,"mflag":0},{"muin":51861270,"mflag":8},{"muin":61738051,"mflag":0},{"muin":69598120,"mflag":0},{"muin":71817005,"mflag":0},{"muin":77572817,"mflag":0},{"muin":79116087,"mflag":0},{"muin":79963016,"mflag":0},{"muin":80856728,"mflag":4},{"muin":81834430,"mflag":0},{"muin":81981520,"mflag":0},{"muin":83679748,"mflag":0},{"muin":85253609,"mflag":0},{"muin":85405191,"mflag":0},{"muin":86166869,"mflag":0},{"muin":87018669,"mflag":12},{"muin":89238318,"mflag":0},{"muin":103129456,"mflag":0},{"muin":121959581,"mflag":4},{"muin":123698100,"mflag":0},{"muin":124782228,"mflag":4},{"muin":151461152,"mflag":8},{"muin":157220346,"mflag":0},{"muin":172492238,"mflag":0},{"muin":181466808,"mflag":4},{"muin":188921978,"mflag":0},{"muin":214939179,"mflag":0},{"muin":232111579,"mflag":0},{"muin":233596494,"mflag":0},{"muin":247198803,"mflag":0},{"muin":247349875,"mflag":0},{"muin":253404862,"mflag":0},{"muin":254424002,"mflag":0},{"muin":258041961,"mflag":0},{"muin":258251834,"mflag":8},{"muin":263711841,"mflag":0},{"muin":269325221,"mflag":5},{"muin":275531513,"mflag":0},{"muin":277862728,"mflag":20},{"muin":279240400,"mflag":0},{"muin":279856312,"mflag":0},{"muin":283581804,"mflag":0},{"muin":286519897,"mflag":0},{"muin":290124808,"mflag":0},{"muin":306334649,"mflag":0},{"muin":314641308,"mflag":4},{"muin":319753711,"mflag":8},{"muin":339002792,"mflag":0},{"muin":362321092,"mflag":0},{"muin":370367000,"mflag":0},{"muin":373705862,"mflag":8},{"muin":392498898,"mflag":0},{"muin":398313207,"mflag":0},{"muin":407125679,"mflag":0},{"muin":408001091,"mflag":0},{"muin":409233765,"mflag":0},{"muin":417061427,"mflag":5},{"muin":424393980,"mflag":0},{"muin":431533686,"mflag":5},{"muin":431533706,"mflag":4},{"muin":438384921,"mflag":0},{"muin":499359296,"mflag":0},{"muin":513139812,"mflag":0},{"muin":531190266,"mflag":0},{"muin":563202470,"mflag":0},{"muin":574167129,"mflag":4},{"muin":574733842,"mflag":4},{"muin":619188889,"mflag":0},{"muin":648179872,"mflag":0},{"muin":651514102,"mflag":0},{"muin":754102672,"mflag":4},{"muin":805197244,"mflag":4},{"muin":850971576,"mflag":12},{"muin":858883308,"mflag":0},{"muin":896738543,"mflag":4},{"muin":921174218,"mflag":0},{"muin":924866256,"mflag":0},{"muin":970526454,"mflag":4},{"muin":979939929,"mflag":0},{"muin":1009602543,"mflag":0},{"muin":1115295326,"mflag":0},{"muin":1182399370,"mflag":4},{"muin":1439254562,"mflag":0},{"muin":1447881989,"mflag":0},{"muin":1449302409,"mflag":0}]},"minfo":[{"uin":82383,"nick":"\u54B8\u9C7C"},{"uin":203376,"nick":" Youxin "},{"uin":416864,"nick":"Cip2eR"},{"uin":484197,"nick":"Bull_only"},{"uin":753633,"nick":"J7N/yhh"},{"uin":1044498,"nick":"   waffles"},{"uin":1576157,"nick":"sunwind"},{"uin":2299910,"nick":"\u957F\u5DDD"},{"uin":5978110,"nick":"\u706C\u2121oldmon\uFF2B"},{"uin":7281389,"nick":"\u60F0\u6027\u6C14\u4F53"},{"uin":8318138,"nick":"elliott"},{"uin":9124296,"nick":"  \u91CC\u514B\u722C\u722C"},{"uin":9437864,"nick":"\u3128\u8B09\u8B09o\u041E~"},{"uin":9449562,"nick":"KOO"},{"uin":11065738,"nick":"Simon"},{"uin":14061253,"nick":"\u0026\u5C0F\u0026\u96E8"},{"uin":15485581,"nick":"\u5B88\u62A4\u5929\u4F7F"},{"uin":17876934,"nick":"\u5496\u5561fomly.cn"},{"uin":17883560,"nick":".laslin\u2121"},{"uin":17951586,"nick":"fenxiang"},{"uin":18002309,"nick":"\u98CE\u8D77~"},{"uin":18214798,"nick":"\u7B80\u5355\u00B7\u7231"},{"uin":18408777,"nick":"\u9152\u9192\u65E0\u68A6"},{"uin":19424593,"nick":"\u0014\uE7E7\u77F3\u4E4B\u8F69\u0026\uE7E7"},{"uin":19609524,"nick":"\u65E0\u5FC3\u7EC6\u8BED"},{"uin":22661725,"nick":"\u571F\u62E8\u9F20"},{"uin":23849016,"nick":"\u6674\u7A7A"},{"uin":24006854,"nick":"somh"},{"uin":33228978,"nick":"\u690C\u7B11\u61DC"},{"uin":35999355,"nick":"\u5510\u6D41\u6D6A\u6C49\u6D9B"},{"uin":39928040,"nick":"\u53EF\u4E50\u767E\u4E8B"},{"uin":41194629,"nick":" \u967D\u5149\u50B3\u8AAA"},{"uin":44157129,"nick":"\u7B14\u620E"},{"uin":45435887,"nick":"\u6DF1\u6DF1\u7231\u8C01"},{"uin":48151618,"nick":"Awakening\u609F"},{"uin":48767438,"nick":"TONY"},{"uin":50022617,"nick":"\uFE36\u3123    \u701A\u309D"},{"uin":51861270,"nick":"\u4EBA\u751F\u77ED\u6682"},{"uin":61738051,"nick":"\u901D\u8005\u5929\u5D0E\u306E\u53F8"},{"uin":69598120,"nick":" \u72FC\u6717"},{"uin":71817005,"nick":"Moon Tulwar"},{"uin":77572817,"nick":"MR.\u822A"},{"uin":79116087,"nick":"\u300E\u540E\u77E5\u540E\u89C9\u300F"},{"uin":79963016,"nick":"STcoco"},{"uin":80856728,"nick":"\u58EF"},{"uin":81834430,"nick":"Meister            **   "},{"uin":81981520,"nick":"\u7C89\u5AE9\u7F94\u7F8A"},{"uin":83679748,"nick":"\u9EA6\u514B"},{"uin":85253609,"nick":"\u5343\u8349\u98DE\u68A6"},{"uin":85405191,"nick":"\u72C2\u72FC"},{"uin":86166869,"nick":"\u67D2\u5DE7\u4ED4"},{"uin":87018669,"nick":"\u0001\u6D77\u864E\u5730\u7344"},{"uin":89238318,"nick":"kgptzac"},{"uin":103129456,"nick":"Keys\u2299\u014D\u2299"},{"uin":121959581,"nick":"\u90ED\u9756"},{"uin":123698100,"nick":"/\u7948\u798FCharon"},{"uin":124782228,"nick":"\u2605\u51B7\u543B\u5251\u2605"},{"uin":151461152,"nick":"\u6807\u51C6\u867E\u7C73"},{"uin":157220346,"nick":"\u4E0B\u591C"},{"uin":172492238,"nick":"\u6615"},{"uin":181466808,"nick":" Saddhu"},{"uin":188921978,"nick":"\u25CF\u6DD8\u6C23\u5C0F\u8AA0\u25CF"},{"uin":214939179,"nick":"yaor"},{"uin":232111579,"nick":"\u2197 [\u6C89\u6CA6 "},{"uin":233596494,"nick":"amex\u2122"},{"uin":247198803,"nick":"D\u00E8ng\u25A0"},{"uin":247349875,"nick":"Strator\uFF3F]\u0014"},{"uin":253404862,"nick":"\u00ECS\u039D\u2019\u03A4"},{"uin":254424002,"nick":"\u51AC\u7720"},{"uin":258041961,"nick":"M. Klesen"},{"uin":258251834,"nick":"\u7092\u996D"},{"uin":263711841,"nick":"stanley :-)"},{"uin":269325221,"nick":"\u4E0A\u5584\u82E5\u6C34\u2026\u2026"},{"uin":275531513,"nick":"sea"},{"uin":277862728,"nick":"\u5929\u9A6C"},{"uin":279240400,"nick":"\u6563\u573A"},{"uin":279856312,"nick":"\u0026\u221A\uFE34today\u5719"},{"uin":283581804,"nick":"LUHOO"},{"uin":286519897,"nick":"\u5C0FG"},{"uin":290124808,"nick":"/fan`\u996D\u76D2."},{"uin":306334649,"nick":"\u9A6C\u8D5B\u514B007"},{"uin":314641308,"nick":"X\u56E7\u738B"},{"uin":319753711,"nick":"\u4F34\u6708?\u5B64\u5F71"},{"uin":339002792,"nick":"\u261C\u5922\u629D\u5172\u5B50\u261E"},{"uin":362321092,"nick":"\u53F6\uFF0D\u8BED"},{"uin":370367000,"nick":"Lock"},{"uin":373705862,"nick":"\u6230\u3001\u7EC8\u6210\u6BA4"},{"uin":392498898,"nick":"L\u00F9L\u00F9"},{"uin":398313207,"nick":"L."},{"uin":407125679,"nick":"\u5F20\u5C0F\u8000\u202E"},{"uin":408001091,"nick":"\u897F\u90E8\u5C45\u58EB"},{"uin":409233765,"nick":"\u6D77\u5CB8"},{"uin":417061427,"nick":"\u84DD\u8840\u87B3\u8782"},{"uin":424393980,"nick":"Joey Lu"},{"uin":431533686,"nick":"^_^"},{"uin":431533706,"nick":"^_^2"},{"uin":438384921,"nick":"chris"},{"uin":499359296,"nick":"\u5496\u5561\u3001\u6CBE\u5730\u74DC"},{"uin":513139812,"nick":"\u9038\u4E4B\u5154"},{"uin":531190266,"nick":"\u7E93\u57D6\u613A\u2605\u69AE\u5152"},{"uin":563202470,"nick":"QOO"},{"uin":574167129,"nick":"\u6B8B\u6708\u5929\u5FC3"},{"uin":574733842,"nick":"\u6728\u8033"},{"uin":619188889,"nick":"\u0001\u9EC4\u91D1\u5C0F\u53F7"},{"uin":648179872,"nick":"\u5C0FQ"},{"uin":651514102,"nick":"\u7B11\u71AC\u7CE8\u7CCA"},{"uin":754102672,"nick":"skylight"},{"uin":805197244,"nick":"Serjone"},{"uin":850971576,"nick":"[o]\u74DC\u76AE\u4ED4"},{"uin":858883308,"nick":"\u4E00\u65E5\u4ED9"},{"uin":896738543,"nick":"\u65E9\u8D77\u65E9\u7761"},{"uin":921174218,"nick":"\u9E2D\u5B50\u25A1"},{"uin":924866256,"nick":"\u7EA2\u5BA2-\u4E66\u8BB0"},{"uin":970526454,"nick":"Lee"},{"uin":979939929,"nick":"\u4E94\u5E38\u5148\u950B\u79D1\u6280"},{"uin":1009602543,"nick":"\u8304\u8304"},{"uin":1115295326,"nick":"\u5361\u519C"},{"uin":1182399370,"nick":"Serjone"},{"uin":1439254562,"nick":"lilarcor"},{"uin":1447881989,"nick":"\u6563\u573A"},{"uin":1449302409,"nick":"sunwind"}],"stats":[{"uin":82383,"stat":10},{"uin":203376,"stat":20},{"uin":416864,"stat":20},{"uin":484197,"stat":20},{"uin":753633,"stat":20},{"uin":1044498,"stat":20},{"uin":1576157,"stat":20},{"uin":2299910,"stat":20},{"uin":5978110,"stat":20},{"uin":7281389,"stat":20},{"uin":8318138,"stat":20},{"uin":9124296,"stat":20},{"uin":9437864,"stat":20},{"uin":9449562,"stat":20},{"uin":11065738,"stat":20},{"uin":14061253,"stat":20},{"uin":15485581,"stat":20},{"uin":17876934,"stat":20},{"uin":17883560,"stat":20},{"uin":17951586,"stat":10},{"uin":18002309,"stat":20},{"uin":18214798,"stat":20},{"uin":18408777,"stat":20},{"uin":19424593,"stat":20},{"uin":19609524,"stat":20},{"uin":22661725,"stat":20},{"uin":23849016,"stat":20},{"uin":24006854,"stat":20},{"uin":33228978,"stat":20},{"uin":35999355,"stat":20},{"uin":39928040,"stat":20},{"uin":41194629,"stat":20},{"uin":44157129,"stat":20},{"uin":45435887,"stat":20},{"uin":48151618,"stat":20},{"uin":48767438,"stat":20},{"uin":50022617,"stat":20},{"uin":51861270,"stat":20},{"uin":61738051,"stat":30},{"uin":69598120,"stat":20},{"uin":71817005,"stat":20},{"uin":77572817,"stat":20},{"uin":79116087,"stat":20},{"uin":79963016,"stat":20},{"uin":80856728,"stat":20},{"uin":81834430,"stat":10},{"uin":81981520,"stat":20},{"uin":83679748,"stat":20},{"uin":85253609,"stat":20},{"uin":85405191,"stat":20},{"uin":86166869,"stat":20},{"uin":87018669,"stat":20},{"uin":89238318,"stat":10},{"uin":103129456,"stat":20},{"uin":121959581,"stat":20},{"uin":123698100,"stat":20},{"uin":124782228,"stat":20},{"uin":151461152,"stat":20},{"uin":157220346,"stat":20},{"uin":172492238,"stat":20},{"uin":181466808,"stat":20},{"uin":188921978,"stat":20},{"uin":214939179,"stat":20},{"uin":232111579,"stat":20},{"uin":233596494,"stat":20},{"uin":247198803,"stat":20},{"uin":247349875,"stat":20},{"uin":253404862,"stat":20},{"uin":254424002,"stat":20},{"uin":258041961,"stat":20},{"uin":258251834,"stat":20},{"uin":263711841,"stat":20},{"uin":269325221,"stat":20},{"uin":275531513,"stat":10},{"uin":277862728,"stat":20},{"uin":279240400,"stat":20},{"uin":279856312,"stat":10},{"uin":283581804,"stat":20},{"uin":286519897,"stat":20},{"uin":290124808,"stat":20},{"uin":306334649,"stat":20},{"uin":314641308,"stat":20},{"uin":319753711,"stat":20},{"uin":339002792,"stat":20},{"uin":362321092,"stat":20},{"uin":370367000,"stat":20},{"uin":373705862,"stat":20},{"uin":392498898,"stat":20},{"uin":398313207,"stat":20},{"uin":407125679,"stat":20},{"uin":408001091,"stat":20},{"uin":409233765,"stat":20},{"uin":417061427,"stat":10},{"uin":424393980,"stat":20},{"uin":431533686,"stat":10},{"uin":431533706,"stat":10},{"uin":438384921,"stat":20},{"uin":499359296,"stat":20},{"uin":513139812,"stat":10},{"uin":531190266,"stat":20},{"uin":563202470,"stat":20},{"uin":574167129,"stat":20},{"uin":574733842,"stat":20},{"uin":619188889,"stat":20},{"uin":648179872,"stat":20},{"uin":651514102,"stat":20},{"uin":754102672,"stat":20},{"uin":805197244,"stat":30},{"uin":850971576,"stat":20},{"uin":858883308,"stat":20},{"uin":896738543,"stat":20},{"uin":921174218,"stat":20},{"uin":924866256,"stat":20},{"uin":970526454,"stat":50},{"uin":979939929,"stat":20},{"uin":1009602543,"stat":20},{"uin":1115295326,"stat":20},{"uin":1182399370,"stat":30},{"uin":1439254562,"stat":20},{"uin":1447881989,"stat":20},{"uin":1449302409,"stat":20}],"cards":[{"muin":61738051,"card":"\u5929\u5D0E\u306E\u53F8"},{"muin":22661725,"card":"\u571F\u62E8\u9F20/cat"},{"muin":277862728,"card":"\u5929\u9A6C"},{"muin":431533706,"card":"^_^2"},{"muin":87018669,"card":"\u0001\u0001\u4E5D\u5DE7\u4ED4/dk/ll"},{"muin":753633,"card":"\u90A3\u4E2AJ7N"},{"muin":151461152,"card":"\u6807\u51C6\u867E\u7C73"},{"muin":319753711,"card":"\u4F34\u6708"},{"muin":86166869,"card":"\u4E03\u5DE7\u4ED4"},{"muin":424393980,"card":"Joey"},{"muin":9449562,"card":"KOO"},{"muin":1576157,"card":"sunwind"},{"muin":279856312,"card":"today"},{"muin":39928040,"card":"\u53EF\u4E50"},{"muin":17883560,"card":".\u96E8\u98CE"},{"muin":45435887,"card":"MAX"},{"muin":850971576,"card":"[o]\u74DC\u76AE\u4ED4"},{"muin":51861270,"card":"_VB.NET"},{"muin":921174218,"card":"\u5361\u5E03\u5947\u8BFA"},{"muin":5978110,"card":"\u2570\u2606oldmon\uFF2B\u2121"},{"muin":253404862,"card":"Ennie"},[],{"muin":258251834,"card":"\u4ED6\u7239\u2026\u2026"},{"muin":373705862,"card":"\u559C\u6B22 \u968FE\u804A"},{"muin":1044498,"card":"sqn"},{"muin":203376,"card":"xinq"},{"muin":83679748,"card":"\u5BFB\u4EBA\u5F00\u53D1Miranda"}]}}
		return web2_check_result("/api/get_group_info_ext",pszTemp);
	}
	return false;
}

bool CLibWebQQ::web2_api_get_qq_level(unsigned int tuin) {
	LPSTR pszTemp;
	DWORD dwSize;
	char szQuery[MAX_PATH];

	sprintf(szQuery,"http://%s/api/get_qq_level2?tuin=%u&vfwebqq=%s&t=%u%03d",g_server_web2_connection,tuin,m_web2_vfwebqq,(DWORD)time(NULL),GetTickCount()%1000);

	if ((pszTemp=GetHTMLDocument(szQuery, g_referer_web2,&dwSize))!=NULL && dwSize!=0xffffffff) {
		// {"retcode":0,"result":{"tuin":431533686,"hours":30279,"days":2063,"level":43,"remainDays":49}}
		return web2_check_result("/api/get_qq_level",pszTemp);
	}
	return false;
}

bool CLibWebQQ::web2_api_add_need_verify(unsigned int tuin, int groupid, LPSTR msg, LPSTR token, JSONNODE** jnOutput) {
	// {"tuin":431533686,"myallow":1,"groupid":0,"msg":"test","vfwebqq":"a4fd1bb134d9525b235f5471b99a518cded76e63d086fed73f81559e60e8bb7328ce1be068e2a1cb"}
	// Need to escape text, so use JSONNODE
	char szQuery[512]="r=";
	LPSTR pszTemp;
	DWORD dwSize;

	JSONNODE* jn=json_new(JSON_NODE);
	json_push_back(jn,json_new_f("tuin",tuin));
	json_push_back(jn,json_new_i("myallow",1));
	json_push_back(jn,json_new_i("groupid",groupid));
	json_push_back(jn,json_new_a("msg",msg));
	json_push_back(jn,json_new_a("token",token));
	json_push_back(jn,json_new_a("vfwebqq",m_web2_vfwebqq));

	strcpy(szQuery+2,pszTemp=json_write(jn));
	json_free(pszTemp);

	if ((pszTemp=PostHTMLDocument(g_server_web2_connection,"/api/add_need_verify2",GetReferer(WEBQQ_REFERER_WEB2PROXY),szQuery,&dwSize))!=NULL && dwSize!=0xffffffff) {
		// {"retcode":0,"result":{"result":0}}
		if (jnOutput) {
			*jnOutput=json_parse(pszTemp);
			LocalFree(pszTemp);
			return json_as_int(json_get(*jnOutput,"retcode"))==0;
		} else {
			return web2_check_result("/api/add_need_verify",pszTemp);
		}

	}

	return false;
}

bool CLibWebQQ::web2_api_deny_add_request(unsigned int tuin, LPSTR msg, JSONNODE** jnOutput) {
	// {"tuin":431533686,"msg":"try again","vfwebqq":"57860632ea1cce244dfd88db31e5786b8c58ffbdbede24eeb581ab9193604c810ab2ec27fbef6d3e"}
	// Need to escape text, so use JSONNODE
	char szQuery[MAX_PATH]="r=";
	LPSTR pszTemp;
	DWORD dwSize;

	JSONNODE* jn=json_new(JSON_NODE);
	json_push_back(jn,json_new_f("tuin",tuin));
	json_push_back(jn,json_new_a("msg",msg));
	json_push_back(jn,json_new_a("vfwebqq",m_web2_vfwebqq));

	strcpy(szQuery+2,pszTemp=json_write(jn));
	json_free(pszTemp);

	if ((pszTemp=PostHTMLDocument(g_server_web2_connection,"/api/deny_add_request2",GetReferer(WEBQQ_REFERER_WEB2PROXY),szQuery,&dwSize))!=NULL && dwSize!=0xffffffff) {
		// {"retcode":0,"result":{"result":0}}
		if (jnOutput) {
			*jnOutput=json_parse(pszTemp);
			LocalFree(pszTemp);
			return json_as_int(json_get(*jnOutput,"retcode"))==0;
		} else {
			return web2_check_result("/api/deny_add_request",pszTemp);
		}

	}

	return false;
}

bool CLibWebQQ::web2_api_allow_and_add(unsigned int tuin, int gid, LPSTR mname, JSONNODE** jnOutput) {
	// {"tuin":431533686,"gid":0,"mname":"","vfwebqq":"57860632ea1cce244dfd88db31e5786b8c58ffbdbede24eeb581ab9193604c810ab2ec27fbef6d3e"}
	// Need to escape text, so use JSONNODE
	char szQuery[MAX_PATH]="r=";
	LPSTR pszTemp;
	DWORD dwSize;

	JSONNODE* jn=json_new(JSON_NODE);
	json_push_back(jn,json_new_f("tuin",tuin));
	json_push_back(jn,json_new_i("gid",gid));
	json_push_back(jn,json_new_a("mname",mname));
	json_push_back(jn,json_new_a("vfwebqq",m_web2_vfwebqq));

	strcpy(szQuery+2,pszTemp=json_write(jn));
	json_free(pszTemp);

	if ((pszTemp=PostHTMLDocument(g_server_web2_connection,"/api/allow_and_add2",GetReferer(WEBQQ_REFERER_WEB2PROXY),szQuery,&dwSize))!=NULL && dwSize!=0xffffffff) {
		// {"retcode":0,"result":{"result1":0,"tuin":3857862050,"stat":10}}
		if (jnOutput) {
			*jnOutput=json_parse(pszTemp);
			LocalFree(pszTemp);
			return json_as_int(json_get(*jnOutput,"retcode"))==0;
		} else {
			return web2_check_result("/api/allow_and_add",pszTemp);
		}

	}

	return false;
}

bool CLibWebQQ::web2_api_search_qq_by_nick(LPCSTR nick, JSONNODE** jnOutput) {
	// http://web2-b.qq.com/api/search_qq_by_nick?nick=miranda&page=0&t=12885865	GET	proxy

	LPSTR pszTemp;
	DWORD dwSize;
	char szURL[MAX_PATH];
	char szNick[MAX_PATH];
	LPCSTR pszNick=nick;
	LPSTR pszNick2=szNick;

	while(true) {
		if (*pszNick<0) {
			pszNick2+=sprintf(pszNick2,"%%%02X",*(unsigned char*)pszNick);
		} else
			*pszNick2=*pszNick;

		if (!*pszNick) 
			break;
		else {
			*pszNick++;
		}
	}


	sprintf(szURL,"http://%s/api/search_qq_by_nick2?nick=%s&page=0&t=%u",g_server_web2_connection,szNick,time(NULL));


	if ((pszTemp=GetHTMLDocument(szURL, g_referer_web2proxy,&dwSize))!=NULL && dwSize!=0xffffffff) {
		// {"retcode":0,"result":{"endflag":0,"page":0,"count":25,"uinlist":[{"uin":22619484,"sex":2,"stat":10,"age":23,"nick":"miranda","country":"\u4E2D\u56FD","province":"\u6E56\u5317","city":"\u8346\u5DDE","face":0},{"uin":41929705,"sex":2,"stat":20,"age":33,"nick":"miranda","country":"\u4E2D\u56FD","province":"\u4E0A\u6D77","city":"\u6D66\u4E1C\u65B0","face":342},{"uin":707202102,"sex":2,"stat":20,"age":119,"nick":"miranda","country":"\u4E2D\u56FD","province":"\u4E0A\u6D77","city":"\u5B9D\u5C71","face":531},{"uin":1296329393,"sex":2,"stat":20,"age":0,"nick":"miranda","country":"\u4E2D\u56FD","province":"\u4E0A\u6D77","city":"\u6D66\u4E1C\u65B0","face":525},{"uin":292001177,"sex":2,"stat":20,"age":0,"nick":"miranda","country":"\u4E2D\u56FD","province":"\u5C71\u4E1C","city":"\u5FB7\u5DDE","face":0},{"uin":924509919,"sex":2,"stat":20,"age":50,"nick":"miranda","country":"\u4E2D\u56FD","province":"\u5C71\u4E1C","city":"\u9752\u5C9B","face":273},{"uin":27962021,"sex":2,"stat":20,"age":31,"nick":"miranda","country":"\u4E2D\u56FD","province":"\u897F\u85CF","city":"\u65E5\u5580\u5219","face":144},{"uin":24806543,"sex":2,"stat":20,"age":0,"nick":"miranda","country":"\u4E2D\u56FD","province":"0","city":"0","face":171},{"uin":1620965368,"sex":2,"stat":20,"age":0,"nick":"miranda","country":"\u4E2D\u56FD","province":"\u5317\u4EAC","city":"\u897F\u57CE","face":564},{"uin":1275410214,"sex":2,"stat":20,"age":20,"nick":"miranda","country":"\u4E2D\u56FD","province":"\u4E91\u5357","city":"\u6606\u660E","face":486},{"uin":908374675,"sex":1,"stat":20,"age":59,"nick":"miranda","country":"\u4E2D\u56FD","province":"\u4E91\u5357","city":"\u7EA2\u6CB3","face":528},{"uin":1309932818,"sex":2,"stat":20,"age":27,"nick":"miranda","country":"\u4E2D\u56FD","province":"\u5E7F\u4E1C","city":"\u6DF1\u5733","face":546},{"uin":1160803548,"sex":2,"stat":20,"age":32,"nick":"miranda","country":"\u4E2D\u56FD","province":"\u5E7F\u4E1C","city":"\u6C55\u5934","face":15},{"uin":27898299,"sex":2,"stat":20,"age":25,"nick":"miranda","country":"\u4E2D\u56FD","province":"0","city":"0","face":288},{"uin":550263521,"sex":2,"stat":20,"age":21,"nick":"miranda","country":"\u4E2D\u56FD","province":"0","city":"0","face":528},{"uin":23866597,"sex":2,"stat":20,"age":33,"nick":"miranda","country":"\u4E2D\u56FD","province":"\u6C5F\u82CF","city":"\u82CF\u5DDE","face":231},{"uin":450392124,"sex":2,"stat":20,"age":10,"nick":"miranda","country":"\u4E2D\u56FD","province":"\u91CD\u5E86","city":"\u6C5F\u5317","face":543},{"uin":89130422,"sex":2,"stat":20,"age":23,"nick":"miranda","country":"\u4E2D\u56FD","province":"\u6C5F\u897F","city":"\u5357\u660C","face":36},{"uin":1357284932,"sex":2,"stat":20,"age":0,"nick":"miranda","country":"\u4E2D\u56FD","province":"\u6C5F\u897F","city":"\u5357\u660C","face":564},{"uin":1524745801,"sex":2,"stat":20,"age":22,"nick":"miranda","country":"\u4E2D\u56FD","province":"0","city":"0","face":525},{"uin":984105799,"sex":2,"stat":20,"age":25,"nick":"miranda","country":"\u4E2D\u56FD","province":"\u6CB3\u5317","city":"\u79E6\u7687\u5C9B","face":0},{"uin":290002989,"sex":2,"stat":20,"age":120,"nick":"miranda","country":"\u4E2D\u56FD","province":"0","city":"0","face":0},{"uin":290853304,"sex":2,"stat":20,"age":24,"nick":"miranda","country":"\u4E2D\u56FD","province":"0","city":"0","face":339},{"uin":1528129151,"sex":2,"stat":20,"age":16,"nick":"miranda","country":"\u4E2D\u56FD","province":"0","city":"0","face":534},{"uin":26039188,"sex":2,"stat":20,"age":40,"nick":"miranda","country":"\u4E2D\u56FD","province":"0","city":"0","face":48}]}}
		if (jnOutput) {
			*jnOutput=json_parse(pszTemp);
			LocalFree(pszTemp);
			return json_as_int(json_get(*jnOutput,"retcode"))==0;
		} else {
			return web2_check_result("/api/search_qq_by_nick",pszTemp);
		}
	}
	return false;

}

JSONNODE* CLibWebQQ::qun_air_search(unsigned int groupid) {
	char szTemp[16];
	ultoa(groupid,szTemp,10);

	return qun_air_search(szTemp);
}

JSONNODE* CLibWebQQ::qun_air_search(LPCSTR groupname) {
	// http://cgi.qun.qq.com/?w=a&c=index&a=sresult&cl=&cnum=0&tx=2571213&pg=1&st=0&c1=0&c2=0&c3=0&ecd=0&jsonp=jsonp1288148180128&_=1288148188625
	//char szURL[MAX_PATH];
	char szGroupname[MAX_PATH];
	LPCSTR pszGroupname=groupname;
	LPSTR pszGroupname2=szGroupname;
	char szURL[MAX_PATH];
	DWORD dwSize=MAX_PATH;
	JSONNODE* jn=NULL;

	while(true) {
		if (*pszGroupname<0) {
			pszGroupname2+=sprintf(pszGroupname2,"%%%02X",*(unsigned char*)pszGroupname);
		} else
			*pszGroupname2=*pszGroupname;

		if (!*pszGroupname) 
			break;
		else {
			*pszGroupname++;
		}
	}

	sprintf(szURL,"http://cgi.qun.qq.com/?w=a&c=index&a=sresult&cl=&cnum=0&tx=%s&pg=1&st=0&c1=0&c2=0&c3=0&ecd=0&jsonp=jsonp%u%03d&_=%u%03d",szGroupname,(DWORD)time(NULL),GetTickCount()%1000,(DWORD)time(NULL),(GetTickCount()+8192)%1000);

	LPSTR pszData=GetHTMLDocument(szURL,"http://qun.qq.com/air/search",&dwSize);
	if (pszData && *pszData && dwSize!=0xffffffff && dwSize>0) {
		/*
		jsonp1288148180128({"responseHeader": {"Status":"0","CostTime":"2826","TotalNum":"1","CurrentNum":"1","CurrentPage":"1"},"results": [{"MD":"0","TI":"Miranda IM7)op讨论群","QQ":"753633","RQ":"1119325824","CL":"12:187;","DT":"1285005781","UR":"http://qun.qq.com/air/#2571213","TA":"","GA":"10","GB":"4","GC":"125","GD":"0","GE":"2571213","GF":"0","BU":"http://qun.qq.com/air/#2571213","TX":"本群不解答其它任何打包MIM问题 加入须有软件使用基础及DIY精神 还有本群长期成员推荐","HA":"2","HB":"0","HC":"0","HD":"0","HE":"0","HF":"0","PA":"","PB":"","PC":"","PD":""}],"QcResult": [],"HintResult": [],"SmartResult": []}
		)
		*/
		if (LPSTR ppszData=strchr(pszData,'{')) {
			strrchr(ppszData,'}')[1]=0;
			jn=json_parse(ppszData);
		} else {
			Log(__FUNCTION__"(): Bad Data");
		}
	} else {
		Log(__FUNCTION__"(): Search Failed");
	}

	if (pszData) LocalFree(pszData);
	return jn;
}

JSONNODE* CLibWebQQ::qun_air_join(unsigned int groupid, LPCSTR reason) {
	// http://qun.qq.com/air/?w=a&c=index&a=join	POST	Referer=http://qun.qq.com/air/search
/*
X-Requested-With:XMLHttpRequest
groupid:2571213
operation:join
reason:jjjj
verifycode:4444
*/
	char szTemp[MAX_PATH];
	char szVeryCode[5];

	sprintf(szTemp,"http://ptlogin2.qq.com/getimage?aid=3000801&_=%u",(double)rand()/(double)RAND_MAX);

	SendToHub(WEBQQ_CALLBACK_NEEDVERIFY,szTemp,szVeryCode);
	if (*szVeryCode) {
		LPSTR pszData;
		DWORD dwSize;

		strlwr(szVeryCode);
		sprintf(szTemp,"groupid=%u&operation=join&reason=%s&verifycode=%s",groupid, reason, szVeryCode);

		if ((pszData=PostHTMLDocument("qun.qq.com","/air/?w=a&c=index&a=join","http://qun.qq.com/air/search",szTemp,&dwSize))!=NULL && *pszData=='{') {
			// {"r":{"domain":"qun.qq.com","server":"149.19","client":"112.120.135.211","elapsed":"0.0053","memory":"0.33MB","profile":"T_LOAD: 0.0008S|T_ROUTE: 0.0002S|T_DISPATCH: 0.0034S|","module":"default","controller":"index","action":"join","env":"live","way":"async","language":"zh-cn","user":{"id":85379868,"nick":"j85379868","gkey":"v0H+jYmLPTndIDki7VqITlmDrIJDXB6nEqjTYj5XT5Vf+3SDi54CSqAJvXP2inGIBn5Z6EAN1rM=","skey":"@rxMxViqNl","skeyt":1,"age":22,"gender":1,"face":252,"logintime":1288147097,"lastaccess":1288148306,"passport":"\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000","mail":""},"group":{"id":"0","auth":0,"permission":2,"type":1},"params":null},"c":{"code":"-8"}}
			// -8=wrong verycode
			Log(__FUNCTION__"(): ASSERT=%s",pszData);
			JSONNODE* jn=json_parse(pszData);
			LocalFree(pszData);
			return jn;
		} else
			Log(__FUNCTION__"(): Invalid reply: %s",pszData);

		if (pszData) LocalFree(pszData);
	}

	return NULL;
}

bool CLibWebQQ::ParseResponse4b(JSONNODE* jnResult) {
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
void CLibWebQQ::Test() {
	DWORD dwSize;
	m_buffer=(LPSTR)LocalAlloc(LMEM_FIXED,65536);
	m_outbuffer=m_outbuffer_current=(LPSTR)LocalAlloc(LMEM_FIXED,65536);
	/*
	m_storage[WEBQQ_STORAGE_PARAMS]=(LPSTR)LocalAlloc(LMEM_FIXED,MAX_PATH);
	memcpy(m_storage[WEBQQ_STORAGE_PARAMS],"431533706\0""22\0""0\0""00000000\0""@1YfpjmmiF\0""3136bf94112c237fc88bea3e686e7d93fc64491440e2a5823dd0247cf1c02de8\0""0\0",MAX_PATH);
	strcpy(m_buffer,"431533706;17;7856;431533686;346151;09;0b;\x15""3C53FD9F3832C650498BBAC55CCBE7DBA.gif\x1f;0800000010Tahoma;1280051157;1866368475;");
	ParseResponse();
	*/
	m_hInet=InternetOpenA("Mozilla/5.0 (Windows; U; Windows NT 5.1; en-US; rv:1.9.1.3) Gecko/20090824 Firefox/3.5.3 GTB5",m_proxyhost?INTERNET_OPEN_TYPE_PROXY:INTERNET_OPEN_TYPE_DIRECT,m_proxyhost,NULL,0);

	sprintf(m_outbuffer,"r={\"status\":\"%s\",\"ptwebqq\":\"%s\",\"passwd_sig\":\"%s\",\"clientid\":\"%s\"}",m_loginhide?"hidden":"online","ABCD1234",""/*DNA Not supported*/,m_web2_clientid);
	m_web2_logininfo=NULL;

	LPSTR pszTemp=PostHTMLDocument("192.168.238.239","/mimqq4b_test.php",GetReferer(WEBQQ_REFERER_WEB2PROXY),m_outbuffer,&dwSize);
	MessageBoxA(NULL,pszTemp,NULL,0);

}
