#include "StdAfx.h"

CHttpServer* CHttpServer::g_hInst=NULL;

CHttpServer::CHttpServer(CProtocol* lpBaseProtocol):
m_baseProtocol(lpBaseProtocol),
m_connection(NULL) {
	char szDllName[MAX_PATH];
	NETLIBBIND nlb={sizeof(nlb)};
	g_hInst=this;

	lpBaseProtocol->GetModuleName(szDllName);

	nlb.pfnNewConnectionV2=newConnectionProc;
	nlb.wPort=m_port=DBGetContactSettingWord(NULL,szDllName,QQ_HTTPDPORT,170);
	nlb.pExtra=this;

	m_baseProtocol->QLog("[CHttpServer] CHttpServer Initialization, port=%d",nlb.wPort);

	if (!(m_connection=(HANDLE)CallService(MS_NETLIB_BINDPORT,(WPARAM)lpBaseProtocol->GetNetlibUser(),(LPARAM)&nlb))) {
		m_baseProtocol->QLog("[CHttpServer] Binding to port %d failed!",nlb.wPort);
		MessageBoxW(NULL,TranslateT("Warning: Failed to bind port for image web server. Qun image for IEView not available."),L"mimqq4",MB_ICONERROR);
	}

	GetModuleFileNameA(NULL,m_cachepath,MAX_PATH);
	CreateDirectoryA(strcpy(strrchr(m_cachepath,'\\')+1,"QQ"),NULL);
	CreateDirectoryA(strcat(m_cachepath,"\\QunImages"),NULL);
	strcat(m_cachepath,"\\");
}

CHttpServer::~CHttpServer() {
	g_hInst=NULL;

	if (m_connection) Netlib_CloseHandle(m_connection);

	while (m_qunlinks.size()) {
		mir_free(m_qunlinks.front());
		m_qunlinks.pop_front();
	}
}

void CHttpServer::newConnectionProc(HANDLE hNewConnection,DWORD dwRemoteIP, void * pExtra) {
	((CHttpServer*)pExtra)->_newConnectionProc(hNewConnection,dwRemoteIP);
}

void CHttpServer::RegisterQunImage(LPCSTR pszLink, CProtocol *lpProtocol) {
	if (m_connection) {
		LPSTR pszCopy;
		m_baseProtocol->QLog("[CHttpServer] Register link: %s",pszLink);
		m_qunlinks.push_back(pszCopy=mir_strdup(pszLink));
		m_qunlinkProtocols[pszCopy]=lpProtocol;
	}
}

CHttpServer* CHttpServer::GetInstance(CProtocol *lpBaseProtocol) {
	if (g_hInst==NULL && lpBaseProtocol!=NULL) {
		return new CHttpServer(lpBaseProtocol);
	}

	return g_hInst;
}

void CHttpServer::_newConnectionProc(HANDLE hNewConnection,DWORD dwRemoteIP) {
	if ((dwRemoteIP&0xff000000)==0x7f000000/* || DBGetContactSettingByte(NULL,g_dllname,QQ_HTTPDALLOWEXTERNAL,0)==1*/) {
		char szTemp[1024];
		LPBYTE pbIP=(LPBYTE)&dwRemoteIP;

		// dwRemoteIP=htonl(dwRemoteIP);
		m_baseProtocol->QLog("[CHttpServer] Connection from %d.%d.%d.%d",pbIP[3],pbIP[2],pbIP[1],pbIP[0]);
		Netlib_Recv(hNewConnection,szTemp,1024,MSG_DUMPASTEXT);

		if (!strncmp(szTemp,"GET ",4)) {
			char* fileName=szTemp+4;
			char szUTF[MAX_PATH];
			DWORD len=MAX_PATH;
			*strchr(fileName,' ')=0;
			InternetCanonicalizeUrlA(fileName,szUTF,&len,ICU_DECODE|ICU_NO_ENCODE);
			if (strstr(szUTF,"../")) {
				char* szSend="HTTP/1.0 403 Forbidden\nContent-Type: text/plain\nConnection: Close\n\nDirectory traversal attack detected.";
				Netlib_Send(hNewConnection,szSend,(int)strlen(szSend),MSG_NODUMP);
				writeTails(hNewConnection);
				m_baseProtocol->QLog("[CHttpServer] Directory traversal attack detected!");
			} else if (strstr(szUTF,"/cgi/svr/chatimg/get?pic=") || 
				       strstr(szUTF,"/cgi-bin/get_group_pic?gid=") ||
					   strstr(szUTF,"/channel/get_cface?lcid=") ||
					   strstr(szUTF,"/web2p2pimg/") ||
					   strstr(szUTF,"/channel/get_offpic?file_path=")) {
					int type=strstr(szUTF,"/cgi/")?1:strstr(szUTF,"/cgi-")?2:strstr(szUTF,"/get_cface?")?3:strstr(szUTF,"/w")?4:5;

				if ((type==1 && (!strstr(szUTF,"&gid=") || !strstr(szUTF,"&time="))) || 
					(type==2 && (!strstr(szUTF,"&uin=") || !strstr(szUTF,"&rip=") || !strstr(szUTF,"&rport=") || !strstr(szUTF,"&fid=") || !strstr(szUTF,"&pic="))) ||
					(type==3 && (!strstr(szUTF,"&guid=") || !strstr(szUTF,"&to=") || !strstr(szUTF,"&count=") || !strstr(szUTF,"&time=") || !strstr(szUTF,"&clientid="))) ||
					(type==4 && (!strstr(szUTF,"?ver=") || !strstr(szUTF,"&rkey=") || !strstr(szUTF,"&file_path=") || !strstr(szUTF,"&file_size="))) ||
					(type==5 && (!strstr(szUTF,"&f_uin=") || !strstr(szUTF,"&clientid=")))) {
					char* szSend="HTTP/1.0 400 Bad Request\nContent-Type: text/plain\nConnection: Close\n\nError: Qun Image URL is incomplete.";
					Netlib_Send(hNewConnection,szSend,(int)strlen(szSend),MSG_NODUMP);
					writeTails(hNewConnection);
					m_baseProtocol->QLog("[CHttpServer] Method 1 Bad Request (Missing arguments)");
				} else {
					// Note: filename/szUTF does not include hostname
					char szCacheFile[MAX_PATH];
					strcpy(szCacheFile,m_cachepath);
					if (type==1)
						*strstr(strcat(szCacheFile,strchr(szUTF,'=')+1),"&gid=")=0;
					else if (type==2)
						strcat(szCacheFile,strstr(szUTF,"&pic=")+5);
					else if (type==3)
						*strstr(strcat(szCacheFile,strstr(szUTF,"&guid=")+6),"&to=")=0;
					else if (type==4)
						*strstr(strcat(szCacheFile,strstr(szUTF,"&file_name=")+11),"&file_size=")=0;
					else if (type==5)
						*strstr(strcat(szCacheFile,strstr(szUTF,"?file_path=/")+12),"&f_uin=")=0;

					if (GetFileAttributesA(szCacheFile)!=INVALID_FILE_ATTRIBUTES) {
						HANDLE hFile=CreateFileA(szCacheFile,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,0,NULL);
						if (hFile!=INVALID_HANDLE_VALUE) {
							DWORD dwFile=GetFileSize(hFile,NULL);
							DWORD dwRead;
							LPSTR pszBuffer=(LPSTR)LocalAlloc(LMEM_FIXED,dwFile);
							ReadFile(hFile,pszBuffer,dwFile,&dwRead,NULL);
							CloseHandle(hFile);
							*szCacheFile=0; // Clear this to stop fetching

							m_baseProtocol->QLog("[CHttpServer] Method 1 Cache Hit!");
							sprintf(szTemp,"HTTP/1.0 200 OK from Cache\nContent-Type: %s\nConnection: Close\n\n",stricmp(szUTF+strlen(szUTF)-4,".jpg")==0?"image/jpg":stricmp(szUTF+strlen(szUTF)-4,".gif")==0?"image/gif":"unknown/unknown");
							Netlib_Send(hNewConnection,szTemp,(int)strlen(szTemp),MSG_NODUMP);
							Netlib_Send(hNewConnection,pszBuffer,dwRead,MSG_NODUMP);
							Netlib_CloseHandle(hNewConnection);
						}
					}
					
					if (*szCacheFile) {
						int c=0;
						for (list<LPSTR>::iterator iter=m_qunlinks.begin(); iter!=m_qunlinks.end(); iter++) {
							if (!strcmp(*iter,fileName)) {
								CProtocol* prot=m_qunlinkProtocols[*iter];
								if (type==4) {
									*strstr(strcat(strcpy(szTemp,"http://"),strstr(szUTF,"/web2p2pimg/")+12),"&file_path=")=0;
								} else
									strcat(strcpy(szTemp,type==1?"http://qun.qq.com":type==2?"http://web2.qq.com":/*(type==3||type==5)?*/"http://web2-b.qq.com"),szUTF);
								// Sleep(1000);

								DWORD dwLen=0;
								CLibWebQQ* webqq=prot->GetWebQQ();

								if (!webqq) {
									char* szSend="HTTP/1.0 500 Internal Server Error\nContent-Type: text/plain\nConnection: Close\n\nThe protocol is currently offline.";
									m_baseProtocol->QLog("[CHttpServer] Method 1 No WebQQ!");
									Netlib_Send(hNewConnection,szSend,(int)strlen(szSend),MSG_NODUMP);
									writeTails(hNewConnection);
								} else {
									LPSTR pszDoc=prot->GetWebQQ()->GetHTMLDocument(szTemp,prot->GetWebQQ()->GetReferer(CLibWebQQ::WEBQQ_REFERER_WEBQQ),&dwLen);
									m_baseProtocol->QLog("[CHttpServer] url=%s, len=%u",szTemp,dwLen);
									if (!pszDoc || dwLen==(DWORD)-1 || dwLen==0) {
										char* szSend="HTTP/1.0 500 Internal Server Error\nContent-Type: text/plain\nConnection: Close\n\nError relaying qun image from remote server.";
										Netlib_Send(hNewConnection,szSend,(int)strlen(szSend),MSG_NODUMP);
										writeTails(hNewConnection);
									} else {
										if ((dwLen!=5991 || memcmp(pszDoc+1,"PNG",3) || memcmp(pszDoc+57,"OiCCPPhotoshop ICC profile",26))/* && memcmp(pszDoc,"{\"",2)*/) {
											HANDLE hFile=CreateFileA(szCacheFile,GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,CREATE_ALWAYS,0,NULL);
											if (hFile!=INVALID_HANDLE_VALUE) {
												DWORD dwWritten;
												WriteFile(hFile,pszDoc,dwLen,&dwWritten,NULL);
												CloseHandle(hFile);
												if (dwLen!=dwWritten) {
													m_baseProtocol->QLog("[CHttpServer] dwLen!=dwWritten! dwLen=%u dwWritten=%u GetLastError=%x",dwLen,dwWritten,GetLastError());
												}
											}
										} else {
											m_baseProtocol->QLog("[CHttpServer] Broken Image from Server");
										}
										sprintf(szTemp,"HTTP/1.0 200 OK\nContent-Type: %s\nConnection: Close\n\n",stricmp(szUTF+strlen(szUTF)-4,".jpg")==0?"image/jpg":stricmp(szUTF+strlen(szUTF)-4,".gif")==0?"image/gif":"unknown/unknown");
										Netlib_Send(hNewConnection,szTemp,(int)strlen(szTemp),MSG_NODUMP);
										Netlib_Send(hNewConnection,pszDoc,dwLen,MSG_NODUMP);
										Netlib_CloseHandle(hNewConnection);
										LocalFree(pszDoc);
									}
								}
								m_qunlinkProtocols.erase(*iter);
								m_qunlinks.erase(iter);
								c=-1;
								break;
							}
							c++;
						}
						if (c!=-1) {
							char* szSend="HTTP/1.0 400 Bad Request\nContent-Type: text/plain\nConnection: Close\n\nRequested URL is not registered in the queue. I don't know where to forward the request...";
							m_baseProtocol->QLog("[CHttpServer] Method 1 Bad Request (Not Registered for %s)",fileName);
							Netlib_Send(hNewConnection,szSend,(int)strlen(szSend),MSG_NODUMP);
							writeTails(hNewConnection);
						}
					}
				}
#if 0 // Web1
			} else if (strstr(szUTF,"/p2p?sender=")) {
				if (!strstr(szUTF,"&ts=") || !strstr(szUTF,"&pic=")) {
					char* szSend="HTTP/1.0 400 Bad Request\nContent-Type: text/plain\nConnection: Close\n\nError: P2P Image URL is incomplete.";
					Netlib_Send(hNewConnection,szSend,(int)strlen(szSend),MSG_NODUMP);
					writeTails(hNewConnection);
					m_baseProtocol->QLog("[CHttpServer] Method 2 Bad Request (Missing arguments)");
				} else {
					// Note: filename/szUTF does not include hostname

					char szCacheFile[MAX_PATH];
					strcpy(szCacheFile,m_cachepath);
					strcat(szCacheFile,strstr(szUTF,"&pic=")+5);

					if (GetFileAttributesA(szCacheFile)!=INVALID_FILE_ATTRIBUTES) {
						HANDLE hFile=CreateFileA(szCacheFile,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,0,NULL);
						if (hFile!=INVALID_HANDLE_VALUE) {
							DWORD dwFile=GetFileSize(hFile,NULL);
							DWORD dwRead;
							LPSTR pszBuffer=(LPSTR)LocalAlloc(LMEM_FIXED,dwFile);
							ReadFile(hFile,pszBuffer,dwFile,&dwRead,NULL);
							CloseHandle(hFile);
							*szCacheFile=0; // Clear this to stop fetching

							m_baseProtocol->QLog("[CHttpServer] Method 2 Cache Hit!");
							sprintf(szTemp,"HTTP/1.0 200 OK from Cache\nContent-Type: %s\nConnection: Close\n\n",stricmp(szUTF+strlen(szUTF)-4,".jpg")==0?"image/jpg":stricmp(szUTF+strlen(szUTF)-4,".gif")==0?"image/gif":"unknown/unknown");
							Netlib_Send(hNewConnection,szTemp,(int)strlen(szTemp),MSG_NODUMP);
							Netlib_Send(hNewConnection,pszBuffer,dwRead,MSG_NODUMP);
							Netlib_CloseHandle(hNewConnection);
						}
					}
					
					if (*szCacheFile) {
						int c=0;
						for (list<LPSTR>::iterator iter=m_qunlinks.begin(); iter!=m_qunlinks.end(); iter++) {
							if (!strcmp(*iter,fileName)) {
								CProtocol* prot=m_qunlinkProtocols[*iter];
								CLibWebQQ* webqq=prot->GetWebQQ();

								if (!webqq) {
									char* szSend="HTTP/1.0 500 Internal Server Error\nContent-Type: text/plain\nConnection: Close\n\nThe protocol is currently offline.";
									Netlib_Send(hNewConnection,szSend,(int)strlen(szSend),MSG_NODUMP);
									writeTails(hNewConnection);
									m_baseProtocol->QLog("[CHttpServer] Method 2 No WebQQ!");
								} else {
									P2PARGS pa;
									pa.hConnection=hNewConnection;
									pa.protocol=prot;
									pa.uri=*iter;
									pa.sender=strtoul(strstr(pa.uri,"?sender=")+8,NULL,10);
									pa.receiver=webqq->GetQQID();
									pa.timestamp=strtoul(strstr(pa.uri,"&ts=")+4,NULL,10);
									pa.filename=strstr(pa.uri,"&pic=")+5;
									m_qunlinkProtocols.erase(*iter);
									m_qunlinks.erase(iter);
									webqq->SendP2PRetrieveRequest(pa.sender,"C");
									m_p2psessions[pa.receiver]=pa;
								}

								c=-1;
								break;
							}
							c++;
						}
						if (c!=-1) {
							char* szSend="HTTP/1.0 400 Bad Request\nContent-Type: text/plain\nConnection: Close\n\nRequested URL is not registered in the queue. I don't know where to forward the request...";
							Netlib_Send(hNewConnection,szSend,(int)strlen(szSend),MSG_NODUMP);
							writeTails(hNewConnection);
							m_baseProtocol->QLog("[CHttpServer] Method 2 Bad Request (Not Registered for %s)",fileName);
						}
					}
				}
#endif // Web1
			} else if (strstr(szUTF,"/cgi-bin/webqq_app/?cmd=2&bd=")) { // P2P cface preview
				// Note: filename/szUTF does not include hostname
				char szCacheFile[MAX_PATH];
				strcat(strcpy(szCacheFile,m_cachepath),strstr(szUTF,"&bd=")+4);

				if (GetFileAttributesA(szCacheFile)!=INVALID_FILE_ATTRIBUTES) {
					HANDLE hFile=CreateFileA(szCacheFile,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,0,NULL);
					if (hFile!=INVALID_HANDLE_VALUE) {
						DWORD dwFile=GetFileSize(hFile,NULL);
						DWORD dwRead;
						LPSTR pszBuffer=(LPSTR)LocalAlloc(LMEM_FIXED,dwFile);
						ReadFile(hFile,pszBuffer,dwFile,&dwRead,NULL);
						CloseHandle(hFile);
						*szCacheFile=0; // Clear this to stop fetching

						m_baseProtocol->QLog("[CHttpServer] Method 3 Cache Hit!");
						sprintf(szTemp,"HTTP/1.0 200 OK from Cache\nContent-Type: %s\nConnection: Close\n\n",stricmp(szUTF+strlen(szUTF)-4,".jpg")==0?"image/jpg":stricmp(szUTF+strlen(szUTF)-4,".gif")==0?"image/gif":"unknown/unknown");
						Netlib_Send(hNewConnection,szTemp,(int)strlen(szTemp),MSG_NODUMP);
						Netlib_Send(hNewConnection,pszBuffer,dwRead,MSG_NODUMP);
						Netlib_CloseHandle(hNewConnection);
					}
				}
				
				if (*szCacheFile) {
					int c=0;
					for (list<LPSTR>::iterator iter=m_qunlinks.begin(); iter!=m_qunlinks.end(); iter++) {
						if (!strcmp(*iter,fileName)) {
							CProtocol* prot=m_qunlinkProtocols[*iter];
							strcat(strcpy(szTemp,"http://web.qq.com"),szUTF);
							// Sleep(1000);

							DWORD dwLen=0;
							CLibWebQQ* webqq=prot->GetWebQQ();

							if (!webqq) {
								char* szSend="HTTP/1.0 500 Internal Server Error\nContent-Type: text/plain\nConnection: Close\n\nThe protocol is currently offline.";
								Netlib_Send(hNewConnection,szSend,(int)strlen(szSend),MSG_NODUMP);
								writeTails(hNewConnection);
								m_baseProtocol->QLog("[CHttpServer] Method 3 No WebQQ!");
							} else {
								LPSTR pszDoc=prot->GetWebQQ()->GetHTMLDocument(szTemp,prot->GetWebQQ()->GetReferer(CLibWebQQ::WEBQQ_REFERER_WEBQQ),&dwLen);
								if (!pszDoc || dwLen==(DWORD)-1 || dwLen==0) {
									char* szSend="HTTP/1.0 500 Internal Server Error\nContent-Type: text/plain\nConnection: Close\n\nError relaying qun image from remote server.";
									Netlib_Send(hNewConnection,szSend,(int)strlen(szSend),MSG_NODUMP);
									writeTails(hNewConnection);
									m_baseProtocol->QLog("[CHttpServer] Method 3 Bad Reply!");
								} else {
									if (dwLen!=5991 || memcmp(pszDoc+1,"PNG",3) || memcmp(pszDoc+57,"OiCCPPhotoshop ICC profile",26)) {
										HANDLE hFile=CreateFileA(szCacheFile,GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,CREATE_ALWAYS,0,NULL);
										if (hFile!=INVALID_HANDLE_VALUE) {
											DWORD dwWritten;
											WriteFile(hFile,pszDoc,dwLen,&dwWritten,NULL);
											CloseHandle(hFile);
										}
									} else {
										m_baseProtocol->QLog("[CHttpServer] Method 3 Broken Image from Server");
									}
									sprintf(szTemp,"HTTP/1.0 200 OK\nContent-Type: %s\nConnection: Close\n\n",stricmp(szUTF+strlen(szUTF)-4,".jpg")==0?"image/jpg":stricmp(szUTF+strlen(szUTF)-4,".gif")==0?"image/gif":"unknown/unknown");
									Netlib_Send(hNewConnection,szTemp,(int)strlen(szTemp),MSG_NODUMP);
									Netlib_Send(hNewConnection,pszDoc,dwLen,MSG_NODUMP);
									Netlib_CloseHandle(hNewConnection);
								}
							}
							m_qunlinkProtocols.erase(*iter);
							m_qunlinks.erase(iter);
							c=-1;
							break;
						}
						c++;
					}
					if (c!=-1) {
						char* szSend="HTTP/1.0 400 Bad Request\nContent-Type: text/plain\nConnection: Close\n\nRequested URL is not registered in the queue. I don't know where to forward the request...";
						Netlib_Send(hNewConnection,szSend,(int)strlen(szSend),MSG_NODUMP);
						writeTails(hNewConnection);
						m_baseProtocol->QLog("[CHttpServer] Method 3 Bad Request (Not Registered for %s)",fileName);
					}
				}
			} else {
				char* szSend="HTTP/1.0 404 Not Found\nContent-Type: text/plain\nConnection: Close\n\nThis server only serves qun images for now.";
				Netlib_Send(hNewConnection,szSend,(int)strlen(szSend),MSG_NODUMP);
				writeTails(hNewConnection);
				m_baseProtocol->QLog("[CHttpServer] No Service");
			}
		} else {
			char* szSend="HTTP/1.0 405 Method Not Allowed\nContent-Type: text/plain\nConnection: Close\n\nThis server only serves GET requests.";
			Netlib_Send(hNewConnection,szSend,(int)strlen(szSend),MSG_NODUMP);
			writeTails(hNewConnection);
			m_baseProtocol->QLog("[CHttpServer] Method not allowed");
		}
	} else
		Netlib_CloseHandle(hNewConnection);
}

void CHttpServer::writeTails(HANDLE hConnection) {
	char szTemp[MAX_PATH]="\n\nMIMQQ/";
	strcat(szTemp,VERSION_DOT);
	while (strchr(szTemp,',')) *strchr(szTemp,',')='.';
	Netlib_Send(hConnection,szTemp,(int)strlen(szTemp),MSG_NODUMP);
	Netlib_CloseHandle(hConnection);
}

void CHttpServer::UnregisterQunImages(CProtocol* lpProtocol) {
	for (list<LPSTR>::iterator iter=m_qunlinks.begin(); iter!=m_qunlinks.end();) {
		if (m_qunlinkProtocols[*iter]==lpProtocol) {
			m_qunlinkProtocols.erase(*iter);
			iter=m_qunlinks.erase(iter);
		} else
			iter++;
	}
}

void CHttpServer::HandleP2PImage(DWORD uin, LPWEBQQ_MESSAGE lpMsg) {
	P2PARGS pa=m_p2psessions[uin];
	if (pa.uri!=NULL) {
		pa.timestamp=lpMsg->timestamp;
		pa.objectid=strtoul(lpMsg->fileID,NULL,10);
		m_p2psessions[uin]=pa;
		CreateThreadObj(&CHttpServer::DownloadP2PImage,(LPVOID)uin);
	} else
		m_baseProtocol->QLog("CHttpServer error: no p2p session found for uin %u",uin);
}

void __cdecl CHttpServer::DownloadP2PImage(LPVOID pUin) {
	P2PARGS pa=m_p2psessions[(DWORD)pUin];
	if (pa.uri!=NULL) {
		char szUrl[MAX_PATH];
		CLibWebQQ* webqq=pa.protocol->GetWebQQ();
		LPSTR pszParams=webqq->GetStorage(WEBQQ_STORAGE_PARAMS);

		sprintf(szUrl,"http://file1.web.qq.com/%u/%u/%u/%s/%s/1/c/%u/%s?t=%u",pa.receiver,pa.sender,pa.objectid,webqq->GetArgument(pszParams,SC0X22_SVRINDEX_AND_PORT_0),webqq->GetArgument(pszParams,SC0X22_SVRINDEX_AND_PORT_1),webqq->ReserveSequence(),pa.filename,pa.timestamp+1);
		DWORD dwSize;
		pa.protocol->QLog("%s",szUrl);
		LPSTR pszFile=webqq->GetHTMLDocument(szUrl,webqq->GetReferer(CLibWebQQ::WEBQQ_REFERER_WEBQQ),&dwSize);
		if (pszFile && dwSize!=0xffffffff && dwSize>0) {
			char szCacheFile[MAX_PATH];
			strcpy(szCacheFile,m_cachepath);
			strcat(szCacheFile,pa.filename);

			if (dwSize!=5991 || memcmp(pszFile+1,"PNG",3) || memcmp(pszFile+57,"OiCCPPhotoshop ICC profile",26)) {
				HANDLE hFile=CreateFileA(szCacheFile,GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,CREATE_ALWAYS,0,NULL);
				if (hFile!=INVALID_HANDLE_VALUE) {
					DWORD dwWritten;
					WriteFile(hFile,pszFile,dwSize,&dwWritten,NULL);
					CloseHandle(hFile);
				}
			} else {
				m_baseProtocol->QLog("[CHttpServer] Broken Image from Server");
			}

			sprintf(szUrl,"HTTP/1.0 200 OK\nContent-Type: %s\nConnection: Close\n\n",stricmp(pa.filename+strlen(pa.filename)-4,".jpg")==0?"image/jpg":stricmp(pa.filename+strlen(pa.filename)-4,".gif")==0?"image/gif":"unknown/unknown");
			Netlib_Send(pa.hConnection,szUrl,(int)strlen(szUrl),MSG_NODUMP);
			Netlib_Send(pa.hConnection,pszFile,dwSize,MSG_NODUMP);
			Netlib_CloseHandle(pa.hConnection);
		} else {
			char* szSend="HTTP/1.0 500 Internal Server Error\nContent-Type: text/plain\nConnection: Close\n\nError relaying p2p image from remote server.";
			Netlib_Send(pa.hConnection,szSend,(int)strlen(szSend),MSG_NODUMP);
			writeTails(pa.hConnection);
		}
		mir_free(pa.uri);
		m_p2psessions.erase((DWORD)pUin);
	}
}

void CHttpServer::CreateThreadObj(ThreadFunc2 func, void* arg) {
	unsigned int threadid;
	mir_forkthreadowner((pThreadFuncOwner) *(void**)&func,this,arg,&threadid);
}

