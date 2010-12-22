class CHttpServer;

typedef void (__cdecl CHttpServer::*ThreadFunc2)(LPVOID);

class CHttpServer {
public:
	typedef struct {
		LPSTR uri;
		CProtocol* protocol;
		HANDLE hConnection;
		DWORD sender;
		DWORD receiver;
		DWORD timestamp;
		LPSTR filename;
		DWORD objectid;
	} P2PARGS, *PP2PARGS, *LPP2PARGS;

private:
	CHttpServer(CProtocol* lpBaseProtocol);
	static CHttpServer* g_hInst;
	static void newConnectionProc(HANDLE hNewConnection,DWORD dwRemoteIP, void * pExtra);
	void _newConnectionProc(HANDLE hNewConnection,DWORD dwRemoteIP);
	void writeTails(HANDLE hConnection);
	void CreateThreadObj(ThreadFunc2 func, void* arg);

	list<LPSTR> m_qunlinks;
	map<LPSTR,CProtocol*> m_qunlinkProtocols;
	map<DWORD,P2PARGS> m_p2psessions;
	CProtocol* m_baseProtocol;
	HANDLE m_connection;
	WORD m_port;
	char m_cachepath[MAX_PATH];
public:

	~CHttpServer();
	static CHttpServer* GetInstance(CProtocol* lpBaseProtocol=NULL);
	const WORD GetPort() const { return m_port; }
	void RegisterQunImage(LPCSTR pszLink, CProtocol* lpProtocol);
	void UnregisterQunImages(CProtocol* lpProtocol);
	void HandleP2PImage(DWORD uin, LPWEBQQ_MESSAGE lpMsg);
	void __cdecl DownloadP2PImage(LPVOID pUin);
};
