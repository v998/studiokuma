#ifndef IPC_H
#define IPC_H

//#define QQIPCSRV_REQUEST_MEMBERS	1
#define QQIPCSVC_QUN_UPDATE_ONLINE_MEMBERS	1
#define QQIPCSVC_QUN_SEND_MESSAGE			2
#define QQIPCSVC_QUN_UPDATE_INFORMATION		3
#define QQIPCSVC_FIND_CONTACT				4
#define QQIPCSVC_SHOW_DETAILS				5
#define QQIPCSVC_QUN_KICK_USER				6
#define QQIPCSVC_GET_NETWORK				7

#define QQIPCEVT_RECEIVE_MEMBERS	1
#define QQIPCEVT_QUN_ONLINE			2
#define QQIPCEVT_QUN_REFRESHINFO	5
#define QQIPCEVT_PROTOCOL_OFFLINE	3
#define QQIPCEVT_QUN_REFRESH		4
#define QQIPCEVT_LOGGED_IN			6
#define QQIPCEVT_RECV_MESSAGE		7
#define QQIPCEVT_QUN_UPDATE_NAMES	8
#define QQIPCEVT_QUN_UPDATE_ONLINE_MEMBERS	9
#define QQIPCEVT_QUN_MESSAGE_SENT	10
#define QQIPCEVT_CREATE_OPTION_PAGE	11
#define QQIPCEVT_CREATE_MAIN_MENU	12

#define IPCMFLAG_EXISTS		(1<<0)
#define IPCMFLAG_ONLINE		(1<<1)
#define IPCMFLAG_CREATOR	(1<<2)
#define IPCMFLAG_MODERATOR	(1<<3)
#define IPCMFLAG_INVESTOR	(1<<4)
/*
typedef struct {
unsigned int memberid;
unsigned char flag;
} qunmemberinfo_t;

typedef struct {
unsigned int qunid;
qunmemberinfo_t* qmi;
} qunmemberinfo_outter_t;
*/

typedef struct {
	HANDLE hContact;
	UINT qunid;
	time_t time;
	LPWSTR message;
} ipcmessage_t;

typedef struct {
	UINT qqid;
	USHORT face;
	UCHAR flag;
	string name; // in CP936
} ipcmember_t;

typedef struct {
	HANDLE hContact;
	UINT qunid;
	list<ipcmember_t> members;
} ipcmembers_t;

typedef struct {
	HANDLE hContact;
	UINT qunid;
	list<UINT> members;
} ipconlinemembers_t;

typedef struct {
	UINT qunid;
	LPWSTR message;
} ipcsendmessage_t;
#endif
