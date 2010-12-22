#include "StdAfx.h"
#include "../libJSON.h"
#pragma comment(lib,"libJSON")
#define PTLOGIN "http://ui.ptlogin2.qq.com/cgi-bin/login?style=4&appid=1003903&enable_qlogin=0&no_verifyimg=1&s_url=http://webqq.qq.com/main.shtml?direct__2&f_url=loginerroralert"

class CCodeVerifyWindow {
public:
	CCodeVerifyWindow::CCodeVerifyWindow(HINSTANCE hInstance, LPWSTR pszText, LPSTR pszImagePath):
	m_text(pszText), m_imagepath(pszImagePath), m_bitmap(NULL) {
		DialogBoxParam(hInstance,MAKEINTRESOURCE(IDD_CODEVERIFY),NULL,_DialogProc,(LPARAM)this);
	}

	CCodeVerifyWindow::CCodeVerifyWindow(HINSTANCE hInstance, LPWSTR pszText, LPSTR pszURL, CQunInfoExt* webqq):
	m_text(pszText), m_imagepath(pszURL), m_bitmap(NULL), m_webqq(webqq) {
		DialogBoxParam(hInstance,MAKEINTRESOURCE(IDD_CODEVERIFY),NULL,_DialogProc,(LPARAM)this);
	}
	virtual ~CCodeVerifyWindow() {};
	LPCSTR GetCode() const { return m_code; }

protected:
	INT_PTR CCodeVerifyWindow::DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
		switch (uMsg) {
			case WM_INITDIALOG: 
				{
					// if (m_text) SetDlgItemText(hwndDlg,IDC_TITLE,m_text);
					*m_code=0;
					PostMessage(hwndDlg,WM_COMMAND,MAKEWPARAM(IDC_CODEIMAGE,0),0);
				}
				break;
			case WM_DESTROY:
				DeleteObject(m_bitmap);
				break;
			case WM_COMMAND:
				switch (LOWORD(wParam)) {
					case IDC_CODEIMAGE:
						{
							LPCSTR pszBasePath=m_webqq->GetBasePath();
							char szPath[MAX_PATH]={0};
							LPSTR pszData;
							DWORD dwSize;

							if (pszBasePath) {
								strcat(strcpy(szPath,pszBasePath),"\\");
							}
							sprintf(szPath+strlen(szPath),"verycode-%u.jpg",m_webqq->GetUIN());

							if (!(pszData=m_webqq->GetHTMLDocument(m_imagepath,PTLOGIN,&dwSize))) {
								MessageBox(hwndDlg,TranslateT("Failed retrieving verification code,"),NULL,MB_ICONERROR);
								return false;
							}

							HANDLE hFile=CreateFileA(szPath,GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,CREATE_ALWAYS,0,NULL);
							if (hFile==INVALID_HANDLE_VALUE) {
								LocalFree(pszData);
								MessageBox(hwndDlg,TranslateT("Failed saving verification code,"),NULL,MB_ICONERROR);
							} else {
								DWORD dwWritten;
								WriteFile(hFile,pszData,dwSize,&dwWritten,NULL);
								CloseHandle(hFile);
								LocalFree(pszData);

								m_bitmap=(HBITMAP)CallService(MS_UTILS_LOADBITMAP,0,(LPARAM)szPath);
								if (HBITMAP hBmpOld=(HBITMAP)SendDlgItemMessage(hwndDlg,IDC_CODEIMAGE,STM_SETIMAGE,(WPARAM)IMAGE_BITMAP,(LPARAM)m_bitmap)) {
									DeleteObject(hBmpOld);
								}
							}
						}
						break;
					case IDOK:
						GetDlgItemTextA(hwndDlg,IDC_CODE,m_code,16);
					case IDCANCEL:
						EndDialog(hwndDlg,LOWORD(wParam));
					}
				break;
		}
		return FALSE;
	}

private:
	CHAR m_code[16];
	LPWSTR m_text;
	LPSTR m_imagepath;
	HBITMAP m_bitmap;
	CQunInfoExt* m_webqq;

	static INT_PTR CALLBACK CCodeVerifyWindow::_DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
		if (uMsg==WM_INITDIALOG) {
			SetWindowLong(hwndDlg,GWL_USERDATA,(LONG)lParam);
		}
		if (uMsg==WM_DESTROY)
			return FALSE;
		else
			return ((CCodeVerifyWindow*)GetWindowLong(hwndDlg,GWL_USERDATA))->DialogProc(hwndDlg, uMsg, wParam, lParam);
	}
};



CQunInfoExt* CQunInfoExt::m_inst=NULL;

bool CQunInfoExt::Login(CNetwork *network, DWORD dwUIN, LPCSTR pszPassword, BOOL fAutoMode) {
	if (m_inst==NULL) {
		m_inst=new CQunInfoExt(network,dwUIN,pszPassword,fAutoMode);
		return true;
	} else {
		util_log(0,"[CQunInfoExt] Login: Already registered! Ignore login request!");
		return false;
	}
}

void CQunInfoExt::Logout(DWORD dwUIN) {
	if (m_inst) {
		if (dwUIN==m_inst->GetUIN()) {
			delete m_inst;
		} else
			util_log(0,"[CQunInfoExt] Logout: Ignored logout request for incorrect UIN");
	} else {
		util_log(0,"[CQunInfoExt] Logout: No registered client");
	}
}

CQunInfoExt::CQunInfoExt(CNetwork *network, DWORD dwUIN, LPCSTR pszPassword, BOOL fAutoMode):
m_status(0), m_network(network), m_uin(dwUIN), m_password(mir_strdup(pszPassword)), m_event(NULL), m_auto(fAutoMode) {
	DWORD dwThreadID;

	if (m_hInet=InternetOpenA("Mozilla/5.0 (Windows; U; Windows NT 5.1; en-US; rv:1.9.1.3) Gecko/20090824 Firefox/3.5.3 GTB5",INTERNET_OPEN_TYPE_PRECONFIG,NULL,NULL,0)) {
		m_event=CreateEvent(NULL,TRUE,FALSE,NULL);
	}

	FoldersGetCustomPath(network->m_avatarFolder,m_basepath,MAX_PATH,"QQ");
	// AddJob(dwUIN,0,0,0);

	CreateThread(NULL,0,_ThreadProc,this,0,&dwThreadID);
}

CQunInfoExt::~CQunInfoExt() {
	CloseHandle(m_event);
	InternetCloseHandle(m_hInet);
	m_inst=NULL;

	if (m_password) mir_free(m_password);

	util_log(0,"[CQunInfoExt] Destroying client");
}

DWORD WINAPI CQunInfoExt::_ThreadProc(LPVOID lpParameter) {
	return ((CQunInfoExt*)lpParameter)->ThreadProc();
}

DWORD CQunInfoExt::ThreadProc() {
	if (!m_event) {
		delete this;
		return 0;
	}

	char szTemp[MAX_PATH];
	char szCode[16];
	LPSTR pszData;
	DWORD dwSize;
	bool canstart=false;
	util_log(0,"[CQunInfoExt] ThreadProc: Start initialization");

	srand(GetTickCount());
	// sprintf(szTemp,"http://ptlogin2.qq.com/check?uin=%u&appid=1002101&r=%f",m_qqid,(double)rand()/(double)RAND_MAX);
	sprintf(szTemp,"http://ptlogin2.qq.com/check?uin=%u&appid=3000801&r=%f",m_uin,(double)rand()/(double)RAND_MAX);

	util_log(0,"[CQunInfoExt] ThreadProc: 1-Check");

	if (!(pszData=GetHTMLDocument(szTemp,PTLOGIN,&dwSize))) {
		util_log(0,"[CQunInfoExt] ThreadProc: Download failed in [check]");
		delete this;
		return 0;
	}

	if (strstr(pszData,"ptui_checkVC('0'")) {
		LPSTR pt1=strstr(pszData,"','")+3;
		*strchr(pt1,'\'')=0;
		strcpy(szCode,pt1);
		LocalFree(pszData);
		canstart=true;
	} else if (strstr(pszData,"ptui_checkVC('1'")) {
		util_log(0,"[CQunInfoExt] ThreadProc: VeryCode required!");

		if (m_auto) {
			m_network->ShowNotification(TranslateT("Note: Unable to update extended Qun information. You will need to perform manual update from main menu."),NIIF_INFO);
		} else {
			// ptui_checkVC('1','b422d0878a7017b6ed554ee6fe8d166a52b500e9e7b5e250');
			char szPath[MAX_PATH]={0};
			// DWORD dwWritten;
			LPSTR pszVC_TYPE=strstr(pszData,"','")+3;

			*strchr(pszVC_TYPE,'\'')=0;

/*
GET /getimage?aid=1002101&r=0.754857305282543 HTTP/1.1
Host: ptlogin2.qq.com
Referer: http://ui.ptlogin2.qq.com/cgi-bin/login?style=4&appid=1002101&enable_qlogin=0&no_verifyimg=1&s_url=http://webqq.qq.com/main.shtml?direct__2&f_url=loginerroralert
Cookie: pgv_pvid=1527858512; pgv_flv=9.0 r100; pgv_info=ssid=s7498192482; pgv_r_cookie=106981946296; ptvfsession=38b11f2837efd6e0a3cead6ab1dbaea1315345b6668ab9e9d7bf7116f139652d4a8e28c44636f8bee26bfaa1dbcf9002
*/
// http://captcha.qq.com/getimage?aid=1002101&r=0.8544500968419015&uin=431533706&vc_type=b422d0878a7017b6ed554ee6fe8d166a52b500e9e7b5e250

			srand(GetTickCount());
			// sprintf(m_buffer,"http://ptlogin2.qq.com/getimage?aid=%s&r=%f",m_appid,(double)rand()/(double)RAND_MAX);
			sprintf(szTemp,"http://captcha.qq.com/getimage?aid=3000801&r=%f&uin=%u&vc_type=%s",(double)rand()/(double)RAND_MAX,m_uin,pszVC_TYPE);
			LocalFree(pszData); // Free here because pszVC_TYPE

			CCodeVerifyWindow cvw(hinstance,L"1",szTemp,this);
			strcpy(szCode,cvw.GetCode());

			if (*szCode) {
				canstart=true;
			} else {
				util_log(0,"[CQunInfoExt] ThreadProc: VeryCode cancelled!");
			}
		}
	}

	if (canstart) {
		util_log(0,"[CQunInfoExt] ThreadProc: 2-Login");
		if (Login(szCode)) {
			QUNINFOOP qio;

			mir_free(m_password);
			m_password=NULL;

			util_log(0,"[CQunInfoExt] ThreadProc: 3-Ready");
			while (WaitForSingleObject(m_event,INFINITE)==WAIT_OBJECT_0) {
				util_log(0,"[CQunInfoExt] ThreadProc: 4-Job");
				qio=m_queue.front();
				m_queue.pop();

				switch (qio.op) {
					case 0: EXT_DownloadGroupInfo(); break;
					case 1: EXT_DownloadCardInfo(qio.id,qio.ver); break;
				}

				if (m_queue.size()==0) ResetEvent(m_event);
			}
		}
	}

	util_log(0,"[CQunInfoExt] ThreadProc: End loop");
	if (m_inst) delete this;
	return 0;
}

bool CQunInfoExt::Login(LPSTR pszCode) {
	char szHash[33];
	char m_buffer[1024];
	DWORD dwSize;

	GetPasswordHash(pszCode,szHash);

	// GET /login?u=431533706&p=E9765C268D7E93343A681C646C37E5C5&verifycode=RMZQ&remember_uin=1&aid=1002101&u1=http%3A%2F%2Fweb2.qq.com%2Floginproxy.html%3Fstrong%3Dtrue&h=1&ptredirect=0&ptlang=2052&from_ui=1&pttype=1&dumy=&fp=loginerroralert
	sprintf(m_buffer,"http://ptlogin2.qq.com/login?u=%u&p=%s&verifycode=%s&remember_uin=1&aid=3000801&u1=%s&h=1&ptredirect=1%s&ptlang=2052&from_ui=1&pttype=1&dumy=&fp=loginerroralert",m_uin,szHash,pszCode,"http%3A%2F%2Fweb2.qq.com%2Floginproxy.html%3Frun%3Deqq%26strong%3Dtrue","");
	if (LPSTR pszData=GetHTMLDocument(m_buffer,"http://ui.ptlogin2.qq.com/cgi-bin/login?style=4&appid=3000801&enable_qlogin=0&no_verifyimg=1&s_url=http://webqq.qq.com/main.shtml?direct__2&f_url=loginerroralert",&dwSize)) {
		if (!strstr(pszData,"ptuiCB('0','0','")) {
			util_log(0,"[CQunInfoExt] Login: %s",pszData);
			LocalFree(pszData);
			return false;
		}
		LocalFree(pszData);
	} else {
		util_log(0,"[CQunInfoExt] Login: Download failed in [login]");
		return false;
	}
	return true;
}

void CQunInfoExt::GetPasswordHash(LPCSTR pszVerifyCode, LPSTR pszOut) {
	char szTemp[40];
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

LPSTR CQunInfoExt::GetHTMLDocument(LPCSTR pszUrl, LPCSTR pszReferer, LPDWORD pdwLength) {
	// char szFileName[MAX_PATH];
	LPSTR pszBuffer;

	*pdwLength=0;
	HINTERNET hInet=m_hInet;

	LPSTR pszServer=(LPSTR)strstr(pszUrl,"//")+2;
	LPSTR pszUri=strchr(pszServer,'/');
	*pszUri=0;

	HINTERNET hInetConnect=InternetConnectA(hInet,pszServer,INTERNET_DEFAULT_HTTP_PORT,NULL,NULL,INTERNET_SERVICE_HTTP,0,NULL);
	*pszUri='/';

	if (!hInetConnect) {
		util_log(0,"[CQunInfoExt] GetHTMLDocument: Connection failed: %d, hInet=%p",GetLastError(),hInet);
		*pdwLength=(DWORD)-1;
		// InternetCloseHandle(hInet);
		return false;
	}

	HINTERNET hInetRequest=HttpOpenRequestA(hInetConnect,"GET",pszUri,NULL,pszReferer,NULL,INTERNET_FLAG_PRAGMA_NOCACHE|INTERNET_FLAG_RELOAD,NULL);
	if (!hInetRequest) {
		DWORD err=GetLastError();
		util_log(0,"[CQunInfoExt] GetHTMLDocument: HttpOpenRequestA() failed: %d",err);
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
		util_log(0,"[CQunInfoExt] GetHTMLDocument: HttpSendRequestA() failed, reason=%d",err);
		InternetCloseHandle(hInetRequest);
		InternetCloseHandle(hInetConnect);
		*pdwLength=(DWORD)-1;
		SetLastError(err);
		return false;
	}


	dwRead=0;
	DWORD dwWritten=sizeof(DWORD);
	
	HttpQueryInfo(hInetRequest,HTTP_QUERY_CONTENT_LENGTH|HTTP_QUERY_FLAG_NUMBER,pdwLength,&dwWritten,&dwRead);

	pszBuffer=NULL;

	if (strlen(pszUrl)>200 && (pszBuffer=(LPSTR)strchr(pszUrl,'?'))) *pszBuffer=0;

	if (strlen(pszUrl)<200) 
		util_log(0,"[CQunInfoExt] GetHTMLDocument: url=%s size=%d",pszUrl,*pdwLength);
	else
		util_log(0,"[CQunInfoExt] GetHTMLDocument: size=%d",*pdwLength);

	if (pszBuffer) *pszBuffer='?';

	if (!*pdwLength) *pdwLength=65536;

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

LPSTR CQunInfoExt::PostHTMLDocument(LPCSTR pszServer, LPCSTR pszUri, LPCSTR pszReferer, LPCSTR pszPostBody, LPDWORD pdwLength) {
	DWORD dwRead=65536;
	LPSTR pszBuffer;
	bool passvar=false;
	bool vfpost=!strncmp(pszPostBody,"r={",3);

	util_log(0,"%s > %s",pszUri,pszPostBody);
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

	HINTERNET hInetConnect=InternetConnectA(m_hInet,pszServer,INTERNET_DEFAULT_HTTP_PORT,NULL,NULL,INTERNET_SERVICE_HTTP,0,NULL);
	if (!hInetConnect) {
		util_log(0,"[CQunInfoExt] PostHTMLDocument: Connection failed: %d",GetLastError());
		*pdwLength=(DWORD)-1;
		LocalFree(pszPOSTBuffer);
		return false;
	}

	HINTERNET hInetRequest=HttpOpenRequestA(hInetConnect,"POST",pszUri,NULL,pszReferer,NULL,INTERNET_FLAG_PRAGMA_NOCACHE|INTERNET_FLAG_RELOAD,NULL);
	if (!hInetRequest) {
		DWORD err=GetLastError();
		util_log(0,"[CQunInfoExt] PostHTMLDocument: HttpOpenRequestA() failed: %d", err);
		*pdwLength=(DWORD)-1;
		LocalFree(pszPOSTBuffer);
		InternetCloseHandle(hInetConnect);
		SetLastError(err);
		return false;
	}

	if (!(HttpSendRequestA(hInetRequest,"Content-Type: application/x-www-form-urlencoded; charset=UTF-8\r\nX-Requested-With: XMLHttpRequest",-1,(LPVOID)pszPOSTBuffer,(DWORD)strlen(pszPOSTBuffer)))) {
		DWORD err=GetLastError();
		util_log(0,"[CQunInfoExt] PostHTMLDocument: HttpSendRequestA() failed, reason=%d",err);
		InternetCloseHandle(hInetRequest);
		// m_hInetRequest=NULL;
		InternetCloseHandle(hInetConnect);
		*pdwLength=(DWORD)-1;
		LocalFree(pszPOSTBuffer);
		return false;
	}
	LocalFree(pszPOSTBuffer);

	dwRead=0;
	DWORD dwWritten=sizeof(DWORD);

	if (!HttpQueryInfo(hInetRequest,HTTP_QUERY_CONTENT_LENGTH|HTTP_QUERY_FLAG_NUMBER,pdwLength,&dwWritten,&dwRead)) {
		util_log(0,"[CQunInfoExt] PostHTMLDocument: Warning - HttpQueryInfo failed(%d), *pdwLength set to default!",GetLastError());
		*pdwLength=65536;
	}
	util_log(0,"[CQunInfoExt] PostHTMLDocument: size=%d",*pdwLength);

	if (!*pdwLength) *pdwLength=65536;

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

bool CQunInfoExt::IsValid(DWORD dwUIN) { 
	return m_inst!=NULL && m_inst->GetUIN()==dwUIN; 
}

bool CQunInfoExt::AddOneJob(DWORD dwUIN, BYTE op, DWORD extid, DWORD ver) {
	if (m_inst==NULL)
		util_log(0,"[CQunInfoExt] AddJob: Command rejected: No client");
	else if (m_inst->GetUIN()!=dwUIN)
		util_log(0,"[CQunInfoExt] AddJob: Command rejected: Wrong owner");
	else {
		m_inst->AddOneJob(op,extid,ver);
		return true;
	}
	return false;
}

void CQunInfoExt::AddOneJob(BYTE op, DWORD extid, DWORD ver) {
	QUNINFOOP qio={op,extid,ver};
	m_queue.push(qio);
	SetEvent(m_event);
}

void CQunInfoExt::EXT_DownloadGroupInfo() {
	char m_buffer[MAX_PATH];
	DWORD dwSize;

	sprintf(m_buffer,"http://qun.qq.com/air/group/mine?w=a&_=%f",(double)rand()/(double)RAND_MAX);
	if (LPSTR pszData=GetHTMLDocument(m_buffer,"http://qun.qq.com",&dwSize)) {
		if (*pszData=='{') {
			JSONNODE* jn=json_parse(pszData);
			JSONNODE* jnC=json_get(jn,"c");
			JSONNODE* jnItem;
			HANDLE hContact;
			LPCSTR m_szModuleName=m_network->m_szModuleName;
			LPSTR pszTemp;
			WCHAR szNick[MAX_PATH];
			LPWSTR szInfo;

			int count=json_size(jnC);

			util_log(0,"[CQunInfoExt] OP-0: %d quns in reply",count);
			for (int c=0; c<count; c++) {
				jnItem=json_at(jnC,c);
				if (hContact=FindContactByExtID(json_as_float(json_get(jnItem,"g")))) {
					WRITEC_B("QunInfoExt",1);

					// WRITEC_U8S("Nick",pszTemp=json_as_string(json_get(jnItem,"name"))); json_free(pszTemp);
					pszTemp=json_as_string(json_get(jnItem,"name")); 
					szInfo=mir_utf8decodeW(pszTemp);
					_stprintf(szNick,TranslateT("(QQ Qun) %s"),szInfo);
					WRITEC_TS("Nick",szNick);
					mir_free(szInfo);
					json_free(pszTemp);

					WRITEC_U8S("Description",pszTemp=json_as_string(json_get(jnItem,"brief"))); json_free(pszTemp);
					DBWriteContactSettingUTF8String(hContact,"CList","StatusMsg",pszTemp=json_as_string(json_get(jnItem,"memo"))); json_free(pszTemp);
					WRITEC_D("CreateTime",json_as_float(json_get(jnItem,"create_time")));
					WRITEC_W("MaxMember",json_as_int(json_get(jnItem,"max_member")));
				} else
					util_log(0,"[CQunInfoExt] OP-0: Unable to find qun contact with ExtID %u",json_as_float(json_get(jnItem,"g")));
			}

			json_delete(jn);
		} else
			util_log(0,"[CQunInfoExt] OP-0: Non JSON reply!");

		LocalFree(pszData);
	}
}

HANDLE CQunInfoExt::FindContactByExtID(DWORD dwExtID) {
	HANDLE hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, (WPARAM)NULL, (LPARAM)NULL);
	LPCSTR m_szModuleName=m_network->m_szModuleName;

	while (hContact) {
		if (!lstrcmpA(m_szModuleName,(char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact,(LPARAM)NULL)) && 
			READC_D2("ExternalID")==dwExtID)
			 return hContact;

		hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, (LPARAM)NULL);
	}
	return NULL;
}

void CQunInfoExt::EXT_DownloadCardInfo(DWORD dwExtID, DWORD dwVersion) {
	LPCSTR m_szModuleName=m_network->m_szModuleName;
	if (HANDLE hContact=FindContactByExtID(dwExtID)) {
		if (READC_D2("ExtCardVersion")==dwVersion) {
			util_log(0,"[CQunInfoExt] OP-1: Same card version for Qun %u",dwExtID);
		} else {
			char m_buffer[MAX_PATH];
			DWORD dwSize;

			WRITEC_D("ExtCardVersion",dwVersion);
			sprintf(m_buffer,"http://qun.qq.com/air/?w=a&c=json&a=members&g=%u&ty=0&_=%f",dwExtID,(double)rand()/(double)RAND_MAX);
			if (LPSTR pszData=GetHTMLDocument(m_buffer,"http://qun.qq.com/air/",&dwSize)) {
				if (*pszData=='{') {
					JSONNODE* jn=json_parse(pszData);
					JSONNODE* jnC=json_get(json_get(json_get(jn,"c"),"mbs"),"cards");
					JSONNODE* jnItem;
					LPSTR pszTemp;
					LPSTR pszName;

					int count=json_size(jnC);

					// WRITEC_B("CardsUTF8",1);

					util_log(0,"[CQunInfoExt] OP-1: %d cards in reply",count);
					if (count==0) {
						// Something wrong
						WRITEC_D("ExtCardVersion",0);
					}
					/*
					if (count==0) {
						util_log(0,"Dump: %s",pszData);
						FILE* fp=fopen("C:\\json.txt","w");
						fputs(pszData,fp);
						fclose(fp);
					}
					*/

					for (int c=0; c<count; c++) {
						jnItem=json_at(jnC,c);
						WRITEC_S(pszName=json_name(jnItem),mir_utf8decodecp(pszTemp=json_as_string(jnItem),936,NULL));
						json_free(pszName);
						json_free(pszTemp);
					}

					json_delete(jn);
				} else
					util_log(0,"[CQunInfoExt] OP-1: Non JSON reply!");

				LocalFree(pszData);
			}
		}
	} else
		util_log(0,"[CQunInfoExt] OP-1: Unable to find qun contact with ExtID %u!",dwExtID);
}
