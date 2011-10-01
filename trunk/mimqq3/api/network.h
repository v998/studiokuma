#ifndef NETWORK_H
#define NETWORK_H

typedef struct {
	short total;                       //used to show missing part
	std::map<short,string> content;
} pcMsg;

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
typedef void (__thiscall CNetwork::*CallbackFunc)(LPSTR);

class CUserHead;
class CQunImage;
class CodeVerifyWindow;
class XGraphicVerifyCode;

typedef void (__cdecl CNetwork::*ThreadFunc)(LPVOID);

//
class InPacket;
class OutPacket;

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
	void append(OutPacket* out);
	void sendOut(OutPacket* out);
	void removePacket(const int hashCode);

	static tagPROTO_INTERFACE* CNetwork::InitAccount(LPCSTR szModuleName, LPCTSTR szUserName);
	static int UninitAccount(struct tagPROTO_INTERFACE*);
	static int DestroyAccount(struct tagPROTO_INTERFACE*);
	void LoadAccount();
	void UnloadAccount();
	static void RemoveQunPics();

private:
	void redirect(int host, int port);

	std::map<short, pcMsg> pcMsgCache;

	// New //
	HANDLE QCreateService(LPCSTR pszService, ServiceFunc);
	HANDLE QHookEvent(LPCSTR pszEvent, EventFunc);

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

	virtual	HANDLE __cdecl FileAllow( HANDLE hContact, HANDLE hTransfer, const PROTOCHAR* szPath );
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
	virtual	HANDLE __cdecl SendFile( HANDLE hContact, const PROTOCHAR* szDescription, PROTOCHAR** ppszFiles );
	virtual	int    __cdecl SendMsg( HANDLE hContact, int flags, const char* msg );
	virtual	int    __cdecl SendUrl( HANDLE hContact, int flags, const char* url );

	virtual	int    __cdecl SetApparentMode( HANDLE hContact, int mode );
	virtual	int    __cdecl SetStatus( int iNewStatus );

	virtual	HANDLE    __cdecl GetAwayMsg( HANDLE hContact );
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
	void InitFontService();
	void InitFoldersService();

	void SetResident();
	// void dumppacket(bool in, Packet* packet);

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
	INT __cdecl CreateAccMgrUI(WPARAM,LPARAM);
	INT __cdecl IPCService(WPARAM,LPARAM);
	INT __cdecl RecvAuth(WPARAM,LPARAM);
	INT __cdecl TestService(WPARAM,LPARAM);
	INT __cdecl ForceRefresh(WPARAM,LPARAM);

	void _CopyAndPost(HANDLE hContact, LPCWSTR szFile);
	void __cdecl FetchQunAvatar(LPVOID data);
	void __cdecl FetchAvatar(void *data);
	void __cdecl GetAwayMsgThread(void* gatt);

	// utils.cpp
	void SetServerStatus(int newStatus);
	void EnableMenuItems(BOOL parEnable);
	void BroadcastStatus(int newStatus);
	void SetContactsOffline();
	int ShowNotification(LPCWSTR info, DWORD flags);
	void ForkThread(ThreadFunc func, void* arg=NULL);
	void __cdecl ThreadMsgBox(void* szMsg);
	void RemoveAllCardNames(HANDLE hContact);
	static int _RemoveAllCardNamesProc(const char *szSetting,LPARAM lParam);

	// callbacks.cpp
	void _writeVersion(HANDLE hContact, int version, LPCSTR iniFile);
	void _qunImCallback2(const unsigned int qunID, const unsigned int senderQQ, const bool hasFontAttribute, const bool isBold, const bool isItalic, const bool isUnderline, const char fontSize, const char red, const char green, const char blue, const int sentTime, const std::string message);
	void _sysRequestJoinQunCallback(int qunid, int extid, int userid, LPCSTR msg, const unsigned char *token, const WORD tokenLen);
	void _sysRejectJoinQunCallback(int qunid, int extid, int userid, LPCSTR msg);
	void __cdecl delayReport(LPVOID);
	void __cdecl _tempSessionGraphicalVerification(LPVOID adp);
public:
	// myqq
	void _buddyMsgCallback(qqclient* qq, uint uid, time_t t, char* msg);
	void _qunMsgCallback(qqclient* qq, uint uid, uint int_uid, time_t t, char* msg);
	void _eventCallback(char* msg);

public:
	void GoOffline();

	bool uhCallbackHub(int msg, int qqid, const char* md5, unsigned int session);
	void qunPicCallbackHub(int msg, int qunid, void* aux);

	UINT_PTR m_timer; // For SetCurrentMedia

	// details.cpp
	void UpdateQunContacts(HWND hwndDlg, unsigned int qunid);
	HANDLE FindContact(const unsigned int QQID);
	HANDLE AddContact(const unsigned int QQID, bool not_on_list, bool hidden);

	const int GetMyQQ() const { return m_myqq; }
	void AddContactWithSend(DWORD qqid);
	const bool IsConservative() const { return m_conservative; }
	const bool TriggerConservativeState();
private:
	list<HANDLE> m_serviceList;
	list<HANDLE> m_menuItemList;
	list<HANDLE> m_contextMenuItemList;
	list<HANDLE> m_hookList;
	map<unsigned short,unsigned int> m_imSender;

	CUserHead* m_userhead;
	CQunImage* m_qunimage;

	DWORD m_searchUID;
	LISTENINGTOINFO m_currentMedia;

	map<int,HANDLE> m_hGroupList;
	int m_qqusers;
	bool m_downloadGroup;
	DWORD m_myqq;
	HANDLE m_hMenuRoot;
	BOOL m_conservative;

	time_t m_deferActionTS;
	BOOL m_uhTriggered;

	void registerCallbacks();
	void CNetwork::processProcess(LPSTR pszArgs);
	void CNetwork::processClusterInfo(LPSTR pszArgs);
	void CNetwork::processBuddyList(LPSTR pszArgs);
	void CNetwork::processGroupList(LPSTR pszArgs);
	void CNetwork::processBuddyStatus(LPSTR pszArgs);
	void CNetwork::processStatus(LPSTR pszArgs);
	void CNetwork::processBuddyInfo(LPSTR pszArgs);
	void CNetwork::processSearchUid(LPSTR pszArgs);
	void CNetwork::processQunSearch(LPSTR pszArgs);
	void CNetwork::processRequestAddBuddy(LPSTR pszArgs);
	void CNetwork::processRequestToken(LPSTR pszArgs);
	void CNetwork::processDelBuddy(LPSTR pszArgs);
	void CNetwork::processBroadcast(LPSTR pszArgs);
	void CNetwork::processLoginTouchRedirect(LPSTR pszArgs);
	void CNetwork::processClusterMemberNames(LPSTR pszArgs);
	void CNetwork::processIMSendMsgReply(LPSTR pszArgs);
	void CNetwork::processBuddyIMTextAdv(LPSTR pszArgs);
	void CNetwork::processQunIMAdv(LPSTR pszArgs);
	void CNetwork::processNews(LPSTR pszArgs);
	void CNetwork::processMail(LPSTR pszArgs);
	void CNetwork::processQunJoin(LPSTR pszArgs);
	void CNetwork::processBuddySignature(LPSTR pszArgs);
	void CNetwork::processBuddyWriting(LPSTR pszArgs);
	void CNetwork::processWeather(LPSTR pszArgs);
	void CNetwork::processQunMyJoin(LPSTR pszArgs);

public:
	char m_deferActionType;
	DWORD m_deferActionData;
	DWORD m_deferActionAux;

	bool m_needAck;
	HWND m_hwndModifySignatureDlg;
	CodeVerifyWindow* m_codeVerifyWindow;
	XGraphicVerifyCode* m_graphicVerifyCode;
	// SendTempSessionTextIMPacket* m_savedTempSessionMsg; // TODO: Temp Session
	qqclient m_client;

	// Moved from options
	HWND opt_hwndAcc;
	HWND opt_hwndSettings;
	HWND opt_hwndQun;
	bool opt_fInit;
	static HANDLE m_folders[2]; // 0=Avatars 1=QunImages 2=WebServer
	HANDLE m_avatarFolder;
	std::map<LPCSTR,CallbackFunc> callbacks;
	qqpacket* m_packet;	
};

#endif // NETWORK_H