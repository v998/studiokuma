#pragma message("+ MyQQ API")
extern "C" {

#pragma warning(disable:4819)
#include "qqclient.h"
#include "qqcrypt.h"
#include "qun.h"
#include "group.h"
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
#define md5_INCLUDED
typedef mir_md5_state_t md5_state_t;
typedef unsigned char mir_md5_byte_t;
typedef mir_md5_byte_t md5_byte_t;

static __inline void md5_init(mir_md5_state_t *pms) {
	md5i.md5_init(pms);
}

static __inline void md5_append(mir_md5_state_t *pms, const mir_md5_byte_t *data, int nbytes) {
	md5i.md5_append(pms,data,nbytes);
}

static __inline void md5_finish(mir_md5_state_t *pms, mir_md5_byte_t digest[16]) {
	md5i.md5_finish(pms,digest);
}

int handle_packet( qqclient* qq, qqpacket* p, uchar* data, int len );
}

#pragma message("+ libeva2")
#include "libeva2/evadefines.h"
// #include "libeva2/evapacket.h"
#include "libeva2/evautil.h"
// #include "libeva2/evaimreceive.h"
// #include "libeva2/evaimsend.h"
// #include "libeva2/evaqun.h"
// #include "libeva2/evaweather.h"
// #include "libeva2/evagroup.h"
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

#pragma message("+ MIMQQ2 API")
#include "socket.h"
#include "network.h"
#include "userhead.h"
#include "qunimageserver.h"
#include "qunimage.h"
