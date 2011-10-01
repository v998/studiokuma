#include <stdafx.h>
// #define NOFILTER

void CClientConnection::connectionError() {
	util_log(0,"%s: Default Connection Error",m_name.c_str());
}
void CClientConnection::connectionEstablished() {
	util_log(0,"%s: Default Connection Established",m_name.c_str());
}
void CClientConnection::connectionClosed() {
	util_log(0,"%s: Default Connection Closed",m_name.c_str());
}
void CClientConnection::waitTimedOut() {
	util_log(0,"%s: Default Wait Timed Out",m_name.c_str());
}
int CClientConnection::dataReceived(NETLIBPACKETRECVER* nlpr) {
	util_log(0,"%s: Default Data Received",m_name.c_str());
	return 0;
}
bool CClientConnection::crashRecovery() {
	util_log(0,"%s: Default Crash Recovery",m_name.c_str());
	return false;
}

CClientConnection::CClientConnection(LPSTR szName, int timeout): 
m_name(szName), m_timeout(timeout), m_stopping(false), m_connection(NULL), m_crashts(0) {
	//m_hEvDisconnect=CreateEvent(NULL,TRUE,FALSE,NULL);
}

void CClientConnection::setServer(const bool udp, LPCSTR host, int port) {
	util_log(0,"[CClientConnection] setServer(udp=%d, host=%s, port=%d)",udp,host,port);
	m_host=host;
	if (port!=-1) m_port=port;
	m_redirect=true;
	m_udp=udp;
}

void CClientConnection::setRedirect() {
	m_redirect=true;
}

void CClientConnection::setServer(const bool udp, int host, int port) {
	host=htonl(host);
	setServer(udp, inet_ntoa(*(in_addr*)&host),port);
}
#if defined(STACKWALK) || defined(NOFILTER)
DWORD WINAPI CClientConnection::_dbg_ThreadProc(LPVOID param) {
	((CClientConnection*)param)->_connect();
	return 0;
}
#endif
bool CClientConnection::connect() {
	m_connection=NULL;
#if defined(STACKWALK) || defined(NOFILTER)
	CreateThread(NULL,0,_dbg_ThreadProc,this,0,NULL);
#else
	ForkThread((SocketThreadFunc)&CClientConnection::ThreadProc);
#endif

	return true;
}

extern DWORD Filter( EXCEPTION_POINTERS *ep );

void __cdecl CClientConnection::ThreadProc(void* secondcrash) {
#ifdef NOFILTER
	_connect();
#else
	__try {
		_connect();
#ifdef STACKWALK
	} __except ( Filter( GetExceptionInformation() ) ) {
#else
	} __except (EXCEPTION_EXECUTE_HANDLER) {
#endif
		WCHAR szTemp[MAX_PATH];
		MIRANDASYSTRAYNOTIFY msn={sizeof(msn),"MirandaQQ"};
		swprintf(szTemp,TranslateT("Warning: %S Thread caused an unhandled exception. MirandaQQ is trying to resume operation.\nIf this doesn't work, please restart Miranda IM."),getName().c_str());
		util_log(0,"[CClientConnection] Unhandled Exception occurred inside ThreadProc");
		msn.tszInfo=szTemp;
		msn.tszInfoTitle=L"MirandaQQ";
		msn.dwInfoFlags=NIIF_ERROR|NIIF_INTERN_UNICODE;

		CallService(MS_CLIST_SYSTRAY_NOTIFY,0,(LPARAM)&msn);
		//DebugBreak();

		if (!crashRecovery() && m_connection) {
			if (!secondcrash || time(NULL)-m_crashts>30) {
				m_redirect=true;
				m_crashts=time(NULL);
			} else
				MessageBox(NULL,TranslateT("MirandaQQ is forced to disconnect due to chained crash events. Please reconnect or restart Miranda."),NULL,NIIF_ERROR);

			ThreadProc((LPVOID)1);
		}
	}
#endif // NOFILTER
}

void CClientConnection::_disconnect(LPVOID m_connection) {
	__try {
		CallService(MS_NETLIB_CLOSEHANDLE,(WPARAM)m_connection,NULL);
#ifdef STACKWALK
	} __except ( Filter( GetExceptionInformation() ) ) {
#else
	} __except (EXCEPTION_EXECUTE_HANDLER) {
#endif
		util_log(0,"[CClientConnection] Unhandled Exception occurred inside _disconnect");
		DebugBreak();
	}
}

void CClientConnection::_connect() {
	bool redirect=m_redirect;
	bool error=false;

	while (redirect) {
		NETLIBOPENCONNECTION nloc={sizeof(nloc),m_host.c_str(),m_port,m_udp?NLOCF_UDP:0};
		m_stopping=false;
		m_redirect=false;

		if (m_connection)
			util_log(0,"[%s] Resuming from exception state",getName().c_str());
		else {
			/*
			if (redirect) {
				util_log(0,"[%s] Wait 1 sec before starting new connection",getName().c_str());
				Sleep(1000);
			}
			*/
			m_connection=(HANDLE)CallService(MS_NETLIB_OPENCONNECTION,(WPARAM)hNetlibUser,(LPARAM)&nloc);
		}

		if (m_connection/*=(HANDLE)CallService(MS_NETLIB_OPENCONNECTION,(WPARAM)hNetlibUser,(LPARAM)&nloc)*/) {
			connectionEstablished();
			NETLIBPACKETRECVER nlpr={sizeof(nlpr),m_timeout};
			HANDLE hReceiver=(HANDLE)CallService(MS_NETLIB_CREATEPACKETRECVER,(WPARAM)m_connection,(LPARAM)65535/*&nlpr*/);

			while (!m_stopping) {
				switch (CallService(MS_NETLIB_GETMOREPACKETS,(WPARAM)hReceiver,(LPARAM)&nlpr)) {
					case 0: // Connection closed
						m_stopping=true;
						break;
					case SOCKET_ERROR:
						if (GetLastError()==ERROR_TIMEOUT)
							waitTimedOut();
						else
							m_stopping=true;
						break;
					default:
						dataReceived(&nlpr);
						break;
				}
			}
			util_log(0,"[%s] Connection loop terminated, reconnect=%s",getName().c_str(),m_redirect?"true":"false");
			CallService(MS_NETLIB_CLOSEHANDLE,(WPARAM)hReceiver,NULL);
			if (m_connection) {
				CallService(MS_NETLIB_CLOSEHANDLE,(WPARAM)m_connection,NULL);
			}
			// I need to clone m_redirect here as instance may be deleted.
			redirect=m_redirect;
			m_connection=NULL;
			//connectionClosed();
		} else {
			error=true;
			m_redirect=redirect=false;
			connectionError();
		}
	}

	/*if (!error)*/ connectionClosed();
	//SetEvent(m_hEvDisconnect);
}

CClientConnection::~CClientConnection() {
	disconnect();
	//CloseHandle(m_hEvDisconnect);
	//unregisterConnection(this);
}

void CClientConnection::disconnect() {
	if (m_connection) {
		util_log(0,"[%s] Send disconnect command",getName().c_str());
		m_stopping=true;
		ForkThread((SocketThreadFunc)&CClientConnection::_disconnect,(LPVOID)m_connection);
		m_connection=NULL;
		//util_log(0,"[%s] Wait for connection close",getName().c_str());
		//WaitForSingleObject(m_hEvDisconnect,INFINITE);
		util_log(0,"[%s] Connection closed, exit disconnect thread",getName().c_str());
	}
}

int CClientConnection::sendData(const char* data, int len) {
	return Netlib_Send(m_connection,data,len,MSG_NODUMP);
}
#if 0
/************************************************************************/
CRITICAL_SECTION CClientConnection::m_csReg={0};
map<CClientConnection::CONN_TYPES,CClientConnection*> CClientConnection::m_connections;

void CClientConnection::registerConnection(CONN_TYPES conntype, CClientConnection* connection )
{
	if (!m_csReg.DebugInfo) InitializeCriticalSection(&m_csReg);
	EnterCriticalSection(&m_csReg);

	if (m_connections[conntype]!=NULL) {
		util_log(0,"[CClientConnection] Critical: %s instance recreated! Throwing away old instance!",connection->getName().c_str());
#ifdef _DEBUG
		WCHAR szTemp[MAX_PATH];
		swprintf(szTemp,L"%S object recreated! There is something wrong in code!\n\nDebug?",connection->getName().c_str());
		if (MessageBox(NULL,szTemp,L"[CClientConnection] Critical",MB_ICONERROR|MB_YESNO)==IDYES)
			DebugBreak();

		delete m_connections[conntype];
#endif
	}

	util_log(0,"[CClientConnection]: Register %s instance",connection->getName().c_str());
	m_connections[conntype]=connection;
	LeaveCriticalSection(&m_csReg);
}

void CClientConnection::unregisterConnection( CClientConnection* connection )
{
	util_log(0,"[CClientConnection] Search to unregister %s instance",connection->getName().c_str());
	if (m_csReg.DebugInfo) {
		EnterCriticalSection(&m_csReg);
		for (map<CONN_TYPES,CClientConnection*>::iterator iter=m_connections.begin(); iter!=m_connections.end(); iter++)
			if (iter->second==connection) {
				util_log(0,"[CClientConnection]: Unregister %s instance",iter->second->getName().c_str());
				m_connections.erase(iter->first);
				break;
			}
			LeaveCriticalSection(&m_csReg);
	} else
		util_log(0,"[CClientConnection] Warning: unregisterConnection not executed because m_csReg not initialized");
	//util_log(0,"[CClientConnection] End of unregisterConnection");
}

void CClientConnection::unregisterAllConnections()
{
	if (m_csReg.DebugInfo) {
		CONN_TYPES currentConnection=CONN_TYPE_INVALID;

		while (m_connections.size()) {
			map<CONN_TYPES,CClientConnection*>::iterator iter=m_connections.begin();
			currentConnection=iter->first;
			util_log(0,"[CClientConnection]: Waiting for %s to destroy",iter->second->getName().c_str());
			delete iter->second;
			while (m_connections.size()>0 && m_connections.begin()->first==currentConnection) Sleep(100);
		}
		DeleteCriticalSection(&m_csReg);
		m_csReg.OwningThread=NULL;
	} else
		util_log(0,"[CClientConnection] Warning: unregisterAllConnections not executed because m_csReg not initialized");
}
#endif

int CClientConnection::send(LPCSTR szData, const int len) {
	if (m_connection) return Netlib_Send(m_connection,(LPCSTR)szData,len,MSG_NODUMP);
	return -1;
}

void CClientConnection::disbleWriteBuffer() {
	int val=0;
	if (setsockopt(CallService(MS_NETLIB_GETSOCKET,(WPARAM)m_connection,0),SOL_SOCKET,SO_SNDBUF,(char*)&val,sizeof(int))) {
		util_log(0,"[CClientConnection] setsockopt returned %d",WSAGetLastError());
		DebugBreak();
	}
}

void CClientConnection::ForkThread(SocketThreadFunc func, void* arg) {
	unsigned int threadid;
	mir_forkthreadowner(( pThreadFuncOwner ) *( void** )&func,this,arg,&threadid);
	util_log(0,"ForkThread: tid=%x ep=%p",threadid,func);
}
