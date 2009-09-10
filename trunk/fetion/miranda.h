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

#include "m_database.h"
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
// #include "m_xml.h"

#pragma message("Processing: Local Headers")
#include "resource.h"
#include "version.h"

#pragma warning(disable:4535)
#pragma warning(disable:4309 4800)

#pragma message("Precompiled Headers Processing Completed")
#pragma message("========================================")
