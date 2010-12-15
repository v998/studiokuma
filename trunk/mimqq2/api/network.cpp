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
m_userhead(NULL), m_qunimage(NULL), m_savedTempSessionMsg(NULL) {
	InitializeCriticalSection(&m_cs);
	m_szModuleName=m_szProtoName=mir_strdup(szModuleName);
	if (szUserName) {
		//m_tszUserName=mir_tstrdup(szUserName);
		m_tszUserName=(LPWSTR)mir_alloc((wcslen(szUserName)+5)*sizeof(wchar_t));
		swprintf(m_tszUserName,L"QQ(%s)",szUserName);
	} else {
		//m_tszUserName=mir_a2u(m_szModuleName);
		m_tszUserName=(LPWSTR)mir_alloc((strlen(m_szModuleName)+5)*sizeof(wchar_t));
		swprintf(m_tszUserName,L"QQ(%S)",m_szModuleName);
	}
	
	m_iStatus=ID_STATUS_OFFLINE;
	m_iXStatus=0;

	SetResident();
	LoadAccount();

	ZeroMemory(&m_currentMedia, sizeof(m_currentMedia));
}

CNetwork::~CNetwork() {
	DeleteCriticalSection(&m_cs);
	if (m_savedTempSessionMsg) delete m_savedTempSessionMsg;

	UnloadAccount();
	mir_free(m_szModuleName);
	mir_free(m_tszUserName);
	util_log(0,"[CNetwork] Instance Destruction Complete");

	// Move out
	Packet::clearAllKeys();
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
}

bool CNetwork::setConnectString(LPCSTR pszConnectString) {
	LPSTR pszBuffer=mir_strdup(pszConnectString);
	LPSTR pszServer=strstr(pszBuffer,"://");
	LPSTR pszPort=strrchr(pszBuffer,':');
	HANDLE hContact=NULL;
	Packet::setUDP(*pszBuffer!='t');
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

	setServer(Packet::isUDP(),pszServer+3,port);
	mir_free(pszBuffer);
	return true;
}

void CNetwork::connectionError() {
	util_log(0,"[%d:CNetwork] Connection Error",m_myqq);
	if (!Packet::isClientKeySet()) {
		ShowNotification(TranslateT("Failed connecting to server"),NIIF_ERROR);
		QQ_SendBroadcast( NULL, ACKTYPE_LOGIN, ACKRESULT_FAILED, NULL, LOGINERR_NOSERVER );
	}

	//QQ_GoOffline();
	//disconnect();
	connectionClosed();
}

void CNetwork::connectionEstablished() {
	CEvaAccountSwitcher::ProcessAs(m_myqq);
	if (Packet::isLoginTokenSet()) {
		m_IsDetecting=false;
		util_log(0,"[%d:CNetwork] Login token already set, ???",m_myqq);
	} else {
		if (!m_IsDetecting) {
			ServerDetectorPacket::setStep(0);
			ServerDetectorPacket::setFromIP(0);
			m_numOfLostKeepAlivePackets=0;
			m_keepAliveTime=0;
			m_loggedIn=false;
			m_currentDefaultServer=NULL;
			m_keepaliveCount=0;
			m_qunList.clearQunList();
			m_qunInitList.clear();
			m_myInfoRetrieved=false;
			m_currentDefaultServer=NULL;
			m_addUID=0;
			m_searchUID=0;
			m_qunMemberCountList.clear();
			m_currentQunMemberCountList.clear();
			m_storedIM.clear();
			if (m_currentMedia.ptszArtist) mir_free(m_currentMedia.ptszArtist);
			if (m_currentMedia.ptszTitle) mir_free(m_currentMedia.ptszTitle);
			ZeroMemory(&m_currentMedia, sizeof(m_currentMedia));
			m_timer=NULL;
			m_userhead=NULL;
			m_qunimage=NULL;
			m_hGroupList.clear();
			m_downloadGroup=false;
			m_addQunNumber=0;
			m_hwndModifySignatureDlg=NULL;
			m_codeVerifyWindow=NULL;
			m_graphicVerifyCode=NULL;
		}
		util_log(0,"[%d:CNetwork] Start detect server",m_myqq);
		append(new ServerDetectorPacket);
	}
	CEvaAccountSwitcher::EndProcess();
}

void CNetwork::connectionClosed() {
	util_log(0,"[%d:CNetwork] Connection Closed",m_myqq);
	if (!Miranda_Terminated()) GoOffline();
}

void CNetwork::waitTimedOut() {
	if (m_checkTime!=0 && time(NULL)-m_checkTime>=2) {
		for (list<OutPacket*>::iterator iter=m_outPool.begin(); iter!=m_outPool.end();iter++) {
			if ((*iter)->needResend()) {
				util_log(0,"[%d:CNetwork] Resend 0x%02x packet, remaining %d times",m_myqq,(*iter)->getCommand(),(*iter)->getResendCount());
				sendOut(*iter);
			} else {
				//util_log(0,"[CNetwork] Remove 0x%02x packet",(*iter)->getCommand());
				short cmd = (*iter)->getCommand();
				if(cmd == QQ_CMD_SEND_IM){
					SendIM *im = dynamic_cast<SendIM *>(*iter);
					if(im)
						emit sendMessage(im->getReceiver(), false);
					else
						emit packetException( cmd);
				} else if( cmd == QQ_CMD_QUN_CMD ){
					QunPacket *qun = dynamic_cast<QunPacket *>(*iter);
					if(qun){
						char qunCmd = qun->getQunCommand();
						if(qunCmd == QQ_QUN_CMD_SEND_IM || qunCmd == QQ_QUN_CMD_SEND_IM_EX)
							emit sendQunMessage(qun->getQunID(), false);
						else
							emit packetException(cmd);
					} else
						emit packetException(cmd);
				} else
					emit packetException(cmd);
				removePacket((*iter)->hashCode());
				//if(!m_outPool.size() && /*!inPool.size() &&*/ m_checkTime)	m_checkTime=0;
				//return;
				break;
			}
		}
		/*
		if(inPool.count()>0){
		emit newPacket();
		}
		*/

		if(!m_outPool.size() && /*!inPool.size() &&*/ m_checkTime)	m_checkTime=0;
	}

	if (m_keepAliveTime!=0 && time(NULL)-m_keepAliveTime>=60) {
		util_log(0,"[CNetwork] Keepalive");
		append(new KeepAlivePacket());
		m_keepAliveTime=time(NULL);
		//emit packetException(QQ_CMD_KEEP_ALIVE);
	}
}

int CNetwork::dataReceived(NETLIBPACKETRECVER* nlpr) {
	CEvaAccountSwitcher::ProcessAs(m_myqq);
	if(Packet::isUDP()) {
		// Direct process
		processPacket(nlpr->buffer,nlpr->bytesAvailable);
		nlpr->bytesUsed+=nlpr->bytesAvailable;
	} else {
		// Segmentation
		//util_log(0,"[CNetwork] TCP data received");
		USHORT packetLen;
		while (true) {
			if (nlpr->bytesUsed>=nlpr->bytesAvailable) break;
			packetLen=ntohs(*(USHORT*)(nlpr->buffer+nlpr->bytesUsed));
			if (nlpr->bytesUsed+packetLen>nlpr->bytesAvailable) break;
			processPacket(nlpr->buffer+nlpr->bytesUsed,packetLen);
			nlpr->bytesUsed+=packetLen;
			//if (nlpr->bytesUsed>=nlpr->bytesAvailable) break;
			//util_log(0,"[CNetwork] Segmentation occured");
		}
	}
	CEvaAccountSwitcher::EndProcess();
	return 0;
}

//DEFAULT_HANDLER(processRequestKeyResponse,EvaRequestKeyReplyPacket)
DEFAULT_HANDLER(processChangeStatusResponse,ChangeStatusReplyPacket)
DEFAULT_HANDLER(processGetUserInfoResponse,GetUserInfoReplyPacket)
DEFAULT_HANDLER(processSignatureOpResponse,SignatureReplyPacket)
DEFAULT_HANDLER(processGetLevelResponse,EvaGetLevelReplyPacket)
DEFAULT_HANDLER(processRecvMsgFriendChangeStatusResponse,FriendChangeStatusPacket)
DEFAULT_HANDLER(processWeatherOpResponse,WeatherOpReplyPacket)
DEFAULT_HANDLER(processSearchUserResponse,SearchUserReplyPacket)
DEFAULT_HANDLER(processAddFriendResponse,EvaAddFriendExReplyPacket)
DEFAULT_HANDLER(processAddFriendAuthResponse,AddFriendAuthReplyPacket)
DEFAULT_HANDLER(processSystemMessageResponse,SystemNotificationPacket)
DEFAULT_HANDLER(processDeleteMeResponse,DeleteMeReplyPacket)
DEFAULT_HANDLER(processGroupNameOpResponse,GroupNameOpReplyPacket)
DEFAULT_HANDLER(processDeleteFriendResponse,DeleteFriendReplyPacket)
DEFAULT_HANDLER(processAddFriendAuthInfoReply,EvaAddFriendGetAuthInfoReplyPacket)
DEFAULT_HANDLER(processRequestExtraInfoResponse,RequestExtraInfoReplyPacket)
DEFAULT_HANDLER(processUploadGroupFriendResponse,UploadGroupFriendReplyPacket)
DEFAULT_HANDLER(processMemoOpResponse,EvaMemoReplyPacket)

void CNetwork::processPacket(LPCBYTE lpData, const USHORT len)
{
	m_curmsg=NULL;

	InPacket* packet=new InPacket((UCHAR*)lpData, len);
	if(!packet->getLength()){
		util_log(0,"[CNetwork] Bad packet (cmd: %d), len=%d, ignore it",packet->getCommand(),len);
		delete packet;
		return;
	}

	dumppacket(true,packet);

	// for the case of keep alive, once we got one, we could ignore all keep alive
	// packets in the outPool
	if(packet->getCommand() == QQ_CMD_KEEP_ALIVE)
		removeOutRequests(QQ_CMD_KEEP_ALIVE);

	// same reason as above
	if(packet->getCommand() == QQ_CMD_GET_FRIEND_ONLINE)
		removeOutRequests(QQ_CMD_GET_FRIEND_ONLINE);

	if (packet->getCommand()!=QQ_CMD_RECV_IM) {
		for (list<int>::iterator iter=receivedPacketList.begin(); iter!=receivedPacketList.end();iter++)
			if (*iter==packet->hashCode()) {
				delete packet;
				return;
			}
		receivedPacketList.push_front(packet->hashCode());
		if (receivedPacketList.size()>50) receivedPacketList.pop_back();
	}

	m_curmsg=packet;

	switch (packet->getCommand()) {
		HANDLE(QQ_CMD_SERVER_DETECT, processServerDetectorResponse);
		HANDLE(QQ_CMD_REQUEST_LOGIN_TOKEN_EX, processRequestLoginTokenExResponse);
		HANDLE(QQ_CMD_LOGIN, processLoginResponse);
		HANDLE(QQ_CMD_KEEP_ALIVE, processKeepAliveResponse);
		HANDLE(QQ_CMD_CHANGE_STATUS, processChangeStatusResponse);
		HANDLE(QQ_CMD_REQUEST_KEY, processRequestKeyResponse);
		HANDLE(QQ_CMD_GET_USER_INFO, processGetUserInfoResponse);
		HANDLE(QQ_CMD_GET_FRIEND_LIST, processGetFriendListResponse);
		HANDLE(QQ_CMD_GET_FRIEND_ONLINE, processGetFriendOnlineResponse);
		HANDLE(QQ_CMD_DOWNLOAD_GROUP_FRIEND, processDownloadGroupFriendResponse);
		HANDLE(QQ_CMD_REQUEST_EXTRA_INFORMATION, processRequestExtraInfoResponse);
		HANDLE(QQ_CMD_SIGNATURE_OP, processSignatureOpResponse);
		HANDLE(QQ_CMD_RECV_IM, processIMResponse);
		HANDLE(QQ_CMD_QUN_CMD, processQunResponse);
		HANDLE(QQ_CMD_RECV_MSG_FRIEND_CHANGE_STATUS, processRecvMsgFriendChangeStatusResponse);
		HANDLE(QQ_CMD_SEARCH_USER, processSearchUserResponse);
		HANDLE(QQ_CMD_GET_LEVEL, processGetLevelResponse);
		HANDLE(QQ_CMD_RECV_MSG_SYS, processSystemMessageResponse);
		HANDLE(QQ_CMD_ADD_FRIEND_AUTH, processAddFriendAuthResponse);
		HANDLE(QQ_CMD_ADD_FRIEND_EX, processAddFriendResponse);
		HANDLE(QQ_CMD_DELETE_FRIEND, processDeleteFriendResponse);
		HANDLE(QQ_CMD_DELETE_ME, processDeleteMeResponse);
		HANDLE(QQ_CMD_GROUP_NAME_OP, processGroupNameOpResponse);
		HANDLE(QQ_CMD_SEND_IM, processSendImResponse);
		HANDLE(QQ_CMD_TEMP_SESSION_OP, processTempSessionOpResponse);
		HANDLE(QQ_CMD_WEATHER, processWeatherOpResponse);
		HANDLE(QQ_CMD_ADD_FRIEND_AUTH_INFO, processAddFriendAuthInfoReply);
		HANDLE(QQ_CMD_UPLOAD_GROUP_FRIEND, processUploadGroupFriendResponse);
		HANDLE(QQ_CMD_MEMO_OP, processMemoOpResponse);
		default: util_log(0,"Received unknown packet %d",packet->getCommand()); break;
	}
	
	m_curmsg=NULL;

	removePacket(packet->hashCode());
	delete packet;
}

void CNetwork::removePacket(const int hashCode)
{
	EnterCriticalSection(&m_cs);
	for (list<OutPacket*>::iterator iter=m_outPool.begin(); iter!=m_outPool.end();) {
		if (hashCode==0 || (*iter)->hashCode()==hashCode) {
			delete (*iter);
			iter=m_outPool.erase(iter);
		} else
			iter++;
	}
	LeaveCriticalSection(&m_cs);
}

void CNetwork::removeOutRequests(const short cmd)
{
	for (list<OutPacket*>::iterator iter=m_outPool.begin(); iter!=m_outPool.end();) {
		if ((*iter)->getCommand()==cmd) {
			delete (*iter);
			iter=m_outPool.erase(iter);
		} else
			iter++;
	}
}

void CNetwork::processServerDetectorResponse(InPacket* in) {
	PARSE2(ServerDetectorReplyPacket);

	if(packet.isServerReady()){
		if (!m_graphicVerifyCode) {
			util_log(0,"[%d:CNetwork] Server Ready, request login token (no verification code)",m_myqq);
			append(new RequestLoginTokenExPacket());
		}else {
			RequestLoginTokenExPacket *packet = new RequestLoginTokenExPacket(QQ_LOGIN_TOKEN_VERIFY);
			packet->setToken(m_graphicVerifyCode->m_SessionToken,m_graphicVerifyCode->m_SessionTokenLen);
			packet->setCode(m_graphicVerifyCode->m_code);
			append(packet);
		}
		
	}else if(packet.needRedirect()){
		ServerDetectorPacket::nextStep();
		setServer(isUDP(),packet.getRedirectIP(),getPort());
		util_log(0,"[%d:CNetwork] Server Detector redirect to %s:%d",m_myqq,getHost().c_str(),getPort());
		clearOutPool();
		//m_inPool.clear();
		//_disconnect(this);
		disconnect();
	}else{
		util_log(0, "[%d:CNetwork] unknown server detect reply ( reply code: %d)",m_myqq,packet.getReplyCode());
	}
}

void CNetwork::processRequestLoginTokenExResponse(InPacket* in) {
	if (Packet::isLoginTokenSet()) {
		util_log(0,"[%d:CNetwork] Login Token already set! Ignore response",m_myqq);
	} else {
		PARSE2(RequestLoginTokenExReplyPacket,append(new RequestLoginTokenExPacket()));

		if(packet.getReplyCode() == QQ_LOGIN_TOKEN_NEED_VERI){
			util_log(0,"[%d:CNetwork] LoginToken response, graphical confirmation required.",m_myqq);
			DEFCB();
		} else {
			util_log(0,"[%d:CNetwork] LoginToken set, Start to login",m_myqq);
			LoginPacket *packet = new LoginPacket(m_iDesiredStatus==ID_STATUS_INVISIBLE?QQ_LOGIN_MODE_INVISIBLE:QQ_LOGIN_MODE_NORMAL);
			DBVARIANT dbv;
			READ_S2(NULL,QQ_PASSWORD,&dbv);
			CallService(MS_DB_CRYPT_DECODESTRING, lstrlenA(dbv.pszVal) + 1, (LPARAM)dbv.pszVal);

			Packet::setPasswordKey((const unsigned char*)EvaUtil::doMd5Md5(dbv.pszVal,strlen(dbv.pszVal)));
			DBFreeVariant(&dbv);

			packet->setNumProcess(1); // we simple set it to 1 for now
			packet->setComputerUUID(Computer_UUID); // use hard-coded UUID, might change it to follow QQ's logic
			append(packet);

			if (READ_B2(NULL,"RemovePassword")==1) {
				DBDeleteContactSetting(NULL,m_szModuleName,"RemovePassword");
				DBDeleteContactSetting(NULL,m_szModuleName,QQ_PASSWORD);
			}
		}
	}
}

void CNetwork::processLoginResponse(InPacket* in) {
	// if client key is set, the user has logged in.

	if(!Packet::isClientKeySet()) {
		LoginReplyPacket packet;
		clearOutPool();
		packet.setInPacket(in);
		if (!packet.parse()) {
			append(new LoginPacket(m_iDesiredStatus==ID_STATUS_INVISIBLE?QQ_LOGIN_MODE_INVISIBLE:QQ_LOGIN_MODE_NORMAL));
			return;
		}

		switch (packet.getReplyCode()) {
			case QQ_LOGIN_REPLY_OK: // Login successful
				{
					EvaIPAddress ip(packet.getMyIP());
					time_t tm=(time_t)packet.getLastLoginTime();
					WRITE_D(NULL,"LoginTS",(DWORD)time(NULL));
					WRITE_D(NULL,"LastLoginTS",(DWORD)tm);
					m_keepaliveCount=0;
					m_keepAliveTime=time(NULL);
					WRITE_D(NULL,"IP",packet.getMyIP());
					WRITE_D(NULL,"LastLoginIP",packet.getLastLoginIP());
					m_clockSkew=(INT)(time(NULL)-packet.getLoginTime());
					util_log(0, "[%d:CNetwork] Login successful, My IP=%s:%d, Clock skew=%d s",m_myqq,ip.toString().c_str(),packet.getMyPort(),m_clockSkew);

					/*
					if(fileManager) delete fileManager;
					fileManager = new EvaFileManager(Packet::getQQ(),this);
					fileManager->customEventRedirector=ftEventRedirector;
					*/
				}
			case QQ_LOGIN_REPLY_PWD_ERROR: // Password error
			case QQ_LOGIN_REPLY_MISC_ERROR: // Unknown, possibly bug of libeva
			case QQ_LOGIN_REPLY_NEED_REACTIVATE:
				callbackHub(in->getCommand(),0,(WPARAM)&packet,(LPARAM)0);
				break;
			case QQ_LOGIN_REPLY_REDIRECT: // Redirection to other address
			case QQ_LOGIN_REPLY_REDIRECT_EX:
				{
					EvaIPAddress ip(packet.getRedirectedIP());
					string ipstr=ip.toString();

					if (getHost().compare(ipstr)) {
						setServer(isUDP(),packet.getRedirectedIP()==0?getHost().c_str():ipstr.c_str(),getPort()==443?443:packet.getRedirectedPort());
						util_log(0,"[CNetwork] Redirect to %s:%d",getHost().c_str(),getPort());
						clearOutPool();
						//_disconnect(this);
						disconnect();
					} else {
						util_log(0,"[CNetwork] Redirect with same server, login again");
						append(new LoginPacket(m_iDesiredStatus==ID_STATUS_INVISIBLE?QQ_LOGIN_MODE_INVISIBLE:QQ_LOGIN_MODE_NORMAL));
					}
					break;
				}
			default:
				util_log(98,"[CNetwork] Unknown login reply: %d",(int)packet.getReplyCode());
		}
	}
}

void CNetwork::processDownloadGroupFriendResponse(InPacket* in) {
	PARSE2(DownloadGroupFriendReplyPacket);
	DEFCB();

	int nextID = packet.getNextStartID();
	if(nextID != 0x0000){
		append(new DownloadGroupFriendPacket(nextID));
	}
}

void CNetwork::processGetFriendListResponse(InPacket* in) {
	PARSE2(GetFriendListReplyPacket);

	DEFCB();

	if(packet.getPosition()!=QQ_FRIEND_LIST_POSITION_END){
		// Not end of list, query next contact
		append(new GetFriendListPacket(packet.getPosition()));
	}else{
		// End of list, query for online cottacts
		append(new GetOnlineFriendsPacket());
		append(new DownloadGroupFriendPacket());
	}
}

void CNetwork::processGetFriendOnlineResponse(InPacket* in) {
	PARSE2(GetOnlineFriendReplyPacket);
	onlineList l=packet.getOnlineFriendList();

	for (onlineList::iterator iter=l.begin(); iter!=l.end(); iter++)
		m_tempOnlineList.push_back(*iter);

	util_log(0,"ASSERT: packet.getPosition()=%d",packet.getPosition());
	/*if (packet.getPosition()==0) {
		util_log(0,"ASSERT: online friend response packet.getPosition()==0!");
	} else */if(packet.getPosition() != QQ_FRIEND_ONLINE_LIST_POSITION_END){
		util_log(0,"ASSERT: continue send");
		append(new GetOnlineFriendsPacket(packet.getPosition()+packet.getOnlineFriendList().size()));
	} else {
		DEFCB();
	}
}

void CNetwork::processRequestKeyResponse(InPacket* in) {
	PARSE2(EvaRequestKeyReplyPacket);

	if (packet.getKeyType()==QQ_REQUEST_FILE_AGENT_KEY) {
		//fileManager->setMyBasicInfo(Packet::getFileAgentKey(), Packet::getFileAgentToken(), Packet::getFileAgentTokenLength());
		append(new EvaRequestKeyPacket(QQ_REQUEST_UNKNOWN_KEY));
	}
}

void CNetwork::processKeepAliveResponse(InPacket* in) {
	PARSE2(KeepAliveReplyPacket);
	m_clockSkew=(INT)(time(NULL)-packet.getTime());
	m_numOfLostKeepAlivePackets=0;
	util_log(0,"[CNetwork] Keep Alive completed, online users=%d, clock skew=%d s",packet.numOnlineUsers(),m_clockSkew);

	removeOutRequests(QQ_CMD_KEEP_ALIVE);
	DEFCB();
}

void CNetwork::processSendImResponse(InPacket* in) {
	PARSE2(SendIMReplyPacket);

	for (list<OutPacket*>::iterator iter=m_outPool.begin(); iter!=m_outPool.end(); iter++)
		if ((*iter)->getSequence()==packet.getSequence()) {
			CB(0,&packet,*iter);
			break;
		}
}

void CNetwork::processIMResponse(InPacket* in) {
	PARSE2(ReceiveIMPacket);

	util_log(0,"[CNetwork] IM: seq: %d ----- msg seq: %d, from: %d, type: %02x", packet.getIntSequence(), packet.getSequence(), packet.getSender(), packet.getIMType());

	// This packet requires acknowledge to server, so send it now
	ReceiveIMReplyPacket packetReply(packet.getReplyKey());
	packetReply.setSequence(packet.getSequence());
	sendOut(&packetReply);

	for (list<int>::iterator iter=receivedCacheList.begin(); iter!=receivedCacheList.end(); iter++)
		if ((*iter)==packet.hashCode()) {
			util_log(0,"[CNetwork] Duplicated message, ignore");
			return;
		}

	receivedCacheList.push_front(packet.hashCode());
	if (receivedCacheList.size()>50) receivedCacheList.pop_back();

	switch (packet.getIMType()) {
		case 0x1f:
			{
				ReceivedTempSessionTextIMPacket qtsp(packet.getBodyData(),packet.getBodyLength());
				CB(0,&packet,&qtsp);
				return;
			}
			/*
			case 0x4a: 
			{
			// Update notification
			break;
			}
			*/
		case QQ_RECV_IM_TO_BUDDY:
		case QQ_RECV_IM_FROM_BUDDY_2006:
		case QQ_RECV_IM_TO_UNKNOWN:
			{
				// Normal IM
				NormalIMBase base(packet.getBodyData(), packet.getBodyLength());		
				base.parseData();

				switch(base.getNormalIMType())
				{
				case QQ_IM_NORMAL_TEXT:
					{
						// Text IM  //¤å¥» IM
						ReceivedNormalIM *received = new ReceivedNormalIM();
						received->setNormalIMBase(&base);
						received->parseData();

#if 0
						char fn[MAX_PATH];
						sprintf(fn,"f:\\imdebug-%d.txt",received->getSequence());
						FILE* fp=fopen(fn,"wb");
						fwrite(received->getBodyData(),1,received->getBodyLength(),fp);
						fclose(fp);
#endif

						//check fragments
						if (received->getNumFragments()>1)
						{
							unsigned short ropID = received->getMessageID();

							//push fragment inormation into map
							unsigned short total = received->getNumFragments();
							pcMsgCache[ropID].total = total;
							pcMsgCache[ropID].content[received->getSeqOfFragments()] = received->getMessage().c_str();

							if (total == pcMsgCache[ropID].content.size())
							{
								//all fragments received
								string allText = "";
								for (int idx=0; idx<total; idx++) 
									allText += pcMsgCache[ropID].content[idx];
								pcMsgCache.erase(ropID);
								received->setMessage(allText);
							} else
							{
								delete received;
								return;
							}

						}

						util_log(0,"%s(): IP of contact %d is %d:%d",__FUNCTION__,packet.getSender(),(unsigned int)packet.getSenderIP(),packet.getSenderPort());
						CB(0,&packet,received);

						delete received;
					}
					break;
				case QQ_IM_UDP_REQUEST:
					util_log(0,"EvaPacketManager::processReceiveIM -- QQ_IM_UDP_REQUEST <- old version used");
					break;
				case QQ_IM_TCP_REQUEST:
				case QQ_IM_ACCEPT_UDP_REQUEST:
				case QQ_IM_NOTIFY_IP:
				case QQ_IM_REQUEST_CANCELED:
					util_log(0, "Received file command from \"%d\"(v:%d). Command not supported, Buddy might use a client earlier than QQ2005 beta1!",
						packet.getSender(), 0xffff&packet.getVersion());
					break;
				case QQ_IM_NOTIFY_FILE_AGENT_INFO:
				case QQ_IM_EX_UDP_REQUEST:
				case QQ_IM_EX_REQUEST_CANCELLED:
				case QQ_IM_EX_REQUEST_ACCEPTED:
					{
						ReceivedFileIM received;
						util_log(0, "QQNetwork: Received FT IM Request");

						received.setNormalIMBase(&base);
						received.parseData();
						unsigned int ip=ntohl(received.getWanIp());
						unsigned char* cip=(unsigned char*)&ip;

						if(received.getTransferType() != QQ_TRANSFER_FILE && received.getTransferType() != QQ_TRANSFER_IMAGE )
						{
							util_log(0, "Unknown transfer type(0x%02x) from %d, breaking out", 0xff & received.getTransferType(),received.getSender());
							CB(-1,received.getSender(),0);
							break;
						}
						util_log(0, "TransferType=0x%02x, ConnectMode=0x%02x", received.getTransferType(), received.getConnectMode());
						//util_log(0, "WanIP=0x%8x:%d", received.getWanIp(), received.getWanPort());
						util_log(0, "WanIP=%d.%d.%d.%d:%d", cip[0],cip[1],cip[2],cip[3], htons(received.getWanPort()));
						util_log(0, "FileName=%s(%d bytes)", received.getFileName().c_str(), received.getFileSize());
						/*
						util_log(0, "Sequence=%d, MajorPort=%d", 0xffff & received.getSequence(), received.getMajorPort());
						util_log(0, "LanIP=0x%8x:%d", received.getLanIp(), received.getLanPort());
						*/
						CB(base.getNormalIMType(),&packet,&received);
					}
					break;
				case QQ_IM_EX_NOTIFY_IP:
					{
						ReceivedFileExIpIM received;
						unsigned int ip;
						unsigned char* cip=(unsigned char*)&ip;

						util_log(0,"QQNetwork: Received FT IM: QQ_IM_EX_NOTIFY_IP");
						received.setNormalIMBase(&base);
						received.parseData();
						util_log(0, "TransferType=0x%02x, ConnectMode=0x%02x", received.getTransferType(), received.getConnectMode());
						ip=htonl(received.getWanIp1());
						util_log(0, "WanIP1=%d.%d.%d.%d:%d",cip[0],cip[1],cip[2],cip[3], htons(received.getWanPort1()));
						ip=htonl(received.getSyncIp());
						util_log(0, "SynIP=%d.%d.%d.%d:%d",cip[0],cip[1],cip[2],cip[3], htons(received.getSyncPort()));
						util_log(0, "SyncSession=0x%08x", received.getSyncSession());
						if(received.isSender())
							util_log(0,"isSender -- true");
						else 
							util_log(0,"isSender -- false");

						CB(base.getNormalIMType(),&packet,&received);
					}
					break;
				default:
					util_log(0,"Got a non-text msg, can't process it, ignore it");
				}
			}
			break;
		case QQ_RECV_IM_QUN_IM:
		case QQ_RECV_IM_TEMP_QUN_IM:
		case QQ_RECV_IM_UNKNOWN_QUN_IM:
			{
				ReceivedQunIM *received = new ReceivedQunIM(packet.getIMType(),packet.getBodyData(),packet.getBodyLength());
#if 0
				char fn[MAX_PATH];
				sprintf(fn,"f:\\qimdebug-%d.txt",received->getSequence());
				FILE* fp=fopen(fn,"wb");
				fwrite(packet.getBodyData(),1,packet.getBodyLength(),fp);
				fclose(fp);
#endif
				//check fragments
				if (received->getNumFragments()>1)
				{
					unsigned short ropID = received->getMessageID();

					//push fragment inormation into map
					unsigned short total = received->getNumFragments();
					pcMsgCache[ropID].total = total;
					pcMsgCache[ropID].content[received->getSeqOfFragments()] = received->getMessage().c_str();

					if (total == pcMsgCache[ropID].content.size())
					{
						//all fragments received
						string allText = "";
						for (int idx=0; idx<total; idx++) 
							allText += pcMsgCache[ropID].content[idx];
						pcMsgCache.erase(ropID);
						received->setMessage(allText);
					} else
					{
						delete received;
						return;
					}

				}

				EvaHtmlParser parser;
				std::list<CustomizedPic> picList = parser.parseCustomizedPics((char*)received->getMessage().c_str(),true);

				if (received->getSenderQQ()==m_myqq) {
					if (strstr(parser.getConvertedMessage().c_str(),"[img]")) {
						delayReport_t* dr=(delayReport_t*)mir_alloc(sizeof(delayReport_t));
						dr->hContact=FindContact(packet.getSender());
						dr->ackType=ACKTYPE_MESSAGE;
						dr->ackResult=ACKRESULT_SUCCESS;
						dr->aux=1;
						dr->aux2=NULL;
						ForkThread((ThreadFunc)&CNetwork::delayReport,dr);
					}

					delete received;
					return;
				}

				if(picList.size()){
#if 0
					{
						char szFileName[MAX_PATH]="f:\\qqimmsg-";
						itoa(GetTickCount(),szFileName+strlen(szFileName),16);
						strcat(szFileName,".txt");

						FILE* fp=fopen(szFileName,"wb");
						fputs(received->getMessage().c_str(),fp);
						fclose(fp);
						util_log(0,"[CNetwork] pic message saved.");
					}
#endif
					received->setMessage(parser.getConvertedMessage());

					for (list<CustomizedPic>::iterator iter=picList.begin(); iter!=picList.end();) {
						if (GetFileAttributesA(iter->tmpFileName.c_str())!=INVALID_FILE_ATTRIBUTES)
							// File exists
							iter=picList.erase(iter);
						else if (iter->sessionID==0 || iter->ip==0 || iter->port==0)
							// Error on sending side (Continue may crash MIMQQ2 QunImage thread)
							iter=picList.erase(iter);
						else
							iter++;
					}
#if 1
					if (picList.size()) {
						// Still items in picList
						EvaAskForCustomizedPicEvent *event = new EvaAskForCustomizedPicEvent();
						event->setPicList(picList);
						event->setQunID(packet.getSender());
						//CQunImage::PostEvent(event);
						if (!m_qunimage) m_qunimage=new CQunImage(this);
						m_qunimage->customEvent(event);
					}
#else
					if (picList.size()) {
						// Still items in picList
						EvaAskForCustomizedPicEvent *event = new EvaAskForCustomizedPicEvent();
						CustomizedPic pic=picList.front();
						event->setPicList(picList);
						event->setQunID(packet.getSender());
						event->setQunIM(received);

						util_log(0,"BugCheck: qqQunPicThread=0x%p",qqQunPicThread);

						if (!qqQunPicThread) {
							ThreadData* newThread = new ThreadData();
							qqQunPicThread=newThread;

							/*
							in_addr addr;
							addr.S_un.S_addr=htonl(pic.ip);
							*/
							unsigned int ip=htonl(pic.ip);

							newThread->mType = SERVER_QUNIMAGE;
							strcpy(newThread->mServer,inet_ntoa(/*addr*/*(in_addr*)&ip));
							newThread->mPort = pic.port;
							newThread->mTCP=true;
							//newThread->mIsMainThread = false;

							newThread->addQunPicEvent(event);
							newThread->hEvent=CreateEvent(NULL,TRUE,FALSE,NULL);
							util_log(0,"BugCheck: startThread",qqQunPicThread);
							newThread->startThread((pThreadFunc)QQServerThread);
							if (WaitForSingleObject(newThread->hEvent,5000)==WAIT_TIMEOUT) {
								util_log(0,"Thread wait timed out. why?");
							}
							CloseHandle(newThread->hEvent);
							util_log(0,"BugCheck: continue",qqQunPicThread);
						} else {
							qqQunPicThread->addQunPicEvent(event);
							delete event;
						}
						return;
					}
#endif
				}

				CB(0,&packet,received);

				delete received;
				return;
			}
			break;
		default:
			/*
			if (packet.getSender()==10000) {
				FILE* fp=fopen("f:\\mimqq-10000.txt","wb");
				fwrite(packet.getBodyData(),packet.getBodyLength(),1,fp);
				fclose(fp);
			}
			*/
			DEFCB();
			break;
	}
}

void CNetwork::processQunResponse(InPacket* in) {
	PARSE2(QunReplyPacket);

	switch (packet.getQunCommand()) {
		case QQ_QUN_CMD_MODIFY_CARD:
			append(new QunGetInfoPacket(packet.getQunID()));
			//append(new QunRequestAllRealNames(packet.getQunID()));
			break;
		case QQ_QUN_CMD_JOIN_QUN_AUTH: // Requested to join a Qun with authorization message
			if (packet.isReplyOK())
				append(new QunGetInfoPacket(packet.getQunID()));
			break;
#if 0
		case QQ_QUN_CMD_CREATE_QUN:
			if(packet->isReplyOK()){
				Qun q(packet->getQunID());
				user->getQunList()->add(q);
				doRequestQunInfo(packet->getQunID());
				emit qunCreateDone(packet->getQunID());
				connecter->append(new QunActivePacket(packet->getQunID()));
				emit qunRequestUpdateDisplay();
			}else
				emit qunCreateFailed(codec->toUnicode(packet->getErrorMessage().c_str()));
			break;
		case QQ_QUN_CMD_CREATE_TEMP_QUN:
			break;
		case QQ_QUN_CMD_ACTIVATE_QUN:
			if(!packet->isReplyOK())
				emit qunActiveQunFailed(codec->toUnicode(packet->getErrorMessage().c_str()));
			break;
		case QQ_QUN_CMD_MODIFY_QUN_INFO:
			if(packet->isReplyOK())
				doRequestQunInfo(packet->getQunID());
			emit qunModifyInfoReply(packet->getQunID(), packet->isReplyOK(), codec->toUnicode(packet->getErrorMessage().c_str()));
			break;
		case QQ_QUN_CMD_GET_TEMP_QUN_INFO:
			break;
		case QQ_QUN_CMD_EXIT_TEMP_QUN:
			break;
		case QQ_QUN_CMD_GET_TEMP_QUN_MEMBERS:
			break;
		case QQ_QUN_CMD_MODIFY_CARD:
			emit qunModifyQunCardReply(packet->getQunID(), packet->isReplyOK(), packet->getTargetQQ(),
				codec->toUnicode(packet->getErrorMessage().c_str()));
			break;
#endif
	}
	DEFCB();
}

void CNetwork::processTempSessionOpResponse(InPacket* in) {
	//PARSE2(TempSessionOpReplyPacket);
	TempSessionOpReplyPacket packet(in->getBody(),in->getLength());
	DEFCB();
}

#define SETCMD(x) {x,#x}
void CNetwork::dumppacket(bool in, Packet* packet) {
	typedef struct {
		short cmd;
		LPCSTR name;
	} cmd_t;

	static cmd_t cmds[]={
		SETCMD(QQ_CMD_LOGOUT),
		SETCMD(QQ_CMD_KEEP_ALIVE),
		SETCMD(QQ_CMD_MODIFY_INFO),
		SETCMD(QQ_CMD_SEARCH_USER),
		SETCMD(QQ_CMD_GET_USER_INFO),
		SETCMD(QQ_CMD_ADD_FRIEND),
		SETCMD(QQ_CMD_DELETE_FRIEND),
		SETCMD(QQ_CMD_ADD_FRIEND_AUTH),
		SETCMD(QQ_CMD_CHANGE_STATUS),
		SETCMD(QQ_CMD_ACK_SYS_MSG),
		SETCMD(QQ_CMD_SEND_IM),
		SETCMD(QQ_CMD_RECV_IM),
		SETCMD(QQ_CMD_DELETE_ME),
		SETCMD(QQ_CMD_REQUEST_KEY),
		SETCMD(QQ_CMD_CELL_PHONE_1),
		SETCMD(QQ_CMD_LOGIN),
		SETCMD(QQ_CMD_GET_FRIEND_LIST),
		SETCMD(QQ_CMD_GET_FRIEND_ONLINE),
		SETCMD(QQ_CMD_CELL_PHONE_2),
		SETCMD(QQ_CMD_QUN_CMD),
		SETCMD(QQ_CMD_GROUP_NAME_OP),
		SETCMD(QQ_CMD_UPLOAD_GROUP_FRIEND),
		SETCMD(QQ_CMD_MEMO_OP),
		SETCMD(QQ_CMD_DOWNLOAD_GROUP_FRIEND),
		SETCMD(QQ_CMD_GET_LEVEL),
		SETCMD(QQ_CMD_ADVANCED_SEARCH),
		SETCMD(QQ_CMD_REQUEST_LOGIN_TOKEN),
		SETCMD(QQ_CMD_REQUEST_EXTRA_INFORMATION),
		SETCMD(QQ_CMD_TEMP_SESSION_OP),
		SETCMD(QQ_CMD_SIGNATURE_OP),
		SETCMD(QQ_CMD_RECV_MSG_SYS),
		SETCMD(QQ_CMD_RECV_MSG_FRIEND_CHANGE_STATUS),
		SETCMD(QQ_CMD_SERVER_DETECT),
		SETCMD(QQ_CMD_WEATHER),
		SETCMD(QQ_CMD_ADD_FRIEND_EX),
		SETCMD(QQ_CMD_ADD_FRIEND_AUTH_EX),
		SETCMD(QQ_CMD_ADD_FRIEND_AUTH_INFO),
		SETCMD(QQ_CMD_VERIFY_ADDING_MSG),
		SETCMD(QQ_CMD_ADD_FRIEND_AUTH_QUESTION),
		SETCMD(QQ_CMD_REQUEST_LOGIN_TOKEN_EX),
		SETCMD(QQ_CMD_LOGIN_LOCATION_CHECK),
		NULL
	};

	static char msg[MAX_PATH];
	short cmdnum=packet->getCommand();
	strcpy(msg,in?"<<":">>");
	for (cmd_t* cmd=cmds; cmd->cmd; cmd++) {
		if (cmd->cmd==cmdnum) {
			strcat(msg,cmd->name);
		}
	}
	util_log(0,msg);
}

void CNetwork::sendOut(OutPacket* out) {
	m_checkTime=0;
	UCHAR* buf=(UCHAR*)mir_alloc(MAX_PACKET_SIZE * sizeof(UCHAR));
	int len;

	CEvaAccountSwitcher::ProcessAs(m_myqq);
	dumppacket(false,out);
	out->fill(buf, &len);
	CEvaAccountSwitcher::EndProcess();
	send((LPSTR)buf,len);
	mir_free(buf);
	m_checkTime=time(NULL);
}

void CNetwork::append(OutPacket* out) {
	sendOut(out);   // force to send
	if(out->needAck()){
		m_outPool.push_back(out);
		m_checkTime=time(NULL);
	}else
		delete out;
}

void CNetwork::packetException(const short cmd) {
	util_log(0,"[%d:CNetwork] packet exception: (cmd)0x%4x",m_myqq,cmd);

	if(cmd == QQ_CMD_SERVER_DETECT){
		connectionError();
		return;
	}

	if(cmd == QQ_CMD_KEEP_ALIVE){
		if(m_numOfLostKeepAlivePackets < 2) {
			m_numOfLostKeepAlivePackets++; 
			util_log(0,"[%d:CNetwork] KeepAlive timeout, retrying",m_myqq);
			append(new KeepAlivePacket());
		}
	}
	if(!m_loggedIn && (cmd == QQ_CMD_LOGIN || cmd == QQ_CMD_REQUEST_LOGIN_TOKEN)){
		util_log(0,"[%d:CNetwork] Login/LoginToken timeout",m_myqq);
		//connectionError();
		disconnect();
	}

	if(m_numOfLostKeepAlivePackets>=2){
		util_log(0,"[%d:CNetwork] KeepAlive timeout, connection lost",m_myqq);
		ShowNotification(TranslateT("Connection to QQ server lost"),NIIF_ERROR);
		disconnect();
		//connectionError();
	}
}

void CNetwork::sendMessage(const int receiver, bool result) {

}

void CNetwork::sendQunMessage(const int qunid, bool result) {

}

void CNetwork::clearOutPool() {
	while (m_outPool.size()) {
		delete m_outPool.front();
		m_outPool.pop_front();
	}
}

bool CNetwork::crashRecovery() {
	util_log(0,"CNetwork Crash Recovery");
	if (m_curmsg) {
		FILE* fp=fopen("c:\\mirandaqq.log","wb");
		fwrite(m_curmsg->getBody(),m_curmsg->getLength(),1,fp);
		fclose(fp);
		ShowNotification(L"MirandaQQ error packet dump has been written to c:\\mimqq.log. Please sumbit for support.",NIIF_ERROR);
		delete m_curmsg;
		m_curmsg=NULL;
	}
	return false;
}
