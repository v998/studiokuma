#ifndef NETWORK_H
#define NETWORK_H

typedef struct {
	short total;                       //used to show missing part
	std::map<short,string> content;
} pcMsg;

class CNetwork;

typedef struct {
	//CNetwork* network;
	HANDLE hContact;
	int ackType;
	int ackResult;
	int aux;
	LPVOID aux2;
} delayReport_t;

typedef int (__cdecl CNetwork::*EventFunc)(WPARAM, LPARAM);
typedef int (__cdecl CNetwork::*ServiceFunc)(WPARAM, LPARAM);

class CUserHead;
class CQunImage;
class CodeVerifyWindow;
class XGraphicVerifyCode;

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
	void append(OutPacket* out);
	void sendOut(OutPacket* out);
	const int getClockSkew() const { return m_clockSkew; }
	void removePacket(const int hashCode);

	static tagPROTO_INTERFACE* CNetwork::InitAccount(LPCSTR szModuleName, LPCTSTR szUserName);
	static int UninitAccount(struct tagPROTO_INTERFACE*);
	static int DestroyAccount(struct tagPROTO_INTERFACE*);
	void LoadAccount();
	void UnloadAccount();
	static void RemoveQunPics();

private:
	void processPacket(LPCBYTE lpData, const USHORT len);
	void processServerDetectorResponse(InPacket* in);
	void processRequestLoginTokenExResponse(InPacket* in);
	void processLoginResponse(InPacket* in);
	void processKeepAliveResponse(InPacket* in);
	void processRequestKeyResponse(InPacket* in);
	void processChangeStatusResponse(InPacket* in);
	void processGetUserInfoResponse(InPacket* in);
	void processGetFriendListResponse(InPacket* in);
	void processRequestExtraInfoResponse(InPacket* in);
	void processUploadGroupFriendResponse(InPacket* in);
	void processIMResponse(InPacket* in);
	void processGetFriendOnlineResponse(InPacket* in);
	void processDownloadGroupFriendResponse(InPacket* in);
	void processKeepaliveResponse(InPacket* in);
	void processQunResponse(InPacket* in);
	void processSendImResponse(InPacket* in);
	void processSignatureOpResponse(InPacket* in);
	void processGetLevelResponse(InPacket* in);
	void processRecvMsgFriendChangeStatusResponse(InPacket* in);
	void processTempSessionOpResponse(InPacket* in);
	void processWeatherOpResponse(InPacket* in);
	void processSearchUserResponse(InPacket* in);
	void processAddFriendResponse(InPacket* in);
	void processAddFriendAuthResponse(InPacket* in);
	void processSystemMessageResponse(InPacket* in);
	void processDeleteMeResponse(InPacket* in);
	void processGroupNameOpResponse(InPacket* in);
	void processDeleteFriendResponse(InPacket* in);
	void processAddFriendAuthInfoReply(InPacket* in);

	void removeOutRequests(const short cmd);
	void packetException(const short cmd);
	void sendMessage(const int receiver, bool result);
	void sendQunMessage(const int qunid, bool result);
	void clearOutPool();
	//void newPacket();

	CRITICAL_SECTION m_cs;
	list<OutPacket*> m_outPool;
	//list<InPacket*> m_inPool;
	list<int>receivedPacketList;
	list<int>receivedCacheList;
	time_t m_checkTime;
	time_t m_keepAliveTime;
	bool m_IsDetecting;
	UCHAR m_numOfLostKeepAlivePackets;
	bool m_loggedIn;
	//bool m_invisible;
	INT m_clockSkew;

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
	void InitFontService();
	void InitFoldersService();

	void SetResident();
	void dumppacket(bool in, Packet* packet);

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

	void _CopyAndPost(HANDLE hContact, LPCWSTR szFile);
	void __cdecl FetchQunAvatar(LPVOID data);
	void __cdecl FetchAvatar(void *data);
	void __cdecl GetAwayMsgThread(void* gatt);

	// utils.cpp
	void SetServerStatus(int newStatus);
	void EnableMenuItems(BOOL parEnable);
	void BroadcastStatus(int newStatus);
	void SetContactsOffline();
	//int ShowNotification(const char *info, DWORD flags);
	int ShowNotification(LPCWSTR info, DWORD flags);
	void ForkThread(ThreadFunc func, void* arg=NULL);
	void __cdecl ThreadMsgBox(void* szMsg);
	void RemoveAllCardNames(HANDLE hContact);
	static int _RemoveAllCardNamesProc(const char *szSetting,LPARAM lParam);

	// callbacks.cpp
	void callbackHub(int command, int subcommand, WPARAM wParam, LPARAM lParam);
	void _requestLoginTokenExCallback(RequestLoginTokenExReplyPacket* packet);
	void _loginCallback(LoginReplyPacket* packet);
	void _keepAliveCallback(KeepAliveReplyPacket*);
	void _changeStatusCallback(ChangeStatusReplyPacket* packet);
	void _getUserInfoCallback(GetUserInfoReplyPacket* packet);
	void _getFriendListCallback(GetFriendListReplyPacket* packet);
	void _getOnlineFriendCallback(GetOnlineFriendReplyPacket* packet);
	void _downloadGroupFriendCallback(DownloadGroupFriendReplyPacket* packet);
	void _requestExtraInfoCallback(RequestExtraInfoReplyPacket* packet);
	void _signatureOpCallback(SignatureReplyPacket* packet);
	void _writeIP(HANDLE hContact, int ip);
	void _writeVersion(HANDLE hContact, int version, const char* iniFile);
	void _imCallback(int subCommand, ReceiveIMPacket* packet, void* auxpacket);
	void _qunImCallback2(const unsigned int qunID, const unsigned int senderQQ, const bool hasFontAttribute, const bool isBold, const bool isItalic, const bool isUnderline, const char fontSize, const char red, const char green, const char blue, const int sentTime, const std::string message);
	void _updateQunCard(HANDLE hContact, const int qunid);
	void _imCallback(const int imType, const void* data);
	void _sysRequestJoinQunCallback(int qunid, int extid, int userid, const char* msg, const unsigned char *token, const unsigned short tokenLen);
	void _sysRejectJoinQunCallback(int qunid, int extid, int userid, const char* msg);
	void _qunCommandCallback(QunReplyPacket* packet);
	void _friendChangeStatusCallback(FriendChangeStatusPacket* packet);
	void _qunGetInfoCallback(QunReplyPacket* packet);
	void _searchUserCallback(SearchUserReplyPacket* packet);
	void _getLevelCallback(EvaGetLevelReplyPacket* packet);
	void _systemMessageCallback(SystemNotificationPacket* packet);
	void _addFriendCallback(EvaAddFriendExReplyPacket* packet);
	void _addFriendAuthInfoCallback(EvaAddFriendGetAuthInfoReplyPacket* packet);
	void _deleteFriendCallback(DeleteFriendReplyPacket* packet);
	void _deleteMeCallback(DeleteMeReplyPacket* packet);
	void _groupNameOpCallback(GroupNameOpReplyPacket* packet);
	void _sendImCallback(SendIMReplyPacket* packet,SendTextIMPacket* im);
	void _tempSessionOpCallback(TempSessionOpReplyPacket* packet);
	void _weatherOpCallback(WeatherOpReplyPacket* packet);
	void _uploadGroupFriendCallback(UploadGroupFriendReplyPacket* packet);
	void __cdecl delayReport(LPVOID);
	void __cdecl _addFriendAuthGraphicalVerification(LPVOID adp);
	void __cdecl _tempSessionGraphicalVerification(LPVOID adp);

public:
	void GoOffline();

	bool uhCallbackHub(int msg, int qqid, const char* md5, unsigned int session);
	void qunPicCallbackHub(int msg, int qunid, void* aux);

	UINT_PTR m_timer;

	// details.cpp
	void UpdateQunContacts(HWND hwndDlg, unsigned int qunid);
	HANDLE FindContact(const unsigned int QQID);
	HANDLE AddContact(const unsigned int QQID, bool not_on_list, bool hidden);

	const int GetMyQQ() const { return m_myqq; }
	void AddContactWithSend(int qqid);
private:
	list<HANDLE> m_serviceList;
	//list<HANDLE> m_menuServicesList;
	list<HANDLE> m_menuItemList;
	list<HANDLE> m_contextMenuItemList;
	list<HANDLE> m_hookList;
	int m_keepaliveCount;
	map<unsigned short,OutPacket*> m_pendingImList;
	list<int> m_qunInitList;
	bool m_myInfoRetrieved;

	LPSTR m_currentDefaultServer;
	CUserHead* m_userhead;
	CQunImage* m_qunimage;

	map<int,unsigned char> m_qunMemberCountList;
	map<int,unsigned char> m_currentQunMemberCountList;

	list<ReceivedNormalIM> m_storedIM;

	int m_searchUID;
	LISTENINGTOINFO m_currentMedia;

	map<int,HANDLE> m_hGroupList;
	int m_qqusers;
	bool m_downloadGroup;
	int m_myqq;
	HANDLE m_hMenuRoot;

	InPacket* m_curmsg;

public:
	int m_addUID;
	int m_addQunNumber;
	bool m_needAck;
	HWND m_hwndModifySignatureDlg;
	CodeVerifyWindow* m_codeVerifyWindow;
	XGraphicVerifyCode* m_graphicVerifyCode;
	QunList m_qunList;
	SendTempSessionTextIMPacket* m_savedTempSessionMsg;

	// Moved from options
	HWND opt_hwndAcc;
	HWND opt_hwndSettings;
	HWND opt_hwndQun;
	//HWND hwndHelper;
	bool opt_fInit;
	static HANDLE m_folders[2]; // 0=Avatars 1=QunImages 2=WebServer
	HANDLE m_avatarFolder;
};

#endif // NETWORK_H