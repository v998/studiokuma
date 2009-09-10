#include "StdAfx.h"
#include <WinInet.h>

CQunImageServer::CQunImageServer(): m_pszBadBMP(NULL), m_docroot(NULL), m_connection(NULL) {
	m_cs.DebugInfo=NULL;

	if (!DBGetContactSettingByte(NULL,g_dllname,QQ_DISABLEHTTPD,0)) {
		NETLIBBIND nlb={sizeof(nlb)};
		nlb.pfnNewConnectionV2=newConnectionProc;
		nlb.wPort=DBGetContactSettingWord(NULL,g_dllname,QQ_HTTPDPORT,170);
		//nlb.dwInternalIP=htonl(inet_addr("127.0.0.1"));
		nlb.pExtra=this;

		m_connection=(HANDLE)CallService(MS_NETLIB_BINDPORT,(WPARAM)hNetlibUser,(LPARAM)&nlb);
		if (!m_connection) {
			util_log(0,"[CQunImageServer] Binding to port 170 failed. Switch to service mode");
			MessageBoxW(NULL,TranslateT("Warning: Failed to bind port for image web server. Qun image for IEView not available."),L"MIRANDAQQ2",MB_ICONERROR);
		} else {
			DBVARIANT dbv;

			InitializeCriticalSection(&m_cs);

			if (!DBGetContactSettingTString(NULL,g_dllname,QQ_HTTPDROOT,&dbv)) {
				m_docroot=mir_wstrdup(dbv.ptszVal);
				DBFreeVariant(&dbv);
			} else {
				WCHAR szTemp[MAX_PATH];
				//CallService(MS_UTILS_PATHTOABSOLUTEW,(WPARAM)L"QQ\\WebServer",(LPARAM)szTemp);
				FoldersGetCustomPathW(CNetwork::m_folders[1],szTemp,MAX_PATH,L"QQ\\WebServer");
				m_docroot=mir_wstrdup(szTemp);
			}

			m_hService=QISCreateService("MirandaQQ/HTTPDAddFile",&CQunImageServer::ServiceAddFile);
			m_hService2=QISCreateService("MirandaQQ/HTTPDSignalFile",&CQunImageServer::ServiceSignalFile);
		}
	}
}

CQunImageServer::~CQunImageServer() {
	if (m_connection) Netlib_CloseHandle(m_connection);
	if (m_cs.DebugInfo) DeleteCriticalSection(&m_cs);
	if (m_docroot) mir_free(m_docroot);
	m_connection=NULL;
	m_docroot=NULL;
	m_cs.DebugInfo=NULL;
}

void CQunImageServer::newConnectionProc(HANDLE hNewConnection,DWORD dwRemoteIP, void * pExtra) {
	((CQunImageServer*)pExtra)->_newConnectionProc(hNewConnection,dwRemoteIP);
}

void CQunImageServer::_newConnectionProc(HANDLE hNewConnection,DWORD dwRemoteIP) {
	if ((dwRemoteIP>=2130706433 && dwRemoteIP<=2147483647) || DBGetContactSettingByte(NULL,g_dllname,QQ_HTTPDALLOWEXTERNAL,0)==1) {
		char szTemp[1024];
		dwRemoteIP=htonl(dwRemoteIP);
		util_log(0,"[CQunImageServer] Connection from %s",inet_ntoa(*(in_addr*)&dwRemoteIP));
		Netlib_Recv(hNewConnection,szTemp,1024,MSG_DUMPASTEXT);

		if (!strncmp(szTemp,"GET ",4)) {
			char* fileName=szTemp+4;
			char szUTF[MAX_PATH];
			DWORD len=MAX_PATH;
			*strchr(fileName,' ')=0;
			InternetCanonicalizeUrlA(fileName,szUTF,&len,ICU_DECODE|ICU_NO_ENCODE);
			if (strstr(szUTF,"../")) {
				char* szSend="HTTP/1.0 403 Forbidden\nContent-Type: text/plain\nConnection: Close\n\nDirectory traversal attack detected.";
				Netlib_Send(hNewConnection,szSend,strlen(szSend),MSG_NODUMP);
				writeTails(hNewConnection);
			} else {
				WCHAR szPath[MAX_PATH];

				MultiByteToWideChar(CP_UTF8,0,szUTF,-1,szPath,MAX_PATH);

				if (!strncmp(fileName,"/qunimage/",10)) {
					LPWSTR pszPath=wcsrchr(szPath,'/')+1;

					EnterCriticalSection(&m_cs);
					if (!loadFile(hNewConnection,pszPath)) {
						if (isFetching(pszPath)) {
							util_log(0,"[CQunImageServer] Start wait for thread download");
							if (m_acceptedconnections[pszPath]!=NULL) {
								wcscat(pszPath,L"|");
								_itow(GetTickCount(),pszPath+wcslen(pszPath),16);
							}
							m_acceptedconnections[pszPath]=hNewConnection;
						} else
							writeBadFile(hNewConnection);
					}
					LeaveCriticalSection(&m_cs);
					return;
				} else
					documentHandler(hNewConnection,szPath);
			}
		} else {
			char* szSend="HTTP/1.0 405 Method Not Allowed\nContent-Type: text/plain\nConnection: Close\n\nThis server only serves GET requests.";
			Netlib_Send(hNewConnection,szSend,strlen(szSend),MSG_NODUMP);
			writeTails(hNewConnection);
		}
	} else
		Netlib_CloseHandle(hNewConnection);
}

void CQunImageServer::addFile(LPCSTR pszFileName) {
	if (m_connection) {
		LPWSTR pwszFileName=mir_a2u_cp(pszFileName,936);
		//util_convertToNative(&pwszFileName,pszFileName);
		EnterCriticalSection(&m_cs);
		m_fetchingfiles.push_back(pwszFileName);
		LeaveCriticalSection(&m_cs);
		util_log(0,"[CQunImageServer] Added %s to waiting list",pszFileName);
		mir_free(pwszFileName);
	} else
		CallService("MirandaQQ/HTTPDAddFile",0,(LPARAM)pszFileName);
}

void CQunImageServer::signalFile(LPCWSTR pwszFileName) {
	if (m_connection) {
		EnterCriticalSection(&m_cs);
		util_log(0,"[CQunImageServer] Searching %S to signal",pwszFileName);
		for (map<wstring,HANDLE>::iterator iter=m_acceptedconnections.begin(); iter!=m_acceptedconnections.end();)	{
			if (pwszFileName==NULL || iter->first.compare(pwszFileName)==0) {
				util_log(0,"[CQunImageServer] Thread found, signal");
				if (!loadFile(iter->second, iter->first.c_str())) {
					/*
					char* szSend="HTTP/1.0 404 File Not Found\nConnection: Close\n\nThe requested file could not be found";
					Netlib_Send(iter->second,szSend,strlen(szSend),MSG_NODUMP);
					*/
					writeBadFile(iter->second);
				}
				iter=m_acceptedconnections.erase(iter);
				//return;
			} else
				iter++;
		}

		for (list<wstring>::iterator iter=m_fetchingfiles.begin(); iter!=m_fetchingfiles.end();)	{
			if (pwszFileName==NULL || iter->compare(pwszFileName)==0) {
				iter=m_fetchingfiles.erase(iter);
			} else
				iter++;
		}
		LeaveCriticalSection(&m_cs);
	} else if (pwszFileName!=NULL) {
		CallService("MirandaQQ/HTTPDSignalFile",0,(LPARAM)pwszFileName);
	}
}

bool CQunImageServer::isFetching(LPCWSTR pszFilename) {
	EnterCriticalSection(&m_cs);
	for (list<wstring>::iterator iter=m_fetchingfiles.begin(); iter!=m_fetchingfiles.end(); iter++) {
		if (!iter->compare(pszFilename)) {
			util_log(0,"[CQunImageServer] %S is in waiting list",pszFilename);
			LeaveCriticalSection(&m_cs);
			return true;
		}
	}
	util_log(0,"[CQunImageServer] %S is NOT in waiting list",pszFilename);
	LeaveCriticalSection(&m_cs);
	return false;
}

bool CQunImageServer::loadFile(HANDLE hConnection, LPCWSTR szFile) {
	WCHAR szPath[MAX_PATH];
	HANDLE hFile;
	// CallService(MS_UTILS_PATHTOABSOLUTEW,(WPARAM)L"QQ\\QunImages\\",(LPARAM)szPath);
	FoldersGetCustomPathW(CNetwork::m_folders[0],szPath,MAX_PATH,L"QQ\\QunImages");
	wcscat(szPath,L"\\");
	wcscat(szPath,szFile);
	if (wcschr(szPath,L'|')) *wcschr(szPath,L'|')=0;
	
	hFile=CreateFileW(szPath,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,NULL,NULL);
	if (hFile==INVALID_HANDLE_VALUE)
		return FALSE;
	else {
		char szTemp[64];
		DWORD fileSize=GetFileSize(hFile,NULL);
		sprintf(szTemp,"HTTP/1.0 200 OK\nContent-Length: %d\nConnection: Close\n\n",fileSize);
		Netlib_Send(hConnection,szTemp,strlen(szTemp),MSG_NODUMP);
		char* szFile=(char*)mir_alloc(fileSize);
		DWORD fileRead;
		ReadFile(hFile,szFile,fileSize,&fileRead,NULL);
		Netlib_Send(hConnection,szFile,fileSize,MSG_NODUMP);
		CloseHandle(hFile);
		mir_free(szFile);
		Netlib_CloseHandle(hConnection);
		return TRUE;
	}
}

void CQunImageServer::writeBadFile(HANDLE hConnection) {
	char szTemp[64];
	if (!m_pszBadBMP) {
		HRSRC hRsrc=FindResource(hinstance,MAKEINTRESOURCE(IDR_ERRORBMP),L"GIF");
		HGLOBAL hGlobal=LoadResource(hinstance,hRsrc);
		m_pszBadBMP=(LPSTR)LockResource(hGlobal);
		m_cbBadBMP=SizeofResource(hinstance,hRsrc);
	}

	sprintf(szTemp,"HTTP/1.0 200 OK\nContent-Length: %d\nConnection: Close\n\n",m_cbBadBMP);
	Netlib_Send(hConnection,szTemp,strlen(szTemp),MSG_NODUMP);
	Netlib_Send(hConnection,m_pszBadBMP,m_cbBadBMP,MSG_NODUMP);
	Netlib_CloseHandle(hConnection);
}

void CQunImageServer::documentHandler(HANDLE hConnection, LPCWSTR szFile) {
	WCHAR szPath[MAX_PATH];

	//CallService(MS_UTILS_PATHTOABSOLUTEW,(WPARAM)L"QQ\\WebServer",(LPARAM)szPath);
	wcscpy(szPath,m_docroot);
	wcscat(szPath,szFile);
	while (wcschr(szPath,L'/')) *wcschr(szPath,L'/')=L'\\';
	if (szPath[wcslen(szPath)-1]==L'\\') wcscat(szPath,L"index.html");

	HANDLE hFile=CreateFileW(szPath,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,NULL,NULL);
	if (hFile==INVALID_HANDLE_VALUE) {
		char* szSend="HTTP/1.0 404 File Not Found\nContent-Type: text/plain\nConnection: Close\n\nThe requested file could not be found.";
		Netlib_Send(hConnection,szSend,strlen(szSend),MSG_NODUMP);
		writeTails(hConnection);
	} else {
		char szTemp[64];
		DWORD fileSize=GetFileSize(hFile,NULL);
		sprintf(szTemp,"HTTP/1.0 200 OK\nContent-Length: %d\nConnection: Close\n\n",fileSize);
		Netlib_Send(hConnection,szTemp,strlen(szTemp),MSG_NODUMP);
		char* szFile=(char*)mir_alloc(fileSize);
		DWORD fileRead;
		ReadFile(hFile,szFile,fileSize,&fileRead,NULL);
		Netlib_Send(hConnection,szFile,fileSize,MSG_NODUMP);
		CloseHandle(hFile);
		mir_free(szFile);
		Netlib_CloseHandle(hConnection);
	}
}

void CQunImageServer::writeTails( HANDLE hConnection )
{
	char szTemp[MAX_PATH]="\n\nMIMQQ/";
	strcat(szTemp,VERSION_DOT);
	while (strchr(szTemp,',')) *strchr(szTemp,',')='.';
	Netlib_Send(hConnection,szTemp,strlen(szTemp),MSG_NODUMP);
	Netlib_CloseHandle(hConnection);
}

HANDLE CQunImageServer::QISCreateService(LPCSTR pszService, QISServiceFunc pFunc) {
	return CreateServiceFunctionObj(pszService,(MIRANDASERVICEOBJ)*(void**)&pFunc,this);
}

int __cdecl CQunImageServer::ServiceAddFile(WPARAM wParam,LPARAM lParam) {
	addFile((LPSTR)lParam);
	return 0;
}

int __cdecl CQunImageServer::ServiceSignalFile(WPARAM wParam, LPARAM lParam) {
	signalFile((LPWSTR)lParam);
	return 0;
}
