class CProtocol;
class CHttpServer;

#define MIMQQ4_MSG_CACHE_SIZE 100

typedef int (__cdecl CProtocol::*EventFunc)(WPARAM, LPARAM);
typedef int (__cdecl CProtocol::*ServiceFunc)(WPARAM, LPARAM);
typedef void (__cdecl CProtocol::*ThreadFunc)(LPVOID);

class CProtocol: public PROTO_INTERFACE {
public:
	CProtocol(LPCSTR szModuleName, LPCTSTR szUserName);
	~CProtocol();

	static tagPROTO_INTERFACE* InitAccount(LPCSTR szModuleName, LPCTSTR szUserName);
	static int UninitAccount(struct tagPROTO_INTERFACE*);
	static int DestroyAccount(struct tagPROTO_INTERFACE*);

	// Utils
	HANDLE QCreateService(LPCSTR pszService, ServiceFunc pFunc);
	HANDLE QHookEvent(LPCSTR pszEvent, EventFunc pFunc);
	void QLog(char *fmt,...);
	void BroadcastStatus(int newStatus);
	int FindGroupByName(LPCSTR name);
	HANDLE AddOrFindContact(DWORD qqid, bool not_on_list=false, bool hidden=false);
	HANDLE FindContact(DWORD qqid);
	int MapStatus(int status);
	static void GetModuleName(LPSTR pszOutput);
	CLibWebFetion* GetWebQQ() const { return m_webqq; }
	const HANDLE GetNetlibUser() const { return m_hNetlibUser; }
	int ShowNotification(LPCWSTR info, DWORD flags);

	// AccMgrUI
	int __cdecl CreateAccMgrUI(WPARAM, LPARAM lParam);

	static HINSTANCE g_hInstance;
	stack<HANDLE> m_services;
	stack<HANDLE> m_hooks;
	stack<HANDLE> m_menuItems;
	vector<HANDLE> m_contextMenuItems;
	map<int,int> m_groups;
	HANDLE m_folders[4];
	HANDLE m_hMenuRoot;
	HANDLE m_hNetlibUser;
	CLibWebFetion* m_webqq;
	// static CHttpServer* g_httpServer;

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

	void HandleWeb2Result(bool fSuccess, LPSTR szCommand, JSONNODE* jnResult);

	void HandleWebIMPersonalInfo(JSONNODE* jnValue);
	void HandleWebIMContactList(JSONNODE* jnValue);
	void HandleWebIMConnect(int type, JSONNODE* jnResult);

	void UpdateContactInfo(HANDLE hContact, JSONNODE* jnInfo);

	int __cdecl TestService(WPARAM wParam, LPARAM lParam);
	int __cdecl GetAvatarInfo(WPARAM wParam, LPARAM lParam);
	int __cdecl DownloadGroup(WPARAM wParam, LPARAM lParam);
	int __cdecl ForceSMS(WPARAM wParam, LPARAM lParam);
	void __cdecl FetchAvatar(HANDLE hContact);
	void CreateThreadObj(ThreadFunc func, void* arg);
	void SetContactsOffline();
	void __cdecl BroadcastMsgAck(LPVOID lpParameter);
	void GetAllAvatars();
	void __cdecl GetAllAvatarsThread(LPVOID);
	void __cdecl SearchBasicThread(LPVOID lpParameter);
	void __cdecl SendMsgThread(LPVOID lpParameter);
	void __cdecl GetInfoThread(LPVOID lpParameter);
};
