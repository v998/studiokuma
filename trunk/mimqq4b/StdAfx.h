#pragma warning(disable: 4651 4996 4311 4312)
#define _CRT_SECURE_NO_WARNINGS
#ifndef STDAFX_CPP
#error OOPS! You forgot to set __FILE__ to use precompiled headers!
#endif

#if _MSC_VER < 1400
#error You must compile MirandaQQ4 using Visual Studio 2005 or above because STL in older compilers does not handle wchar_t correctly.
#endif

#ifndef _UNICODE
#error This version of MirandaQQ must be built with Unicode enabled
#endif

#ifdef _DEBUG
#define TESTSERVICE
#endif

#pragma message("Build of MirandaQQ4 Started on " __DATE__ " " __TIME__)
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

#ifdef MIRANDAQQ_IPC
#error OOPS! This version of MirandaQQ does not support IPC!
#pragma message("Compiling with IPC support: Yes")
#else
#pragma message("Compiling with IPC support: No")
#endif

#ifdef TESTSERVICE
#pragma message("*** Compiling with Test Service: Yes")
#endif

#pragma message("==============================")
#pragma message("Processing Precompiled Headers")

#pragma message("Processing: Windows Headers")
#include <windows.h>
#pragma message("Processing: Standard Headers")
#include <time.h>
#pragma message("Processing: STL Headers")
#include <list>
#include <stack>
#include <map>
#include <string>
#include <vector>
using namespace std;

//Miranda SDK headers
#pragma message("Processing: Miranda Headers")
// What the hell? 0x0900 always causing problems even in MIM9.0!
#define MIRANDA_VER    0x0900
#include "include/newpluginapi.h"
#include "include/m_assocmgr.h"
// #include "m_chat.h"
#pragma warning(disable: 4819)
#include "include/m_clist.h"
#pragma warning(default: 4819)
#include "include/m_clistint.h"

#include "include/m_protocols.h"
#include "include/m_protomod.h"
#include "include/m_protosvc.h"
#include "include/m_protoint.h"	// Must place after m_protosvc.h

#include "include/m_fontservice.h"
#include "include/m_ieview.h"
#include "include/m_langpack.h"
#include "include/m_message.h"
#include "include/m_netlib.h"
#include "include/m_options.h"
#include "include/m_popup.h"
//#include "include/m_proto_listeningto.h"
//#include "include/m_updater.h"
#include "include/m_userinfo.h"
#include "include/m_utils.h"
#include "include/m_system.h"
#include "include/m_database.h" // Some inline functions are only activated when m_utils.h and m_system.h are loaded
#include "include/m_folders.h"
#include "include/m_avatars.h"

#include "include/m_libJSON.h"

#ifdef MIRANDAQQ_IPC
#pragma message("Processing: IPC Header")
#ifndef _DEBUG
#error You cannot build MirandaQQ2 Release with MIRANDAQQ_IPC defined due to unsolved memory block incompatibility problem.
#endif
#include "ipc.h"
#endif

#pragma message("Processing: QQAPI")
#include <Wininet.h>
#include <Shlwapi.h>
#include <math.h>

// #include "libJSON.h"
// #pragma comment(lib,"libJSON.lib")
#pragma comment(lib,"shlwapi")

//#include "json.h"
#include "httpclient.h"
#include "webqq2.h"

#include "libwebqq.h"

#pragma message("Processing: Local Headers")
#include "version.h"
#pragma message("+ MirandaQQ4 Version " VERSION_DOT)
#include "resource.h"
#include "mimqq4.h"
#include "Protocol.h"
#include "httpserver.h"
#include "utils.h"
#include "codeverify.h"

#pragma warning(disable:4535)
#pragma warning(disable:4309 4800)

#pragma message("Precompiled Headers Processing Completed")
#pragma message("========================================")
