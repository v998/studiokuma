#include "StdAfx.h"

#define QUEUE_INTERVAL 10000

CQunImage* CQunImage::m_inst=NULL;
CRITICAL_SECTION CQunImage::m_cs={0};
CQunImageServer* CQunImage::m_imageServer=NULL;

//extern void qunPicCallbackHub(int msg, int qunid, void* aux);
/*
void CQunImage::PostEvent(QCustomEvent* e) {
	EnterCriticalSection(&m_cs);
	if (m_inst==NULL) m_inst=new CQunImage();
	m_inst->customEvent(e);
	LeaveCriticalSection(&m_cs);
}
*/
void CQunImage::customEvent(QCustomEvent *e) {
	if (!DBGetContactSettingByte(NULL,g_dllname,QQ_DISABLEHTTPD,0)) {
		if(e->type() == EvaRequestCustomizedPicEvent){
			EvaAskForCustomizedPicEvent *se = (EvaAskForCustomizedPicEvent *)e;
			Session session;
			session.qunID = se->getQunID();
			session.list = se->getPicList();
			downloadList.push_back(session);
			//if(isBusy) printf("EvaPicManager::customEvent -- isBusy \n");
			list<CustomizedPic> picList=session.list;

			for (list<CustomizedPic>::iterator iter=picList.begin(); iter!=picList.end(); iter++)
				m_imageServer->addFile(iter->fileName.c_str());

			if(!isBusy) doProcessEvent();

		}else if(e->type() == EvaSendPictureReadyEvent){
			EvaSendCustomizedPicEvent *se = (EvaSendCustomizedPicEvent *) e;
			OutSession session;
			session.qunID = se->getQunID();
			session.list = se->getPicList();
			session.msg=se->getMessage();
			sendList.push_back(session);
			if(!isBusy) doProcessOutEvent();
		}
	}
	delete e;
}

CQunImage::CQunImage(CNetwork* network): CClientConnection("CQUNIMAGE",QUEUE_INTERVAL),
isBusy(false), currentIndex(0), /*bufLength(0),*/ expectedSequence(0), isSend(false), isAppending(false), isRemoving(false), m_network(network), m_hWndPopup(NULL), m_timeoutCount(0) {
	//registerConnection(CONN_TYPE_QUNIMAGE,this);
	currentFile.buf=NULL;
	EvaPicPacket::setQQ(network->GetMyQQ());
	if (!m_imageServer) m_imageServer=new CQunImageServer();

	if (ServiceExists(MS_POPUP_ADDPOPUPW) && !DBGetContactSettingByte(NULL,network->m_szModuleName,QQ_NOPROGRESSPOPUPS,0)) {
		POPUPDATAW ppd={0};
		swprintf(ppd.lpwzContactName,TranslateT("QQ Qun Image Operation (%s)"),network->m_tszUserName);
		wcscpy(ppd.lpwzText,L"...");
		ppd.lchIcon=(HICON)LoadImage(hinstance, MAKEINTRESOURCE(IDI_TM), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0);
		ppd.iSeconds=-1;
		ppd.PluginWindowProc=PopupWndProc;
		ppd.PluginData=&m_hWndPopup;
		m_popupTextP=m_popupText;
		if (PUAddPopUpW(&ppd)!=-1) {
			time_t curtime=time(NULL);
			while (time(NULL)-curtime<2 && !m_hWndPopup) Sleep(100);
		}
		util_log(0,"Returned from MS_POPUP_ADDPOPUPW: hWndPopup=0x%08p",m_hWndPopup);
	}

}

CQunImage::~CQunImage() {
	EnterCriticalSection(&m_cs);
	m_imageServer->signalFile(NULL);
	clearManager();
	m_inst=NULL;
	if (m_hWndPopup) PUDeletePopUp(m_hWndPopup);
	LeaveCriticalSection(&m_cs);
}

void CQunImage::clearManager()
{
	while (outPool.size()) {
		delete outPool.front();
		outPool.pop_front();
	}
	outList.clear();
	currentIndex = -1;
	//bufLength = 0;
	isSend = false;
	isBusy = false;
	disconnect();
}

void CQunImage::doProcessEvent() {
	if(!downloadList.size()) {
		isBusy = false;
		if(sendList.size()) 
			doProcessOutEvent();
		else
			clearManager();
		util_log(0,"[CQunImage] doProcessEvent -- downloadList size is ZERO, return");
		return;
	}
	isBusy = true;
	isSend = false;
	Session session = downloadList.front();
	downloadList.pop_front();
	qunID = session.qunID;
	picList = session.list;
	util_log(0,"EvaPicManager::doProcessEvent");
	if(!picList.size()){
		isBusy = false;
		doProcessEvent();
		util_log(0,"[CQunImage] doProcessEvent -- picList size is ZERO, return");
		return;
	}
	currentIndex = -1;
	if(!Packet::getFileAgentKey()) {
		clearManager();
		return;
	}
	EvaPicPacket::setFileAgentKey(Packet::getFileAgentKey());
	
	if (m_hWndPopup) {
		wcscpy(m_popupText,TranslateT("Recv: "));
		m_popupTextP=m_popupText+wcslen(m_popupText);
		wcscpy(m_popupTextP,TranslateT("Connecting to Server"));
		PUChangeTextW(m_hWndPopup,m_popupText);
	}

	currentPic = *(picList.begin());
	initConnection(currentPic.ip, currentPic.port);
}

void CQunImage::initConnection(const int ip, const short port) {
	int ip2=htonl(ip);
	util_log(0,"EvaPicManager::initConnection ip:%s, port:%d", inet_ntoa(*(in_addr*)&ip2), port);
	if (ip==-1)
		setServer(false,GROUP_FILE_AGENT,port);
	else
		setServer(false,ip,port);

	sendIP = htonl(inet_addr(getHost().c_str()));
	sendPort = getPort();

	if (sendIP==0) {
		doProcessEvent();
	} else {
		if (m_hWndPopup) {
			swprintf(m_popupTextP,TranslateT("Connecting to %S"),inet_ntoa(*(in_addr*)&ip2));
			PUChangeTextW(m_hWndPopup,m_popupText);
		}

		if (isConnected())
			disconnect();
		else
			connect();
	}
}

void CQunImage::doProcessOutEvent() {
	if(!sendList.size()) {
		isBusy = false;
		if(downloadList.size()) 
			doProcessEvent();
		else
			disconnect();
		return;
	}
	isBusy = true;
	isSend = true;
	OutSession session = sendList.front();
	sendList.pop_front();
	qunID = session.qunID;
	outList = session.list;
	outMsg=session.msg;
	if(!outList.size()){
		isBusy = false;
		doProcessOutEvent( );
		return;
	}
	currentIndex = -1;
	if(!Packet::getFileAgentKey()) {
		clearManager();
		return;
	}
	EvaPicPacket::setFileAgentKey(Packet::getFileAgentKey());
	currentOutPic = *(outList.begin());
	if (m_hWndPopup) {
		wcscpy(m_popupText,TranslateT("Send: "));
		m_popupTextP=m_popupText+wcslen(m_popupText);
		wcscpy(m_popupTextP,TranslateT("Connecting to Server"));
		PUChangeTextW(m_hWndPopup,m_popupText);
	}
	initConnection(-1, 443);
}

void CQunImage::waitTimedOut() {
	if(isAppending || isRemoving ) return;

	/*
	util_log(0,"[CQunImage] waitTimedOut, outPool.size=%d",outPool.size());
	for (list<EvaPicOutPacket*>::iterator iter=outPool.begin(); iter!=outPool.end(); iter++) {
		if ((*iter)->needResend()) {
			sendPacket(*iter);
		} else {
			removePacket((*iter)->hashCode());
			isBusy = false;
			util_log(0, "[CQunImage] packetMonitor -- time out");
			return;
		}
		if(isAppending || isRemoving ) break;
	}
	*/
	m_timeoutCount++;

	if (m_timeoutCount>=1) {
		util_log(0,"[CQunImage] connection holds for too long, disconnect");
		disconnect();
	}
}

void CQunImage::sendPacket(EvaPicOutPacket *packet) {
	if(!isConnected()) {
		util_log(0,"[CQunImage] sendPacket -- connecter NULL");
		return;
	}
	//EnterCriticalSection(&m_cs);
	unsigned char *buf = new unsigned char[MAX_PACKET_SIZE];
	int len;
	CEvaAccountSwitcher::ProcessAs(m_network->GetMyQQ());
	packet->fill(buf, &len);
	CEvaAccountSwitcher::EndProcess();

	/*
	FILE* fp=fopen("mimqq2-qi.bin","wb");
	fwrite(buf,len,1,fp);
	fclose(fp);
	DebugBreak();
	*/

	send((LPSTR)buf,len);
	delete buf;
	//LeaveCriticalSection(&m_cs);
}

void CQunImage::removePacket(const int hashCode) {
	isRemoving = true;
	//EnterCriticalSection(&m_cs);

	for (list<EvaPicOutPacket*>::iterator iter=outPool.begin(); iter!=outPool.end();) {
		if ((*iter)->hashCode()==hashCode) {
			delete *iter;
			iter=outPool.erase(iter);
		} else
			iter++;
	}

	//LeaveCriticalSection(&m_cs);
	isRemoving = false;
}

void CQunImage::connectionError() {
	//isRemoving = true;
	/*
	delete outPool.front();
	outPool.pop_front();
	*/
	//isRemoving = false;
	picList.clear();
	//doRequestNextPic();
	doProcessEvent();
}

void CQunImage::connectionEstablished() {
	//util_log(0,"EvaPicManager::slotReady -- connection ready");
	/*
	EvaPicPacket::setFileAgentKey(Packet::getFileAgentKey());
	EvaPicPacket::setQQ(m_network->GetMyQQ());
	*/

	if(!isSend)
		doRequestNextPic();
	else
		doRequestAgent();
}

bool CQunImage::crashRecovery() {
	util_log(0,"CQunImage::crashRecovery()");
	clearManager();
	// doProcessEvent();
	m_network->qunPicCallbackHub(-1,0,NULL);
	delete this;

	return true;
}

void CQunImage::connectionClosed() {
	//if (isRedirect()) return;
	util_log(0,"CQunImage::connectionClosed(), isBusy=%d",isBusy);

	if (isBusy) {
		clearManager();
		doProcessEvent();
	}
	util_log(0,"CQunImage::connectionClosed(2), isBusy=%d",isBusy);
	if (!isBusy) {
		m_network->qunPicCallbackHub(-1,0,NULL);
		delete this;
	}
}

int CQunImage::dataReceived(NETLIBPACKETRECVER* nlpr) {
	//util_log(0,"EvaPicManager::dataReceived -- data received");
	m_timeoutCount=0;

	if(nlpr->buffer[0] == FAMILY_05_TAG && nlpr->buffer[nlpr->bytesAvailable-1] == FAMILY_05_TAIL){
		CEvaAccountSwitcher::ProcessAs(m_network->GetMyQQ());
		EvaPicInPacket *packet = new EvaPicInPacket(nlpr->buffer+nlpr->bytesUsed, nlpr->bytesAvailable-nlpr->bytesUsed);
		int pLen = packet->getPacketLength();
		while(pLen>0 && pLen <= nlpr->bytesAvailable-nlpr->bytesUsed){
			packet->cutOffPacketData();
			removePacket(packet->hashCode());
			parseInData(packet);
			nlpr->bytesUsed+=pLen;

			if (isRedirect()) break;
			//if (nlpr->bytesUsed>=nlpr->bytesAvailable) break;
			delete packet;
			packet = new EvaPicInPacket(nlpr->buffer+nlpr->bytesUsed, nlpr->bytesAvailable-nlpr->bytesUsed);
			pLen = packet->getPacketLength();
		}
		delete packet;
		CEvaAccountSwitcher::EndProcess();
	}

	return 0;
}

void CQunImage::doRequestNextPic() {
	currentIndex++;
	if(currentIndex >= (int)picList.size()){
		picList.clear();
		doProcessEvent();
		return;
	}

	currentFile.offset = 0;
	if(currentFile.buf) 
		delete currentFile.buf; 
	currentFile.buf = NULL;
	currentFile.lastPacketSeq = 0xffff;
	int index = 0;
	std::list<CustomizedPic>::iterator iter;
	for(iter = picList.begin(); iter!=picList.end(); ++iter){
		if(index == currentIndex){
			break;
		}
		index++;
	}
	if(iter != picList.end()){
		currentPic = *iter;
		doRequestPic(currentPic);
	}
}

void CQunImage::doRequestPic(CustomizedPic pic) {
	if( pic.fileName.empty() ){
		doRequestNextPic();
		return;
	}

	LPWSTR tmpFileName=mir_a2u_cp(currentPic.tmpFileName.c_str(),936);

	//util_convertToNative(&tmpFileName,currentPic.tmpFileName.c_str());

	if(GetFileAttributes(tmpFileName)!=INVALID_FILE_ATTRIBUTES){
		mir_free(tmpFileName);
		pictureReady(qunID, tmpFileName);
		doRequestNextPic();
		return;
	}
	mir_free(tmpFileName);

	EvaRequestFacePacket *packet = new EvaRequestFacePacket();
	if (m_hWndPopup) {
		wcscpy(m_popupTextP,TranslateT("Requesting Face"));
		PUChangeTextW(m_hWndPopup,m_popupText);
	}
	packet->setQunID(qunID);
	packet->setKey(pic.fileAgentKey);
	packet->setFileAgentToken(Packet::getFileAgentToken(), Packet::getFileAgentTokenLength());
	packet->setSessionID(pic.sessionID);
	sessionID = pic.sessionID;
	append(packet);
}

void CQunImage::append(EvaPicOutPacket *packet) {
	isAppending = true;
	sendPacket(packet);   // force to send
	if(packet->needAck())
		outPool.push_back(packet);
	else
		delete packet;
	isAppending = false;
}
#if 0
void CQunImage::slotProcessBuffer() {
	if(buf[0] == FAMILY_05_TAG && buf[bufLength-1] == FAMILY_05_TAIL){
		EvaPicInPacket *packet = new EvaPicInPacket(buf, bufLength);
		unsigned int pLen = packet->getPacketLength();
		util_log(0,"[CQunImage] slotProcessBuffer - pLen=%d",pLen);
		while(pLen <= bufLength){
			packet->cutOffPacketData();
			memcpy(buf, buf+pLen, bufLength-pLen);
			bufLength -= pLen;
			removePacket(packet->hashCode());
			parseInData(packet);
			delete packet;
			packet = new EvaPicInPacket(buf, bufLength);
			pLen = packet->getPacketLength();
			util_log(0,"[CQunImage] slotProcessBuffer - pLen=%d",pLen);
		}
		delete packet;
	}
}
#endif
void CQunImage::parseInData(const EvaPicInPacket *in) {
	if(in->isValid()){
		switch(in->getCommand()){
		case QQ_05_CMD_REQUEST_AGENT:
			processRequestAgentReply(in);
			break;
		case QQ_05_CMD_REQUEST_FACE:
			processRequestFaceReply(in);
			break;
		case QQ_05_CMD_TRANSFER:
			processTransferReply(in);
			break;
		case QQ_05_CMD_REQUEST_START:
			processRequestStartReply(in);
			break;
		}
	} else {
		util_log(0,"CQunImage parseInData -- invalid packet!");
		DebugBreak();
	}
}

void CQunImage::processRequestAgentReply(const EvaPicInPacket *in) {
	util_log(0,__FUNCTION__);
	RequestAgentReplyPacket *packet = new RequestAgentReplyPacket(in->getRawBody(), in->getRawBodyLength());
	packet->parse();
	if(expectedSequence != packet->getSequence()){
		delete packet;
		return;
	}
	switch(packet->getReplyCode()){
	case QQ_REQUEST_AGENT_REPLY_OK:
		{
			sessionID = packet->getSessionID();
			currentOutPic.ip=packet->getServerIP();
			currentOutPic.port=packet->getServerPort();
			//printf("EvaPicManager::processRequestAgentReply -- \n\tmessage:%s\n", packet->getMessage().c_str());
			if(currentIndex == -1)
				doRequestStart();
			else
				doSendFileInfo();
	   }
	   break;
	case QQ_REQUEST_AGENT_REPLY_REDIRECT:
		util_log(0,"[CQunImage] processRequestAgentReply -- redirect");
		initConnection(packet->getRedirectIP(), packet->getRedirectPort());
		break;
	case QQ_REQUEST_AGENT_REPLY_TOO_LONG:
	default:
		clearManager();
		std::string str = packet->getMessage();
		util_log(0,"[CQunmage] processRequestAgentReply -- agent error:%s", packet->getMessage().c_str());
		emit sendErrorMessage(qunID, str.c_str());
		break;
	}
	delete packet;
}

// We are not interested in this, so just ignore it :)
void CQunImage::processRequestFaceReply(const EvaPicInPacket *in) {
	util_log(0,"[CQunImage] processRequestFaceReply -- got request face reply");
}

void CQunImage::processTransferReply(const EvaPicInPacket *in) {
	EvaPicTransferReplyPacket *packet = new EvaPicTransferReplyPacket(in->getRawBody(), in->getRawBodyLength());
	packet->parse();
	if(packet->getLength() == 0) {
		fprintf(stderr, "EvaPicManager::processTransferReply -- bodyLength is Zero!!!\n");
		delete packet;
		return;
	}
	if(packet->getData()){
		if(!isSend){ // handleImageDataAcknowledged
			if(currentFile.lastPacketSeq!=0xffff && currentFile.lastPacketSeq >= packet->getSequence()){
				delete packet;
				return;
			}
			if(!currentFile.buf){
				delete packet;
				doRequestNextPic();
				return;
			}
			currentFile.lastPacketSeq = packet->getSequence();
			memcpy(currentFile.buf + currentFile.offset, packet->getData(), packet->getDataLength());
			if (m_hWndPopup) {
				swprintf(m_popupTextP,TranslateT("Receiving %d/%d"),currentFile.offset,currentFile.length);
				PUChangeTextW(m_hWndPopup,m_popupText);
			}
			util_log(0,"[CQunImage] %d/%d",currentFile.offset,currentFile.length);
			currentFile.offset += packet->getDataLength();
			if(currentFile.offset >= currentFile.length){
				doSaveFile();
				doRequestNextPic();
				delete packet;
				return;
			}
			doRequestData(currentPic, true);
		} else { // handleImageInfoAcknowledged
			doSendNextFragment();
		}
	} else if (packet->getFileName().length()>0) {
		currentFile.filename = packet->getFileName().c_str();
		currentFile.length = packet->getImageLength();
		currentFile.buf = new unsigned char [ currentFile.length ];
		memset(currentFile.buf, 0, currentFile.length);
		currentFile.offset = 0;
		currentFile.lastPacketSeq = 0xffff;
		doRequestData(currentPic, false);
	}
	delete packet;
}

void CQunImage::processRequestStartReply(const EvaPicInPacket *in) {
	EvaRequestStartReplyPacket *packet = new EvaRequestStartReplyPacket(in->getRawBody(), in->getRawBodyLength());
	packet->parse();
	if(sessionID != packet->getSessionID()){
		delete packet;
		return;
	}
	if(isSend){
		currentIndex++;// now we start
		doSendFileInfo();
	}
	delete packet;
}

void CQunImage::doRequestAgent() {
	util_log(0,__FUNCTION__);
	EvaRequestAgentPacket *packet = new EvaRequestAgentPacket(Packet::getFileAgentToken(), Packet::getFileAgentTokenLength());
	if (m_hWndPopup) {
		wcscpy(m_popupTextP,TranslateT("Requesting Agent"));
		PUChangeTextW(m_hWndPopup,m_popupText);
	}

	packet->setQunID(qunID);
	packet->setMd5(currentOutPic.md5);
	packet->setImageLength(currentOutPic.imageLength);
	packet->setFileName(currentOutPic.fileName);
	packet->setTransferType(currentOutPic.transferType=/*currentOutPic.imageLength>61440?1000:*/1100);
	expectedSequence = packet->getSequence();
	append(packet);
}

void CQunImage::doSendFileInfo() {
	currentFile.filename = strrchr(currentOutPic.fileName.c_str(),'\\')+1; // in GBK
	currentFile.length = currentOutPic.imageLength;
	currentFile.offset = 0;
	currentFile.buf = new unsigned char[currentFile.length];

	LPWSTR pszFileName=mir_a2u_cp(currentOutPic.fileName.c_str(),936);
	HANDLE hFile=CreateFile(pszFileName,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,NULL,NULL);
	if (hFile==INVALID_HANDLE_VALUE) {
		util_log(0,"[CQunImage] doSendFileInfo -- cannot open file \'%s\' !", currentOutPic.fileName.c_str());
		free(pszFileName);
		return;
	}

	mir_free(pszFileName);

	DWORD dwRead;
	ReadFile(hFile,currentFile.buf,currentFile.length,&dwRead,NULL);
	CloseHandle(hFile);

	EvaPicTransferPacket *packet = new EvaPicTransferPacket(false, false);
	if (m_hWndPopup) {
		wcscpy(m_popupTextP,TranslateT("Sending File Info"));
		PUChangeTextW(m_hWndPopup,m_popupText);
	}
	packet->setSessionID(sessionID);
	packet->setMd5(currentOutPic.md5);
	packet->setImageLength(currentFile.length);
	packet->setFileName(currentFile.filename.c_str());
	expectedSequence = packet->getSequence();

	// EvaPicTransferPacket *packet2 = new EvaPicTransferPacket(packet);
	append(packet);
	/*
	expectedSequence = packet2->getSequence();
	append(packet2);
	*/
}

void CQunImage::doRequestStart() {
	EvaRequestStartPacket *packet = new EvaRequestStartPacket();
	if (m_hWndPopup) {
		wcscpy(m_popupTextP,TranslateT("Requesting Start"));
		PUChangeTextW(m_hWndPopup,m_popupText);
	}
	packet->setSessionID(sessionID);
	packet->setMd5(currentOutPic.md5);
	packet->setTransferType(currentOutPic.transferType);
	append(packet);
}

void CQunImage::doSendNextFragment() {
	if(currentFile.length <= currentFile.offset){
		currentIndex++;
		if(currentIndex >= (int)(outList.size())){
			m_network->qunPicCallbackHub(1,qunID,&sentList);
			doProcessOutEvent();
			return;
		}
		currentFile.offset = 0;
		if(currentFile.buf) 
			delete currentFile.buf; 
		currentFile.buf = NULL;
		int index = 0;
		std::list<OutCustomizedPic>::iterator iter;
		for(iter = outList.begin(); iter!=outList.end(); ++iter){
			if(index == currentIndex){
				break;
			}
			index++;
		}
		if(iter != outList.end()){
			currentOutPic = *iter;
			//doSendFileInfo();
			doRequestAgent();
			return;
		}
	}
	disbleWriteBuffer();
	for(int i=0; i<10; i++){
	//while (true) {
		bool isLast = ( (currentFile.length - currentFile.offset) <= 1024 );
		unsigned int len = isLast?((currentFile.length - currentFile.offset)):1024;
		EvaPicTransferPacket *packet = new EvaPicTransferPacket(true, isLast);
		packet->setSessionID(sessionID);
		packet->setFragment(currentFile.buf + currentFile.offset, len);
		currentFile.offset += len;

		if (m_hWndPopup) {
			swprintf(m_popupTextP,TranslateT("Sending %d/%d"),currentFile.offset,currentFile.length);
			PUChangeTextW(m_hWndPopup,m_popupText);
		}
		util_log(0,"[CQunImage] %d/%d",currentFile.offset,currentFile.length);
		//if (isLast) disbleWriteBuffer();
		append(packet);
		if(isLast) {
			//if((currentIndex+1) == (int)(outList.size())){
				postqunimage_t pqi;
				pqi.ip=sendIP;
				pqi.port=sendPort;
				pqi.message=outMsg;
				pqi.sessionid=sessionID;
				pqi.md5=EvaHelper::md5ToString((char*)currentOutPic.md5);
				//m_callbackHub(1,currentOutSession.qunID,&pqi);
				//m_network->qunPicCallbackHub(1,qunID,&pqi);
				sentList.push_back(pqi);

				//pictureSent(QQNetwork::getInstance(), qunID, sessionID, htonl(addr.sin_addr.S_un.S_addr), sendPort);

				//outList.clear();
				//emit pictureSent(qunID, sessionID, sendIP, sendPort);
				return;
			//}
			break;
		}
	}
}

void CQunImage::doRequestData(CustomizedPic pic, const bool isReply) {
	EvaPicTransferPacket *packet = new EvaPicTransferPacket();
	/*
	if (m_hWndPopup) {
		wcscpy(m_popupTextP,TranslateT("Requesting Data"));
		PUChangeTextW(m_hWndPopup,m_popupText);
	}
	*/
	packet->setSessionID(pic.sessionID);
	packet->setDataReply(isReply);
	append(packet);
}

void CQunImage::doSaveFile() {
	LPWSTR fileName=mir_a2u_cp(currentPic.tmpFileName.c_str(),936);
	HANDLE hFile;
	// WCHAR szFileOut[MAX_PATH];

	/*
	CallService(MS_UTILS_PATHTOABSOLUTEW,(WPARAM)L"QQ",(LPARAM)szFileOut);

	if (GetFileAttributes(szFileOut)==INVALID_FILE_ATTRIBUTES) CreateDirectory(szFileOut,NULL);
	wcscat(szFileOut,L"\\QunImages");
	if (GetFileAttributes(szFileOut)==INVALID_FILE_ATTRIBUTES) CreateDirectory(szFileOut,NULL);
	wcscat(szFileOut,L"\\");
	*/
	// FoldersGetCustomPathW(m_network->m_folders[1],szFileOut,MAX_PATH,"QQ\\QunImages\\");

	//util_convertToNative(&fileName,currentPic.tmpFileName.c_str());
	hFile=CreateFile(fileName,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,NULL,0);
	if (hFile==INVALID_HANDLE_VALUE) {
		util_log(0,"[CQunImage] doSaveFile -- cannot open file \'%S\'!\n", fileName);
		mir_free(fileName);
		return;
	}

	DWORD dwWritten;
	WriteFile(hFile,currentFile.buf,currentFile.offset,&dwWritten,NULL);
	CloseHandle(hFile);

	if(currentFile.buf)
		delete currentFile.buf;
	currentFile.buf = NULL;
	currentFile.offset = 0;
	currentFile.lastPacketSeq = 0xffff;
	pictureReady(qunID, wcsrchr(fileName,'\\')+1);
	mir_free(fileName);
}

void CQunImage::pictureReady(const UINT id, LPCWSTR fileName) {
	m_imageServer->signalFile(fileName);
}

void CQunImage::pictureSent(const unsigned int id, const unsigned int sessionID, const unsigned int ip, const unsigned short port) {
}

void CQunImage::sendErrorMessage(const UINT id, LPCSTR msg) {
	m_network->qunPicCallbackHub(2,id,(LPVOID)msg);
}

void CQunImage::shutdownImageServer() {
	if (m_imageServer) {
		delete m_imageServer;
		m_imageServer=NULL;
	}
}