#ifndef NETWORK_H
#define NETWORK_H

class CNetwork;

typedef struct {
	HANDLE hContact;
	int ackType;
	int ackResult;
	int aux;
	LPVOID aux2;
} delayReport_t;

typedef int (__cdecl CNetwork::*EventFunc)(WPARAM, LPARAM);
typedef int (__cdecl CNetwork::*ServiceFunc)(WPARAM, LPARAM);
typedef void (__cdecl CNetwork::*ThreadFunc)(LPVOID);

class CNetwork: public CClientConnection, public PROTO_INTERFACE {
public:
	CNetwork(LPCSTR szModuleName, LPCTSTR szUserName);
	~CNetwork();
	bool setConnectString(LPCSTR pszConnectString);
	void connectionError();
	void connectionEstablished();
	void connectionClosed();
	void waitTimedOut();
	int dataReceived(NETLIBPACKETRECVER* nlpr);
	bool crashRecovery();
#if 0
	void append(OutPacket* out);
	void sendOut(OutPacket* out);
	const int getClockSkew() const { return m_clockSkew; }
	void removePacket(const int hashCode);
#endif

	static tagPROTO_INTERFACE* InitAccount(LPCSTR szModuleName, LPCTSTR szUserName);
	static int UninitAccount(struct tagPROTO_INTERFACE*);
	static int DestroyAccount(struct tagPROTO_INTERFACE*);
	void LoadAccount();
	void UnloadAccount();

	bool m_loggedIn;

	HANDLE FCreateService(LPCSTR pszService, ServiceFunc);
	HANDLE FHookEvent(LPCSTR pszEvent, EventFunc);

	//====================================================================================
	// PROTO_INTERFACE
	//====================================================================================
	virtual	HANDLE __cdecl AddToList( int flags, PROTOSEARCHRESULT* psr );
	virtual	HANDLE __cdecl AddToListByEvent( int flags, int iContact, HANDLE hDbEvent );

	virtual	int    __cdecl Authorize( HANDLE hContact );
	virtual	int    __cdecl AuthDeny( HANDLE hContact, const char* szReason );
	virtual	int    __cdecl AuthRecv( HANDLE hContact, PROTORECVEVENT* );
	virtual	int    __cdecl AuthRequest( HANDLE hContact, const char* szMessage );

	virtual	HANDLE __cdecl ChangeInfo( int iInfoType, void* pInfoData );

	virtual	int    __cdecl FileAllow( HANDLE hContact, HANDLE hTransfer, const char* szPath );
	virtual	int    __cdecl FileCancel( HANDLE hContact, HANDLE hTransfer );
	virtual	int    __cdecl FileDeny( HANDLE hContact, HANDLE hTransfer, const char* szReason );
	virtual	int    __cdecl FileResume( HANDLE hTransfer, int* action, const char** szFilename );

	virtual	DWORD  __cdecl GetCaps( int type, HANDLE hContact);
	virtual	HICON  __cdecl GetIcon( int iconIndex );
	virtual	int    __cdecl GetInfo( HANDLE hContact, int infoType );

	virtual	HANDLE __cdecl SearchBasic( const char* id );
	virtual	HANDLE __cdecl SearchByEmail( const char* email );
	virtual	HANDLE __cdecl SearchByName( const char* nick, const char* firstName, const char* lastName );
	virtual	HWND   __cdecl SearchAdvanced( HWND owner );
	virtual	HWND   __cdecl CreateExtendedSearchUI( HWND owner );

	virtual	int    __cdecl RecvContacts( HANDLE hContact, PROTORECVEVENT* );
	virtual	int    __cdecl RecvFile( HANDLE hContact, PROTORECVFILE* );
	virtual	int    __cdecl RecvMsg( HANDLE hContact, PROTORECVEVENT* );
	virtual	int    __cdecl RecvUrl( HANDLE hContact, PROTORECVEVENT* );

	virtual	int    __cdecl SendContacts( HANDLE hContact, int flags, int nContacts, HANDLE* hContactsList );
	virtual	int    __cdecl SendFile( HANDLE hContact, const char* szDescription, char** ppszFiles );
	virtual	int    __cdecl SendMsg( HANDLE hContact, int flags, const char* msg );
	virtual	int    __cdecl SendUrl( HANDLE hContact, int flags, const char* url );

	virtual	int    __cdecl SetApparentMode( HANDLE hContact, int mode );
	virtual	int    __cdecl SetStatus( int iNewStatus );

	virtual	int    __cdecl GetAwayMsg( HANDLE hContact );
	virtual	int    __cdecl RecvAwayMsg( HANDLE hContact, int mode, PROTORECVEVENT* evt );
	virtual	int    __cdecl SendAwayMsg( HANDLE hContact, HANDLE hProcess, const char* msg );
	virtual	int    __cdecl SetAwayMsg( int m_iStatus, const char* msg );

	virtual	int    __cdecl UserIsTyping( HANDLE hContact, int type );
	virtual	int    __cdecl OnEvent( PROTOEVENTTYPE eventType, WPARAM wParam, LPARAM lParam );

	//====| Events |======================================================================
	/*
	void __cdecl OnAddContactForever( DBCONTACTWRITESETTING* cws, HANDLE hContact );
	*/
	int  __cdecl OnContactDeleted( WPARAM, LPARAM );
	/*
	int  __cdecl OnDbSettingChanged( WPARAM, LPARAM );
	int  __cdecl OnIdleChanged( WPARAM, LPARAM );
	int  __cdecl OnModernOptInit( WPARAM, LPARAM );
	int  __cdecl OnModernToolbarInit( WPARAM, LPARAM );
	*/
	int  __cdecl OnModulesLoadedEx( WPARAM, LPARAM );
	int  __cdecl OnOptionsInit( WPARAM, LPARAM );
	int  __cdecl OnPreShutdown( WPARAM, LPARAM );
	int  __cdecl OnPrebuildContactMenu( WPARAM, LPARAM );
	int  __cdecl OnDetailsInit(WPARAM, LPARAM);
	/*
	int  __cdecl OnMsgUserTyping( WPARAM, LPARAM );
	int  __cdecl OnProcessSrmmIconClick( WPARAM, LPARAM );
	int  __cdecl OnProcessSrmmEvent( WPARAM, LPARAM );
	int  __cdecl OnReloadIcons( WPARAM, LPARAM );
	void __cdecl OnRenameContact( DBCONTACTWRITESETTING* cws, HANDLE hContact );
	void __cdecl OnRenameGroup( DBCONTACTWRITESETTING* cws, HANDLE hContact );
	int  __cdecl OnUserInfoInit( WPARAM, LPARAM );
	*/
	int __cdecl ForceSMS(WPARAM wParam, LPARAM lParam);

	void __cdecl SearchBasicDeferred(LPSTR pszID);

	void SetResident();

#if 0
	// services.cpp
	INT __cdecl GetStatus(WPARAM,LPARAM);
	INT __cdecl GetName(WPARAM,LPARAM);
	INT __cdecl SetMyNickname(WPARAM,LPARAM);
	INT __cdecl GetAvatarInfo(WPARAM,LPARAM);
	INT __cdecl GetCurrentMedia(WPARAM,LPARAM);
	INT __cdecl SetCurrentMedia(WPARAM,LPARAM);
	INT __cdecl ChangeNickname(WPARAM,LPARAM);
	INT __cdecl ModifySignature(WPARAM,LPARAM);
	INT __cdecl QQMail(WPARAM,LPARAM);
	INT __cdecl CopyMyIP(WPARAM,LPARAM);
	INT __cdecl DownloadGroup(WPARAM,LPARAM);
	INT __cdecl UploadGroup(WPARAM,LPARAM);
	INT __cdecl RemoveNonServerContacts(WPARAM,LPARAM);
	INT __cdecl SuppressAddRequests(WPARAM,LPARAM);
	INT __cdecl DownloadUserHead(WPARAM,LPARAM);
	INT __cdecl GetWeather(WPARAM,LPARAM);
	INT __cdecl SuppressQunMessages(WPARAM,LPARAM);
	INT __cdecl RemoveMe(WPARAM,LPARAM);
	INT __cdecl AddQunMember(WPARAM,LPARAM);
	INT __cdecl SilentQun(WPARAM,LPARAM);
	INT __cdecl Reauthorize(WPARAM,LPARAM);
	INT __cdecl ChangeCardName(WPARAM,LPARAM);
	INT __cdecl PostImage(WPARAM,LPARAM);
	INT __cdecl QunSpace(WPARAM,LPARAM);
	INT __cdecl IPCService(WPARAM,LPARAM);
	INT __cdecl RecvAuth(WPARAM,LPARAM);

#endif

	INT __cdecl CreateAccMgrUI(WPARAM,LPARAM);
	void __cdecl delayReport(LPVOID);

	// utils.cpp
	void SetServerStatus(int newStatus);
	void EnableMenuItems(BOOL parEnable);
	void BroadcastStatus(int newStatus);
	void SetContactsOffline();
	int ShowNotification(LPCWSTR info, DWORD flags);
	void ForkThread(ThreadFunc func, void* arg=NULL);
	void __cdecl ThreadMsgBox(void* szMsg);
	int ConvertStatus(int fromstatus, bool tomimstatus);

	int __cdecl GetName(WPARAM wParam, LPARAM lParam);
	int __cdecl GetStatus(WPARAM wParam, LPARAM lParam);

	// protocol.cpp
	void fetion_login();
	void __cdecl RetriveSysCfg(LPVOID);
	void __cdecl LoginToSsiPortal(LPVOID);
	void do_register();
	void do_register_exp(int expire);
	void send_sip_request(LPCSTR method, LPCSTR url, LPCSTR to, LPCSTR addheaders, LPCSTR body, LPCSTR callid2, TransCallback tc);
	void sendout_pkt(LPCSTR buf);
	void transactions_remove(struct transaction *trans);
	void transactions_add_buf(LPCSTR buf, TransCallback callback);
	void transactions_free_all();
	struct transaction *transactions_find(struct sipmsg *msg);
	bool __cdecl process_register_response(struct sipmsg *msg, struct transaction *tc);
	bool GetContactList();
	bool __cdecl GetContactList_cb(struct sipmsg *msg, struct transaction *tc);
	void process_input_message(struct sipmsg *msg);
	void process_incoming_message(struct sipmsg *msg);
	void send_sip_response(struct sipmsg *msg, int code, LPCSTR text, LPCSTR body);
	void fetion_subscribe_exp(HANDLE hContact);
	bool __cdecl process_subscribe_response(struct sipmsg *msg, struct transaction *tc);
	void process_incoming_notify(struct sipmsg *msg);
	void process_incoming_BN(struct sipmsg *msg);
	void CheckPortrait(LPCSTR who, LPCSTR crc);
	void GetPortrait(HANDLE hContact, LPCSTR sip, LPCSTR crc, LPCSTR host);
	void process_incoming_invite(struct sipmsg *msg);
	void fetion_keep_alive();
	void fetion_send_message(LPCSTR to, LPCSTR msg, LPCSTR type, const bool sms);
	void SendInvite(LPCSTR who, LPCSTR callid);
	void __cdecl SendInvite_cb(struct sipmsg *msg, struct transaction *tc);
	bool GetPersonalInfo();
	bool __cdecl GetPersonalInfo_cb(struct sipmsg *msg, struct transaction *tc);
	void __cdecl SetPresence_cb(struct sipmsg *msg, struct transaction *tc);
	bool __cdecl AddBuddy_cb(struct sipmsg *msg, struct transaction *tc);
	void __cdecl AddMobileBuddy_cb(struct sipmsg *msg, struct transaction *tc);
	void AddMobileBuddy(struct sipmsg *msg, struct transaction *tc);
	bool __cdecl SearchBasicDeferred_cb(struct sipmsg *msg, struct transaction *tc);
	bool __cdecl SearchBasicDeferred_cb2(struct sipmsg *msg, struct transaction *tc);
	bool __cdecl PGApplyGroup_cb(struct sipmsg *msg, struct transaction *tc);
	bool __cdecl PGGetGroupList_cb(struct sipmsg *msg, struct transaction *tc);
	bool __cdecl PGGetGroupInfo_cb(struct sipmsg *msg, struct transaction *tc);
	bool __cdecl PGGetGroupMembers_cb(struct sipmsg *msg, struct transaction *tc);
	void ProcessGroups();
	bool __cdecl GetBuddyInfo_cb(struct sipmsg *msg, struct transaction *tc);
	void GetBuddyInfo(const char * who);
	void __cdecl SetImpresa(LPCSTR u8msg);
	LPSTR ProcessHTMLMessage(LPSTR pszMsg, ezxml_t xml);
	void WriteLocationInfo(HANDLE hContact);

	static void ReceivedConnection(HANDLE hNewConnection,DWORD dwRemoteIP, void * pExtra);

public:
	void GoOffline();

	bool uhCallbackHub(int msg, int qqid, const char* md5, unsigned int session);
	void qunPicCallbackHub(int msg, int qunid, void* aux);

	UINT_PTR m_timer;

	// details.cpp
	HANDLE FindContact(const unsigned int QQID);
	HANDLE AddContact(const unsigned int QQID, bool not_on_list, bool hidden);

	// sip
	LPCSTR username() const { return m_username; }
	LPCSTR password() const { return m_password; }

	void ResumeConnection(HANDLE hNewConnection);
private:
	list<HANDLE> m_serviceList;
	list<HANDLE> m_menuItemList;
	list<HANDLE> m_contextMenuItemList;
	list<HANDLE> m_hookList;

	int m_searchUID;
	int m_myqq;
	HANDLE m_hMenuRoot;

	// protocol
	LPSTR m_ssic;
	LPSTR m_uri;
	LPSTR m_username;
	LPSTR m_mobileno;
	LPSTR m_password;
	int m_tg; //for temp group chat id
	//int m_registerexpire;
	LPSTR m_impresa;
	DWORD m_GetContactFlag;
	//LPSTR m_SysCfgServer;
	//LPSTR m_status;
	time_t m_reregister;
	int m_registerstatus; /* 0 nothing, 1 first registration send, 2 auth received, 3 registered */
	struct sip_auth m_registrar;
	struct sip_auth m_proxy;
	list<transaction*> m_transactions;
	map<long, group_chat*> m_tempgroup;
	LPSTR m_regcallid;
	int m_cseq;
	int m_registerexpire;
	bool m_notdisconnect;
	in_addr m_myip;
	long m_myport;

	struct sipmsg* m_curmsg;
public:
	int m_addUID;

	void test();
};

#endif // NETWORK_H
