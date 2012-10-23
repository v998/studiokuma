#include "libOpenProtocol.h"

#pragma comment(lib,"libcurl/libcurl.lib")

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

static list<COpenProtocol*> s_instances;
static COpenProtocol* s_firstInstance=NULL;

/*** COpenProtocol ***/
void lock_function(CURL *handle, curl_lock_data data, curl_lock_access access, void *userptr) {
	WaitForSingleObject(((COpenProtocol*)userptr)->m_curlmutex,INFINITE);
}

void unlock_function(CURL *handle, curl_lock_data data, void *userptr) {
	ReleaseMutex(((COpenProtocol*)userptr)->m_curlmutex);
}

COpenProtocol* COpenProtocol::FindProtocol(const char* pcszUIN) {
	for (list<COpenProtocol*>::iterator iter=s_instances.begin(); iter!=s_instances.end(); iter++) {
		if (!strcmp((*iter)->m_uin,pcszUIN)) return *iter;
	}

	return NULL;
}

void COpenProtocol::print_debug(LPCSTR pcszFormat,...) {
#ifdef OP_SHOWDEBUGMSG
	if (!m_debugMsgBuffer) {
		m_debugMsgBuffer=(LPSTR)m_handler->oph_malloc(OP_DEBUGMSGSIZE);
	}

	va_list vl;
	va_start(vl,pcszFormat);
	strcpy(m_debugMsgBuffer+vsprintf(m_debugMsgBuffer,pcszFormat,vl),"\n");
	va_end(vl);

	m_handler->oph_printdebug(m_debugMsgBuffer);
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
		_cprintf(__FUNCTION__"() File already exists (#1), return it\n");
		return fp;
	}

	WaitForSingleObject(m_qunimagemutex,NULL);

	if (fp=fopen(szFilename,"rb")) {
		_cprintf(__FUNCTION__"() File already exists (#2), return it\n");
		ReleaseMutex(m_qunimagemutex);
		return fp;
	}
	
	callFunction(isP2P?"HandleP2PImage":"HandleQunImage",pcszUri,TRUE);

	if (fp=fopen(szFilename,"rb")) {
		_cprintf(__FUNCTION__"() Qun/P2P image %s saved, return it\n",pszFilename);
		return fp;
	} else {
		_cprintf(__FUNCTION__"() Qun/P2P image %s failed\n",pszFilename);
		return NULL;
	}
}

int test(lua_State* L) {
	lua_getglobal(L,"OP_inst");

	COpenProtocol* pOP=(COpenProtocol*)lua_touserdata(L,-1);
	lua_pop(L,1);

	_cprintf(__FUNCTION__"(): pOP=%p params=%d\n",pOP,lua_gettop(L));
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
		_cprintf("Warning: no Content-Length in header, reserving buffer of %d\n",DEFAULTPACKETSIZE);
		pCB->cb=-1;
		pCB->current=pCB->data=(unsigned char*) pCB->op->m_handler->oph_malloc(DEFAULTPACKETSIZE);
		//pCB->data[8192]=0;
		memset(pCB->data,0,DEFAULTPACKETSIZE);
	}

	int cb=(int)(size*nmemb);
	if (pCB->cb==-1 && (pCB->current-pCB->data)+cb>=DEFAULTPACKETSIZE) {
		_cprintf("Error: Buffer too small for current content!!!\n");
	}
	memcpy(pCB->current,ptr,cb);
	pCB->current+=cb;

	return cb;
}

size_t header_function( void *ptr, size_t size, size_t nmemb, void *stream) {
	if (!memcmp(ptr,"Content-Length: ",16)) {
		curl_buf_t* pCB=(curl_buf_t*) stream;
		char* pData=(char*)pCB->op->m_handler->oph_malloc(size*nmemb+1);
		memcpy(pData,ptr,size*nmemb);
		pData[size*nmemb-1]=0;

		if (pCB->cb>0) {
			_cprintf("Note: Removing previous data buffer of length %d (Redirect?)\n",pCB->cb);
			pCB->op->m_handler->oph_free(pCB->data);
		}
		pCB->cb=atoi(pData+16);
		pCB->current=pCB->data=(unsigned char*) pCB->op->m_handler->oph_malloc(pCB->cb+1);
		pCB->data[pCB->cb]=0;

		// _cprintf("header: %s\n",(char*)pData);

		pCB->op->m_handler->oph_free(pData);
	} else if (!memcmp(ptr,"Set-Cookie: ",12)) {
		curl_buf_t* pCB=(curl_buf_t*) stream;
		char* pData=(char*)pCB->op->m_handler->oph_malloc(size*nmemb+1);
		memcpy(pData,ptr,size*nmemb);
		pData[size*nmemb-1]=0;

		//luaL_dofile(pCB->op->m_L,"lua/cookies.lua");
		*strchr(pData,';')=0;
		char* pszKey=pData+12;
		char* pszValue=strchr(pData,'=')+1;
		pszValue[-1]=0;
		_cprintf("Set LUA global for cookie %s=%s\n",pszKey,pszValue);
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
	_cprintf("Start Sleeping...\n");
	Sleep((DWORD)lua_tointeger(L,1));
	_cprintf("End Sleeping\n");
	return 0;
}

int OP_VeryCode(lua_State* L) {
	lua_getglobal(L,"OP_inst");

	COpenProtocol* pOP=(COpenProtocol*)lua_touserdata(L,-1);
	lua_pop(L,1);

	LPSTR pcszFile=strdup(lua_tostring(L,1));
	pOP->getHandler()->handler(OPEVENT_VERYCODE,NULL,pcszFile);

	if (strlen(pcszFile)==4) {
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
	CryptHashData(hCH,(LPCBYTE)pszData,(int)len,0);
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
	_cprintf(__FUNCTION__"(): Temp file=%s\n",szTempFile);

	lua_pushstring(L,szTempFile);
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

#define ENSURE_ARGUMENTS(x) if (lua_gettop(L)<x) { lua_pushstring(L,__FUNCTION__" called with incorrect number of arguments"); lua_error(L); return 0; }

int OP_Post(lua_State* L) {
	lua_getglobal(L,"OP_inst");

	COpenProtocol* pOP=(COpenProtocol*)lua_touserdata(L,-1);
	int fields;
	lua_pop(L,1);

	if ((fields=lua_gettop(L))<3) {
		_cprintf(__FUNCTION__"(): Incorrect number of parameters\n");
		lua_pushstring(L,"");
	} else {
		LPCSTR pcszUrl=lua_tostring(L,1);
		LPCSTR pcszReferer=lua_tostring(L,2);
		LPCSTR pcszPostdata=lua_tostring(L,3);
		int nPostSize=fields==3?(int)strlen(pcszPostdata):lua_tointeger(L,4);
		// * If nPostSize==-1, then the data is multipart/formdata, otherwise it is application/x-www-form-urlencoded
		// * if nPostSize==-1, part 5 is valid as table to get upload contents

		curl_buf_t cb={0};
		cb.op=pOP;
		
		// _cprintf(__FUNCTION__"(): url=%s referer=%s\n",pcszUrl,pcszReferer);

		initUA(L,pOP);

		CURL* pCU=newCurl(pcszUrl,pcszReferer,pOP->m_ua,pOP->m_proxytype,pOP->m_proxyurl,pOP->m_proxyuserpwd,pOP->m_curlshare,&cb);

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

		if (cb.cb==-1) cb.cb=(int)strlen((const char*)cb.data);
		lua_pushlstring(L,(const char*)cb.data,cb.cb);
		pOP->m_handler->oph_free(cb.data);
	}

	return 1;
}

int OP_Get(lua_State* L) {
	lua_getglobal(L,"OP_inst");

	COpenProtocol* pOP=(COpenProtocol*)lua_touserdata(L,-1);
	int fields;
	lua_pop(L,1);

	if ((fields=lua_gettop(L))<2) {
		_cprintf(__FUNCTION__"(): Incorrect number of parameters\n");
		lua_pushstring(L,"");
	} else {
		LPCSTR pcszUrl=lua_tostring(L,1);
		LPCSTR pcszReferer=lua_tostring(L,2);
		LPCSTR pcszCookies=(fields==2)?"":lua_tostring(L,3);

		curl_buf_t cb={0};
		cb.op=pOP;
		
		// _cprintf(__FUNCTION__"(): url=%s referer=%s\n",pcszUrl,pcszReferer);

		initUA(L,pOP);

		CURL* pCU=newCurl(pcszUrl,pcszReferer,pOP->m_ua,pOP->m_proxytype,pOP->m_proxyurl,pOP->m_proxyuserpwd,pOP->m_curlshare,&cb);
		curl_easy_setopt(pCU,CURLOPT_HTTPGET,1);
		curl_easy_perform(pCU);

		curl_easy_cleanup(pCU);

		if (cb.cb==-1) cb.cb=(int)strlen((const char*)cb.data);
		lua_pushlstring(L,(const char*)cb.data,cb.cb);
		pOP->m_handler->oph_free(cb.data);
	}

	return 1;
}

int OP_LoginSuccess(lua_State* L) {
	lua_getglobal(L,"OP_inst");

	COpenProtocol* pOP=(COpenProtocol*)lua_touserdata(L,-1);
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
		_cprintf(__FUNCTION__"(): tuin=%u %s=%s\n",tuin,lua_tostring(L,-2),lua_tostring(L,-1));
		lua_pop(L,1); // Remove value and keep key
	}

	// no need to pop! lua_pop(L,1);
	*/

	pOP->m_handler->handler(OPEVENT_CONTACTINFO,(LPCSTR)&tuin,L);

	return 0;
}

int OP_UpdateAvatar(lua_State* L) {
	ENSURE_ARGUMENTS(2);

	_cprintf(__FUNCTION__"(): Update avatar of %u to %s\n",lua_tounsigned(L,1),lua_tostring(L,2));

	return 0;
}

int OP_Group(lua_State* L) {
	ENSURE_ARGUMENTS(2);

	lua_getglobal(L,"OP_inst");

	COpenProtocol* pOP=(COpenProtocol*)lua_touserdata(L,-1);

	int index;
	_cprintf(__FUNCTION__"(): Group #%d: %s\n",index=lua_tointeger(L,1),lua_tostring(L,2));
	pOP->m_handler->handler(OPEVENT_LOCALGROUP,lua_tostring(L,2),&index);

	return 0;
}

int OP_ContactStatus(lua_State* L) {
	ENSURE_ARGUMENTS(3);

	_cprintf(__FUNCTION__"(): Contact %u changed status to %d, client_type=%d\n",lua_tounsigned(L,1),lua_tointeger(L,2),lua_tointeger(L,3));

	lua_getglobal(L,"OP_inst");

	COpenProtocol* pOP=(COpenProtocol*)lua_touserdata(L,-1);

	DWORD dwParam=MAKELPARAM(lua_tointeger(L,2),lua_tointeger(L,3));
	DWORD dwTUIN=lua_tounsigned(L,1);
	pOP->m_handler->handler(OPEVENT_CONTACTSTATUS,(LPCSTR)&dwTUIN,&dwParam);

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

void COpenProtocol::_start() {
	int ret;

	lua_pushinteger(m_L,m_initstatus);
	lua_setglobal(m_L,"OP_status");

	

	while (luaL_dofile(m_L,m_definitionFile)) {
		char* pszError;
		_cprintf("err: %s\n",pszError=m_handler->oph_strdup(lua_tostring(m_L,-1)));
		lua_pop(m_L,1);

		if (!m_handler->handler(OPEVENT_ERROR,pszError,NULL)) {
			// Don't continue
			_cprintf("Breaking loop due to error\n");
			m_handler->oph_free(pszError);
			break;
		}

		_cprintf("Non-vital error, resume loop\n");
		m_handler->oph_free(pszError);

		/*
		LPWSTR pszMsg=(LPWSTR)m_handler->oph_malloc(strlen(pcszError)*2);
		MultiByteToWideChar(CP_UTF8,0,pcszError,-1,pszMsg,(int)strlen(pcszError));
		MessageBox(NULL,pszMsg,NULL,MB_ICONERROR);
		*/
	}

	_cprintf("End of start function\n");
}

///
void COpenProtocol::_def_precheck() {
	struct _stat st;
	ZEROASSERTDEFAULT(_stat(m_definitionFile,&st));
}

void COpenProtocol::test() {
	_cprintf(__FUNCTION__ "(): m_L=%p\n",m_L);
}

COpenProtocol::COpenProtocol(LPCSTR pszDefinitionFile, COpenProtocolHandler* handler):
m_debugMsgBuffer(NULL), m_handler(handler), 
m_ua(NULL), m_uin(NULL), m_qunimagepath(NULL), 
m_initstatus(40071), m_avatarpath(NULL),
m_proxytype(-1), m_proxyurl(NULL),
m_proxyuserpwd(NULL)
{
	if (handler==NULL) {
		_cprintf(__FUNCTION__ ": handler==NULL!\n");
		DebugBreak();
	}

	NULLASSERTDEFAULT(pszDefinitionFile);

	m_handler->m_protocol=this;
	m_definitionFile=handler->oph_strdup(pszDefinitionFile);
	print_debug(__FUNCTION__ "(): Definition file/dir=%s",m_definitionFile);

	_def_precheck();

	m_curlshare=curl_share_init();
	curl_share_setopt(m_curlshare,CURLSHOPT_SHARE,CURL_LOCK_DATA_DNS);
	curl_share_setopt(m_curlshare,CURLSHOPT_SHARE,CURL_LOCK_DATA_COOKIE);
	curl_share_setopt(m_curlshare,CURLSHOPT_USERDATA,this);
	curl_share_setopt(m_curlshare,CURLSHOPT_LOCKFUNC,lock_function);
	curl_share_setopt(m_curlshare,CURLSHOPT_UNLOCKFUNC,unlock_function);

	m_curlmutex=CreateMutex(NULL,FALSE,NULL);
	m_luamutex=CreateMutex(NULL,FALSE,NULL);
	m_qunimagemutex=CreateMutex(NULL,FALSE,NULL);

	if (s_instances.empty()) {
		s_firstInstance=this;
		s_instances.push_back(this);
	}

	m_L=luaL_newstate();
	
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

	curl_global_init(/*CURL_GLOBAL_NOTHING*/ CURL_GLOBAL_WIN32);

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
	if (m_debugMsgBuffer) {
		m_handler->oph_free(m_debugMsgBuffer);
	}

	if (m_curlshare) {
		curl_share_cleanup(m_curlshare);
	}

	if (m_curlmutex) {
		CloseHandle(m_curlmutex);
	}

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
		lua_close(m_L);
	}

	curl_global_cleanup();

	s_instances.remove(this);
	if (s_firstInstance==this) s_firstInstance=NULL;
}

void COpenProtocol::callFunction(LPCSTR pcszName, LPCSTR pcszArgs, BOOL fNeedMutex) {
	char szThreadName[64];
	lua_State* L=m_L;
	int ret;

	if (fNeedMutex) {
		sprintf(szThreadName,"T_%u_%u",GetCurrentThreadId(),GetTickCount());
		WaitForSingleObject(m_luamutex,INFINITE);
		L=lua_newthread(m_L);
		lua_pushthread(L);
		lua_setglobal(m_L,szThreadName);
		ReleaseMutex(m_luamutex);
	}

	lua_getglobal(L,pcszName);
	if (pcszArgs) lua_pushstring(L,pcszArgs);
	if (lua_pcall(L,pcszArgs?1:0,0,0)!=0) {
		char* pszError;
		_cprintf("err: %s\n",pszError=m_handler->oph_strdup(lua_tostring(L,-1)));
		lua_pop(L,1);

		m_handler->handler(OPEVENT_ERROR,pszError,""); // Empty string allows error to continue
		m_handler->oph_free(pszError);
	}

	if (fNeedMutex) {
		WaitForSingleObject(m_luamutex,INFINITE);
		lua_pushnil(m_L);
		lua_setglobal(m_L,szThreadName);
		ReleaseMutex(m_luamutex);
	}
}

void COpenProtocol::setProxy(int type, LPCSTR pcszProxy, LPCSTR pcszUserPwd) {
	if (m_proxyurl) m_handler->oph_free(m_proxyurl);
	if (m_proxyuserpwd) m_handler->oph_free(m_proxyuserpwd);
	m_proxytype=type;
	m_proxyurl=m_handler->oph_strdup(pcszProxy);
	m_proxyuserpwd=(pcszUserPwd && *pcszUserPwd)?m_handler->oph_strdup(pcszUserPwd):NULL;
}
