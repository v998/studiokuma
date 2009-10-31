#pragma message("+ MyQQ API")
extern "C" {

#pragma warning(disable:4819)
#include "qqclient.h"
#include "qqcrypt.h"
#include "qun.h"
#pragma warning(default:4819)
// packetmgr.cpp
#define buddy_msg_callback qq->mimnetwork->_buddyMsgCallback
#define qun_msg_callback qq->mimnetwork->_qunMsgCallback
// md5 wrappers
/*
#define md5_init(x) mir_md5_init(x)
#define md5_state_t mir_md5_state_t
#define md5_append(x,y,z) mir_md5_append(x,y,z)
#define md5_byte_t mir_md5_byte_t
#define md5_finish(x,y) mir_md5_finish(x,y)
*/
int handle_packet( qqclient* qq, qqpacket* p, uchar* data, int len );
}

#pragma message("+ libeva2")
#include "libeva2/evadefines.h"
#include "libeva2/evapacket.h"
#include "libeva2/evautil.h"
#include "libeva2/evaimreceive.h"
#include "libeva2/evaimsend.h"
#include "libeva2/evaqun.h"
#include "libeva2/evaonlinestatus.h"
#include "libeva2/evaweather.h"
#include "libeva2/evagroup.h"
#include "libeva2/evaextrainfo.h"
#include "libeva2/evauserinfo.h"
#include "libeva2/evasearchuser.h"
#include "libeva2/evaaddfriend.h"
#include "libeva2/evaaddfriendex.h"
/*
#pragma message("+ libeva")
#pragma warning(disable:4520) // evalogin.h multiple default constructors
#include "libeva.h"
#pragma warning(default:4520)
*/
#include "libeva2/libuh/evauhpacket.h"
#include "libeva2/libuh/evauhprotocols.h"
#include "libeva2/libcustompic/evapicpacket.h"
#include "libeva2/libcustompic/evarequestface.h"
#include "libeva2/libcustompic/evarequestagent.h"
#include "libeva2/libcustompic/evarequeststart.h"
#include "libeva2/libcustompic/evatransfer.h"

#pragma message("+ EVA API")
#include "evahtmlparser.h"
#include "evaqtutil.h"
/*
#include "evautil.h"
#include "evaipaddress.h"
#include "evaipseeker.h"
*/

#pragma message("+ MIMQQ2 API")
#include "socket.h"
#include "network.h"
#include "userhead.h"
#include "qunimageserver.h"
#include "qunimage.h"
// #include "EvaAccountSwitcher.h"

/*
#pragma message("qqapi.evafiledownloader")
#include "filetrans/evafiledownloader.h"
#pragma message("qqapi.evafilemanager")
#include "filetrans/evafilemanager.h"
#pragma message("qqapi.evacachedfile")
#include "filetrans/evacachedfile.h"
#pragma message("qqapi.evaftprotocols")
#include "libft/evaftprotocols.h" // This is in libeva! Take care!
*/
