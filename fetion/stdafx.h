#pragma message("Build of Fetion Started on " __DATE__ " " __TIME__)
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
#include <windows.h>
// These two are workaround for inclusion errors in ShlObj.h
#undef __urlmon_h__
#undef INET_E_ERROR_LAST
#include <ShlObj.h>
#pragma warning(default: 4005)
#pragma message("Processing: Standard Headers")
#pragma message("Processing: STL Headers")
#if _MSC_VER < 1400
#error You must compile MirandaQQ2 using Visual Studio 2005 or above because STL in older compilers does not handle wchar_t correctly.
#endif
#include <list>
#include <map>
#include <string>
#include <time.h>
using namespace std;

#include "miranda.h"

#include "fetion.h"
#include "sipmsg.h"
#include "utils.h"
#include "socket.h"
#include "ezxml.h"
#include "network.h"
#include "f_util.h"
#include "ssl.h"
