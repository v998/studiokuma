#include "StdAfx.h"

// Take care!
#pragma warning(disable: 4003)

#define PARSE2(packetType,action) packetType packet; packet.setInPacket(in); if (!packet.parse()) {action;return;}
#define HANDLE(cmd,handler) case cmd: handler(packet); break
#define CB(subcmd,w,l) callbackHub(in->getCommand(),subcmd,(WPARAM)w,(LPARAM)l)
#define DEFCB() callbackHub(in->getCommand(),0,(WPARAM)&packet,(LPARAM)0)
#define DEFAULT_HANDLER(fn,pn) void CNetwork::fn(InPacket* in) { PARSE2(pn); DEFCB(); }
//#define CMD2NAME(x) case x: src=#x; break
void callbackHub(int command, int subcommand, WPARAM wParam, LPARAM lParam);

CNetwork::CNetwork(LPCSTR szModuleName, LPCTSTR szUserName):
CClientConnection("CNETWORK",1000),
m_userhead(NULL), m_qunimage(NULL), /*m_savedTempSessionMsg(NULL),*/ m_deferActionType(0), m_uhTriggered(FALSE), m_packet(NULL) {
	m_szModuleName=m_szProtoName=mir_strdup(szModuleName);
	if (szUserName) {
		m_tszUserName=(LPWSTR)mir_alloc((wcslen(szUserName)+5)*sizeof(wchar_t));
		swprintf(m_tszUserName,L"QQ(%s)",szUserName);
	} else {
		m_tszUserName=(LPWSTR)mir_alloc((strlen(m_szModuleName)+5)*sizeof(wchar_t));
		swprintf(m_tszUserName,L"QQ(%S)",m_szModuleName);
	}
	
	m_iStatus=ID_STATUS_OFFLINE;
	m_iXStatus=0;

	SetResident();
	LoadAccount();

	ZeroMemory(&m_currentMedia, sizeof(m_currentMedia));
	ZeroMemory(&m_client,sizeof(m_client));

	// m_libevabuffer=(unsigned char*)mir_alloc(MAX_PACKET_SIZE);

	if (m_conservative=READ_B2(NULL,QQ_CONSERVATIVE)) {
		util_log(0,"WARNING - Conservative Mode in effect for %S",m_szModuleName);
		util_log(0,"Conservative State: %d",(int)READ_B2(NULL,QQ_CONSERVATIVESTATE));
	}

	registerCallbacks();
}

CNetwork::~CNetwork() {
	// if (m_savedTempSessionMsg) delete m_savedTempSessionMsg;

	UnloadAccount();
	mir_free(m_szModuleName);
	mir_free(m_tszUserName);
	util_log(0,"[CNetwork] Instance Destruction Complete");
}

#define SETRESIDENTVALUE(a) strcpy(pszTemp,a); CallService(MS_DB_SETSETTINGRESIDENT,TRUE,(LPARAM)szTemp)
void CNetwork::SetResident() {
	CHAR szTemp[MAX_PATH];
	LPSTR pszTemp;
	strcpy(szTemp,m_szProtoName);
	strcat(szTemp,"/");
	pszTemp=szTemp+strlen(szTemp);

	//SETRESIDENTVALUE("Status");
	SETRESIDENTVALUE("LoginTS");
	SETRESIDENTVALUE("IP");
	SETRESIDENTVALUE("LastLoginTS");
	SETRESIDENTVALUE("LastLoginIP");
	SETRESIDENTVALUE("LoginTS");
	SETRESIDENTVALUE("QunInit");
	SETRESIDENTVALUE("ServerQun");
	SETRESIDENTVALUE("QunVersion");
	SETRESIDENTVALUE("CardVersion");
}

bool CNetwork::setConnectString(LPCSTR pszConnectString) {
	LPSTR pszBuffer=mir_strdup(pszConnectString);
	LPSTR pszServer=strstr(pszBuffer,"://");
	LPSTR pszPort=strrchr(pszBuffer,':');
	HANDLE hContact=NULL;
	m_client.network=*pszBuffer=='t'?TCP:UDP;
	USHORT port;

	if ((*pszBuffer!='t' && *pszBuffer!='u') || pszServer==NULL) {
		mir_free(pszBuffer);
		return false;
	}

	if (pszPort[1]=='/') pszPort=NULL;
	if (pszPort[1])
		*pszPort=0;
	else
		pszPort=":80";

	port=(USHORT)atoi(pszPort+1);

	if (READC_B2("NLUseProxy")==1 && READC_B2("NLProxyType")==3) {
		// HTTP Proxy Enabled
		port=443;
	}

	setServer(m_client.network==UDP,pszServer+3,port);
	mir_free(pszBuffer);
	return true;
}

void CNetwork::connectionError() {
	util_log(0,"[%u:CNetwork] Connection Error",m_myqq);
	if (m_client.login_finish!=1) {
		ShowNotification(TranslateT("Failed connecting to server"),NIIF_ERROR);
		QQ_SendBroadcast( NULL, ACKTYPE_LOGIN, ACKRESULT_FAILED, NULL, LOGINERR_NOSERVER );
	}
}

void CNetwork::connectionEstablished() {
	if (m_client.data.server_info.w_redirect_count) {
		packetmgr_new_seqno(&m_client);
		waitTimedOut();
		prot_login_touch_with_info(&m_client, m_client.data.server_data, sizeof(m_client.data.server_data));
	} else {
		m_downloadGroup=false;
		qqclient_login(&m_client);
	}
}

void CNetwork::connectionClosed() {
	util_log(0,"[%u:CNetwork] Connection Closed",m_myqq);
	if (!Miranda_Terminated()) GoOffline();
}

extern "C" void qqclient_keepalive( qqclient* data );

void CNetwork::waitTimedOut() {
	qqclient_keepalive(&m_client);
}

int CNetwork::dataReceived(NETLIBPACKETRECVER* nlpr) {
	qqpacketmgr* mgr = &m_client.packetmgr;
	qqpacket* p=packetmgr_new_packet(&m_client);

	if (m_client.network==UDP) {
		// Direct process

		handle_packet(&m_client, p, nlpr->buffer,nlpr->bytesAvailable);

		nlpr->bytesUsed+=nlpr->bytesAvailable;
	} else {
		// Segmentation
		USHORT packetLen;
		while (true) {
			if (nlpr->bytesUsed>=nlpr->bytesAvailable) break;
			packetLen=ntohs(*(USHORT*)(nlpr->buffer+nlpr->bytesUsed));
			if (nlpr->bytesUsed+packetLen>nlpr->bytesAvailable) break;

			if (nlpr->bytesUsed!=0) util_log(0,"Segmentation, offset=%d, size=%d",nlpr->bytesUsed,packetLen);
			handle_packet(&m_client, p, nlpr->buffer+nlpr->bytesUsed,packetLen);

			nlpr->bytesUsed+=packetLen;
		}
	}
	packetmgr_del_packet(mgr,p);
	return 0;
}

void CNetwork::redirect(int host, int port) {
	setServer(m_client.network==UDP,host,port);
	util_log(0,"[%u:CNetwork] Server Detector redirect to %s:%d",m_myqq,getHost().c_str(),getPort());
	disconnect();
}

const bool CNetwork::TriggerConservativeState() {
	bool fRet=READ_B2(NULL,QQ_CONSERVATIVESTATE);
	if (!fRet) WRITE_B(NULL,QQ_CONSERVATIVESTATE,1);
	return fRet;
}

bool CNetwork::crashRecovery() {
	util_log(0,"CNetwork Crash Recovery");

	if (!IsBadReadPtr(m_packet,4)) {
		HANDLE hFile=CreateFile(L"C:\\mimqq3.log",GENERIC_WRITE,FILE_SHARE_READ,NULL,CREATE_ALWAYS,0,NULL);
		if (hFile!=INVALID_HANDLE_VALUE) {
			DWORD dwWritten;
			WriteFile(hFile,m_packet->buf->data,m_packet->buf->len,&dwWritten,NULL);
			CloseHandle(hFile);
			ShowNotification(L"MirandaQQ error packet dump has been written to c:\\mimqq3.log. Please sumbit for support.",NIIF_ERROR);
		} else {
			util_log(0,"Unable to write log file");
		}
	}
	return false;
}
