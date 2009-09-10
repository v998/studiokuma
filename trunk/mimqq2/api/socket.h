#ifndef SOCKET_H
#define SOCKET_H

#define slot protected
#define emit

class CClientConnection;
typedef void (__cdecl CClientConnection::*SocketThreadFunc)(LPVOID);

class CClientConnection {
public:
	//CClientConnection(CConnectionCallback* callback, bool udp, int timeout);
	CClientConnection(char* name, int timeout);
	virtual ~CClientConnection();
	void setServer(const bool udp, LPCSTR host, int port);
	void setServer(const bool udp, int host, int port);
	bool connect();
	void disconnect();
	int sendData(const char* data, int len);
	bool isUDP() const { return m_udp; }
	string getHost() const { return m_host; }
	int getPort() const { return m_port; }
	const string &getName() const { return m_name; }

	//static void unregisterAllConnections();

protected:
	enum CONN_TYPES {
		CONN_TYPE_INVALID,
		CONN_TYPE_MAIN,
		CONN_TYPE_USERHEAD,
		CONN_TYPE_QUNIMAGE,
		CONN_TYPE_FT
	};

	void _connect();
	void __cdecl ThreadProc(LPVOID lpParameter);
	void __cdecl _disconnect(LPVOID);

	/*
	static void registerConnection(CONN_TYPES conntype, CClientConnection* connection);
	static void unregisterConnection(CClientConnection* connection);
	*/
	const bool isConnected() const { return m_connection!=NULL; }
	const bool isRedirect() const { return m_redirect; }
	void disbleWriteBuffer();
	void send(LPCSTR szData, const int len);

	static DWORD WINAPI _dbg_ThreadProc(LPVOID param);

slot:
	virtual void connectionError();
	virtual void connectionEstablished();
	virtual void connectionClosed();
	virtual void waitTimedOut();
	virtual int dataReceived(NETLIBPACKETRECVER* nlpr);
	virtual bool crashRecovery();

private:
	void ForkThread(SocketThreadFunc func, void* arg=NULL);

	//CConnectionCallback* m_callback;
	string m_host;
	int m_port;
	bool m_udp;
	HANDLE m_connection;
	int m_timeout;
	bool m_stopping;
	bool m_redirect;
	string m_name;

	/*
	static map<CONN_TYPES,CClientConnection*> m_connections;
	static CRITICAL_SECTION m_csReg;
	HANDLE m_hEvDisconnect;
	*/
};


#endif // SOCKET_H