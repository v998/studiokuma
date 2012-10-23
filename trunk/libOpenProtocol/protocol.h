#ifndef PROTOCOL_H
#define PROTOCOL_H

#define TESTSERVICE

// 20101222-built
#define VERSION_COMMA 0,0,0,1
#define VERSION_DOT "0,0,0,1"
#define PLUGINVERSION PLUGIN_MAKE_VERSION(0,0,0,1)

#if _MSC_VER < 1400
#error You must compile MirandaQQ4 using Visual Studio 2005 or above because STL in older compilers does not handle wchar_t correctly.
#endif

#ifndef _UNICODE
#error This version of MirandaQQ must be built with Unicode enabled
#endif

#pragma message("Build of libOpenProtocol Started on " __DATE__ " " __TIME__)
#pragma message("====================")
#pragma message("Build Configuration:")

#ifdef _DEBUG
#pragma message("Debug Mode: Yes")
#else
#pragma message("Debug Mode: No")
#endif

#ifdef _WIN64
#pragma message("Compiling for WIN64: Yes")
#else
#pragma message("Compiling for WIN64: No")
#endif

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

#include "resource.h"

// Database helper functions
#define WRITE_S(c,k,v) DBWriteContactSettingString(c,m_szModuleName,k,v)
#define WRITE_TS(c,k,v) DBWriteContactSettingTString(c,m_szModuleName,k,v)
#define WRITE_U8S(c,k,v) DBWriteContactSettingUTF8String(c,m_szModuleName,k,v)
#define WRITE_B(c,k,v) DBWriteContactSettingByte(c,m_szModuleName,k,v)
#define WRITE_W(c,k,v) DBWriteContactSettingWord(c,m_szModuleName,k,v)
#define WRITE_D(c,k,v) DBWriteContactSettingDword(c,m_szModuleName,k,v)
#define WRITEC_S(k,v) WRITE_S(hContact,k,v)
#define WRITEC_TS(k,v) WRITE_TS(hContact,k,v)
#define WRITEC_U8S(k,v) WRITE_U8S(hContact,k,v)
#define WRITEC_B(k,v) WRITE_B(hContact,k,v)
#define WRITEC_W(k,v) WRITE_W(hContact,k,v)
#define WRITEC_D(k,v) WRITE_D(hContact,k,v)
#define WRITEINFO_S(k,i) WRITEC_S(k,info.at(i).c_str())
//#define WRITEINFO_TS(k,i) pszTemp=mir_a2u_cp(info.at(i).c_str(),936); WRITEC_TS(k,pszTemp); mir_free(pszTemp)
#define WRITEINFO_U8S(k,i) if (info.at(i)) WRITEC_U8S(k,info.at(i)+2)
#define WRITEINFO_B(k,i) if (info.at(i)) WRITEC_B(k,*(unsigned char*)(info.at(i)+2))
#define WRITEINFO_W(k,i) if (info.at(i)) WRITEC_W(k,htons(*(unsigned short*)(info.at(i)+2)))
#define WRITEINFO_D(k,i) if (info.at(i)) WRITEC_D(k,htonl(*(unsigned int*)(info.at(i)+2)))

#define READ_S(c,k,v) if (!DBGetContactSetting(c,m_szModuleName,k,&dbv)) {strcpy(v,dbv.pszVal);DBFreeVariant(&dbv);} else *v=0
#define READ_S2(c,k,v) DBGetContactSetting(c,m_szModuleName,k,v)
#define READ_TS2(c,k,v) DBGetContactSettingTString(c,m_szModuleName,k,v)
#define READ_U8S2(c,k,v) DBGetContactSettingUTF8String(c,m_szModuleName,k,v)
#define READ_B2(c,k) DBGetContactSettingByte(c,m_szModuleName,k,0)
#define READ_B(c,k,v) v=DBGetContactSettingByte(c,m_szModuleName,k,0)
#define READ_W2(c,k) DBGetContactSettingWord(c,m_szModuleName,k,0)
#define READ_W(c,k,v) v=DBGetContactSettingWord(c,m_szModuleName,k,0)
#define READ_D2(c,k) DBGetContactSettingDword(c,m_szModuleName,k,0)
#define READ_D(c,k,v) v=DBGetContactSettingDword(c,m_szModuleName,k,0)
#define READ_2(c,k,v) DBGetContactSetting(c,m_szModuleName,k,v)
#define READC_2(c,k,v) READ_2(hContact,k,v)
#define READC_S2(k,v) READ_S2(hContact,k,v)
#define READC_TS2(k,v) READ_TS2(hContact,k,v)
#define READC_U8S2(k,v) READ_U8S2(hContact,k,v)
#define READC_B2(k) READ_B2(hContact,k)
#define READC_B(k,v) READ_B(hContact,k,v)
#define READC_W(k,v) READ_W(hContact,k,v)
#define READC_D(k,v) READ_D(hContact,k,v)
#define READC_D2(k) READ_D2(hContact,k)
#define READC_W2(k) READ_W2(hContact,k)

#define DELC(k) DBDeleteContactSetting(hContact,m_szModuleName,k)

// Database entries
#define KEY_UIN "UIN"
#define KEY_TUIN "TUIN"
#define KEY_FACE "Face"
#define KEY_FLAG "Flag"
#define KEY_NICK "Nick"
#define KEY_STATUS "Status"
#define KEY_BOOTSTRAP "Bootstrap"
#define VAL_BOOTSTRAP "lua\\qq\\init.lua"
#define KEY_PW "Password"
#define KEY_RECEIVEGCIMAGES "ReceiveGCImages"
#define VAL_RECEIVEGCIMAGES TRUE
#define KEY_SERVERPORT "ServerPort"
#define VAL_SERVERPORT 170
#define KEY_SHOWCONSOLE "ShowConsole"

#define CKEY_REALUIN "RealUIN" /* Without this key, the contact will be removed */

#endif // PROTOCOL_H
