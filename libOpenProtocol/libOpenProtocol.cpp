#define LUA52
#define WINHTTP

#include "libOpenProtocol.h"
#ifdef WINHTTP
#include <winhttp.h>
#pragma comment(lib,"winhttp")
#else
#pragma comment(lib,"libcurl/libcurl_a_debug.lib")
#endif

#define DEFAULTPACKETSIZE 131072

#define NULLASSERT(var,act) if (var==NULL) { print_debug("%s: ASSERT - %s==NULL!",__FUNCTION__,#var); act; }
#define NULLASSERTDEFAULT(var) NULLASSERT(var,OP_ASSERTACTION)
#define ZEROASSERT(op,act) if (op!=0) { print_debug("%s: ASSERT - %s!=0!",__FUNCTION__,#op); act; }
#define ZEROASSERTDEFAULT(op) ZEROASSERT(op,OP_ASSERTACTION)
#define NONZEROASSERT(op,act) ZEROASSERT(!(op),act);
#define NONZEROASSERTDEFAULT(op) NONZEROASSERT(op,OP_ASSERTACTION)
#define IFASSERT(var,act) if (!(var)) { print_debug("%s: ASSERT - (%s)==false!",__FUNCTION__,#var); act; }
#define IFASSERTDEFAULT(var) NULLASSERT(var,OP_ASSERTACTION)
#define ASSERT(msg,act) { print_debug("%s: ASSERT - %s!",__FUNCTION__,msg); act; }
#define ASSERTDEFAULT(msg) ASSERT(msg,OP_ASSERTACTION)
#define ENSURE_ARGUMENTS(x) if (lua_gettop(L)<x) { lua_pushstring(L,"__FUNCTION__(fixme) called with incorrect number of arguments"); lua_error(L); return 0; }

static list<COpenProtocol*> s_instances;
static COpenProtocol* s_firstInstance=NULL;
// static char s_logtext[1024];

int OP_CreateThreads(lua_State* L);

/*** COpenProtocol ***/
#ifdef LIBCURL
void lock_function(CURL *handle, curl_lock_data data, curl_lock_access access, void *userptr) {
	WaitForSingleObject(((COpenProtocol*)userptr)->m_curlmutex,INFINITE);
}

void unlock_function(CURL *handle, curl_lock_data data, void *userptr) {
	ReleaseMutex(((COpenProtocol*)userptr)->m_curlmutex);
}
#endif

COpenProtocol* COpenProtocol::FindProtocol(const char* pcszUIN) {
	for (list<COpenProtocol*>::iterator iter=s_instances.begin(); iter!=s_instances.end(); iter++) {
		if (!strcmp((*iter)->m_uin,pcszUIN)) return *iter;
	}

	return NULL;
}

void COpenProtocol::print_debug(LPCSTR pcszFormat,...) {
#ifdef OP_SHOWDEBUGMSG
	/*
	if (!m_debugMsgBuffer) {
		//m_debugMsgBuffer=(LPSTR)m_handler->oph_malloc(OP_DEBUGMSGSIZE);
		m_debugMsgBuffer=s_logtext;
	}

	va_list vl;
	va_start(vl,pcszFormat);
	strcpy(m_debugMsgBuffer,"print('");
	vsprintf(m_debugMsgBuffer+strlen(m_debugMsgBuffer),pcszFormat,vl);
	strcat(m_debugMsgBuffer,"')\n");
	va_end(vl);

	// m_handler->oph_printdebug(m_debugMsgBuffer);
	luaL_dostring(m_L,m_debugMsgBuffer);
	*/

	va_list vl;
	va_start(vl,pcszFormat);
	vprintf(pcszFormat,vl);
	printf("\n");
	va_end(vl);
#endif
}

static void threadstart(LPVOID param) {
	((COpenProtocol*)param)->_start();
}

void COpenProtocol::start() {
	m_handler->oph_pthread_create(threadstart,this);
	// _start();
}

void COpenProtocol::stop() {
}

FILE* COpenProtocol::handleQunImage(LPCSTR pcszUri, BOOL isP2P) {
	char szFilename[MAX_PATH];
	char szState[MAX_PATH];
	LPCSTR pszFilename=strrchr(pcszUri,'/')+1;
	LPSTR pszTemp;
	FILE* fp;

	if (isP2P) {
		LPSTR pszTemp;
		strcat(strcpy(szFilename,m_qunimagepath),"\\");
		pszTemp=szFilename+strlen(szFilename);
		strcpy(pszTemp,pcszUri);
		while (strchr(pszTemp,'/')) *strchr(pszTemp,'/')='_';
	} else
		strcat(strcat(strcpy(szFilename,m_qunimagepath),"\\"),pszFilename);

	// strcat(strcat(strcpy(szFilename,m_qunimagepath),"\\"),pszFilename);
	if (pszTemp=strchr(strcpy(szState,pszFilename),'.')) *pszTemp=0;

	if (fp=fopen(szFilename,"rb")) {
		printf("%s() File already exists (#1), return it\n",__FUNCTION__);
		return fp;
	}

	WaitForSingleObject(m_qunimagemutex,INFINITE);

	if (fp=fopen(szFilename,"rb")) {
		printf("%s() File already exists (#2), return it\n",__FUNCTION__);
		ReleaseMutex(m_qunimagemutex);
		return fp;
	}
	
	callFunction(m_threads[THREAD_QUNIMAGE],isP2P?"HandleP2PImage":"HandleQunImage",pcszUri);
	ReleaseMutex(m_qunimagemutex);

	if (fp=fopen(szFilename,"rb")) {
		printf("%s() Qun/P2P image %s saved, return it\n",__FUNCTION__,pszFilename);
		return fp;
	} else {
		printf("%s() Qun/P2P image %s failed\n",__FUNCTION__,pszFilename);
		return NULL;
	}
}

int test(lua_State* L) {
	lua_getglobal(L,"OP_inst");

	COpenProtocol* pOP=(COpenProtocol*)lua_touserdata(L,-1);
	lua_pop(L,1);

	printf("%s(): pOP=%p params=%d\n",__FUNCTION__,pOP,lua_gettop(L));
	pOP->test();

	return 0; // number of return values
}

typedef struct {
	COpenProtocol* op;
	int cb;
	unsigned char* data;
	unsigned char* current;
} curl_buf_t;

size_t write_function( void *ptr, size_t size, size_t nmemb, void *stream) {
	curl_buf_t* pCB=(curl_buf_t*) stream;
	if (!pCB->cb) {
		printf("Warning: no Content-Length in header, reserving buffer of %d\n",DEFAULTPACKETSIZE);
		pCB->cb=-1;
		pCB->current=pCB->data=(unsigned char*) pCB->op->m_handler->oph_malloc(DEFAULTPACKETSIZE);
		//pCB->data[8192]=0;
		memset(pCB->data,0,DEFAULTPACKETSIZE);
	}

	int cb=(int)(size*nmemb);
	if (pCB->cb==-1 && (pCB->current-pCB->data)+cb>=DEFAULTPACKETSIZE) {
		printf("Error: Buffer too small for current content!!!\n");
	}
	memcpy(pCB->current,ptr,cb);
	pCB->current+=cb;

	return cb;
}

size_t header_function( void *ptr, size_t size, size_t nmemb, void *stream) {
	if (!memcmp(ptr,"Content-Length: ",16)) {
		curl_buf_t* pCB=(curl_buf_t*) stream;
		char* pData=(char*)pCB->op->m_handler->oph_malloc((int)size*nmemb+1);
		memcpy(pData,ptr,size*nmemb);
		pData[size*nmemb-1]=0;

		if (pCB->cb>0) {
			printf("Note: Removing previous data buffer of length %d (Redirect?)\n",pCB->cb);
			pCB->op->m_handler->oph_free(pCB->data);
		}
		pCB->cb=atoi(pData+16);
		pCB->current=pCB->data=(unsigned char*) pCB->op->m_handler->oph_malloc(pCB->cb+1);
		pCB->data[pCB->cb]=0;

		// printf("header: %s\n",(char*)pData);

		pCB->op->m_handler->oph_free(pData);
	} else if (!memcmp(ptr,"Set-Cookie: ",12)) {
		curl_buf_t* pCB=(curl_buf_t*) stream;
		char* pData=(char*)pCB->op->m_handler->oph_malloc((int)size*nmemb+1);
		memcpy(pData,ptr,size*nmemb);
		pData[size*nmemb-1]=0;

		//luaL_dofile(pCB->op->m_L,"lua/cookies.lua");
		*strchr(pData,';')=0;
		char* pszKey=pData+12;
		char* pszValue=strchr(pData,'=')+1;
		pszValue[-1]=0;
		printf("Set LUA global for cookie %s=%s\n",pszKey,pszValue);
		if (!*pszValue) {
			lua_pushnil(pCB->op->m_L);
		} else {
			lua_pushstring(pCB->op->m_L,pszValue);
		}
		lua_setglobal(pCB->op->m_L,pszKey);

		pCB->op->m_handler->oph_free(pData);
	}
	return size*nmemb;
}

int OP_Sleep(lua_State* L) {
	printf("Start Sleeping...\n");
	Sleep((DWORD)lua_tointeger(L,1));
	printf("End Sleeping\n");
	return 0;
}

int OP_VeryCode(lua_State* L) {
	lua_getglobal(L,"OP_inst");

	COpenProtocol* pOP=(COpenProtocol*)lua_touserdata(L,-1);
	lua_pop(L,1);

	LPSTR pcszFile=strdup(lua_tostring(L,1));
	pOP->getHandler()->handler(OPEVENT_VERYCODE,NULL,pcszFile);

	if (strlen(pcszFile)==4 || (*pcszFile==' ' && pcszFile[1]==0)) {
		_strupr(pcszFile);
		lua_pushstring(L,pcszFile);
	} else {
		lua_pushnil(L);
	}

	return 1;
}

int OP_MD5(lua_State* L) {
	bool textout=false;
	LPCSTR pszData;
	size_t len;

	if (lua_gettop(L)>1) {
		textout=lua_toboolean(L,2)!=0;
	}

	pszData=lua_tolstring(L,1,&len);

	HCRYPTPROV hCP;
	HCRYPTHASH hCH;
	BYTE bHash[16];
	DWORD dwHash=16;

	CryptAcquireContextA(&hCP,NULL,NULL,PROV_RSA_FULL,CRYPT_VERIFYCONTEXT);
	CryptCreateHash(hCP,CALG_MD5,0,0,&hCH);
	CryptHashData(hCH,(LPBYTE)pszData,(int)len,0);
	CryptGetHashParam(hCH,HP_HASHVAL,bHash,&dwHash,0);
	CryptDestroyHash(hCH);
	CryptReleaseContext(hCP,0);

	if (textout) {
		char szTemp[33];
		char* pszTemp=szTemp;

		for (int c=0; c<16; c++) {
			pszTemp+=sprintf(pszTemp,"%02X",(int)bHash[c]);
		}
		lua_pushstring(L,szTemp);
	} else {
		lua_pushlstring(L,(char*)bHash,16);
	}

	return 1;
}

int OP_GetTempFile(lua_State* L) {
	char szTempPath[MAX_PATH];
	char szTempFile[MAX_PATH];

	lua_getglobal(L,"OP_avatardir");
	strcpy(szTempPath,lua_tostring(L,-1));
	lua_pop(L,1);
	// GetTempPathA(MAX_PATH,szTempPath);
	GetTempFileNameA(szTempPath,"_op",0,szTempFile);
	printf("%s(): Temp file=%s\n",__FUNCTION__,szTempFile);

	lua_pushstring(L,szTempFile);
	return 1;
}

#ifdef WINHTTP
typedef struct _COOKIE {
	LPSTR name;
	LPSTR value;
	LPSTR path;
	LPSTR domain;
	_COOKIE* next;
} COOKIE, *PCOOKIE, *LPCOOKIE;

int OP_GetCookie(lua_State* L) {
	lua_getglobal(L,"OP_inst");

	COpenProtocol* pOP=(COpenProtocol*)lua_touserdata(L,-1);
	lua_pop(L,1);

	lua_getglobal(L,"OP_cookies");

	LPCOOKIE pCK=(LPCOOKIE)lua_touserdata(L,-1);
	bool found=false;
	lua_pop(L,1);

	if (pCK) {
		LPSTR pcszName=strdup(lua_tostring(L,1));
		LPSTR pcszDomain=NULL;

		if (lua_gettop(L)==2) {
			pcszDomain=strdup(lua_tostring(L,2));
		}

		while (pCK) {
			if (!strcmp(pCK->name,pcszName)) {
				if (pcszDomain==NULL || pCK->domain==NULL || strstr(pCK->domain,pcszDomain)) {
					lua_pushstring(L,pCK->value);
					found=true;
					break;
				}
			}
			pCK=pCK->next;
		}

		if (!found) pOP->print_debug("OP_GetCookie: ASSERT - Unable to find cookie named %s!",pcszName);
	}

	if (!found) {
		lua_pushnil(L);
	}
	return 1;
}

void ParseOneCookie(COpenProtocol* pOP, LPCOOKIE pCK, LPSTR pszCookie) {
	// NOTE: pszCookie will be modified

	LPSTR pszValue;
	LPSTR pszDomain;
	LPSTR pszExpires;
	LPSTR pszName;
	LPSTR pszPath;
	LPSTR pszEqual;
	LPSTR ppszCookie;
	LPCOOKIE pCKTail;

	pszName=pszValue=pszDomain=pszExpires=pszPath=NULL;
	ppszCookie=pszCookie;

// pOP->print_debug("ParseOneCookie: 1");
	do {
		if (!pszName) {
// pOP->print_debug("ParseOneCookie: 2.1");
			pszName=ppszCookie;
			pszValue=strchr(pszName,'=');
			*pszValue++=0;
			ppszCookie=pszValue;
		} else {
// pOP->print_debug("ParseOneCookie: 2.2");
			pszEqual=strchr(ppszCookie,'=');
			if (pszEqual) *pszEqual=0;
			strlwr(ppszCookie);
			if (pszEqual) *pszEqual='=';

			if (!strncmp(ppszCookie,"expires=",8)) {
				pszExpires=ppszCookie+8;
			} else if (!strncmp(ppszCookie,"path=",5)) {
				pszPath=ppszCookie+5;
			} else if (!strncmp(ppszCookie,"domain=",7)) {
				pszDomain=ppszCookie+7;
				if (*pszDomain=='.') pszDomain++;
			}
		}

// pOP->print_debug("ParseOneCookie: 2.3");
		ppszCookie=strchr(ppszCookie,';');
		if (ppszCookie) {
			*ppszCookie=0;
			if (ppszCookie[1]==0)
				break;
			else
				ppszCookie+=2;
		}
	} while (ppszCookie);

// pOP->print_debug("ParseOneCookie: 3");
	if (pCK->name) {
		pCKTail=pCK;
// pOP->print_debug("ParseOneCookie: 4");
		while (pCKTail) {
			if (!strcmp(pCKTail->name,pszName) && 
				(pCKTail->domain==NULL || strstr(pCKTail->domain,pszDomain))) {
// pOP->print_debug("ParseOneCookie: 5");
					LocalFree(pCKTail->name);
					pCKTail->name=pszName;
					pCKTail->domain=pszDomain;
					pCKTail->path=pszPath;
					pCKTail->value=pszValue;
					pOP->print_debug("ParseCookie: Replaced cookie %s=%s@%s",pszName,pszValue,pszDomain);
					pszName=NULL;
					break;
			}

			// pOP->print_debug("ParseOneCookie: 6 pCKTail=%p pCKTail->name=%s pCKTail->next=%p",pCKTail,pCKTail->name,pCKTail->next);
			if (pCKTail->next)
				pCKTail=pCKTail->next;
			else
				break;
		}
	}

	if (pszName) {
// pOP->print_debug("ParseCookie: 7");
		if (pCK->name!=NULL) {
// pOP->print_debug("ParseCookie: 8");
			pCK=(LPCOOKIE)LocalAlloc(LMEM_FIXED,sizeof(COOKIE));
			pCKTail->next=pCK;
			ZeroMemory(pCK,sizeof(COOKIE));
		}

// pOP->print_debug("ParseCookie: 9");
		pCK->name=pszName;
		pCK->domain=pszDomain;
		pCK->path=pszPath;
		pCK->value=pszValue;
		pOP->print_debug("ParseCookie: Added cookie %s=%s@%s",pszName,pszValue,pszDomain);
	}
}

void ParseCookie(lua_State* L, HINTERNET hInetReq) {
	lua_getglobal(L,"OP_inst");

	COpenProtocol* pOP=(COpenProtocol*)lua_touserdata(L,-1);
	lua_pop(L,1);

	// pOP->print_debug("ParseCookie: 1.0");
	lua_getglobal(L,"OP_cookies");

	LPCOOKIE pCK=(LPCOOKIE)lua_touserdata(L,-1);
	lua_pop(L,1);

	if (pCK==NULL) {
		// pOP->print_debug("ParseCookie: 1.1");
		pCK=(LPCOOKIE)LocalAlloc(LMEM_FIXED,sizeof(COOKIE));
		ZeroMemory(pCK,sizeof(COOKIE));
		lua_pushlightuserdata(L,pCK);
		lua_setglobal(L,"OP_cookies");
		pOP->print_debug("ParseCookie: new cookie chain");
	}

	// pOP->print_debug("ParseCookie: 1.2");

	BOOL ret;
	DWORD dwBufferLength;
	DWORD dwIndex=0;
	LPSTR pszCookie;
	int cbCurrentBuffer=0; // 1024*sizeof(WCHAR); // Protocol should be 4096 max
	LPWSTR wszOneCookie=NULL; (LPWSTR)LocalAlloc(LMEM_FIXED,cbCurrentBuffer); 

	// pOP->print_debug("ParseCookie: 1.3");

	while (true) {
		dwBufferLength=cbCurrentBuffer;

		if (WinHttpQueryHeaders(hInetReq,WINHTTP_QUERY_SET_COOKIE,WINHTTP_HEADER_NAME_BY_INDEX,wszOneCookie,&dwBufferLength,&dwIndex)) {
			// HSID=AYQEVnc.DKrdst; Domain=.foo.com; Path=/; Expires=Wed, 13 Jan 2021 22:23:01 GMT; HttpOnly
			// pOP->print_debug("ParseCookie: 1.4");

			pszCookie=(LPSTR)LocalAlloc(LMEM_FIXED,wcslen(wszOneCookie)+1);
	// pOP->print_debug("ParseCookie: 1.5");
			wcstombs(pszCookie,wszOneCookie,wcslen(wszOneCookie)+1);

			pOP->print_debug("%s",pszCookie);

			ParseOneCookie(pOP,pCK,pszCookie);
		} else if (GetLastError()==ERROR_WINHTTP_HEADER_NOT_FOUND) {
	// pOP->print_debug("ParseCookie: 1.17");
			break;
		} else if (GetLastError()==ERROR_INSUFFICIENT_BUFFER) {
	// pOP->print_debug("ParseCookie: 1.18");
			// Not enough buffer!
			pOP->print_debug("ParseCookie: Not enough buffer for at least one cookie! Extending to %d bytes", dwBufferLength);
			if (wszOneCookie) LocalFree(wszOneCookie);
			cbCurrentBuffer=dwBufferLength;
			wszOneCookie=(LPWSTR)LocalAlloc(LMEM_FIXED,cbCurrentBuffer);
			continue;
		} else {
	// pOP->print_debug("ParseCookie: 1.19");
			pOP->print_debug("ParseCookie: Unknown API error %d",GetLastError());
			break;
		}
	}

	// pOP->print_debug("ParseCookie: 1.20");
	if (wszOneCookie) LocalFree(wszOneCookie);
	// pOP->print_debug("ParseCookie: 1.21");
}

#endif

int OP_Print(lua_State* L) {
	ENSURE_ARGUMENTS(1);
	/*
	lua_getglobal(L,"OP_inst");
	COpenProtocol* pOP=(COpenProtocol*)lua_touserdata(L,-1);
	lua_pop(L,1);
	*/

	LPCSTR pszStr=lua_tostring(L,1);
	LPWSTR pwszTemp=(LPWSTR)LocalAlloc(LMEM_FIXED,(strlen(pszStr)+1)*2);
	LPSTR pszStr2=(LPSTR)LocalAlloc(LMEM_FIXED,strlen(pszStr)+1);
	MultiByteToWideChar(CP_UTF8,0,pszStr,-1,pwszTemp,strlen(pszStr)+1);
	WideCharToMultiByte(CP_ACP,0,pwszTemp,-1,pszStr2,strlen(pszStr)+1,NULL,NULL);

	printf("%s\n",pszStr2);

	LocalFree(pwszTemp);
	LocalFree(pszStr2);

	return 0;
}

int OP_SetCookie(lua_State* L) {
	ENSURE_ARGUMENTS(1);

	lua_getglobal(L,"OP_inst");
	COpenProtocol* pOP=(COpenProtocol*)lua_touserdata(L,-1);
	lua_pop(L,1);

	lua_getglobal(L,"OP_cookies");

	LPCOOKIE pCK=(LPCOOKIE)lua_touserdata(L,-1);
	lua_pop(L,1);

	if (pCK==NULL) {
		// pOP->print_debug("ParseCookie: 1.1");
		pCK=(LPCOOKIE)LocalAlloc(LMEM_FIXED,sizeof(COOKIE));
		ZeroMemory(pCK,sizeof(COOKIE));
		lua_pushlightuserdata(L,pCK);
		lua_setglobal(L,"OP_cookies");
		pOP->print_debug("OP_SetCookie: new cookie chain");
	}

	pOP->print_debug("OP_SetCookie: 1");
	// Cannot use strdup as it uses LocalFree()
	LPCSTR pcszCookie=lua_tostring(L,1);
	LPSTR pszCookie=(LPSTR)LocalAlloc(LMEM_FIXED,strlen(pcszCookie)+1);
	strcpy(pszCookie,pcszCookie);
	pOP->print_debug("OP_SetCookie: 2");
	ParseOneCookie(pOP,pCK,pszCookie);
	pOP->print_debug("OP_SetCookie: 3");
	// WARNING! Don't free pszCookie as the cookie object references it directly!
	//free(pszCookie);
	pOP->print_debug("OP_SetCookie: 4");

	return 0;
}

#ifdef LIBCURL
int OP_GetCookie(lua_State* L) {
	lua_getglobal(L,"OP_inst");

	COpenProtocol* pOP=(COpenProtocol*)lua_touserdata(L,-1);
	lua_pop(L,1);

	LPSTR pcszName=strdup(lua_tostring(L,1));
	
	CURL* pCU=curl_easy_init();
	curl_easy_setopt(pCU,CURLOPT_SHARE,pOP->m_curlshare);
	
	CURLcode nResult;
	struct curl_slist * lpCookies = NULL;
	struct curl_slist * lpCurrent;
	int len=strlen(pcszName);
	bool found=false;
	LPCSTR pcszPos;

	nResult = curl_easy_getinfo(pCU, CURLINFO_COOKIELIST, &lpCookies);
	
	if (nResult == CURLE_OK && lpCookies != NULL) {
		lpCurrent=lpCookies;
		while (lpCurrent) {
			// MessageBoxA(NULL,lpCurrent->data,NULL,0);
			pcszPos=strstr(lpCurrent->data,pcszName);
			if (pcszPos!=NULL && pcszPos[-1]=='\t' && pcszPos[len]=='\t') {
				lua_pushstring(L,pcszPos+len+1);
				found=true;
				break;
			}
			lpCurrent=lpCurrent->next;
		}
		curl_slist_free_all(lpCookies);
	}
	
	if (!found) {
		lua_pushnil(L);
	}

	curl_easy_cleanup(pCU);
	free(pcszName);
	
	return 1;
}

CURL* newCurl(LPCSTR pcszUrl, LPCSTR pcszReferer, LPCSTR pcszUA, int proxyType, LPCSTR pcszProxyUrl, LPCSTR pcszProxyUserPwd, CURLSH* pCUSH, curl_buf_t* pCB) {
	CURL* pCU=curl_easy_init();

	curl_easy_setopt(pCU,CURLOPT_VERBOSE,0);
	curl_easy_setopt(pCU,CURLOPT_NOPROGRESS,1);
	curl_easy_setopt(pCU,CURLOPT_WRITEFUNCTION,write_function);
	curl_easy_setopt(pCU,CURLOPT_HEADERFUNCTION,header_function);
	curl_easy_setopt(pCU,CURLOPT_WRITEHEADER,pCB);
	curl_easy_setopt(pCU,CURLOPT_WRITEDATA,pCB);
	curl_easy_setopt(pCU,CURLOPT_SHARE,pCUSH);
	curl_easy_setopt(pCU,CURLOPT_URL,pcszUrl);
	curl_easy_setopt(pCU,CURLOPT_FOLLOWLOCATION,1);
	curl_easy_setopt(pCU,CURLOPT_SSL_VERIFYPEER,FALSE);
	// curl_easy_setopt(pCU,CURLOPT_SSLVERSION, CURL_SSLVERSION_SSLv3);
	if (pcszReferer && *pcszReferer) curl_easy_setopt(pCU,CURLOPT_REFERER,pcszReferer);
	curl_easy_setopt(pCU,CURLOPT_USERAGENT,pcszUA);
	curl_easy_setopt(pCU,CURLOPT_TIMEOUT,150);
	if (proxyType!=-1) {
		curl_easy_setopt(pCU,CURLOPT_PROXYTYPE,proxyType);
		curl_easy_setopt(pCU,CURLOPT_PROXY,pcszProxyUrl);
		if (pcszProxyUserPwd) {
			curl_easy_setopt(pCU,CURLOPT_PROXYUSERPWD,pcszProxyUserPwd);
		}
	}

	return pCU;
}
#endif

void initUA(lua_State* L, COpenProtocol* pOP) {
	if (!pOP->m_ua) {
		LPCSTR pcszUA;
		lua_getglobal(L,"OP_ua");
		pcszUA=lua_tostring(L,-1);
		if (pcszUA && *pcszUA)
			pOP->m_ua=pOP->m_handler->oph_strdup(pcszUA);
		else
			pOP->m_ua=pOP->m_handler->oph_strdup("Mozilla/5.0 (Windows NT 6.2; WOW64) AppleWebKit/537.1 (KHTML, like Gecko) Chrome/21.0.1180.49 Safari/537.1");

		lua_pop(L,1);
	}
}

#ifdef WINHTTP
static HINTERNET GetWinHttpSession(COpenProtocol* pOP, lua_State* L) {
	lua_getglobal(L,"OP_hInet");
	HINTERNET hInetSession=(HINTERNET*)lua_touserdata(L,-1);
	lua_pop(L,1);

	if (hInetSession==NULL) {
		WCHAR wszTemp[MAX_PATH];
		DWORD dwOption=WINHTTP_OPTION_REDIRECT_POLICY_ALWAYS;

		initUA(L,pOP);
		mbstowcs(wszTemp,pOP->m_ua,MAX_PATH);

		hInetSession=WinHttpOpen(wszTemp,WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,WINHTTP_NO_PROXY_NAME,WINHTTP_NO_PROXY_BYPASS,0);
		WinHttpSetOption(hInetSession,WINHTTP_OPTION_REDIRECT_POLICY,&dwOption,sizeof(DWORD));
		dwOption=150000; // ms
		WinHttpSetOption(hInetSession,WINHTTP_OPTION_RECEIVE_TIMEOUT,&dwOption,sizeof(DWORD));
		WinHttpSetOption(hInetSession,WINHTTP_OPTION_RECEIVE_RESPONSE_TIMEOUT,&dwOption,sizeof(DWORD));
		/*
		dwOption=WINHTTP_ENABLE_SSL_REVERT_IMPERSONATION;
		WinHttpSetOption(hInetSession,WINHTTP_OPTION_ENABLE_FEATURE,&dwOption,sizeof(DWORD));
		dwOption=WINHTTP_FLAG_SECURE_PROTOCOL_ALL;
		WinHttpSetOption(hInetSession,WINHTTP_OPTION_SECURE_PROTOCOLS,&dwOption,sizeof(DWORD));
		dwOption=SECURITY_FLAG_IGNORE_CERT_CN_INVALID|SECURITY_FLAG_IGNORE_CERT_DATE_INVALID|SECURITY_FLAG_IGNORE_CERT_WRONG_USAGE|SECURITY_FLAG_IGNORE_UNKNOWN_CA;
		WinHttpSetOption(hInetSession,WINHTTP_OPTION_SECURITY_FLAGS,&dwOption,sizeof(DWORD));
		*/

		lua_pushlightuserdata(L,hInetSession);
		lua_setglobal(L,"OP_hInet");
	}

	return hInetSession;
}

int OP_Post(lua_State* L) {
	lua_getglobal(L,"OP_inst");

	COpenProtocol* pOP=(COpenProtocol*)lua_touserdata(L,-1);
	lua_pop(L,1);

	int fields;
	HINTERNET hInetSession=GetWinHttpSession(pOP,L);

	lua_getglobal(L,"cookie");
	const char* pcszCookies=lua_tostring(L,-1);

	if ((fields=lua_gettop(L))<4) {
		printf("%s(): Incorrect number of parameters\n",__FUNCTION__);
		lua_pop(L,1); // cookie
		lua_pushstring(L,"");
	} else {
		int cbUrl, cbReferer;
		LPCSTR pcszUrl=lua_tostring(L,1);
		LPCSTR pcszReferer=lua_tostring(L,2);
		LPWSTR pwszUrl=(LPWSTR)LocalAlloc(LMEM_FIXED,(cbUrl=strlen(pcszUrl)+1)*2);
		LPWSTR pwszReferer=pcszReferer?(LPWSTR)LocalAlloc(LMEM_FIXED,(cbReferer=strlen(pcszReferer)+1)*2):NULL;

		mbstowcs(pwszUrl,pcszUrl,cbUrl);
		if (pwszReferer) mbstowcs(pwszReferer,pcszReferer,cbReferer);

		LPWSTR pwszProtocol=pwszUrl;
		LPWSTR pwszServer=wcsstr(pwszProtocol,L"://");
		LPWSTR pwszUri=wcschr(pwszServer+3,'/');
		LPWSTR pwszPort=NULL;
		BOOL isHTTPS;

		*pwszServer=0; pwszServer+=3;
		*pwszUri++=0;
		if (pwszPort=wcschr(pwszServer,':')) {
			*pwszPort++=0;
		}

		isHTTPS=wcslen(pwszProtocol)==5;

		HINTERNET hInetConn=WinHttpConnect(hInetSession,pwszServer,pwszPort?wcstol(pwszPort,NULL,10):isHTTPS?INTERNET_DEFAULT_HTTPS_PORT:INTERNET_DEFAULT_HTTP_PORT,0);

		HINTERNET hInetReq=WinHttpOpenRequest(hInetConn,L"POST",pwszUri,NULL,pwszReferer,WINHTTP_DEFAULT_ACCEPT_TYPES,isHTTPS?WINHTTP_FLAG_SECURE:0);

		LPCSTR pcszPostdata=lua_tostring(L,3);
		int nPostSize=(int)(fields==4?strlen(pcszPostdata):lua_tointeger(L,4));
		// * If nPostSize==-1, then the data is multipart/formdata, otherwise it is application/x-www-form-urlencoded
		// * if nPostSize==-1, part 5 is valid as table to get upload contents

		// Remove existing custom cookies
		WinHttpAddRequestHeaders(hInetReq,L"Cookie:",-1,WINHTTP_ADDREQ_FLAG_REPLACE);

		if (pcszCookies!=NULL) {
			LPWSTR pwszCookies=(LPWSTR)LocalAlloc(LMEM_FIXED,(strlen(pcszCookies)+9)*sizeof(WCHAR));
			wcscpy(pwszCookies,L"Cookie: ");
			mbstowcs(pwszCookies+8,pcszCookies,strlen(pcszCookies)+1);
			WinHttpAddRequestHeaders(hInetReq,pwszCookies,-1,WINHTTP_ADDREQ_FLAG_ADD|WINHTTP_ADDREQ_FLAG_REPLACE);
			LocalFree(pwszCookies);
		}

		LPSTR pszBuffer=NULL;
		WCHAR wszHost[MAX_PATH]=L"Origin: ";
		mbstowcs(wszHost+8,pcszUrl,MAX_PATH-8);
		LPWSTR pwszHost=wcschr(wcschr(wszHost,'.'),'/');
		if (pwszHost) *pwszHost=0;

		WinHttpAddRequestHeaders(hInetReq,wszHost,-1,WINHTTP_ADDREQ_FLAG_ADD|WINHTTP_ADDREQ_FLAG_REPLACE);

		if (nPostSize!=-1) {
			WinHttpAddRequestHeaders(hInetReq,L"Content-Type: application/x-www-form-urlencoded",-1,WINHTTP_ADDREQ_FLAG_ADD|WINHTTP_ADDREQ_FLAG_REPLACE);
			WinHttpSendRequest(hInetReq,WINHTTP_NO_ADDITIONAL_HEADERS,0,(LPVOID)pcszPostdata,nPostSize,nPostSize,(DWORD_PTR)pOP);\
		} else {
			// HTTPPOST
			WinHttpAddRequestHeaders(hInetReq,L"Content-Type: multipart/form-data, boundary=----WebKitFormBoundary9zCD31eJSHkdb8ul",-1,WINHTTP_ADDREQ_FLAG_ADD|WINHTTP_ADDREQ_FLAG_REPLACE);

			DWORD cbBuffer=1024;
			DWORD cbFilled=0;
			pszBuffer=(LPSTR)LocalAlloc(LMEM_FIXED,1024);
			LPSTR ppszBuffer=pszBuffer;

			LPCSTR pcszKey;
			LPCSTR pcszValue;
			DWORD cbStr;
			
			lua_pushnil(L); // For storing key of list

			while (lua_next(L,5)!=0) {
				// Overhead of 160 bytes for section header should be enough
				// -2 is key, -1 is value
				pcszKey=lua_tostring(L,-2);
				pcszValue=lua_tostring(L,-1);

				if (*pcszValue!='\t') {
					if ((int)cbBuffer-(int)cbFilled-128-(int)strlen(pcszKey)-(int)strlen(pcszValue)<0) {
						// Not enough buffer
						cbBuffer+=((128+strlen(pcszKey)+strlen(pcszValue)+1023)/1024)*1024;
						LPSTR pszOldBuffer=pszBuffer;
						pszBuffer=(LPSTR)LocalAlloc(LMEM_FIXED,cbBuffer);
						memcpy(pszBuffer,pszOldBuffer,cbFilled);
						ppszBuffer=pszBuffer+cbFilled;
						LocalFree(pszOldBuffer);
					}
					cbStr=strlen(strcpy(ppszBuffer,"------WebKitFormBoundary9zCD31eJSHkdb8ul\r\n"));
					cbFilled+=cbStr; ppszBuffer+=cbStr;
					cbStr=sprintf(ppszBuffer,"Content-Disposition: form-data; name=\"%s\"\r\n\r\n",pcszKey);
					cbFilled+=cbStr; ppszBuffer+=cbStr;
					cbStr=strlen(strcpy(ppszBuffer,pcszValue));
					cbFilled+=cbStr; ppszBuffer+=cbStr;
					/*
					cbStr=strlen(strcpy(ppszBuffer,"\r\n"));
					cbFilled+=cbStr; ppszBuffer+=cbStr;
					*/
				} else {
					const char* pszFile=strrchr(pcszValue,'\\');
					if (!pszFile) pszFile=strrchr(pcszValue,'/');
					pszFile++;
					char* pszExt=strdup(strrchr(pszFile,'.'));
					strlwr(pszExt);

					HANDLE hFile=CreateFileA(pcszValue+1,GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
					DWORD dwFile=GetFileSize(hFile,NULL);

					if ((int)cbBuffer-(int)cbFilled-192-(int)strlen(pcszKey)-(int)strlen(pcszValue)-(int)dwFile<0) {
						// Not enough buffer
						cbBuffer+=((192+strlen(pcszKey)+strlen(pcszValue)+dwFile+1023)/1024)*1024;
						LPSTR pszOldBuffer=pszBuffer;
						pszBuffer=(LPSTR)LocalAlloc(LMEM_FIXED,cbBuffer);
						memcpy(pszBuffer,pszOldBuffer,cbFilled);
						ppszBuffer=pszBuffer+cbFilled;
						LocalFree(pszOldBuffer);
					}
					cbStr=strlen(strcpy(ppszBuffer,"------WebKitFormBoundary9zCD31eJSHkdb8ul\r\n"));
					cbFilled+=cbStr; ppszBuffer+=cbStr;
					cbStr=sprintf(ppszBuffer,"Content-Disposition: form-data; name=\"%s\"; filename=\"%s\"\r\n",pcszKey,pszFile);
					cbFilled+=cbStr; ppszBuffer+=cbStr;
					cbStr=sprintf(ppszBuffer,"Content-Type: %s\r\n",strcmp(pszExt,".jpg")?strcmp(pszExt,".gif")?strcmp(pszExt,".png")?"application/octet-stream":"image/png":"image/gif":"image/jpeg");
					cbFilled+=cbStr; ppszBuffer+=cbStr;
					// cbStr=strlen(strcpy(ppszBuffer,"Content-Transfer-Encoding: binary\r\n\r\n"));
					cbStr=strlen(strcpy(ppszBuffer,"\r\n"));
					cbFilled+=cbStr; ppszBuffer+=cbStr;
					//cbStr=strlen(strcpy(ppszBuffer,pcszValue));
					ReadFile(hFile,ppszBuffer,dwFile,&cbStr,NULL);
					cbFilled+=cbStr; ppszBuffer+=cbStr;
					/*cbStr=strlen(strcpy(ppszBuffer,"\r\n"));
					cbFilled+=cbStr; ppszBuffer+=cbStr;*/

					CloseHandle(hFile);
					free(pszExt);
				}
				cbStr=strlen(strcpy(ppszBuffer,"\r\n"));
				cbFilled+=cbStr; ppszBuffer+=cbStr;

				lua_pop(L,1); // Remove value and keep key
				/*
				HANDLE hFile=CreateFileA("R:\\test.bin",GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,CREATE_ALWAYS,0,NULL);
				DWORD dwWritten;
				WriteFile(hFile,pszBuffer,cbFilled,&dwWritten,NULL);
				CloseHandle(hFile);
				*/
			}

			// cbStr=strlen(strcpy(ppszBuffer,"\r\n------WebKitFormBoundary9zCD31eJSHkdb8ul--\r\n"));
			cbStr=strlen(strcpy(ppszBuffer,"------WebKitFormBoundary9zCD31eJSHkdb8ul--\r\n"));
			cbFilled+=cbStr; ppszBuffer+=cbStr;

			WinHttpSendRequest(hInetReq,WINHTTP_NO_ADDITIONAL_HEADERS,0,(LPVOID)pszBuffer,cbFilled,cbFilled,(DWORD_PTR)pOP);
		}

		LPBYTE pszBuffer2;
		DWORD cb2;

		if (WinHttpReceiveResponse(hInetReq,NULL)) {
		// pOP->print_debug("OP_Post: 1.1");
			ParseCookie(L,hInetReq);
		// pOP->print_debug("OP_Post: 1.2");

			WCHAR szcbData[16]={0};
			DWORD cbData=0;
			cb2=32;
			DWORD dwAvailable;
			DWORD dwRead;
			WinHttpQueryHeaders(hInetReq,WINHTTP_QUERY_CONTENT_LENGTH,WINHTTP_HEADER_NAME_BY_INDEX,szcbData,&cb2,WINHTTP_NO_HEADER_INDEX);
		// pOP->print_debug("OP_Post: 1.3");

			cbData=wcstoul(szcbData,NULL,10);
			if (cbData==0) cbData=DEFAULTPACKETSIZE;
			pszBuffer2=(LPBYTE)LocalAlloc(LMEM_FIXED,cbData+1);
			LPBYTE ppszBuffer=pszBuffer2;

			cb2=0;

			while (cb2<cbData) {
				if (WinHttpQueryDataAvailable(hInetReq,&dwAvailable) && dwAvailable>0) {
					WinHttpReadData(hInetReq,ppszBuffer,dwAvailable,&dwRead);
					cb2+=dwRead;
					ppszBuffer+=dwRead;
				} else {
					break;
				}
			}

			*ppszBuffer=0;
		// pOP->print_debug("OP_Post: 1.4");
		} else {
			pOP->print_debug("OP_Get: Fetched failed for %s",pcszUrl);
			pszBuffer2=(LPBYTE)LocalAlloc(LMEM_FIXED,1);
			cb2=0;
		}

		WinHttpCloseHandle(hInetReq);
		WinHttpCloseHandle(hInetConn);

		if (cb2>10 && cb2<1024 && pszBuffer2[10]>0x20 && pszBuffer2[10]<0x7f) {
			printf("WinHttp POST dump:\n%s\n",pszBuffer2);
		}

		lua_pop(L,1); // Cookies
		lua_pushlstring(L,(const char*)pszBuffer2,cb2);

		LocalFree(pszBuffer2);
		if (pszBuffer) LocalFree(pszBuffer);
		LocalFree(pwszUrl);
		LocalFree(pwszReferer);
		// pOP->print_debug("OP_Post: 6");
	}

	return 1;
}

int OP_Get(lua_State* L) {
	lua_getglobal(L,"OP_inst");

	COpenProtocol* pOP=(COpenProtocol*)lua_touserdata(L,-1);
	lua_pop(L,1);

	int fields;
	struct curl_slist* headerlist=NULL;

	HINTERNET hInetSession=GetWinHttpSession(pOP,L);

	lua_getglobal(L,"cookie");
	const char* pcszCookies=lua_tostring(L,-1);

	if ((fields=lua_gettop(L))<3) {
		printf("%s(): Incorrect number of parameters\n",__FUNCTION__);
		lua_pop(L,1);
		lua_pushstring(L,"");
	} else {
		int cbUrl, cbReferer;
		LPCSTR pcszUrl=lua_tostring(L,1);
		LPCSTR pcszReferer=lua_tostring(L,2);
		LPWSTR pwszUrl=(LPWSTR)LocalAlloc(LMEM_FIXED,(cbUrl=strlen(pcszUrl)+1)*2);
		LPWSTR pwszReferer=pcszReferer?(LPWSTR)LocalAlloc(LMEM_FIXED,(cbReferer=strlen(pcszReferer)+1)*2):NULL;

		mbstowcs(pwszUrl,pcszUrl,cbUrl);
		if (pwszReferer) mbstowcs(pwszReferer,pcszReferer,cbReferer);

		LPWSTR pwszProtocol=pwszUrl;
		LPWSTR pwszServer=wcsstr(pwszProtocol,L"://");
		LPWSTR pwszUri=wcschr(pwszServer+3,'/');
		LPWSTR pwszPort=NULL;
		BOOL isHTTPS;

		*pwszServer=0; pwszServer+=3;
		if (pwszUri) {
			*pwszUri++=0;
			if (pwszPort=wcschr(pwszServer,':')) {
				*pwszPort++=0;
			}
		}

		isHTTPS=wcslen(pwszProtocol)==5;

		HINTERNET hInetConn=WinHttpConnect(hInetSession,pwszServer,pwszPort?wcstol(pwszPort,NULL,10):isHTTPS?INTERNET_DEFAULT_HTTPS_PORT:INTERNET_DEFAULT_HTTP_PORT,0);

		HINTERNET hInetReq=WinHttpOpenRequest(hInetConn,L"GET",pwszUri,NULL,pwszReferer,WINHTTP_DEFAULT_ACCEPT_TYPES,isHTTPS?WINHTTP_FLAG_SECURE:0);

		if (fields==4) {
			// Advanced: all headers
			WinHttpAddRequestHeaders(hInetReq,L"Connection: close",-1,WINHTTP_ADDREQ_FLAG_ADD|WINHTTP_ADDREQ_FLAG_REPLACE);
			WinHttpAddRequestHeaders(hInetReq,L"Accept-Encoding: gzip,deflate,sdch",-1,WINHTTP_ADDREQ_FLAG_ADD|WINHTTP_ADDREQ_FLAG_REPLACE);
			WinHttpAddRequestHeaders(hInetReq,L"Accept-Language: ja,en-US;q=0.8,en;q=0.6",-1,WINHTTP_ADDREQ_FLAG_ADD|WINHTTP_ADDREQ_FLAG_REPLACE);
		} else {
			WinHttpAddRequestHeaders(hInetReq,L"Accept-Encoding: ",-1,WINHTTP_ADDREQ_FLAG_ADD|WINHTTP_ADDREQ_FLAG_REPLACE);
			WinHttpAddRequestHeaders(hInetReq,L"Accept-Language: ja,en-US;q=0.8,en;q=0.6",-1,WINHTTP_ADDREQ_FLAG_ADD|WINHTTP_ADDREQ_FLAG_REPLACE);
		}

		// Remove existing custom cookies
		WinHttpAddRequestHeaders(hInetReq,L"Cookie:",-1,WINHTTP_ADDREQ_FLAG_REPLACE);

		if (pcszCookies!=NULL) {
			LPWSTR pwszCookies=(LPWSTR)LocalAlloc(LMEM_FIXED,(strlen(pcszCookies)+9)*sizeof(WCHAR));
			wcscpy(pwszCookies,L"Cookie: ");
			mbstowcs(pwszCookies+8,pcszCookies,strlen(pcszCookies)+1);
			WinHttpAddRequestHeaders(hInetReq,pwszCookies,-1,WINHTTP_ADDREQ_FLAG_ADD|WINHTTP_ADDREQ_FLAG_REPLACE);
			LocalFree(pwszCookies);
		}

		LPBYTE pszBuffer;
		DWORD cb2;

		if (WinHttpSendRequest(hInetReq,WINHTTP_NO_ADDITIONAL_HEADERS,0,WINHTTP_NO_REQUEST_DATA,0,0,(DWORD_PTR)pOP)) {
			// pOP->print_debug("OP_Get: 1");
			if (WinHttpReceiveResponse(hInetReq,NULL)) {
				// pOP->print_debug("OP_Get: 1.1");
				ParseCookie(L,hInetReq); // FIXME: Problem here!!!
				// pOP->print_debug("OP_Get: 1.2");

				WCHAR szcbData[16]={0};
				DWORD cbData=0;
				cb2=32;
				DWORD dwAvailable;
				DWORD dwRead;
				WinHttpQueryHeaders(hInetReq,WINHTTP_QUERY_CONTENT_LENGTH,WINHTTP_HEADER_NAME_BY_INDEX,szcbData,&cb2,WINHTTP_NO_HEADER_INDEX);

				// pOP->print_debug("OP_Get: 1.3");
				cbData=wcstoul(szcbData,NULL,10);
				if (cbData==0) cbData=DEFAULTPACKETSIZE;
				pszBuffer=(LPBYTE)LocalAlloc(LMEM_FIXED,cbData+1);
				LPBYTE ppszBuffer=pszBuffer;

				cb2=0;

				// pOP->print_debug("OP_Get: 1.4");
				while (cb2<cbData) {
					if (WinHttpQueryDataAvailable(hInetReq,&dwAvailable) && dwAvailable>0) {
						WinHttpReadData(hInetReq,ppszBuffer,dwAvailable,&dwRead);
						cb2+=dwRead;
						ppszBuffer+=dwRead;
					} else {
						break;
					}
				}
				*ppszBuffer=0;
			} else {
				pOP->print_debug("OP_Get: Receive failed for %s, GetLastError()=%d",pcszUrl,GetLastError());
				cb2=0;
				pszBuffer=(LPBYTE)LocalAlloc(LMEM_FIXED,1);
				*pszBuffer=0;
			}
			// pOP->print_debug("OP_Get: 2");
		} else {
			pOP->print_debug("OP_Get: Fetched failed for %s, GetLastError()=%d",pcszUrl,GetLastError());
			cb2=0;
			pszBuffer=(LPBYTE)LocalAlloc(LMEM_FIXED,1);
			*pszBuffer=0;
		}

		WinHttpCloseHandle(hInetReq);
		WinHttpCloseHandle(hInetConn);

		if (cb2>10 && cb2<1024 && pszBuffer[10]>0x20 && pszBuffer[10]<0x7f) {
			printf("WinHttp GET dump:\n%s\n",pszBuffer);
		} else {
			printf("WinHttp GET url=%s size=%d\n",pcszUrl,cb2);
		}

		lua_pop(L,1); // Cookies
		lua_pushlstring(L,(const char*)pszBuffer,cb2);

		LocalFree(pszBuffer);
		LocalFree(pwszUrl);
		LocalFree(pwszReferer);

		// pOP->print_debug("OP_Get: Phase 5");
	}

	return 1;
}
#endif

#ifdef LIBCURL
int OP_Post(lua_State* L) {
	lua_getglobal(L,"OP_inst");

	COpenProtocol* pOP=(COpenProtocol*)lua_touserdata(L,-1);
	int fields;
	lua_pop(L,1);

	lua_getglobal(L,"cookie");
	const char* pcszCookies=lua_tostring(L,-1);

	if ((fields=lua_gettop(L))<4) {
		printf("%s(): Incorrect number of parameters\n",__FUNCTION__);
		lua_pop(L,1); // cookie
		lua_pushstring(L,"");
	} else {
		LPCSTR pcszUrl=lua_tostring(L,1);
		LPCSTR pcszReferer=lua_tostring(L,2);
		LPCSTR pcszPostdata=lua_tostring(L,3);
		int nPostSize=(int)(fields==4?strlen(pcszPostdata):lua_tointeger(L,4));
		// * If nPostSize==-1, then the data is multipart/formdata, otherwise it is application/x-www-form-urlencoded
		// * if nPostSize==-1, part 5 is valid as table to get upload contents

		curl_buf_t cb={0};
		cb.op=pOP;
		
		// printf(__FUNCTION__"(): url=%s referer=%s\n",pcszUrl,pcszReferer);

		initUA(L,pOP);

		CURL* pCU=newCurl(pcszUrl,pcszReferer,pOP->m_ua,pOP->m_proxytype,pOP->m_proxyurl,pOP->m_proxyuserpwd,pOP->m_curlshare,&cb);

		if (*pcszCookies) curl_easy_setopt(pCU,CURLOPT_COOKIE,pcszCookies);

		char szHost[MAX_PATH]="Origin: ";
		strcat(szHost,pcszUrl);
		LPSTR pszHost=strchr(strchr(szHost,'.'),'/');
		if (pszHost) *pszHost=0;

		struct curl_slist* headerlist=curl_slist_append(NULL,szHost);
		curl_easy_setopt(pCU,CURLOPT_HTTPHEADER,headerlist);

		// curl_easy_setopt(pCU,CURLOPT_HEADER
		if (nPostSize!=-1) {
			curl_easy_setopt(pCU,CURLOPT_POST,1);
			curl_easy_setopt(pCU,CURLOPT_POSTFIELDS,pcszPostdata);
			curl_easy_setopt(pCU,CURLOPT_POSTFIELDSIZE,nPostSize);
		} else {
			// HTTPPOST
			/*
			Pass a pointer to a linked list of curl_httppost structs as parameter. 
			The easiest way to create such a list, is to use curl_formadd(3) as documented. 
			The data in this list must remain intact until you close this curl handle again 
			with curl_easy_cleanup(3).
 			*/
			struct curl_httppost* formpost=NULL;
			struct curl_httppost* lastptr=NULL;
			LPCSTR pcszKey;
			LPCSTR pcszValue;
			
			lua_pushnil(L); // For storing key of list

			while (lua_next(L,5)!=0) {
				// -2 is key, -1 is value
				pcszKey=lua_tostring(L,-2);
				pcszValue=lua_tostring(L,-1);

				if (*pcszValue!='\t')
					curl_formadd(&formpost,&lastptr,CURLFORM_COPYNAME,pcszKey,CURLFORM_COPYCONTENTS,pcszValue,CURLFORM_END);
				else
					curl_formadd(&formpost,&lastptr,CURLFORM_COPYNAME,pcszKey,CURLFORM_FILE,pcszValue+1,CURLFORM_END);

				lua_pop(L,1); // Remove value and keep key
			}

			curl_easy_setopt(pCU,CURLOPT_HTTPPOST,formpost);
		}

		curl_easy_perform(pCU);
		curl_easy_cleanup(pCU);
		curl_slist_free_all(headerlist);

		if (cb.cb==-1) cb.cb=(int)strlen((const char*)cb.data);
		lua_pop(L,1); // cookie
		lua_pushlstring(L,(const char*)cb.data,cb.cb);
		pOP->m_handler->oph_free(cb.data);
	}

	return 1;
}

int OP_Get(lua_State* L) {
	lua_getglobal(L,"OP_inst");

	COpenProtocol* pOP=(COpenProtocol*)lua_touserdata(L,-1);
	int fields;
	struct curl_slist* headerlist=NULL;
	lua_pop(L,1);

	lua_getglobal(L,"cookie");
	const char* pcszCookies=lua_tostring(L,-1);

	if ((fields=lua_gettop(L))<3) {
		printf("%s(): Incorrect number of parameters\n",__FUNCTION__);
		lua_pop(L,1);
		lua_pushstring(L,"");
	} else {
		LPCSTR pcszUrl=lua_tostring(L,1);
		LPCSTR pcszReferer=lua_tostring(L,2);
		//LPCSTR pcszCookies=(fields==3)?"":lua_tostring(L,3);

		curl_buf_t cb={0};
		cb.op=pOP;
		
		// printf(__FUNCTION__"(): url=%s referer=%s\n",pcszUrl,pcszReferer);

		initUA(L,pOP);

		CURL* pCU=newCurl(pcszUrl,pcszReferer,pOP->m_ua,pOP->m_proxytype,pOP->m_proxyurl,pOP->m_proxyuserpwd,pOP->m_curlshare,&cb);
		curl_easy_setopt(pCU,CURLOPT_HTTPGET,1);
		curl_easy_setopt(pCU,CURLOPT_COOKIE,pcszCookies);

		if (fields==4) {
			// Advanced: all headers
			char* szHost[]={
				"Connection: close",
				"Accept-Encoding: gzip,deflate,sdch",
				"Accept-Language: ja,en-US;q=0.8,en;q=0.6"
			};

			struct curl_slist* headerlist=curl_slist_append(NULL,szHost[0]);
			curl_slist_append(headerlist,szHost[1]);
			curl_slist_append(headerlist,szHost[2]);
			curl_easy_setopt(pCU,CURLOPT_HTTPHEADER,headerlist);
		}

		curl_easy_perform(pCU);

		curl_easy_cleanup(pCU);

		if (cb.cb==-1) cb.cb=(int)strlen((const char*)cb.data);
		lua_pop(L,1);
		lua_pushlstring(L,(const char*)cb.data,cb.cb);
		pOP->m_handler->oph_free(cb.data);
	}

	if (headerlist!=NULL) curl_slist_free_all(headerlist);

	return 1;
}
#endif

int OP_LoginSuccess(lua_State* L) {
	lua_getglobal(L,"OP_inst");

	COpenProtocol* pOP=(COpenProtocol*)lua_touserdata(L,-1);
	lua_pop(L,1);

	pOP->m_handler->handler(OPEVENT_LOGINSUCCESS,NULL,NULL);

	return 0;
}

int OP_UpdateProfile(lua_State* L) {
	ENSURE_ARGUMENTS(2);

	lua_getglobal(L,"OP_inst");
	COpenProtocol* pOP=(COpenProtocol*)lua_touserdata(L,-1);
	lua_pop(L,1);

	unsigned int tuin=lua_tounsigned(L,1);

	/*
	// 
	lua_pushnil(L); // For first key

	// pop key, push key+value
	while (lua_next(L,2)!=0) {
		printf(__FUNCTION__"(): tuin=%u %s=%s\n",tuin,lua_tostring(L,-2),lua_tostring(L,-1));
		lua_pop(L,1); // Remove value and keep key
	}

	// no need to pop! lua_pop(L,1);
	*/

	pOP->m_handler->handler(OPEVENT_CONTACTINFO,(LPCSTR)&tuin,L);

	return 0;
}

int OP_UpdateAvatar(lua_State* L) {
	ENSURE_ARGUMENTS(2);

	printf("%s(): Update avatar of %u to %s\n",__FUNCTION__,lua_tounsigned(L,1),lua_tostring(L,2));

	return 0;
}

int OP_Group(lua_State* L) {
	ENSURE_ARGUMENTS(2);

	lua_getglobal(L,"OP_inst");

	COpenProtocol* pOP=(COpenProtocol*)lua_touserdata(L,-1);
	lua_pop(L,1);

	int index;
	printf("%s(): Group #%d: %s\n",__FUNCTION__,index=(int)lua_tointeger(L,1),lua_tostring(L,2));
	pOP->m_handler->handler(OPEVENT_LOCALGROUP,lua_tostring(L,2),&index);

	return 0;
}

int OP_ContactStatus(lua_State* L) {
	ENSURE_ARGUMENTS(3);

	printf("%s(): Contact %u changed status to %d, client_type=%d\n",__FUNCTION__,lua_tounsigned(L,1),lua_tointeger(L,2),lua_tointeger(L,3));

	lua_getglobal(L,"OP_inst");

	COpenProtocol* pOP=(COpenProtocol*)lua_touserdata(L,-1);
	lua_pop(L,1);

	DWORD dwParam=(DWORD)MAKELPARAM(lua_tointeger(L,2),lua_tointeger(L,3));
	DWORD dwTUIN=lua_tounsigned(L,1);
	pOP->m_handler->handler(OPEVENT_CONTACTSTATUS,(LPCSTR)&dwTUIN,&dwParam);

	printf("%s(): End\n",__FUNCTION__);

	return 0;
}

int OP_GroupMessage(lua_State* L) {
	ENSURE_ARGUMENTS(4);

	lua_getglobal(L,"OP_inst");
	COpenProtocol* pOP=(COpenProtocol*)lua_touserdata(L,-1);
	lua_pop(L,1);

	unsigned int tuin=lua_tounsigned(L,1);
	pOP->m_handler->handler(OPEVENT_GROUPMESSAGE,(LPCSTR)&tuin,L);

	return 0;
}

int OP_SessionMessage(lua_State* L) {
	ENSURE_ARGUMENTS(4);

	lua_getglobal(L,"OP_inst");
	COpenProtocol* pOP=(COpenProtocol*)lua_touserdata(L,-1);
	lua_pop(L,1);

	char szID[32];
	sprintf(szID,"%u_%u",lua_tounsigned(L,1),lua_tounsigned(L,2));
	pOP->m_handler->handler(OPEVENT_SESSIONMESSAGE,szID,L);

	return 0;
}

int OP_ContactMessage(lua_State* L) {
	ENSURE_ARGUMENTS(3);

	lua_getglobal(L,"OP_inst");
	COpenProtocol* pOP=(COpenProtocol*)lua_touserdata(L,-1);
	lua_pop(L,1);

	unsigned int tuin=lua_tounsigned(L,1);
	pOP->m_handler->handler(OPEVENT_CONTACTMESSAGE,(LPCSTR)&tuin,L);

	return 0;
}

int OP_UpdateQunInfo(lua_State* L) {
	ENSURE_ARGUMENTS(2);

	lua_getglobal(L,"OP_inst");
	COpenProtocol* pOP=(COpenProtocol*)lua_touserdata(L,-1);
	lua_pop(L,1);

	unsigned int tuin=lua_tounsigned(L,1);
	pOP->m_handler->handler(OPEVENT_GROUPINFO,(LPCSTR)&tuin,L);

	return 0;
}

int OP_UpdateQunMembers(lua_State* L) {
	ENSURE_ARGUMENTS(2);

	lua_getglobal(L,"OP_inst");
	COpenProtocol* pOP=(COpenProtocol*)lua_touserdata(L,-1);
	lua_pop(L,1);

	unsigned int tuin=lua_tounsigned(L,1);
	pOP->m_handler->handler(OPEVENT_GROUPMEMBERS,(LPCSTR)&tuin,L);

	return 0;
}

int OP_TypingNotify(lua_State* L) {
	ENSURE_ARGUMENTS(1);

	lua_getglobal(L,"OP_inst");
	COpenProtocol* pOP=(COpenProtocol*)lua_touserdata(L,-1);
	lua_pop(L,1);

	unsigned int tuin=lua_tounsigned(L,1);
	pOP->m_handler->handler(OPEVENT_TYPINGNOTIFY,(LPCSTR)&tuin,NULL);

	return 0;
}

int OP_AddTempContact(lua_State* L) {
	ENSURE_ARGUMENTS(2);

	lua_getglobal(L,"OP_inst");
	COpenProtocol* pOP=(COpenProtocol*)lua_touserdata(L,-1);
	lua_pop(L,1);

	unsigned int tuin=lua_tounsigned(L,1);
	unsigned int status=lua_tounsigned(L,2);
	pOP->m_handler->handler(OPEVENT_ADDTEMPCONTACT,(LPCSTR)&tuin,&status);

	return 0;
}

int OP_AddSearchResult(lua_State* L) {
	ENSURE_ARGUMENTS(5);

	printf("%s(): Add search result for uin %d\n",__FUNCTION__,lua_tounsigned(L,2));

	lua_getglobal(L,"OP_inst");

	COpenProtocol* pOP=(COpenProtocol*)lua_touserdata(L,-1);
	lua_pop(L,1);

	unsigned int tuin=lua_tounsigned(L,2);
	pOP->m_handler->handler(OPEVENT_ADDSEARCHRESULT,(LPCSTR)&tuin,L);

	return 0;
}

int OP_EndOfSearch(lua_State* L) {
	lua_getglobal(L,"OP_inst");

	COpenProtocol* pOP=(COpenProtocol*)lua_touserdata(L,-1);
	lua_pop(L,1);

	pOP->m_handler->handler(OPEVENT_ENDOFSEARCH,NULL,NULL);
	
	return 0;
}

int OP_RequestJoin(lua_State* L) {
	ENSURE_ARGUMENTS(7);
	
	lua_getglobal(L,"OP_inst");
	COpenProtocol* pOP=(COpenProtocol*)lua_touserdata(L,-1);
	lua_pop(L,1);

	unsigned int tuin=lua_tounsigned(L,2);
	pOP->m_handler->handler(OPEVENT_REQUESTJOIN,(LPCSTR)&tuin,L);
	
	return 0;
}

int OP_CreateThreads(lua_State* L) {
	lua_getglobal(L,"OP_inst");
	COpenProtocol* pOP=(COpenProtocol*)lua_touserdata(L,-1);
	lua_pop(L,1);

	char szThreadName[32]="thread_";

	for (int c=0; c<THREAD_COUNT; c++) {
		itoa(c,szThreadName+7,10);
		pOP->m_threads[c]=lua_newthread(L);
		lua_pushthread(pOP->m_threads[c]);
		lua_setglobal(L,szThreadName);
	}

	return 0;
}

int loadstring(lua_State* L) {
	lua_getglobal(L,"OP_inst");

	COpenProtocol* pOP=(COpenProtocol*)lua_touserdata(L,-1);
	lua_pop(L,1);

	const char* str=lua_tostring(L,1);

	if (luaL_dostring(pOP->m_threads[THREAD_INNER],str)) {
		char* pszError;
		printf("err: %s\n",pszError=strdup(lua_tostring(pOP->m_threads[THREAD_INNER],-1)));
		// MessageBoxA(NULL,pszError,NULL,MB_ICONERROR);
		lua_pop(pOP->m_threads[THREAD_INNER],1);

		pOP->m_handler->handler(OPEVENT_ERROR,pszError,NULL);
		lua_pushboolean(L,false);
		free(pszError);
	} else
		lua_pushboolean(L,true);
	

	return 1;
}

void COpenProtocol::_start() {
	lua_pushinteger(m_L,m_initstatus);
	lua_setglobal(m_L,"OP_status");

	OP_CreateThreads(m_L);

	while (luaL_dofile(m_L,m_definitionFile)) {
		char* pszError;
		printf("err: %s\n",pszError=m_handler->oph_strdup(lua_tostring(m_L,-1)));
		lua_pop(m_L,1);

		if (!m_handler->handler(OPEVENT_ERROR,pszError,NULL)) {
			// Don't continue
			printf("Breaking loop due to error\n");
			m_handler->oph_free(pszError);
			break;
		}

		printf("Non-vital error, resume loop\n");
		m_handler->oph_free(pszError);

		/*
		LPWSTR pszMsg=(LPWSTR)m_handler->oph_malloc(strlen(pcszError)*2);
		MultiByteToWideChar(CP_UTF8,0,pcszError,-1,pszMsg,(int)strlen(pcszError));
		MessageBox(NULL,pszMsg,NULL,MB_ICONERROR);
		*/
	}

	printf("End of start function\n");

	delete this;
}

///
void COpenProtocol::_def_precheck() {
	struct _stat st;
	ZEROASSERTDEFAULT(_stat(m_definitionFile,&st));
}

void COpenProtocol::test() {
	printf("%s(): m_L=%p\n",__FUNCTION__,m_L);
}

COpenProtocol::COpenProtocol(LPCSTR pszDefinitionFile, COpenProtocolHandler* handler):
m_debugMsgBuffer(NULL), m_handler(handler), 
m_ua(NULL), m_uin(NULL), m_qunimagepath(NULL), 
m_initstatus(40071), m_avatarpath(NULL),
m_proxytype(-1), m_proxyurl(NULL),
m_proxyuserpwd(NULL)
{
	if (handler==NULL) {
		printf("%s(): handler==NULL!\n",__FUNCTION__);
		DebugBreak();
	}

	NULLASSERTDEFAULT(pszDefinitionFile);

	m_handler->m_protocol=this;
	m_definitionFile=handler->oph_strdup(pszDefinitionFile);
	m_L=luaL_newstate();
	print_debug("%s(): Definition file/dir=%s",__FUNCTION__,m_definitionFile);

	_def_precheck();

#ifdef LIBCURL
	print_debug("%s",curl_version());
	m_curlshare=curl_share_init();
	curl_share_setopt(m_curlshare,CURLSHOPT_SHARE,CURL_LOCK_DATA_DNS);
	curl_share_setopt(m_curlshare,CURLSHOPT_SHARE,CURL_LOCK_DATA_COOKIE);
	curl_share_setopt(m_curlshare,CURLSHOPT_USERDATA,this);
	curl_share_setopt(m_curlshare,CURLSHOPT_LOCKFUNC,lock_function);
	curl_share_setopt(m_curlshare,CURLSHOPT_UNLOCKFUNC,unlock_function);

	m_curlmutex=CreateMutex(NULL,FALSE,NULL);
#endif
#ifdef WINHTTP
	print_debug("HTTP/HTTPS Supported by WinHTTP");
#endif

	m_luamutex=CreateMutex(NULL,FALSE,NULL);
	m_qunimagemutex=CreateMutex(NULL,FALSE,NULL);

	if (s_instances.empty()) {
		s_firstInstance=this;
		s_instances.push_back(this);
	}

	
	luaL_openlibs(m_L);

	lua_pushlightuserdata(m_L,this);
	lua_setglobal(m_L,"OP_inst");
	lua_register(m_L,"test",::test);
	lua_register(m_L,"OP_Get",::OP_Get);
	lua_register(m_L,"OP_Post",::OP_Post);
	lua_register(m_L,"OP_GetTempFile",::OP_GetTempFile);
	lua_register(m_L,"OP_MD5",::OP_MD5);
	lua_register(m_L,"OP_VeryCode",::OP_VeryCode);
	lua_register(m_L,"OP_Sleep",::OP_Sleep);
	lua_register(m_L,"OP_LoginSuccess",::OP_LoginSuccess);
	lua_register(m_L,"OP_UpdateProfile",::OP_UpdateProfile);
	lua_register(m_L,"OP_UpdateAvatar",::OP_UpdateAvatar);
	lua_register(m_L,"OP_Group",::OP_Group);
	lua_register(m_L,"OP_ContactStatus",::OP_ContactStatus);
	lua_register(m_L,"OP_GroupMessage",::OP_GroupMessage);
	lua_register(m_L,"OP_ContactMessage",::OP_ContactMessage);
	lua_register(m_L,"OP_SessionMessage",::OP_SessionMessage);
	lua_register(m_L,"OP_UpdateQunInfo",::OP_UpdateQunInfo);
	lua_register(m_L,"OP_UpdateQunMembers",::OP_UpdateQunMembers);
	lua_register(m_L,"OP_TypingNotify",::OP_TypingNotify);
	lua_register(m_L,"OP_AddTempContact",::OP_AddTempContact);
	lua_register(m_L,"OP_GetCookie",::OP_GetCookie);
	lua_register(m_L,"OP_AddSearchResult",::OP_AddSearchResult);
	lua_register(m_L,"OP_EndOfSearch",::OP_EndOfSearch);
	lua_register(m_L,"OP_RequestJoin",::OP_RequestJoin);
	lua_register(m_L,"OP_SetCookie",::OP_SetCookie);
	lua_register(m_L,"loadstring",::loadstring);
	lua_register(m_L,"print",::OP_Print);
	lua_register(m_L,"OP_CreateThreads",::OP_CreateThreads);

#ifdef LIBCURL
	curl_global_init(/*CURL_GLOBAL_NOTHING*/ CURL_GLOBAL_ALL);
#endif

	HDC hDC=GetDC(NULL);
	lua_pushinteger(m_L,GetDeviceCaps(hDC,LOGPIXELSY));
	ReleaseDC(NULL,hDC);
	lua_setglobal(m_L,"OP_pxy");
}

void COpenProtocol::setLogin(LPCSTR pcszUIN, LPCSTR pcszPassword) {
	lua_pushstring(m_L,m_uin=m_handler->oph_strdup(pcszUIN));
	lua_setglobal(m_L,"OP_uin");
	lua_pushstring(m_L,pcszPassword);
	lua_setglobal(m_L,"OP_password");
}

void COpenProtocol::setQunImagePath(LPCSTR pcszPath, int port) {
	lua_pushstring(m_L,m_qunimagepath=m_handler->oph_strdup(pcszPath));
	lua_setglobal(m_L,"OP_qunimagedir");
	lua_pushinteger(m_L,port);
	lua_setglobal(m_L,"OP_imgserverport");

	if (s_firstInstance==this) {
		CHttpServer::GetInstance(port,m_qunimagepath,m_handler);
	}
}

void COpenProtocol::setBBCode(bool yn) {
	lua_pushboolean(m_L,yn);
	lua_setglobal(m_L,"OP_bbcode");
}

void COpenProtocol::setInitStatus(int status) {
	m_initstatus=status;
}

void COpenProtocol::setAvatarPath(LPCSTR pcszPath) {
	lua_pushstring(m_L,m_avatarpath=m_handler->oph_strdup(pcszPath));
	lua_setglobal(m_L,"OP_avatardir");
}

COpenProtocol::~COpenProtocol() {
	/* It is now a static buffer, don't free it!
	if (m_debugMsgBuffer) {
		m_handler->oph_free(m_debugMsgBuffer);
	}
	*/

#ifdef LIBCURL
	if (m_curlshare) {
		curl_share_cleanup(m_curlshare);
	}

	if (m_curlmutex) {
		CloseHandle(m_curlmutex);
	}
#endif

	if (m_luamutex) {
		CloseHandle(m_luamutex);
	}

	if (m_qunimagemutex) {
		CloseHandle(m_qunimagemutex);
	}

	if (m_ua) {
		m_handler->oph_free(m_ua);
	}
	
	if (m_uin) {
		m_handler->oph_free(m_uin);
	}

	if (m_qunimagepath) {
		m_handler->oph_free(m_qunimagepath);
	}

	if (m_proxyurl) {
		m_handler->oph_free(m_proxyurl);
	}

	if (m_proxyuserpwd) {
		m_handler->oph_free(m_proxyuserpwd);
	}

	if (s_firstInstance==this) {
		CHttpServer* pHS=CHttpServer::GetInstance();
		if (pHS) delete pHS;
	}

	if (m_L) {
		lua_getglobal(m_L,"OP_hInet");
		HINTERNET hInetSession=(HINTERNET*)lua_touserdata(m_L,-1);
		lua_pop(m_L,1);

		if (hInetSession) {
			WinHttpCloseHandle(hInetSession);
		}

		lua_getglobal(m_L,"OP_cookies");
		LPCOOKIE lpCookie=(LPCOOKIE)lua_touserdata(m_L,-1);
		LPCOOKIE lpCookie2;
		lua_pop(m_L,1);

		while (lpCookie) {
			lpCookie2=lpCookie->next;
			LocalFree(lpCookie->name);
			LocalFree(lpCookie);
			lpCookie=lpCookie2;
		}

		lua_close(m_L);
	}

#ifdef LIBCURL
	curl_global_cleanup();
#endif

	s_instances.remove(this);
	if (s_firstInstance==this) s_firstInstance=NULL;
}

void COpenProtocol::callFunction(lua_State* L, LPCSTR pcszName, LPCSTR pcszArgs) {
	lua_getglobal(L,pcszName);
	if (pcszArgs) lua_pushstring(L,pcszArgs);
	if (lua_pcall(L,pcszArgs?1:0,0,0)!=0) {
		char* pszError;
		printf("err: %s\n",pszError=m_handler->oph_strdup(lua_tostring(L,-1)));
		lua_pop(L,1);

		m_handler->handler(OPEVENT_ERROR,pszError,""); // Empty string allows error to continue
		m_handler->oph_free(pszError);
	}
}
/*
void COpenProtocol::callFunction(LPCSTR pcszName, LPCSTR pcszArgs, BOOL fNeedMutex) {
	char szThreadName[64];
	lua_State* L=m_L;
	int ret;
	
	if (fNeedMutex) {
		// WaitForSingleObject(m_luamutex,INFINITE);
		L=lua_newthread(m_L);
		/ *
		lua_pushthread(L);
		sprintf(szThreadName,"T_%u_%p",GetCurrentThreadId(),L);
		lua_setglobal(L,szThreadName);
		* /
		// ReleaseMutex(m_luamutex);
	}
	
	callFunction(m_L,pcszName,pcszArgs);
	/ *
	if (fNeedMutex) {
		// WaitForSingleObject(m_luamutex,INFINITE);
		lua_pushnil(m_L);
		lua_setglobal(m_L,szThreadName);
		ReleaseMutex(m_luamutex);
	}
	* /
}
*/

void COpenProtocol::setProxy(int type, LPCSTR pcszProxy, LPCSTR pcszUserPwd) {
	if (m_proxyurl) m_handler->oph_free(m_proxyurl);
	if (m_proxyuserpwd) m_handler->oph_free(m_proxyuserpwd);
	m_proxytype=type;
	m_proxyurl=m_handler->oph_strdup(pcszProxy);
	m_proxyuserpwd=(pcszUserPwd && *pcszUserPwd)?m_handler->oph_strdup(pcszUserPwd):NULL;
}

void COpenProtocol::signalInterrupt() {
	lua_pushboolean(m_L,true);
	lua_setglobal(m_L,"OP_prestop");
}
