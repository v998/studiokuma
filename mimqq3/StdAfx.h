#pragma warning(disable: 4651)
#ifndef STDAFX_CPP
#error OOPS! You forgot to set __FILE__ to use precompiled headers!
#endif

// #define MIRANDAQQ_IPC
#define TESTSERVICE

#pragma message("Build of MirandaQQ3 Started on " __DATE__ " " __TIME__)
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
#define _WIN32_WINNT 0x0400 // For TryEnterCriticalSection()
#include <windows.h>
// These two are workaround for inclusion errors in ShlObj.h
#undef __urlmon_h__
#undef INET_E_ERROR_LAST
#include <ShlObj.h>
#pragma warning(default: 4005)
#pragma message("Processing: Standard Headers")
#pragma message("Processing: STL Headers")
#if _MSC_VER < 1400
#error You must compile MirandaQQ3 using Visual Studio 2005 or above because STL in older compilers does not handle wchar_t correctly.
#endif
#include <list>
#include <map>
#include <string>
#include <fstream>	// EvaIPSeeker
using namespace std;

//Miranda SDK headers
#pragma message("Processing: Miranda Headers")
#define MIRANDA_VER    0x0800
#include "newpluginapi.h"
#include "m_assocmgr.h"
#include "m_chat.h"
#pragma warning(disable: 4819)
#include "m_clist.h"
#pragma warning(default: 4819)
#include "m_clistint.h"

#include "m_protocols.h"
#include "m_protomod.h"
#include "m_protosvc.h"
#include "m_protoint.h"	// Must place after m_protosvc.h

#include "m_fontservice.h"
#include "m_ieview.h"
#include "m_langpack.h"
#include "m_message.h"
#include "m_netlib.h"
#include "m_options.h"
#include "m_popup.h"
#include "m_proto_listeningto.h"
#include "m_updater.h"
#include "m_userinfo.h"
#include "m_utils.h"
#include "m_system.h"
#include "m_database.h" // Some inline functions are only activated when m_utils.h and m_system.h are loaded
#include "m_folders.h"

#ifdef MIRANDAQQ_IPC
#pragma message("Processing: IPC Header")
#ifndef _DEBUG
#error You cannot build MirandaQQ2 Release with MIRANDAQQ_IPC defined due to unsolved memory block incompatibility problem.
#endif
#include "ipc.h"
#endif

#pragma message("Processing: QQAPI")
#include "api/qqapi.h"

#pragma message("Processing: Local Headers")
#include "resource.h"
#include "version.h"
#pragma message("+ MirandaQQ3 Version " VERSION_DOT)
#include "MirandaQQ.h"
#include "codeverify.h"
#include "utils.h"

#pragma warning(disable:4535)
#pragma warning(disable:4309 4800)

#pragma message("Precompiled Headers Processing Completed")
#pragma message("========================================")
