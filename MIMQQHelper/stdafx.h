#pragma once

#pragma message("Build of MirandaQQ_IPC Started on " __DATE__ " " __TIME__)
#pragma message("====================")
#pragma message("Build Configuration:")

#ifdef __STDC__
#pragma message("ANSI C Full Conformance: Yes")
#else
#pragma message("ANSI C Full Conformance: No")
#endif

#ifdef _CHAR_UNSIGNED
#pragma message("char Type is Unsigned: Yes")
#else
#pragma message("char Type is Unsigned: No")
#endif

#ifdef _CPPRTTI
#pragma message("Run-Time Type Information: Enabled")
#else
#pragma message("Run-Time Type Information: Disabled")
#endif

#ifdef _CPPUNWIND
#pragma message("Exception Handling: Enabled")
#else
#pragma message("Exception Handling: Disabled")
#endif

#ifdef _DEBUG
#pragma message("Debug Mode: Yes")
#else
#pragma message("Debug Mode: No")
#endif

#ifdef _MT
#pragma message("MT DLL: Yes")
#else
#pragma message("MT DLL: No")
#endif

#ifdef _WIN64
#pragma message("Compiling for WIN64: Yes")
#else
#pragma message("Compiling for WIN64: No")
#endif

#pragma message("==============================")
#pragma message("Processing Precompiled Headers")

#pragma message("Processing: Windows Headers")
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <CommDlg.h>

#pragma message("Processing: Standard Headers")
#include <stdlib.h>
#include <time.h>

#pragma message("Processing: STL Headers")
#if _MSC_VER < 1400
#error You must compile MirandaQQ2 using Visual Studio 2005 or above because STL in older compilers does not handle wchar_t correctly.
#endif
#include <string>
#include <list>
#include <map>
using namespace std;

//Miranda SDK headers
#pragma message("Processing: Miranda Headers")
#pragma warning(disable:4819)
#include "newpluginapi.h"
#include "m_chat.h"
#include "m_clist.h"
#include "m_database.h"
#include "m_langpack.h"
#include "m_message.h"
#include "m_options.h"
#include "m_protomod.h"
#include "m_protosvc.h"
#include "m_utils.h"
#pragma warning(default:4819)

#pragma message("Processing: IPC.h")
#ifndef _DEBUG
#error You cannot build MirandaQQ_IPC Release due to unsolved memory block incompatibility problem.
#endif
#include <ipc.h>

#pragma message("Processing: Local Headers")
#include "version.h"
#include "resource.h"
#include "cole/cole.h"

extern const LPSTR szMIMQQError;

#define WRITE_B(c,k,v) DBWriteContactSettingByte(c,szMIMQQ,k,v)
#define WRITE_W(c,k,v) DBWriteContactSettingWord(c,szMIMQQ,k,v)
#define WRITE_D(c,k,v) DBWriteContactSettingDword(c,szMIMQQ,k,v)
#define WRITEC_S(k,v) WRITE_S(hContact,k,v)
#define WRITEC_TS(k,v) WRITE_TS(hContact,k,v)
#define WRITEC_B(k,v) WRITE_B(hContact,k,v)
#define WRITEC_W(k,v) WRITE_W(hContact,k,v)
#define WRITEC_D(k,v) WRITE_D(hContact,k,v)
#define WRITEINFO_S(k,i) WRITEC_S(k,info.at(i).c_str())
#define WRITEINFO_TS(k,i) util_convertToNative(&pszTemp,info.at(i).c_str()); WRITEC_TS(k,pszTemp); free(pszTemp)
#define WRITEINFO_B(k,i) WRITEC_B(k,atoi(info.at(i).c_str()))
#define WRITEINFO_W(k,i) WRITEC_W(k,atoi(info.at(i).c_str()))
#define WRITEINFO_D(k,i) WRITEC_D(k,atoi(info.at(i).c_str()))

#define READ_S(c,k,v) if (!DBGetContactSetting(c,szMIMQQ,k,&dbv)) {strcpy(v,dbv.pszVal);DBFreeVariant(&dbv);} else *v=0
#define READ_S2(c,k,v) DBGetContactSetting(c,szMIMQQ,k,v)
#define READ_B2(c,k) DBGetContactSettingByte(c,szMIMQQ,k,0)
#define READ_B(c,k,v) v=DBGetContactSettingByte(c,szMIMQQ,k,0)
#define READ_W2(c,k) DBGetContactSettingWord(c,szMIMQQ,k,0)
#define READ_W(c,k,v) v=DBGetContactSettingWord(c,szMIMQQ,k,0)
#define READ_D2(c,k) DBGetContactSettingDword(c,szMIMQQ,k,0)
#define READ_D(c,k,v) v=DBGetContactSettingDword(c,szMIMQQ,k,0)
#define READ_2(c,k,v) DBGetContactSetting(c,szMIMQQ,k,v)
#define READ_TS2(c,k,v) DBGetContactSettingTString(c,szMIMQQ,k,v)
#define READC_2(c,k,v) READ_2(hContact,k,v)
#define READC_S2(k,v) READ_S2(hContact,k,v)
#define READC_TS2(k,v) READ_TS2(hContact,k,v)
#define READC_B2(k) READ_B2(hContact,k)
#define READC_B(k,v) READ_B(hContact,k,v)
#define READC_W(k,v) READ_W(hContact,k,v)
#define READC_D(k,v) READ_D(hContact,k,v)
#define READC_D2(k) READ_D2(hContact,k)
#define READC_W2(k) READ_W2(hContact,k)
#define GETPROTO() (LPSTR)CallService(MS_PROTO_GETCONTACTBASEPROTO,(WPARAM)hContact,0)
#define MIMQQ "MIRANDAQQ"
#define IPCSVC "/IPCService"

#define DM_CLOSEME		(WM_USER+1)
#define DM_UPDATETITLE	(WM_USER+16)

extern map<UINT,map<UINT,ipcmember_t>> quns;
extern map<UINT,UCHAR> onlinemembers;
HANDLE util_find_contact(const LPSTR szMIMQQ, const unsigned int QQID);

#pragma message("Processing: Qun List")
class CQunListBase {
public:
	static bool isCreated() {return m_inst!=NULL;};
	static void InstallHook(HINSTANCE);
	static void RemoveHook();
	static int getQunid();
	static void DeleteInstance();
	static CQunListBase* getInstance();
	virtual void QunOnline(HANDLE hContact) {}
	virtual void MessageReceived(ipcmessage_t* ipcm) {}
	virtual void NamesUpdated(ipcmembers_t* ipcms) {}
	virtual void OnlineMembersUpdated(ipconlinemembers_t* ipcms) {}
	virtual void MessageSent(HANDLE hContact) {}
protected:
	CQunListBase();
	virtual ~CQunListBase();
	virtual void TabSwitched(CWPRETSTRUCT* cps) {}
	virtual void Hide() {}
	virtual void Show() {}
	virtual void Move() {}
	virtual void Close() {}
	static CQunListBase* m_inst;
	static int m_qunid;
	static HWND m_hwndSRMM;
	static HINSTANCE m_hInstance;
	static HANDLE m_timerEvent;
	static void TimerThread(LPVOID data);

	static HANDLE hContact;
private:
	static LRESULT CALLBACK MessageHookProcPost(int code, WPARAM wParam, LPARAM lParam);
	static HHOOK hHookMessagePost;
};
#include "SRMMQunList.h"
#include "ChatQunList.h"

#pragma message("Precompiled Headers Processing Completed")
#pragma message("========================================")
