#define WEBQQ_CALLBACK_CHANGESTATUS 0xfff00001 /* Required on lib -> client status change */
#define WEBQQ_CALLBACK_DEBUGMESSAGE 0xfff00002
#define WEBQQ_CALLBACK_NEEDVERIFY   0xfff00003
#define WEBQQ_CALLBACK_LOGINFAIL    0xfff00004
#define WEBQQ_CALLBACK_CRASH        0xfff000ff
#define WEBQQ_CALLBACK_WEB2	        0xfff00006
#define WEBQQ_CALLBACK_WEB2_ERROR   0xfff00007

#define WEBQQ_STORAGE_COOKIE 0

// WebIM
#define WEBIM_GETCONNECT_ITEM_CONTACT_STATUS 2
#define WEBIM_GETCONNECT_ITEM_CONTACT_MESSAGE 3
#define WEBIM_GETCONNECT_ITEM_LOGOUT 4
#define WEBIM_GETCONNECT_ITEM_ADD_REQUEST 5
#define WEBIM_GETCONNECT_ITEM_ADD_RESULT 6

#define WEBIM_STATUS_OFFLINE 0
#define WEBIM_STATUS_ONLINE 400
#define WEBIM_STATUS_BUSY 600
#define WEBIM_STATUS_AWAY 100
#define WEBIM_STATUS_ROBOT 499

// webqq definitions
#define FACE_MAX_INDEX 201

typedef void (* WEBQQ_CALLBACK_HUB) (LPVOID pvObject, DWORD dwCommand, LPSTR pszArgs, LPVOID pvCustom);

class CLibWebFetion {
public:
	CLibWebFetion(LPSTR pszID, LPSTR pszPassword, LPVOID pvObject, WEBQQ_CALLBACK_HUB wch, HANDLE hNetlib);
	~CLibWebFetion();

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

	enum WEBQQREFERERENUM {
		WEBQQ_REFERER_WEBQQ,
		WEBQQ_REFERER_PTLOGIN,
		WEBQQ_REFERER_WEBPROXY,
		WEBQQ_REFERER_MAIN,
		WEBQQ_REFERER_WEB2,
		WEBQQ_REFERER_WEB2PROXY
	};

	WEBQQSTATUSENUM GetStatus() const { return m_status; }
	void SetProxy(LPSTR pszHost, LPSTR pszUser, LPSTR pszPass);
	void Start();
	void Stop();
	void CleanUp();
	LPCSTR GetID() const { return m_id; }
	LPSTR GetHTMLDocument(LPCSTR pszUrl=NULL, LPCSTR pszReferer=NULL, LPDWORD pdwLength=NULL, BOOL fWeb2Assign=FALSE);
	LPCSTR GetReferer(WEBQQREFERERENUM type);
	LPSTR GetCookie(LPCSTR pszName);

	int FetchUserHead(DWORD myid, DWORD qqid, LPSTR crc, LPSTR saveto);
	//void SetLoginHide(BOOL val);
	void SetInitialStatus(int status);
	LPSTR GetStorage(int index);

	void SetBasePath(LPCSTR pszPath);
	LPCSTR GetBasePath() const { return m_basepath; }
	void Test();
protected:
	DWORD ThreadProc();
private:
	static DWORD WINAPI _ThreadProc(CLibWebFetion* me);
	void SetStatus(WEBQQSTATUSENUM newstatus);
	void SendToHub(DWORD dwCommand, LPSTR pszArgs, LPVOID pvCustom=NULL);
	void Log(char *fmt,...);
	bool PrepareParams();
	bool Negotiate();
	bool Login(LPSTR pszVerify);
	void RefreshCookie();
	void ThreadLoop4b();

	// web2
private:
	LPSTR PostHTMLDocument(LPCSTR pszServer, LPCSTR pszUri, LPCSTR pszReferer, LPCSTR pszPostBody, LPDWORD pdwLength);
	bool web2_channel_poll();
	bool web2_check_result(LPSTR pszCommand, LPSTR pszResult);
	bool web2_vfwebqq_request(LPSTR uri);
	bool ParseResponse4b(JSONNODE* jnResult);

public:

	LPSTR UrlEncode(LPSTR pszDst, LPCSTR pszSrc);
	bool WebIM_Generic(LPCSTR pszCommand,LPCSTR pszPost=NULL,JSONNODE** jnResponse=NULL);
	bool WebIM_GetPersonalInfo();
	bool WebIM_GetContactList();
	bool WebIM_SetPresence(int presence, LPSTR pszCustom="");
	bool WebIM_SendMsg(DWORD dwUid, LPSTR pszMsg, bool fSms);
	bool WebIM_SetImpresa(LPSTR pszImpresa);
	bool WebIM_HandleAddBuddy(DWORD dwUid, bool fAgree, LPSTR pszLocalName="", int groupIndex=0);
	bool WebIM_GetPortrait(DWORD dwUid, int size, LPSTR pszCrc);
	int WebIM_AddBuddy(LPCSTR pszUidOrPhone, LPCSTR pszDesc, LPCSTR pszLocalName="", int groupIndex=0);

	WEBQQSTATUSENUM m_status;
	LPVOID m_userobject;
	WEBQQ_CALLBACK_HUB m_wch;

	LPSTR m_id;
	LPSTR m_password;

	HINTERNET m_hInet;
	LPSTR m_buffer;
	static LPCSTR g_referer_webqq;
	static LPCSTR g_domain_qq;
	static LPCSTR g_server_webim;
	DWORD m_sequence;
	LPSTR m_storage[10];
	LPSTR m_proxyhost;
	LPSTR m_proxyuser;
	LPSTR m_proxypass;
	LPSTR m_basepath;
	HINTERNET m_hInetRequest;
	// BOOL m_loginhide;
	int m_initialstatus;
	// map<DWORD,LPWEBQQ_OUT_PACKET> m_outpackets;

	DWORD m_processstatus;

	// web2
	HANDLE m_hNetlib;

	char m_postssid[64];
	LPSTR m_web2_psessionid;

	BOOL m_stop;
};