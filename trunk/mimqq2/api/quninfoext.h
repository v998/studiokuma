#include <queue>
#include <wininet.h>

class CQunInfoExt {
public:
	static bool Login(CNetwork* network, DWORD dwUIN, LPCSTR pszPassword, BOOL fAutoMode);
	static void Logout(DWORD dwUIN);
	static bool IsValid(DWORD dwUIN);
	static bool AddOneJob(DWORD dwUIN, BYTE op, DWORD extid, DWORD ver);
	LPSTR GetHTMLDocument(LPCSTR pszUrl, LPCSTR pszReferer, LPDWORD pdwLength);
	LPSTR PostHTMLDocument(LPCSTR pszServer, LPCSTR pszUri, LPCSTR pszReferer, LPCSTR pszPostBody, LPDWORD pdwLength);
	LPCSTR GetBasePath() const { return m_basepath; }

	const DWORD GetUIN() const { return m_uin; }
private:
	typedef struct {
		BYTE op;
		DWORD id;
		DWORD ver;
	} QUNINFOOP, *PQUNINFOOP, *LPQUNINFOOP;

	CQunInfoExt(CNetwork* network, DWORD dwUIN, LPCSTR pszPassword, BOOL fAutoMode);
	~CQunInfoExt();
	static CQunInfoExt* m_inst;
	static DWORD WINAPI _ThreadProc(LPVOID lpParameter);
	void AddOneJob(BYTE op, DWORD extid, DWORD ver);
	DWORD ThreadProc();
	bool Login(LPSTR pszCode);
	void GetPasswordHash(LPCSTR pszVerifyCode, LPSTR pszOut);
	void EXT_DownloadGroupInfo();
	void EXT_DownloadCardInfo(DWORD dwExtID, DWORD dwVersion);
	HANDLE FindContactByExtID(DWORD dwExtID);

	CNetwork* m_network;
	int m_status;
	DWORD m_uin;
	LPSTR m_password;
	HANDLE m_event;
	HINTERNET m_hInet;
	char m_basepath[MAX_PATH];
	queue<QUNINFOOP> m_queue;
	BOOL m_auto;
};
