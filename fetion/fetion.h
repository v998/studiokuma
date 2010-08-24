/*
fetion_login -> RetriveSysCfg -> RetriveSysCfg_cb
LoginToSsiPortal -> Ssi_cb -> read_cookie -> srvresolved
*/

#define UNIQUEIDSETTING					"UID"
#define FETION_VERSION "3.5.2560" // "3.6.1900" // "3.2.0540" //"2.3.0230"
#define FETION_USER_AGENT "IIC2.0/PC " FETION_VERSION
#define SALT "13010501"
#define SALT_BIN "\x13\x01\x05\x01"
#define FETION_REGISTER_RETRY_MAX 1 /*2*/

#define FETION_REGISTER_SENT 1
#define FETION_REGISTER_RETRY 2
#define FETION_REGISTER_COMPLETE 3

#define SETTING_FORCESMS "ForceSMS"
#define CTXMENU_SMS "/ForceSMS"

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

#define READ_S(c,k,v) if (!DBGetContactSetting(c,m_szModuleName,k,&dbv)) {strcpy(v,dbv.pszVal);DBFreeVariant(&dbv);} else *v=0
#define READ_S2(c,k,v) DBGetContactSetting(c,m_szModuleName,k,v)
#define READ_B2(c,k) DBGetContactSettingByte(c,m_szModuleName,k,0)
#define READ_B(c,k,v) v=DBGetContactSettingByte(c,m_szModuleName,k,0)
#define READ_W2(c,k) DBGetContactSettingWord(c,m_szModuleName,k,0)
#define READ_W(c,k,v) v=DBGetContactSettingWord(c,m_szModuleName,k,0)
#define READ_D2(c,k) DBGetContactSettingDword(c,m_szModuleName,k,0)
#define READ_D(c,k,v) v=DBGetContactSettingDword(c,m_szModuleName,k,0)
#define READ_2(c,k,v) DBGetContactSetting(c,m_szModuleName,k,v)
#define READ_TS2(c,k,v) DBGetContactSettingTString(c,m_szModuleName,k,v)
#define READ_U8S2(c,k,v) DBGetContactSettingUTF8String(c,m_szModuleName,k,v)
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

extern HANDLE g_hNetlibUser;
extern HINSTANCE g_hInstance;

struct sip_auth {
	int type; /* 1 = Digest / 2 = NTLM */
	LPSTR nonce;
	LPSTR cnonce;
	LPSTR domain;
	LPSTR target;
	DWORD flags;
	int nc;
	LPSTR digest_session_key;
	int retries;
};

struct transaction;
class CNetwork;

typedef bool (__cdecl CNetwork::*TransCallback) (struct sipmsg *, struct transaction *);
struct transaction {
	time_t time;
	int retries;
	int transport; /* 0 = tcp, 1 = udp */
	int fd;
	LPCSTR cseq;
	struct sipmsg *msg;
	TransCallback callback;
};

struct sip_dialog {
	LPSTR ourtag;
	LPSTR theirtag;
	LPSTR callid;
};

struct group_chat {
	int chatid;
	LPSTR callid;
	LPSTR groupname;
	LPSTR conv;
};
