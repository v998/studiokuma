#include "stdafx.h"

#define BUFFER_IN 1024
#define BUFFER_OUT 16384

#pragma comment(lib,"ws2_32")

static CHttpServer* s_instance=NULL;
static SOCKET s_serversocket=INVALID_SOCKET;
static COpenProtocolHandler* s_handler;
static HANDLE s_mutex=NULL;

static DWORD WINAPI ClientThread(LPVOID lpParameter) {
	SOCKET s=(SOCKET)lpParameter;
	
	if (!s_mutex) {
		_cprintf("%s() Mutex already freed, dropping request!\n",__FUNCTION__);
	} else {
		char* pszBufferIn=(char*)s_handler->oph_malloc(BUFFER_IN);
		char* pszBufferOut=(char*)s_handler->oph_malloc(BUFFER_OUT);
		char* ppszBufferIn=pszBufferIn;
		int remainIn=BUFFER_IN;
		int received;

		while (true) {
			received=recv(s,ppszBufferIn,remainIn,0);
			if (received==0 || received==SOCKET_ERROR) {
				received=0;
				break;
			}

			remainIn-=received;
			ppszBufferIn+=received;
			*ppszBufferIn=0;

			if (!strcmp(ppszBufferIn-4,"\r\n\r\n")) {
				received=(int)(ppszBufferIn-pszBufferIn);
				break;
			}
		}

		if (received) {
			if (!strncmp(pszBufferIn,"GET /",5)) {
				ppszBufferIn=pszBufferIn+4;
				*strchr(ppszBufferIn,'\r')=0;
				_cprintf("%s(%p): Uri=%s\n",__FUNCTION__,s,ppszBufferIn);

				if (!strncmp(ppszBufferIn,"/qunimages/",11) || !strncmp(ppszBufferIn,"/p2pimages/",11)) {
					LPSTR pszFilename;
					char szCacheFile[260];
					BOOL p2p=!strncmp(ppszBufferIn,"/p2pimages/",11);
					ppszBufferIn+=11;
					*strchr(ppszBufferIn,' ')=0; // Remove HTTP/1.1
					pszFilename=strrchr(ppszBufferIn,'/')+1;

					if (p2p) {
						LPSTR pszTemp;
						strcat(strcpy(szCacheFile,s_instance->m_qunimagepath),"\\");
						pszTemp=szCacheFile+strlen(szCacheFile);
						strcpy(pszTemp,strchr(ppszBufferIn,'/')+1);
						while (strchr(pszTemp,'/')) *strchr(pszTemp,'/')='_';
					} else
						strcat(strcat(strcpy(szCacheFile,s_instance->m_qunimagepath),"\\"),pszFilename);

					FILE* fp=NULL;
					/*if (!p2p)*/ fp=fopen(szCacheFile,"rb"); // Because P2P image has expiry, so caching is OK
					if (fp==NULL) {
						const char* pcszUIN=ppszBufferIn;
						ppszBufferIn=strchr(ppszBufferIn,'/');
						*ppszBufferIn=0;

						COpenProtocol* pOP=COpenProtocol::FindProtocol(pcszUIN);
						if (pOP) {
							Sleep(1000); // Wait 1s to prevent image not yet available
							WaitForSingleObject(s_mutex,INFINITE);
							fp=pOP->handleQunImage(ppszBufferIn+1,p2p);
							ReleaseMutex(s_mutex);
						} else {
							_cprintf("%s() Failed finding instance to handle %s\n",__FUNCTION__,ppszBufferIn+1);
						}
					}

					if (fp) {
						fseek(fp,0,SEEK_END);
						int cbFile=remainIn=ftell(fp);
						char pcszResponse[MAX_PATH];
						LPCSTR pcszExt=strrchr(szCacheFile,'.');

						sprintf(pcszResponse,"HTTP/1.0 200 OK\r\nContent-Type: image/%s\r\nContent-Length: %d\r\nConnection: Close\r\n\r\n",strcmp(pcszExt,".png")?strcmp(pcszExt,".gif")?strcmp(pcszExt,".jpg")?"unknown":"jpeg":"gif":"png",cbFile);
						send(s, pcszResponse, (int)strlen(pcszResponse),0);
						
						fseek(fp,0,SEEK_SET);

						while (remainIn>0) {
							received=(int)fread(pszBufferOut,1,min(BUFFER_OUT,remainIn),fp);
							if (received==0) {
								_cprintf("ERROR: received==0!\n");
								break;
							}
							remainIn-=received;
							send(s, pszBufferOut, received,0);
						}

						fclose(fp);
					} else {
						const char* pcszResponse="HTTP/1.0 404 Not Found\r\nContent-Type: text/plain\r\nConnection: Close\r\n\r\nRequest Failed";

						send(s, pcszResponse, (int)strlen(pcszResponse),0);
					}

				} else {
					const char* pcszResponse="HTTP/1.0 404 Not Found\r\nContent-Type: text/plain\r\nConnection: Close\r\n\r\nRequested resource does not exist";
					send(s, pcszResponse, (int)strlen(pcszResponse),0);
				}
			} else {
				const char* pcszResponse="HTTP/1.0 405 Method Not Allowed\r\nContent-Type: text/plain\r\nConnection: Close\r\n\r\nThis server only serves GET requests.";
				send(s, pcszResponse, (int)strlen(pcszResponse),0);
			}
		} 

		s_handler->oph_free(pszBufferIn);
		s_handler->oph_free(pszBufferOut);
	}
	closesocket(s);
	return 0;
}

static DWORD WINAPI ServerThread(LPVOID lpParameter) {
	CHttpServer* pHS=(CHttpServer*) lpParameter;
	SOCKET clientsocket;
	struct sockaddr_in sin={0};
	int length=sizeof(sin);
	DWORD dwThreadID;

	s_mutex=CreateMutex(NULL,FALSE,NULL);

	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(171);

	_cprintf("%s(): Server socket(%p) listen thread start\n",__FUNCTION__,s_serversocket);

	unsigned char* pbAddr=(unsigned char*)&sin.sin_addr.S_un.S_addr;

	while ((clientsocket=accept(s_serversocket,(sockaddr*)&sin,&length))!=INVALID_SOCKET) {
		_cprintf("%s(): Connection from %d.%d.%d.%d\n",__FUNCTION__,(int)pbAddr[0],(int)pbAddr[1],(int)pbAddr[2],(int)pbAddr[3]);
		CreateThread(NULL,0,ClientThread,(LPVOID)clientsocket,0,&dwThreadID);
	}

	_cprintf("%s(): Wait for last request to end",__FUNCTION__);
	HANDLE hMutex=s_mutex;

	WaitForSingleObject(hMutex,INFINITE);
	s_mutex=NULL;

	ReleaseMutex(hMutex);
	CloseHandle(hMutex);

	_cprintf("%s(): Server socket listen thread end, err=%d\n",__FUNCTION__,WSAGetLastError());

	return 0;
}

CHttpServer::CHttpServer(COpenProtocolHandler* handler, unsigned short port, const char* qunimagepath):
m_port(port) {
	s_handler=handler;
	s_serversocket=INVALID_SOCKET;
	s_instance=NULL;

	if (port!=0) {
		_cprintf("%s(): Initialize at port %d\n",__FUNCTION__,port);

		strcpy(m_qunimagepath,qunimagepath);

		WSADATA wsaData;
		struct sockaddr_in sin={0};

		sin.sin_family = AF_INET;
		sin.sin_addr.s_addr = INADDR_ANY;
		sin.sin_port = htons(port);

		if (FAILED(WSAStartup(MAKEWORD(2,0), &wsaData))) {
			_cprintf("%s(): WSAStartup failed\n",__FUNCTION__);
		} else if ((s_serversocket=socket(AF_INET,SOCK_STREAM,0))==INVALID_SOCKET) {
			_cprintf("%s(): socket failed\n",__FUNCTION__);
		} else if (FAILED(bind(s_serversocket, (sockaddr*)&sin, sizeof(sin)))) {
			_cprintf("%s(): Bind failed\n",__FUNCTION__);
		} else if (!SUCCEEDED(listen(s_serversocket, SOMAXCONN))) {
			_cprintf("%s(): Listen failed\n",__FUNCTION__);
		} else {
			DWORD dwThreadID;
			s_instance=this;
			CreateThread(NULL,0,ServerThread,this,0,&dwThreadID);
		}
	}

	if (!s_instance) delete this;
}

CHttpServer::~CHttpServer() {
	if (s_serversocket!=INVALID_SOCKET) closesocket(s_serversocket);
	WSACleanup();
	s_instance=NULL;
}

CHttpServer* CHttpServer::GetInstance(unsigned short port, const char* qunimagepath, COpenProtocolHandler* handler) {
	if (!s_instance && port!=0 && qunimagepath!=NULL) new CHttpServer(handler,port,qunimagepath);

	return s_instance;
}
