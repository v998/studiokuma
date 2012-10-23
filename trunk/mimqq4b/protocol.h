class CProtocol;
class CHttpServer;

#define MIMQQ4_MSG_CACHE_SIZE 100

typedef int (__cdecl CProtocol::*EventFunc)(WPARAM, LPARAM);
typedef int (__cdecl CProtocol::*ServiceFunc)(WPARAM, LPARAM);
typedef void (__cdecl CProtocol::*ThreadFunc)(LPVOID);
typedef void (__thiscall CProtocol::*CallbackFunc)(CWebQQ2*, LPSTR, JSONNODE*);

class CProtocol: public PROTO_INTERFACE, WebQQ2Callback {
public:
	CProtocol(LPCSTR szModuleName, LPCTSTR szUserName);
	~CProtocol();

	static tagPROTO_INTERFACE* InitAccount(LPCSTR szModuleName, LPCTSTR szUserName);
	static int UninitAccount(struct tagPROTO_INTERFACE*);
	static int DestroyAccount(struct tagPROTO_INTERFACE*);

	// Utils
	typedef struct {
		char en[16];
		char py[16];
		int qq2006;
		int qq2009;
	} QQ_SMILEY, *PQQ_SMILEY, *LPQQ_SMILEY;

	typedef struct _GROUP_MEMBER {
		DWORD uin;
		BYTE stat;
		BYTE client_type;
		LPSTR nick;
		LPSTR card;
		DWORD mflag;
		_GROUP_MEMBER* next;
	} GROUP_MEMBER, *PGROUP_MEMBER, *LPGROUP_MEMBER;

	HANDLE QCreateService(LPCSTR pszService, ServiceFunc pFunc);
	HANDLE QHookEvent(LPCSTR pszEvent, EventFunc pFunc);
	void QLog(char *fmt,...);
	void BroadcastStatus(int newStatus);
	int FindGroupByName(LPCSTR name);
	HANDLE AddOrFindContact(DWORD qqid, bool not_on_list=false, bool hidden=false);
	HANDLE FindContact(DWORD qqid);
	int MapStatus(int status);
	static void LoadSmileys();
	static void GetModuleName(LPSTR pszOutput);
	CLibWebQQ* GetWebQQ() const { return m_webqq; }
	const HANDLE GetNetlibUser() const { return m_hNetlibUser; }
	int ShowNotification(LPCWSTR info, DWORD flags);
	void WriteClientType(HANDLE hContact, int type);
	HANDLE AddOrFindQunContact(CWebQQ2* webqq2, DWORD flags, LPCSTR pszName, DWORD uin, BOOL isqun=false);
	HANDLE FindContactWithPseudoUIN(DWORD uin);

	// AccMgrUI
	int __cdecl CreateAccMgrUI(WPARAM, LPARAM lParam);

	static HINSTANCE g_hInstance;
	stack<HANDLE> m_services;
	stack<HANDLE> m_hooks;
	stack<HANDLE> m_menuItems;
	vector<HANDLE> m_contextMenuItems;
	map<int,int> m_groups;
	// map<DWORD,HANDLE> m_sentMessages;
	HANDLE m_folders[4];
	HANDLE m_hMenuRoot;
	HANDLE m_hNetlibUser;
	bool m_enableBBCode;
	CLibWebQQ* m_webqq;
	CWebQQ2* m_webqq2;
	static QQ_SMILEY g_smileys[256];
	static int g_smileysCount;
	static CHttpServer* g_httpServer;

	// map<DWORD,DWORD[200]> m_qunMemberInit;
	
	// map<DWORD,string> m_qunMemberNames;
	/*
	BYTE m_qunsToInit;
	BYTE m_qunsInited;
	map<DWORD,string>::iterator m_qunMembersInited;
	*/
	DWORD m_msgseq[MIMQQ4_MSG_CACHE_SIZE];
	DWORD m_msgseqCurrent;
	// DWORD m_searchuin;

	// ExtSvc
	void InitFontService();
	void InitFoldersService();
	void InitAssocManager();

	//====================================================================================
	// PROTO_INTERFACE
	//====================================================================================
	HANDLE   __cdecl AddToList( int flags, PROTOSEARCHRESULT* psr );
	HANDLE   __cdecl AddToListByEvent( int flags, int iContact, HANDLE hDbEvent );

	int      __cdecl Authorize( HANDLE hDbEvent );
	int      __cdecl AuthDeny( HANDLE hDbEvent, const PROTOCHAR* szReason );
	int      __cdecl AuthRecv( HANDLE hContact, PROTORECVEVENT* );
	int      __cdecl AuthRequest( HANDLE hContact, const PROTOCHAR* szMessage );

	HANDLE   __cdecl ChangeInfo( int iInfoType, void* pInfoData );

	HANDLE   __cdecl FileAllow( HANDLE hContact, HANDLE hTransfer, const PROTOCHAR* szPath );
	int      __cdecl FileCancel( HANDLE hContact, HANDLE hTransfer );
	int      __cdecl FileDeny( HANDLE hContact, HANDLE hTransfer, const PROTOCHAR* szReason );
	int      __cdecl FileResume( HANDLE hTransfer, int* action, const PROTOCHAR** szFilename );

	DWORD_PTR __cdecl GetCaps( int type, HANDLE hContact = NULL );
	HICON     __cdecl GetIcon( int iconIndex );
	int       __cdecl GetInfo( HANDLE hContact, int infoType );

	HANDLE    __cdecl SearchBasic( const PROTOCHAR* id );
	HANDLE    __cdecl SearchByEmail( const PROTOCHAR* email );
	HANDLE    __cdecl SearchByName( const PROTOCHAR* nick, const PROTOCHAR* firstName, const PROTOCHAR* lastName );
	HWND      __cdecl SearchAdvanced( HWND owner );
	HWND      __cdecl CreateExtendedSearchUI( HWND owner );

	int       __cdecl RecvContacts( HANDLE hContact, PROTORECVEVENT* );
	int       __cdecl RecvFile( HANDLE hContact, PROTOFILEEVENT* );
	int       __cdecl RecvMsg( HANDLE hContact, PROTORECVEVENT* );
	int       __cdecl RecvUrl( HANDLE hContact, PROTORECVEVENT* );

	int       __cdecl SendContacts( HANDLE hContact, int flags, int nContacts, HANDLE* hContactsList );
	HANDLE    __cdecl SendFile( HANDLE hContact, const PROTOCHAR* szDescription, PROTOCHAR** ppszFiles );
	int       __cdecl SendMsg( HANDLE hContact, int flags, const char* msg );
	int       __cdecl SendUrl( HANDLE hContact, int flags, const char* url );

	int       __cdecl SetApparentMode( HANDLE hContact, int mode );
	int       __cdecl SetStatus( int iNewStatus );

	HANDLE    __cdecl GetAwayMsg( HANDLE hContact );
	int       __cdecl RecvAwayMsg( HANDLE hContact, int mode, PROTORECVEVENT* evt );
	int       __cdecl SendAwayMsg( HANDLE hContact, HANDLE hProcess, const char* msg );
	int       __cdecl SetAwayMsg( int iStatus, const PROTOCHAR* msg );

	int       __cdecl UserIsTyping( HANDLE hContact, int type );

	int       __cdecl OnEvent( PROTOEVENTTYPE iEventType, WPARAM wParam, LPARAM lParam );

	///
	void OnProtoLoad();
	int __cdecl GetAvatarCaps(WPARAM wParam, LPARAM lParam);
	int __cdecl SetMyAvatar(WPARAM wParam, LPARAM lParam);
	int __cdecl GetMyAvatar(WPARAM wParam, LPARAM lParam);
	int __cdecl OnPrebuildContactMenu(WPARAM wParam, LPARAM lParam);
	int __cdecl OnDetailsInit(WPARAM wParam, LPARAM lParam);

protected:
	void CallbackHub(DWORD dwCommand, LPSTR pszArgs, LPVOID pvCustom);
private:
	static void _CallbackHub(LPVOID pvObject, DWORD dwCommand, LPSTR pszArgs, LPVOID pvCustom);

#if 0 // Web1
	void HandleGroupInfo(LPWEBQQ_GROUPINFO lpGI);
	void HandleListInfo(LPWEBQQ_LISTINFO lpLI);
	void HandleNickInfo(LPWEBQQ_NICKINFO lpNI);
	void HandleClassInfo(LPWEBQQ_CLASSDATA lpCD);
	void HandleRemarkInfo(LPWEBQQ_REMARKINFO lpRI);
	void HandleMessage(LPWEBQQ_MESSAGE lpM);
	void HandleContactStatus(LPWEBQQ_CONTACT_STATUS lpCS);
	void HandleUserInfo(LPSTR pszArgs);
	void HandleClassMemberNicks(LPWEBQQ_NICKINFO lpNI);
	void HandleSignatures(LPWEBQQ_LONGNAMEINFO lpLNI);
	void HandleHeadInfo(LPSTR pszArgs);
	void HandleSystemMessage(LPSTR pszType, LPSTR pszArgs);
#endif // Web1
	void HandleQunImgUploadStatus(LPWEBQQ_QUNUPLOAD_STATUS lpQS);
	void HandleWeb2Result(bool fSuccess, LPSTR szCommand, JSONNODE* jnResult);
	void HandleWeb2GroupNameListMask(JSONNODE* jnResult);
	void HandleWeb2UserFriends(JSONNODE* jnResult);
	void HandleWeb2OnlineBuddies(JSONNODE* jnResult);
	void HandleWeb2SingleLongNick(JSONNODE* jnResult);
	void HandleWeb2FriendInfo(DWORD uin, JSONNODE* jnResult);
	void HandleWeb2GroupInfoExt(JSONNODE* jnResult);
	void HandleWeb2BuddiesStatusChange(JSONNODE* jnValue);
	void HandleWeb2QQLevel(JSONNODE* jnResult);
	void HandleWeb2GroupMessage(JSONNODE* jnValue);
	void HandleWeb2Message(JSONNODE* jnValue);
	void HandleWeb2P2PImgUploadStatus(LPWEBQQ_QUNUPLOAD_STATUS pvCustom);
	void HandleWeb2SystemMessage(JSONNODE* jnValue);
	int Web2StatusToMIM(LPSTR pszStatus);
	LPCSTR Web2StatusFromMIM(int status);
	string Web2ParseMessage(JSONNODE* jnContent, HANDLE hContact=NULL, DWORD gid=0, DWORD uin=0);
	JSONNODE* Web2ConvertMessage(bool isqun, DWORD qunid, LPCSTR message, int fontsize, LPSTR font, DWORD color, BOOL bold, BOOL italic, BOOL underline);
	bool CheckDuplicatedMessage(DWORD seq);
	int __cdecl TestService(WPARAM wParam, LPARAM lParam);
	int __cdecl GetAvatarInfo(WPARAM wParam, LPARAM lParam);
	int __cdecl PostImage(WPARAM wParam, LPARAM lParam);
	int __cdecl SilentQun(WPARAM wParam, LPARAM lParam);
	int __cdecl DownloadGroup(WPARAM wParam, LPARAM lParam);
	void __cdecl FetchAvatar(HANDLE hContact);
	void CreateThreadObj(ThreadFunc func, void* arg);
	void SetContactsOffline();
	int CountSmileys(LPCSTR pszSrc, BOOL encoded);
	void DecodeSmileys(LPSTR pszSrc);
	void EncodeSmileys(LPSTR pszSrc);
	void EncodeP2PImages(LPSTR pszSrc);
	void EncodeQunImages(HANDLE hContact, LPSTR pszSrc);
	void ProcessQunPics(LPSTR pszSrc, DWORD extid, DWORD ts, BOOL isQun);
	int CountQunPics(LPCSTR pszSrc);
	void __cdecl BroadcastMsgAck(LPVOID lpParameter);
	void GetAllAvatars();
	void __cdecl GetAllAvatarsThread(LPVOID);
	void PostImageInner(HANDLE hContact, LPWSTR pszFilename);
	void __cdecl SearchBasicThread(LPVOID lpParameter);
	void __cdecl SearchNickThread(LPVOID lpParameter);
	void __cdecl SendMsgThread(LPVOID lpParameter);
	void __cdecl GetInfoThread(LPVOID lpParameter);
	void HandleQunSearchResult(JSONNODE* jn, HANDLE hSearch);

	void GetFriendInfo2(CWebQQ2* webqq2, LPSTR pszArgs, JSONNODE* jn);
	void GetUserFriends2(CWebQQ2* webqq2, LPSTR pszArgs, JSONNODE* jn);
	void GetGroupListMask2(CWebQQ2* webqq2, LPSTR pszArgs, JSONNODE* jn);
	void GetOnlineBuddies2(CWebQQ2* webqq2, LPSTR pszArgs, JSONNODE* jn);
	void Poll2(CWebQQ2* webqq2, LPSTR pszArgs, JSONNODE* jn);
	void GetGroupInfoExt2(CWebQQ2* webqq2, LPSTR pszArgs, JSONNODE* jn);
	void WebQQ2_Callback(CWebQQ2* webqq2, LPCSTR pcszCommand, LPSTR pszArgs, JSONNODE* jn);

	void Poll2_message(CWebQQ2* webqq2, LPSTR pszArgs, JSONNODE* jn);
	void Poll2_group_message(CWebQQ2* webqq2, LPSTR pszArgs, JSONNODE* jn);

	void registerCallbacks();
	std::map<LPCSTR,CallbackFunc> callbacks;
	std::map<LPCSTR,CallbackFunc> poll2_callbacks;
	std::map<DWORD,LPGROUP_MEMBER> group_members;
};
