#define WEBQQ_CALLBACK_CHANGESTATUS 0xfff00001
#define WEBQQ_CALLBACK_DEBUGMESSAGE 0xfff00002
#define WEBQQ_CALLBACK_NEEDVERIFY   0xfff00003
#define WEBQQ_CALLBACK_LOGINFAIL    0xfff00004
#define WEBQQ_CALLBACK_QUNIMGUPLOAD 0xfff00005
#define WEBQQ_CALLBACK_CRASH        0xfff000ff
#define WEBQQ_CALLBACK_WEB2	        0xfff00006
#define WEBQQ_CALLBACK_WEB2_ERROR   0xfff00007
#define WEBQQ_CALLBACK_WEB2P2PIMGUPLOAD 0xfff00008

#define WEBQQ_STORAGE_COOKIE 0
#define WEBQQ_STORAGE_PARAMS 1
#define WEBQQ_STORAGE_QUNSIG 2
#define WEBQQ_WEB2_STORAGE_LOGININFO 0
#define WEBQQ_WEB2_STORAGE_QUNKEY    1

#define WEBQQ_CMD_RESET_LOGIN 0x01
#define WEBQQ_CMD_GET_GROUP_INFO 0x3c
#define WEBQQ_CMD_GET_LIST_INFO  0x58
#define WEBQQ_CMD_GET_NICK_INFO  0x26
#define WEBQQ_CMD_GET_REMARK_INFO 0x3e
#define WEBQQ_CMD_GET_HEAD_INFO 0x65
#define WEBQQ_CMD_GET_CLASS_SIG_INFO 0x1d
#define WEBQQ_CMD_CLASS_DATA 0x30
#define WEBQQ_CMD_GET_LEVEL_INFO 0x5c
#define WEBQQ_CMD_GET_SIGNATURE_INFO 0x67
#define WEBQQ_CMD_GET_USER_INFO 0x06
#define WEBQQ_CMD_GET_MESSAGE 0x17
#define WEBQQ_CMD_CONTACT_STATUS 0x81
#define WEBQQ_CMD_GET_CLASS_MEMBER_NICKS 0x0126
#define WEBQQ_CMD_SET_STATUS_RESULT 0x0d
#define WEBQQ_CMD_SEND_C2C_MESSAGE_RESULT 0x16
#define WEBQQ_CMD_SYSTEM_MESSAGE 0x80

#define SC0X22_RESULT  0
#define SC0X22_WEB_SESSION 1
#define SC0X22_ONLINE_STAT 2 /* 0=online !0=invisible */
#define SC0X22_LOGIN_TIME 3
#define SC0X22_SVRINDEX_AND_PORT_0 4
#define SC0X22_SVRINDEX_AND_PORT_1 5
#define SC0X22_MODIFY_PORTRAIT_CLIENT_KEY 6
#define SC0X22_WQQS 7
#define SC0X3C_LENGTH 0

#define WEBQQ_LOGIN_RESULT_OK 0
#define WEBQQ_LOGIN_RESULT_CONNECTION_ERROR 2
#define WEBQQ_LOGIN_RESULT_PRIVILEGE_ERROR  4
#define WEBQQ_LOGIN_RESULT_PASSWORD_ERROR   5
#define WEBQQ_LOGIN_RESULT_ERROR_SEE_TEXT   6

#define WEBQQ_USER_STATUS_ONLINE 10
#define WEBQQ_USER_STATUS_OFFLINE 20
#define WEBQQ_USER_STATUS_AWAY 30

#define WEBQQ_MESSAGE_TYPE_CLASS	0x2b
#define WEBQQ_MESSAGE_TYPE_CONTACT1	0x09
#define WEBQQ_MESSAGE_TYPE_CONTACT2	0x0a
#define WEBQQ_MESSAGE_TYPE_ERROR	0x00
#define WEBQQ_MESSAGE_TYPE_FORCE_DISCONNECT	0x30
#define WEBQQ_MESSAGE_TYPE_MAGICAL_EMOTION	0x46
#define WEBQQ_MESSAGE_TYPE_TEMP_SESSION	0x1f

#define WEBQQ_MESSAGE_REQUEST_TEXT 0x0b
#define WEBQQ_MESSAGE_REQUEST_FILE_RECEIVE 0x81
#define WEBQQ_MESSAGE_REQUEST_FILE_APPROVE 0x83
#define WEBQQ_MESSAGE_REQUEST_FILE_REFUSE 0x85
#define WEBQQ_MESSAGE_REQUEST_FILE_SUCCESS 0x87

#define WEBQQ_MESSAGE_REFUSE_TYPE_CANCEL 1
#define WEBQQ_MESSAGE_REFUSE_TYPE_REJECT_OR_CANCEL 2
#define WEBQQ_MESSAGE_REFUSE_TYPE_FAIL 3

#define WEBQQ_CLASS_SUBCOMMAND_CLASSINFO 0x72
#define WEBQQ_CLASS_SUBCOMMAND_REMARKINFO 0x0f
#define WEBQQ_CLASS_SUBCOMMAND_CLASSMESSAGERESULT 0x0a

#define WEBQQ_TERM_STATUS_NORMAL 0
#define WEBQQ_TERM_STATUS_PHONE 1
#define WEBQQ_TERM_STATUS_WEB 2

// webqq definitions
#define FACE_MAX_INDEX 201

typedef void (* WEBQQ_CALLBACK_HUB) (LPVOID pvObject, DWORD dwCommand, LPSTR pszArgs, LPVOID pvCustom);

typedef struct {
	int result;
	LPSTR msg;
} WEBQQ_LOGININFO, *PWEBQQ_LOGININFO,*LPWEBQQ_LOGININFO;

typedef struct WEBQQ_GROUPINFO_T {
	DWORD index;
	LPSTR name;
	WEBQQ_GROUPINFO_T* next;
} WEBQQ_GROUPINFO, *PWEBQQ_GROUPINFO,*LPWEBQQ_GROUPINFO;

typedef WEBQQ_GROUPINFO_T WEBQQ_REMARKINFO, *PWEBQQ_REMARKINFO, *LPWEBQQ_REMARKINFO;
typedef WEBQQ_GROUPINFO_T WEBQQ_LONGNAMEINFO, *PWEBQQ_LONGNAMEINFO, *LPWEBQQ_LONGNAMEINFO;

typedef struct WEBQQ_LISTINFO_T {
	DWORD qqid;
	BOOL isqun;
	int group;
	int status;
	int termstatus;
	WEBQQ_LISTINFO_T* next;
} WEBQQ_LISTINFO, *PWEBQQ_LISTINFO,*LPWEBQQ_LISTINFO;

typedef struct WEBQQ_NICKINFO_T {
	DWORD qqid;
	int face;
	int age;
	BOOL male;
	int viplevel;
	LPSTR name;
	WEBQQ_NICKINFO_T* next;
} WEBQQ_NICKINFO, *PWEBQQ_NICKINFO,*LPWEBQQ_NICKINFO;

typedef struct WEBQQ_CLASSDATA_T {
	int subcommand;
	DWORD dwStub;
} WEBQQ_CLASSDATA, *PWEBQQ_CLASSDATA, *LPWEBQQ_CLASSDATA;

typedef struct WEBQQ_CLASSMEMBER_T {
	DWORD qqid;
	int status;
	DWORD n;
	DWORD D;
} WEBQQ_CLASSMEMBER, *PWEBQQ_CLASSMEMBER, *LPWEBQQ_CLASSMEMBER;

typedef struct WEBQQ_CLASSINFO_T {
	WEBQQ_CLASSDATA classdata;

	DWORD intid;
	DWORD extid;
	DWORD prop;
	DWORD creator;
	BOOL haveinfo;
	BOOL havenext;
	LPSTR name;
	LPSTR notice;
	LPSTR desc;

	WEBQQ_CLASSMEMBER members[101];
} WEBQQ_CLASSINFO, *PWEBQQ_CLASSINFO,*LPWEBQQ_CLASSINFO;

typedef struct WEBQQ_REMARK_T {
	DWORD qqid;
	LPSTR qqid_str;
	LPSTR name;
} WEBQQ_REMARK, *PWEBQQ_REMARK, *LPWEBQQ_REMARK;

typedef struct WEBQQ_CLASS_REMARKS_T {
	WEBQQ_CLASSDATA classdata;
	DWORD qunid;
	BOOL havenext;
	WEBQQ_REMARK remarks[101];
} WEBQQ_CLASS_REMARKS, *PWEBQQ_CLASS_REMARKS, *LPWEBQQ_CLASS_REMARKS;

typedef struct WEBQQ_MESSAGE_T {
	DWORD type;
	DWORD requestType;
	DWORD sender;
	DWORD sequence;
	DWORD msgseq;
	BOOL isevil;
	int size;
	BOOL bold;
	BOOL italic;
	BOOL underline;
	BOOL hasFormat;
	DWORD color;
	BOOL hasMagicalEmotion;
	DWORD timestamp;
	DWORD fileRefuseType;
	DWORD classExtID;
	DWORD classSender;
	LPSTR classSender_str;
	BOOL fileRefuseContactCancelSend;

	LPSTR M;
	LPSTR D;
	LPSTR text;
	LPSTR fileType;
	LPSTR fileID;
} WEBQQ_MESSAGE, *PWEBQQ_MESSAGE,*LPWEBQQ_MESSAGE;

typedef struct WEBQQ_CONTACT_STATUS_T {
	DWORD qqid;
	int status;
	int terminationStat;
} WEBQQ_CONTACT_STATUS, *PWEBQQ_CONTACT_STATUS, *LPWEBQQ_CONTACT_STATUS;

typedef struct WEBQQ_OUT_PACKET_T {
	DWORD type;
	DWORD created;
	int retried;
	LPSTR cmd;
} WEBQQ_OUT_PACKET, *PWEBQQ_OUT_PACKET, *LPWEBQQ_OUT_PACKET;

typedef struct WEBQQ_QUNUPLOAD_STATUS_T {
	DWORD qqid;
	int status;
	union {
		DWORD number;
		LPSTR string;
	};
} WEBQQ_QUNUPLOAD_STATUS, *PWEBQQ_QUNUPLOAD_STATUS, *LPWEBQQ_QUNUPLOAD_STATUS;

class CLibWebQQ {
public:
	CLibWebQQ(DWORD dwQQID, LPSTR pszPassword, LPVOID pvObject, WEBQQ_CALLBACK_HUB wch, HANDLE hNetlib);
	~CLibWebQQ();

	enum WEBQQSTATUSENUM {
		WEBQQ_STATUS_OFFLINE,
		WEBQQ_STATUS_PROBE,
		WEBQQ_STATUS_PREPARE,
		WEBQQ_STATUS_NEGOTIATE,
		WEBQQ_STATUS_VERIFY,
		WEBQQ_STATUS_LOGIN,
		WEBQQ_STATUS_ERROR,
		WEBQQ_STATUS_ONLINE,
		WEBQQ_STATUS_INVISIBLE,
		WEBQQ_STATUS_AWAY
	};

	enum WEBQQPROTOCOLSTATUSENUM {
		WEBQQ_PROTOCOL_STATUS_ONLINE=10,
		WEBQQ_PROTOCOL_STATUS_OFFLINE=20,
		WEBQQ_PROTOCOL_STATUS_AWAY=30,
		WEBQQ_PROTOCOL_STATUS_HIDDEN=40,
		WEBQQ_PROTOCOL_STATUS_BUSY=50,
	};

	enum WEBQQREFERERENUM {
		WEBQQ_REFERER_WEBQQ,
		WEBQQ_REFERER_PTLOGIN,
		WEBQQ_REFERER_WEBPROXY,
		WEBQQ_REFERER_MAIN,
		WEBQQ_REFERER_WEB2,
		WEBQQ_REFERER_WEB2PROXY
	};

	enum WEBQQUSERDETAILSENUM {
		WEBQQ_USER_DETAIL_UIN, // 0
		WEBQQ_USER_DETAIL_NICKNAME,
		WEBQQ_USER_DETAIL_COUNTRY,
		WEBQQ_USER_DETAIL_PROVINCE,
		WEBQQ_USER_DETAIL_POSTCODE,
		WEBQQ_USER_DETAIL_ADDRESS,
		WEBQQ_USER_DETAIL_PHONE,
		WEBQQ_USER_DETAIL_AGE,
		WEBQQ_USER_DETAIL_SEX,
		WEBQQ_USER_DETAIL_REALNAME,
		WEBQQ_USER_DETAIL_EMAIL, // 10
		WEBQQ_USER_DETAIL_PAGERPROVIDER,
		WEBQQ_USER_DETAIL_STATIONNAME,
		WEBQQ_USER_DETAIL_STATIONNO,
		WEBQQ_USER_DETAIL_PAGERNO,
		WEBQQ_USER_DETAIL_PAGERTYPE,
		WEBQQ_USER_DETAIL_OCCUPATION,
		WEBQQ_USER_DETAIL_HOMEPAGE,
		WEBQQ_USER_DETAIL_AUTHOR,
		WEBQQ_USER_DETAIL_ICQNO,
		WEBQQ_USER_DETAIL_ICQPWD, // 20
		WEBQQ_USER_DETAIL_AVATAR,
		WEBQQ_USER_DETAIL_MOBILE,
		WEBQQ_USER_DETAIL_SECRET,
		WEBQQ_USER_DETAIL_PERINFO,
		WEBQQ_USER_DETAIL_CITYNO,
		WEBQQ_USER_DETAIL_SECRETEMAIL,
		WEBQQ_USER_DETAIL_IDCARD,
		WEBQQ_USER_DETAIL_GSMTYPE,
		WEBQQ_USER_DETAIL_GSMOPENINFO,
		WEBQQ_USER_DETAIL_CONTACTOPENINFO, // 30
		WEBQQ_USER_DETAIL_COLLEGE,
		WEBQQ_USER_DETAIL_CONSTELLATION,
		WEBQQ_USER_DETAIL_SHENGXIAO,
		WEBQQ_USER_DETAIL_BLOODTYPE,
		WEBQQ_USER_DETAIL_QQLEVEL,
		WEBQQ_USER_DETAIL_UNKNOWN, // 36
		WEBQQ_USER_DETAIL_ENDOFTABLE
	};

	enum WEBQQUSERHEADENUM {
		WEBQQ_USERHEAD_USER=1,
		WEBQQ_USERHEAD_CLASS=4,
	};

	typedef struct {
		DWORD qqid;
		DWORD cmd;
		DWORD seq;
		LPSTR args;
		LPSTR next;
	} RECEIVEPACKETINFO, *PRECEIVEPACKETINFO, *LPRECEIVEPACKETINFO;

	WEBQQSTATUSENUM GetStatus() const { return m_status; }
	void SetProxy(LPSTR pszHost, LPSTR pszUser, LPSTR pszPass);
	void Start();
	void Stop();
	void CleanUp();
	LPCSTR GetAPPID() const { return m_appid; }
	const DWORD GetQQID() const { return m_qqid; }
	LPSTR GetHTMLDocument(LPCSTR pszUrl=NULL, LPCSTR pszReferer=NULL, LPDWORD pdwLength=NULL, BOOL fWeb2Assign=FALSE);
	LPCSTR GetReferer(WEBQQREFERERENUM type);
	LPSTR GetArgument(LPSTR pszArgs, int n);
	LPSTR GetCookie(LPCSTR pszName);
	LPSTR DecodeText(LPSTR pszText);
	LPSTR EncodeText(LPCSTR pszSrc, LPSTR pszDst);

	void SetOnlineStatus(WEBQQPROTOCOLSTATUSENUM newstatus);
	void GetClassMembersRemarkInfo(DWORD qunid);
	void GetNickInfo(LPDWORD qqid);
	DWORD SendContactMessage(DWORD qqid, LPCSTR message, int fontsize, LPSTR font, DWORD color, BOOL bold, BOOL italic, BOOL underline);
	DWORD SendContactMessage(DWORD qqid, WORD face, bool hasImage, JSONNODE* jnContent);
	DWORD SendClassMessage(DWORD qunid, LPCSTR message, int fontsize, LPSTR font, DWORD color, BOOL bold, BOOL italic, BOOL underline);
	DWORD SendClassMessage(DWORD intid, DWORD extid, bool hasImage, JSONNODE* jnContent);
	void AddFriendPassive(DWORD qqid, LPSTR message);
	void AttemptSendQueue();
	int FetchUserHead(DWORD qqid, WEBQQUSERHEADENUM uhtype, LPSTR saveto);
	void GetLongNames(int count, LPDWORD qqs);
	void SetLoginHide(BOOL val);
	const BOOL GetLoginHide() const { return m_loginhide; }
#if 0 // Web1
	void SendP2PRetrieveRequest(DWORD qqid, LPCSTR type);
#endif // Web1
	void UploadQunImage(HANDLE hFile, LPSTR pszFilename, DWORD respondid);
	DWORD ReserveSequence();
	LPSTR GetStorage(int index);
	void GetGroupInfo();

	void SetBasePath(LPCSTR pszPath);
	LPCSTR GetBasePath() const { return m_basepath; }
	bool GetUseWeb2() const { return m_useweb2; }
	void SetUseWeb2(bool val);
	LPCSTR GetWeb2ClientID() const { return m_web2_clientid; }
	void Test();
protected:
	DWORD ThreadProc();
private:
	static DWORD WINAPI _ThreadProc(CLibWebQQ* me);
	void SetStatus(WEBQQSTATUSENUM newstatus);
	void SendToHub(DWORD dwCommand, LPSTR pszArgs, LPVOID pvCustom=NULL);
	void Log(char *fmt,...);
	bool ProbeAppID();
	bool PrepareParams();
	bool Negotiate();
	bool Login(LPSTR pszVerify);
	void GetPasswordHash(LPCSTR pszVerifyCode, LPSTR pszOut);
	void RefreshCookie();
	DWORD AppendQuery(DWORD (*func)(CLibWebQQ*,LPSTR,LPSTR), LPSTR pszArgs="");
	bool SendQuery();
	bool ParseResponse();
	void AppendCS0x17Reply(LPSTR D, DWORD sender, DWORD seq, int type, LPSTR M);
#if 0 // Web1
	void ThreadLoop4a();
#endif // Web1
	void ThreadLoop4b();

	DWORD GetRND2();
	DWORD GetUV();
	DWORD GetRND();
	LPCSTR GetNRND();
	LPCSTR GetSSID();

#if 0 // Web1
	// SC Packets
	void sc0x22_onSuccLoginInfo(LPSTR pszArgs);
	void sc0x3c_onSuccGroupInfo(LPSTR pszArgs);
	void sc0x58_onSuccListInfo(LPSTR pszArgs);
	void sc0x26_onSuccNickInfo(LPSTR pszArgs);
	void sc0x3e_onSuccRemarkInfo(LPSTR pszArgs);
	void sc0x65_onSuccHeadInfo(LPSTR pszArgs);
	void sc0x1d_onSuccGetQunSigInfo(LPSTR pszArgs);
	void sc0x30_onClassData(LPSTR pszArgs,LPRECEIVEPACKETINFO lpRPI);
	bool sc0x17_onIMMessage(DWORD seq, LPSTR pszArgs);
	void sc0x5c_onSuccLevelInfo(LPSTR pszArgs);
	void sc0x67_onSuccLongNickInfo(LPSTR pszArgs);
	void sc0x06_onSuccUserInfo(LPSTR pszArgs);
	void sc0x0126_onSuccMemberNickInfo(LPSTR pszArgs);
	void sc0x81_onContactStatus(LPSTR pszArgs);
	void sc0x01_onResetLogin(LPSTR pszArgs);
	void sc0x80_onSystemMessage(LPSTR pszArgs);
#endif // Web1

	// web2
private:
	LPSTR PostHTMLDocument(LPCSTR pszServer, LPCSTR pszUri, LPCSTR pszReferer, LPCSTR pszPostBody, LPDWORD pdwLength);
	bool web2_channel_login();
	bool web2_channel_poll();
	bool web2_check_result(LPSTR pszCommand, LPSTR pszResult);
	bool web2_vfwebqq_request(LPSTR uri);
	bool ParseResponse4b(JSONNODE* jnResult);

public:
	void Web2UploadP2PImage(HANDLE hFile, LPSTR pszFilename, DWORD respondid);
	bool web2_channel_change_status(LPCSTR newstatus);
	bool web2_channel_get_online_buddies();
	bool web2_api_get_single_info(unsigned int tuin, JSONNODE** jnOutput=NULL);
	bool web2_api_get_friend_info(unsigned int tuin, JSONNODE** jnOutput=NULL);
	bool web2_api_get_group_name_list_mask();
	bool web2_api_get_user_friends();
	bool web2_api_get_recent_contact();
	bool web2_api_get_group_info_ext(unsigned int gcode);
	bool web2_api_get_qq_level(unsigned int tuin);
	bool web2_api_get_single_long_nick(unsigned int tuin);
	bool web2_api_add_need_verify(unsigned int tuin, int groupid, LPSTR msg, LPSTR token, JSONNODE** jnOutput=NULL);
	bool web2_api_search_qq_by_nick(LPCSTR nick, JSONNODE** jnOutput=NULL);
	bool web2_api_deny_add_request(unsigned int tuin, LPSTR msg, JSONNODE** jnOutput=NULL);
	bool web2_api_allow_and_add(unsigned int tuin, int gid, LPSTR mname="", JSONNODE** jnOutput=NULL);

	bool web2_api_get_vfwebqq2();

	JSONNODE* qun_air_search(unsigned int groupid);
	JSONNODE* qun_air_search(LPCSTR groupname);
	JSONNODE* qun_air_join(unsigned int groupid, LPCSTR reason);

	WEBQQSTATUSENUM m_status;
	HANDLE m_hEventHTML;
	HANDLE m_hEventCONN;
	LPVOID m_userobject;
	WEBQQ_CALLBACK_HUB m_wch;

	DWORD m_qqid;
	LPSTR m_password;

	DWORD m_uv;
	HINTERNET m_hInet;
	LPSTR m_buffer;
	LPSTR m_outbuffer;
	LPSTR m_outbuffer_current;
	static LPCSTR g_referer_webqq;
	CHAR m_referer_ptlogin[180];
	static LPCSTR g_referer_webproxy;
	static LPCSTR g_referer_main;
	static LPCSTR g_referer_web2;
	static LPCSTR g_referer_web2proxy;
	static LPCSTR g_domain_qq;
	static LPCSTR g_server_web2_connection;
	DWORD m_r_cookie;
	LPSTR m_appid;
	DWORD m_sequence;
	LPSTR m_storage[10];
	JSONNODE* m_web2_storage[10];
	LPSTR m_proxyhost;
	LPSTR m_proxyuser;
	LPSTR m_proxypass;
	LPSTR m_basepath;
	HINTERNET m_hInetRequest;
	BOOL m_loginhide;
	map<DWORD,LPWEBQQ_OUT_PACKET> m_outpackets;

	DWORD cs_0x26_timeout;
	DWORD m_processstatus;
	DWORD cs_0x3e_next_pos;
	DWORD cs_0x1d_next_time;

	// web2
	BOOL m_useweb2;
	CHAR m_web2_clientid[16];
	JSONNODE* m_web2_logininfo;
	LPSTR m_web2_vfwebqq;
	LPSTR m_web2_psessionid;
	time_t m_web2_nextqunkey;
	HANDLE m_hNetlib;

	BOOL m_stop;
	/*
	static DWORD cs_0x58_next_uin=0;
	static DWORD cs_0x26_next_pos=0;
	static DWORD cs_0x26_timeout=0;
	static DWORD cs_0x3e_next_pos=0;
	static unsigned int g_packetnum=0;
	*/
};