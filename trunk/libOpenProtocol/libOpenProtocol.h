#ifndef OPENPROTOCOL_H
#define OPENPROTOCOL_H

#define OP_SHOWDEBUGMSG

#define OP_DEBUGMSGSIZE 4096
#define OP_ASSERTACTION DebugBreak();

#define THREAD_COUNT 5
#define THREAD_AVATAR 0
#define THREAD_QUNIMAGE 1
#define THREAD_SENDMESSAGE 2
#define THREAD_GENERIC 3
#define THREAD_INNER 4
// #define THREAD_GLOBAL 5

#include <string.h>
#include <conio.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>
#include <process.h>
#include <map>
#include <list>
using namespace std;

#ifdef WIN32
#include <windows.h>
#include <wincrypt.h>
#else
#include <malloc.h>
#endif
/*
extern "C" {
#include "lua/lua.h"
#include "lua/lualib.h"
#include "lua/lauxlib.h"
}
*/
extern "C" {
#include "lua52/lua.h"
#include "lua52/lauxlib.h"
#include "lua52/lualib.h"
// #define lua_tounsigned(a,b) ((unsigned int)lua_tonumber(a,b))
#pragma comment(lib,"lua52/lua52.lib")
}

#ifndef WIN32
#define LIBCURL
#define CURL_STATICLIB
#include "libcurl/include/curl/curl.h"
#endif

#ifndef LPSTR
#define LPSTR char*
#define LPCSTR const LPSTR
#define LPWSTR wchar_t*
#define LPCWSTR const LPWSTR
#define LPVOID void*
#define LPCVOID const LPVOID
#define LPBYTE unsigned char*
#define LPCBYTE const LPBYTE
#define MAX_PATH 260
#endif

#define OPEVENT_LOGINSUCCESS 1
#define OPEVENT_ERROR 2
#define OPEVENT_LOCALGROUP 3
#define OPEVENT_CONTACTINFO 4
#define OPEVENT_CONTACTSTATUS 5
#define OPEVENT_GROUPMESSAGE 6
#define OPEVENT_CONTACTMESSAGE 7
#define OPEVENT_GROUPINFO 8
#define OPEVENT_GROUPMEMBERS 9
#define OPEVENT_VERYCODE 10
#define OPEVENT_TYPINGNOTIFY 11
#define OPEVENT_ADDTEMPCONTACT 12
#define OPEVENT_SESSIONMESSAGE 13
#define OPEVENT_ADDSEARCHRESULT 14
#define OPEVENT_ENDOFSEARCH 15
#define OPEVENT_REQUESTJOIN 16

class COpenProtocol;

class COpenProtocolHandler {
public:
	/*
	COpenProtocolHandler();
	~COpenProtocolHandler();
	*/

	virtual LPSTR oph_strdup(LPCSTR pcszStr)=0;
	virtual LPVOID oph_malloc(int cbAllocation)=0;
	virtual void oph_free(LPVOID pData)=0;

	virtual void oph_printdebug(LPCSTR pcszStr)=0;
	virtual void oph_pthread_create(void(*start_routine)(LPVOID), LPVOID arg)=0;
	virtual int handler(int nEvent, LPCSTR pcszStatus, LPCVOID pAux)=0;

	COpenProtocol* m_protocol;
};

class COpenProtocol {
public:
	COpenProtocol(LPCSTR pcszDefinitionFile, COpenProtocolHandler* handler);
	~COpenProtocol();
	static COpenProtocol* FindProtocol(const char* pcszUIN);

	void print_debug(LPCSTR pcszFormat,...);
	void start();
	void stop();
	COpenProtocolHandler* getHandler() { return m_handler; }
	LPCSTR getStringValue(LPCSTR pcszName);
	void setStringValue(LPCSTR pcszName, LPCSTR pcszValue, DWORD dwLen=-1);
	FILE* handleQunImage(LPCSTR pcszUri, BOOL isP2P);
	// void callFunction(LPCSTR pcszName, LPCSTR pcszArgs=NULL, BOOL fNeedMutex=FALSE);
	void callFunction(lua_State* L, LPCSTR pcszName, LPCSTR pcszArgs=NULL);
	void signalInterrupt();

	void setLogin(LPCSTR pcszUIN, LPCSTR pcszPassword);
	void setQunImagePath(LPCSTR pcszPath, int port);
	void setAvatarPath(LPCSTR pcszPath);
	void setBBCode(bool yn);
	void setInitStatus(int status);
	void setProxy(int type, LPCSTR pcszProxy,LPCSTR pcszUserPwd);

	void _start();

	void test();

	lua_State* m_L;
#ifdef LIBCURL
	CURLSH* m_curlshare;
	HANDLE m_curlmutex;
#endif
	HANDLE m_luamutex;
	HANDLE m_qunimagemutex;
	LPSTR m_ua;
	int m_proxytype;
	LPSTR m_proxyurl;
	LPSTR m_proxyuserpwd;

	COpenProtocolHandler* m_handler;

	lua_State* m_threads[THREAD_COUNT];
private:
	void _def_precheck();

	LPSTR m_debugMsgBuffer;

	LPSTR m_definitionFile;
	LPSTR m_uin;
	LPSTR m_qunimagepath;
	LPSTR m_avatarpath;
	int m_initstatus;
};

#include "httpserver.h"

#endif // OPENPROTOCOL_H
