#ifndef QUNIMAGESERVER_H
#define QUNIMAGESERVER_H

class CQunImageServer;
typedef int (__cdecl CQunImageServer::*QISServiceFunc)(WPARAM, LPARAM);

class CQunImageServer {
public:
	CQunImageServer();
	~CQunImageServer();
	const bool isConnected() const { return m_connection!=NULL; }
	void addFile(LPCSTR pszFileName);
	void signalFile(LPCWSTR pwszFileName);

private:
	HANDLE m_connection;
	static void newConnectionProc(HANDLE hNewConnection,DWORD dwRemoteIP, void * pExtra);
	void _newConnectionProc(HANDLE hNewConnection,DWORD dwRemoteIP);
	bool isFetching(LPCWSTR pszFilename);
	bool loadFile(HANDLE hConnection, LPCWSTR szFile);
	void writeBadFile(HANDLE hConnection);
	void documentHandler(HANDLE hConnection, LPCWSTR szFile);
	void writeTails(HANDLE hConnection);
	int __cdecl ServiceAddFile(WPARAM,LPARAM);
	int __cdecl ServiceSignalFile(WPARAM,LPARAM);
	HANDLE QISCreateService(LPCSTR pszService, QISServiceFunc);

	map<wstring,HANDLE> m_acceptedconnections;
	list<wstring> m_fetchingfiles;
	LPSTR m_pszBadBMP;
	DWORD m_cbBadBMP;
	CRITICAL_SECTION m_cs;
	LPWSTR m_docroot;
	HANDLE m_hService;
	HANDLE m_hService2;
};

#endif // QUNIMAGESERVER_H