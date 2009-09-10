/* MirandaQQ2 (libeva Version)
* Copyright(C) 2005-2007 Studio KUMA. Written by Stark Wong.
*
* Distributed under terms and conditions of GNU GPLv2.
*
* Plugin framework based on BaseProtocol. Copyright (C) 2004 Daniel Savi (dss@brturbo.com)
*
* This plugin utilizes the libeva library. Copyright(C) yunfan.

Miranda ICQ: the free icq client for MS Windows 
Copyright (C) 2000-5  Richard Hughes, Roland Rabien & Tristan Van de Vreede
*/
/***************************************************************************
 *   Copyright (C) 2005 by yunfan                                          *
 *   yunfan_zg@163.com                                                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
/*
//#include "../MirandaQQ.h"
#include "../qqapi.h"
#include "evacachedfile.h"
#include "evafiledownloader.h"
#include "libft/evaftprotocols.h"
#include "evautil.h"
*/
#include "stdafx.h"

#define RELAY_SERVER_URL             "RelayServer2.tencent.com"
#define RELAY_SERVER_DEFAULT_IP      "219.133.40.38"
#define RELAY_SERVER_PORT            443
#define SYN_SERVER_URL               "synserver.tencent.com"
#define SYN_SERVER_DEFAULT_IP        "219.133.49.80"
#define SYN_SERVER_PORT              8000
#define QQ_VERSION                   0x0D51

#define QQ_TRANSFER_FILE             0x65
#define QQ_TRANSFER_IMAGE            0x66

EvaFileThread::EvaFileThread(void *receiver, const int id, const list<string> &dirList, 
				const list<string> &filenameList,
				const list<unsigned int> sizeList, const bool isSender)
	: m_IsSender(isSender), m_Receiver(receiver),
	m_Id(id), m_Session(0), m_StartOffset(0), m_ExitNow(false),
	m_BytesSent(0), m_File(NULL), m_Connecter(NULL), m_running(false)
{
	if(dirList.size() != filenameList.size()) {
		m_ExitNow = true;
		return;
	}
	m_DirList = dirList;
	m_FileNameList = filenameList;
	m_SizeList = sizeList;
	EvaCachedFile *file;
	list<string>::const_iterator dirListIter;
	list<string>::const_iterator filenameListIter;
	list<unsigned int>::const_iterator sizeListIter;

	//for(unsigned int i = 0; i < dirList.size(); ++i){
	filenameListIter=filenameList.begin();
	sizeListIter=sizeList.begin();
	for (dirListIter=dirList.begin(); dirListIter!=dirList.end(); dirListIter++) {
		util_log(0,"EvaFileThread::EvaFileThread -- dir: %s\t filename:%s\n", (*dirListIter).c_str(), (*filenameListIter).c_str());
		if(m_IsSender)
			file = new EvaCachedFile(*dirListIter, *filenameListIter);
		else
			file = new EvaCachedFile(*dirListIter, *filenameListIter, *sizeListIter);
		m_FileList.push_back(file);
		//delete file;

		filenameListIter++;
		if (sizeListIter!=sizeList.end()) sizeListIter++;
	}
	//m_FileList.setAutoDelete(true);
	m_File = m_FileList.front();
	m_Dir = dirList.front();
	m_FileName = filenameList.front();
}

EvaFileThread::~EvaFileThread()
{
	cleanUp();
}

void EvaFileThread::setDir( const string & dir )
{
	if(!m_DirList.size()) return;
	m_Dir = dir;
	if(!m_File) return;
	m_File->setDestFileDir(dir);
	//m_DirList.push_front(m_Dir); // for file transfer, one file each session, not like images
	if (m_DirList.size()) m_DirList.pop_front();
	m_DirList.push_front(m_Dir);
}

const string EvaFileThread::getDir() const
{
	return m_Dir;
}

const unsigned int EvaFileThread::getFileSize()
{
	if(!m_File) return 0;
	return m_File->getFileSize();
}

void EvaFileThread::notifyTransferStatus()
{
	int timeElapsed = (int)time(NULL)-m_StartTime;
	EvaFileNotifyStatusEvent *event = new EvaFileNotifyStatusEvent();
	event->setFileSize(m_File->getFileSize());
	event->setBytesSent(m_StartOffset + m_BytesSent);
	event->setTimeElapsed(timeElapsed);
	event->setSession(m_Session);
	event->setBuddyQQ(m_Id);
	//QApplication::postEvent(m_Receiver, event);
	customEvent(event);
}

void EvaFileThread::notifyNormalStatus(const EvaFileStatus status)
{
	EvaFileNotifyNormalEvent *event = new EvaFileNotifyNormalEvent();
	event->setSession(m_Session);
	event->setBuddyQQ(m_Id);
	event->setStatus(status);
	event->setFileName(m_FileName);
	event->setFileDir(m_Dir);
	event->setTransferType(m_TransferType);

	if(status == ESResume){
		event->setFileSize(m_File->getNextOffset());
	} else
		event->setFileSize(m_File->getFileSize());
	//QApplication::postEvent(m_Receiver, event);
	customEvent(event);
}

void EvaFileThread::cleanUp()
{
	while (m_FileList.size()) {
		delete m_FileList.front();
		m_FileList.pop_front();
	}
	m_FileList.clear();
	m_DirList.clear();
	m_FileNameList.clear();
	m_SizeList.clear();

//	if(m_File) delete m_File;
	if(m_Connecter) {
		m_Connecter->DisableCallbacks();
		m_Connecter->Disconnect();
		m_Connecter=NULL;
	}
}

/** ================================================================== */


EvaAgentThread::EvaAgentThread(void *receiver, const int id, const list<string> &dirList,
			const list<string> &filenameList, 
			list<unsigned int> sizeList, const bool isSender)
	: EvaFileThread(receiver, id, dirList, filenameList, sizeList, isSender),
	m_State(ENone), m_Token(NULL), m_TokenLength(0),  m_ServerPort(RELAY_SERVER_PORT),
	m_BufferLength(0), m_PacketLength(0)
{
}

EvaAgentThread::~ EvaAgentThread()
{
	if(m_Token) delete []m_Token;
}

void EvaAgentThread::setFileAgentToken(const unsigned char *token, const int len)
{
	if(!token) return;
	if(m_Token) delete [] m_Token;
	m_Token = new unsigned char[len];
	memcpy(m_Token, token, len);
	m_TokenLength = len;
}

void EvaAgentThread::setFileAgentKey(const unsigned char *key)
{
	memcpy(m_FileAgentKey, key, 16);
}


void EvaAgentThread::setServerAddress(const unsigned int ip, const unsigned short port)
{
	m_HostAddresses.clear();
	//m_HostAddresses.append(QHostAddress(ip));
	m_HostAddresses.push_back(ip);
	m_ServerPort = port;
}

/*void EvaAgentThread::setProxySettings(const QHostAddress addr, const short port, const QCString &param)
{
	m_ProxyServer = addr;
	m_ProxyPort = port;
	m_ProxyAuthParam = param;
	m_UsingProxy = true;
}*/

void EvaAgentThread::doCreateConnection()
{
	if(m_Connecter){
		//m_Connecter->close();
		m_Connecter->DisableCallbacks();
		m_Connecter->Disconnect();
		m_Connecter = NULL;
		//delete m_Connecter;
	}
	/*if(m_UsingProxy){
		m_Connecter = new EvaNetwork(m_ProxyServer, m_ProxyPort, EvaNetwork::HTTP_Proxy);
		m_Connecter->setDestinationServer(m_HostAddresses.first().toString(), m_ServerPort);
		m_Connecter->setAuthParameter(m_ProxyAuthParam);
	}else
		m_Connecter = new EvaNetwork(m_HostAddresses.first(), m_ServerPort, EvaNetwork::TCP);*/
	/*
	in_addr addr;
	addr.S_un.S_addr=m_HostAddresses.front();
	*/
	unsigned int ip=m_HostAddresses.front();

	m_Connecter=new QQConnection2(hNetlibUser,QQConnection2::CONNTYPE_TCP, inet_ntoa(/*addr*/*(in_addr*)&ip),m_ServerPort,5000,this,false);
	m_Connecter->SetThreadName("EvaAgentThread");
	//m_Connecter->SetDump(true);

	/*QObject::connect(m_Connecter, SIGNAL(isReady()), SLOT(slotNetworkReady()));
	QObject::connect(m_Connecter, SIGNAL(dataComming(int)), SLOT(slotDataComming(int)));
	QObject::connect(m_Connecter, SIGNAL(exceptionEvent(int)), SLOT(slotNetworkException(int)));*/
	m_State = ENone;
	m_Connecter->Connect();
}

void EvaAgentThread::send(EvaFTAgentPacket *packet)
{
	if(! m_Connecter ){
		util_log(0, "EvaAgentThread::send -- Network invalid!\n");
		delete packet;
		m_State = EError;
		return;
	}
	
	// set the header information & key
	packet->setFileAgentKey(m_FileAgentKey);
	packet->setQQ(m_MyId);
	packet->setVersion(QQ_VERSION);
	packet->setSequence(m_Sequence);
	packet->setSessionId(m_Session);

	unsigned char *buffer = new unsigned char[4096];
	int len = 0;
	packet->fill(buffer, &len);
	//packet->fill(buffer, &len);
	/*FILE* fp=fopen("f:\\mqq1-fd.txt","wb");
	fwrite(buffer,len,1,fp);
	fclose(fp);*/
	/*if(!m_Connecter->write((char *)buffer, len)){
		delete []buffer;
		delete packet;
		m_State = EError;
		return;
	}*/
	m_Connecter->SendData((char*)buffer,len);
	delete []buffer;
	delete packet;
}

void EvaAgentThread::processAgentPacket( unsigned char * /*data*/, int /*len*/ )
{
	util_log(0, "EvaAgentThread::processAgentPacket -- Not Implemented, Error!\n");
	m_State = EError;
}

void EvaAgentThread::ConnectionErrorCallback(int code, void* data) {
	util_log(0, "EvaAgentThread::slotNetworkException -- no: %d\n", code);
	if(m_State != EFinished) m_State = EError;
}

void EvaAgentThread::ConnectionDataReceiveCallback(const char* data, const int len) {
	//util_log(0,"EvaAgentThread::ConnectionDataReceiveCallback");
	//processAgentPacket((unsigned char*)data,len);
	memcpy(m_Buffer + m_BufferLength, data, len);
	m_BufferLength += len;
	m_PacketLength = EvaUtil::read16(m_Buffer + 3);
	while(m_BufferLength >= m_PacketLength){
		char* rawData = new char[m_PacketLength];
		int len2;
		memcpy(rawData, m_Buffer, m_PacketLength);
		memcpy(m_Buffer, m_Buffer + m_PacketLength, m_BufferLength - m_PacketLength);
		len2 = m_PacketLength;
		m_BufferLength -= m_PacketLength;
		processAgentPacket((unsigned char *)rawData, len2);
		delete []rawData;
		if(!m_BufferLength)   break;
		m_PacketLength = EvaUtil::read16(m_Buffer + 3);
	}

}

void EvaAgentThread::ConnectionSelectTimeoutCallback() {

}

void EvaAgentThread::ConnectionCloseCallback() {
	m_Connecter=NULL;
	if(m_State != EFinished) m_State = EError;
}

void EvaAgentThread::ConnectionReadyCallback() {
	util_log(0, "EvaAgentThread::slotNetworkReady\n");
	m_State = ENetworkReady;
}

/** ================================================================== */


EvaAgentUploader::EvaAgentUploader(void *receiver, const int id, const list<string> &dirList,
			const list<string> &filenameList)
	: EvaAgentThread(receiver, id, dirList, filenameList, list<unsigned int>(), true),
	m_IsSendingStart(false)
{
	setThreadType(1);
	m_Sequence = 0x0005;// give it a random number anyway
	m_OutBufferLength = 50 * EVA_FILE_BUFFER_UNIT;
	m_OutBuffer = new unsigned char[m_OutBufferLength];
	m_NumPackets = 0; // send 50 packets every time if the left data is enough
	m_OutBytesSent = 0;
}

EvaAgentUploader::~EvaAgentUploader()
{
	util_log(0,"EvaAgentUploader Destruction");
	if(m_OutBuffer) delete m_OutBuffer;
}

void EvaAgentUploader::__run() {
	m_State = EDnsQuery;
	while(!m_ExitNow){
		switch(m_State){
		case ENone:
			break;
		case EDnsQuery:
			doDnsRequest();
			break;
		case EDnsReady:
			doCreateConnection();
			break;
		case ENetworkReady:
			doCreateRequest();
			break;
		case ECreatingReady:
			doNotifyBuddy();
			break;
		case ENotifyReady:
			doReadyReply();
			break;
		case EAskForStart:
			doStartRequest();
			doSendInfo();
			break;
		case ETransfer:
			doDataTransfering();
			break;
		case EFinished:
			doFinishProcessing();
			break;
		case EError:
			doErrorProcessing();
			break;
		default:
			break;
		}
		Sleep(200);
	}
}

DWORD WINAPI EvaAgentUploader::_run(void* param) {
	((EvaAgentUploader*)param)->__run();
	return 0;
}

void EvaAgentUploader::run()
{
	DWORD dwThreadID;
	util_log(0,"EvaAgentUploader::run \n");
	CreateThread(NULL,0,_run,this,NULL,&dwThreadID);
}

void EvaAgentUploader::doDnsRequest()
{
	printf("EvaAgentUploader::doDnsRequest\n");
	m_HostAddresses.clear();
	slotDnsReady();
	/*
	if(m_Dns) delete m_Dns;
	m_Dns = new QDns(RELAY_SERVER_URL);
	QObject::connect(m_Dns, SIGNAL(resultsReady()), SLOT(slotDnsReady()));
	*/
	
// 	while(!m_HostAddresses.size()){
// 		printf("EvaAgentUploader::doDnsRequest -- waiting for DNS results\n");
// 		if(m_ExitNow) break;
// 		msleep(200);
// 	}
	//m_State = ENone;
}

void EvaAgentUploader::slotDnsReady()
{
/*	m_HostAddresses = m_Dns->addresses();
	if(!m_HostAddresses.size()){
		QHostAddress host;
		host.setAddress(RELAY_SERVER_DEFAULT_IP);
		m_HostAddresses.append(host);
	}*/
	m_HostAddresses.push_back(inet_addr(RELAY_SERVER_DEFAULT_IP));
	m_State = EDnsReady;
}

void EvaAgentUploader::doCreateRequest()
{
	if(!m_Token){
		m_State = EError;
		return;
	}
	//connect(m_Connecter, SIGNAL(writeReady()), SLOT(slotWriteReady()));
	m_Sequence++;
	EvaFTAgentCreate *packet = new EvaFTAgentCreate();
	packet->setBuddyQQ(m_Id);
	packet->setIp(m_BuddyIp);
	packet->setFileAgentToken(m_Token, m_TokenLength);
	send(packet);
	m_State = ENone; // waiting the response from server
	util_log(0,"EvaAgentUploader::doCreateRequest\n");
}

void EvaAgentUploader::doNotifyBuddy()
{
	EvaFileNotifyAgentEvent *event = new EvaFileNotifyAgentEvent();
	event->setOldSession(m_Session);
	event->setAgentSession(m_AgentSession);
	event->setAgentIp(m_HostAddresses.front());
	event->setAgentPort(m_ServerPort);
	event->setMyFileAgentKey(m_FileAgentKey);
	event->setBuddyQQ(m_Id);
	event->setTransferType(m_TransferType);
	//QApplication::postEvent(m_Receiver, event);
	customEvent(event);

	m_Session = m_AgentSession; // now we use agent session
	m_State = ENone;
}

void EvaAgentUploader::doReadyReply()
{
	EvaFTAgentAckReady *readyPacket = new EvaFTAgentAckReady();
	send(readyPacket);
	m_State = EAskForStart;
}

void EvaAgentUploader::doStartRequest()
{
	
	EvaFTAgentStart *startPacket = new EvaFTAgentStart();
	send(startPacket);
	m_State = ENone;
}

void EvaAgentUploader::doSendInfo()
{
	unsigned char *md5 = new unsigned char[16];
	if(!m_File->getSourceFileMd5((char *)md5)){
		m_State = EError;
		delete []md5;
		return;
	}
	
	EvaFTAgentTransfer *TransferPacket = new EvaFTAgentTransfer(QQ_FILE_AGENT_TRANSFER_INFO);

	//QTextCodec *codec = QTextCodec::codecForName("GB18030");
	TransferPacket->setInfo(m_FileName.c_str(),md5, m_File->getFileSize());
	send(TransferPacket);
	delete []md5;
	m_IsSendingStart = false;
	m_State = ENone;
	util_log(0,"EvaAgentUploader::doSendInfo -- sent: %s\n", m_FileName.c_str());
}

// we send 50 packets, if it has 50, every time
void EvaAgentUploader::doDataTransfering()
{
	m_State = ETransfering;
// 	EvaFTAgentTransfer *packet;
// 	unsigned int bytesSent = 0, bytesToSend = 0;
// 	unsigned int bufferLength = 50 * EVA_FILE_BUFFER_UNIT;
// 	unsigned char *buf = new unsigned char [bufferLength];
// 	bufferLength = m_File->getFragment(m_BytesSent, bufferLength, buf);

	m_NumPackets = 0;
	m_OutBytesSent = 0;
	m_OutBufferLength = 50 * EVA_FILE_BUFFER_UNIT;
	m_OutBufferLength = m_File->getFragment(m_BytesSent, m_OutBufferLength, m_OutBuffer);
	//m_Connecter->setWriteNotifierEnabled(true);
	util_log(0,"EvaAgentUploader::doDataTransfering\n");
// 	for(int i = 0; i<50; i++){
// 		m_Sequence++;
// 		packet = new EvaFTAgentTransfer(QQ_FILE_AGENT_TRANSFER_DATA);
// 		bytesToSend = ((bufferLength - bytesSent)>EVA_FILE_BUFFER_UNIT)?EVA_FILE_BUFFER_UNIT:(bufferLength - bytesSent);
// 		packet->setData(buf + bytesSent, bytesToSend);
// 		send(packet);
// 		if(!(i%10)) notifyTransferStatus();
// 		msleep(10);
// 		bytesSent += bytesToSend;
// 		if(bytesSent >= bufferLength) break;
// 		if(m_ExitNow) break;
// 	}
// 	m_BytesSent += bufferLength;
// 	delete []buf;
// 	m_State = ENone;
	slotWriteReady();
}

void EvaAgentUploader::slotWriteReady()
{
	//m_Connecter->setWriteNotifierEnabled(false);
	while (m_Connecter && m_State==ETransfering) {
		/*if(m_State != ETransfering){
		return;
		}*/
		util_log(0,"EvaAgentUploader::slotWriteReady\n");
		//unsigned int bytesSent = 0, bytesToSend = 0;
		//unsigned int bufferLength = 50 * EVA_FILE_BUFFER_UNIT;
		//unsigned char *buf = new unsigned char [bufferLength];
		//bufferLength = m_File->getFragment(m_BytesSent, bufferLength, buf);

		m_Sequence++;
		EvaFTAgentTransfer *packet = new EvaFTAgentTransfer(QQ_FILE_AGENT_TRANSFER_DATA);
		unsigned int bytesToSend = ((m_OutBufferLength - m_OutBytesSent)>EVA_FILE_BUFFER_UNIT)?EVA_FILE_BUFFER_UNIT:(m_OutBufferLength - m_OutBytesSent);
		packet->setData(m_OutBuffer + m_OutBytesSent, bytesToSend);
		try {
			send(packet);
		} catch (...) {
			m_State=EError;
			delete packet;
			break;
		}
		m_OutBytesSent += bytesToSend;
		m_BytesSent += bytesToSend;
		m_NumPackets++;
		/*if(!(m_NumPackets%10))*/ notifyTransferStatus();
		if(m_NumPackets == 50 || m_OutBytesSent >= m_OutBufferLength ){
			util_log(0,"EvaAgentUploader: Sending completed");
			m_State = ENone;
			return;
		}
	}

	m_State=EError;
	//m_Connecter->setWriteNotifierEnabled(true);
}


void EvaAgentUploader::doFinishProcessing()
{	
	notifyTransferStatus();
	notifyNormalStatus(ESSendFinished);
	delete m_FileList.front();

	m_FileList.pop_front();
	//if(m_FileList.pop_back()){
		if(!m_FileList.size()){
			m_ExitNow = true;
			return;
		} 
		m_File = m_FileList.front();
		if(!m_File)
			m_ExitNow = true;
		m_Dir = m_File->getDir();
		m_FileName = m_File->getFileName();
		m_Sequence++;
		doSendInfo();
	//} else 
	//	m_ExitNow = true;
	
}

void EvaAgentUploader::doErrorProcessing()
{
	if(m_Connecter){
		m_Connecter->DisableCallbacks();
		m_Connecter->Disconnect();
		//delete m_Connecter;
		m_Connecter = NULL;
	}
	notifyNormalStatus(ESError);
	m_ExitNow = true;
}

void EvaAgentUploader::processAgentPacket( unsigned char * data, int len )
{
	unsigned short cmd = EvaUtil::read16(data + 5);
	switch(cmd){
	case QQ_FILE_AGENT_CMD_CREATE:
		processCreateReply(new EvaFTAgentCreateReply(data, len));
		break;
	case QQ_FILE_AGENT_CMD_TRANSFER:
		if(!m_IsSendingStart){
			processTransferStart(new EvaFTAgentTransferReply(QQ_FILE_AGENT_TRANSFER_START, data, len));
		}else{
			processTransferReply(new EvaFTAgentTransferReply(QQ_FILE_AGENT_TRANSFER_REPLY, data, len));
		}
		break;
	case QQ_FILE_AGENT_CMD_READY:
		processNotifyReady(new EvaFTAgentAskReady(data, len));
		break;
	case QQ_FILE_AGENT_CMD_START:
		processStartReply(new EvaFTAgentStartReply(data, len));
		break;
	}
}

void EvaAgentUploader::processCreateReply(EvaFTAgentCreateReply *packet)
{
	util_log(0,"EvaAgentUploader::processCreateReply\n");
	packet->setFileAgentKey(m_FileAgentKey);
	if(!packet->parse()){
		m_State = EError;
		delete packet;
		return;
	}
	switch(packet->getReplyCode()){
	case QQ_FILE_AGENT_CREATE_OK:{
		m_AgentSession = packet->getSessionId();
		m_ServerPort = packet->getPort();

		EvaFileNotifySessionEvent *event = new EvaFileNotifySessionEvent();
		event->setBuddyQQ(m_Id);
		event->setOldSession(m_Session);
		event->setNewSession(m_AgentSession);
		//QApplication::postEvent(m_Receiver, event);
		customEvent(event);
		m_State = ECreatingReady;
		}
		break;
	case QQ_FILE_AGENT_CREATE_REDIRECT:
		m_HostAddresses.clear();
		m_HostAddresses.push_back(ntohl(packet->getIp()));
		m_ServerPort = packet->getPort();
		m_State = EDnsReady;
		break;
	case QQ_FILE_AGENT_CREATE_ERROR:
		util_log(0,"EvaAgentUploader::processCreateReply -- :%s\n", packet->getMessage().c_str());
	default:
		m_State = EError;
	}
	delete packet;
}

void EvaAgentUploader::processNotifyReady(EvaFTAgentAskReady *packet) 
{
	packet->setFileAgentKey(m_FileAgentKey);
	if(!packet->parse()){
		m_State = EError;
		delete packet;
		return;
	}
	if(packet->isAskReady()){
		m_Sequence++;
		m_State = ENotifyReady;
	} else {
		m_State = EError;
	}
	delete packet;
}

void EvaAgentUploader::processStartReply(EvaFTAgentStartReply *packet)
{
	packet->setFileAgentKey(m_FileAgentKey);
	if(!packet->parse()){
		m_State = EError;
		delete packet;
		return;
	}
	m_State = ENone;
	delete packet;
}

void EvaAgentUploader::processTransferStart(EvaFTAgentTransferReply *packet)
{
	if(!packet->parse()){
		m_State = EError;
		delete packet;
		return;
	}
	m_BytesSent = packet->getStartPosition();
	m_State = ETransfer;
	m_IsSendingStart = true;
	m_StartTime = time(NULL);
	notifyTransferStatus();
	delete packet;
}

void EvaAgentUploader::processTransferReply(EvaFTAgentTransferReply *packet)
{
	if(!packet->parse()){
		m_State = EError;
		delete packet;
		return;
	}

	if(packet->isReceivedOk()){
		notifyTransferStatus();
		if(m_BytesSent >= m_File->getFileSize())
			m_State = EFinished;
		else
			m_State = ETransfer;
		//printf("EvaAgentUploader::processTransferReply ---- m_State: %d\n", m_State);
	} else {
		util_log(0,"EvaAgentUploader::processTransferReply ---- isReceivedOk -- false \n");
		m_State = EError;
	}
	delete packet;
}



/** ================================================================== */

EvaAgentDownloader::EvaAgentDownloader(void *receiver, const int id, const list<string> &dirList,
			const list<string> &filenameList, 
			list<unsigned int> sizeList)
	: EvaAgentThread(receiver, id, dirList, filenameList, sizeList, false),
	m_IsRecovery(false), m_MaxBufferSize(EVA_FILE_BUFFER_UNIT), 
	m_BufferSize(0), m_IsSendingStart(false)
{
	setThreadType(2);
	m_Sequence = 0x0008;// give it a random number anyway
	m_MaxBufferSize = 50 * EVA_FILE_BUFFER_UNIT; // default is 100K
}

EvaAgentDownloader::~EvaAgentDownloader()
{
	m_ItemBuffer.clear();
}

void EvaAgentDownloader::__run() {
	m_State = EDnsReady;
	while(!m_ExitNow){
		switch(m_State){
		case ENone:
			break;
		case EDnsReady:
			util_log(0,"EDnsReady");
			doCreateConnection();
			break;
		case ENetworkReady:
			util_log(0,"ENetworkReady");
			doLoginRequest();
			break;
		case ENotifyReady:
			util_log(0,"ENotifyReady");
			doReadyReply();
			break;
		case EInfoReady:
			util_log(0,"EInfoReady");
			doStartRequest(); // send info reply as well
			break;
		case EDataReply:
			util_log(0,"EDataReply");
			doDataReply();
			break;
		case EFinished:
			util_log(0,"EFinished");
			doFinishProcessing();
			break;
		case EError:
			util_log(0,"EError");
			doErrorProcessing();
			break;
		default:
			break;
		}
		Sleep( 200 );
	}
}

DWORD WINAPI EvaAgentDownloader::_run(void* param) {
	((EvaAgentDownloader*)param)->__run();
	return 0;
}

void EvaAgentDownloader::run()
{
	util_log(0,"EvaAgentDownloader::run()");
	DWORD dwThreadID;
	CreateThread(NULL,0,_run,this,NULL,&dwThreadID);
}

void EvaAgentDownloader::doLoginRequest()
{
	EvaFTAgentLogin *packet = new EvaFTAgentLogin();
	packet->setFileAgentToken(m_Token, m_TokenLength);
	send(packet);
	m_State = ENone;
}

void EvaAgentDownloader::doReadyReply( )
{
	send(new EvaFTAgentAckReady());
	m_State = ENone;
}

void EvaAgentDownloader::doStartRequest()
{
	send(new EvaFTAgentStart());
	EvaFTAgentTransfer *packet = new EvaFTAgentTransfer(QQ_FILE_AGENT_TRANSFER_START);
	packet->setOffset(m_StartOffset);
	send(packet);
	m_State = ENone;
}

void EvaAgentDownloader::doDataReply()
{
	m_Sequence++;
	send(new EvaFTAgentTransfer(QQ_FILE_AGENT_TRANSFER_REPLY));
	m_State = ENone;
}

void EvaAgentDownloader::doFinishProcessing()
{
	notifyTransferStatus();
	notifyNormalStatus(ESReceiveFinished);

// 	m_File = m_FileList.next();
// 	if(m_FileList.remove()){
// 		m_File = m_FileList.first();
// 		if(!m_File)
// 			m_ExitNow = true;
// 		m_Dir = m_File->getDir();
// 		m_FileName = m_File->getFileName();
// 		m_State = EInfoReady;
// 	} else 
// 		m_ExitNow = true;
	m_IsRecovery = false;
	m_StartOffset = 0;
	m_BufferSize = 0;
	m_BytesSent = 0;
	m_FileSize = 0;
	m_IsSendingStart = false;
	m_ItemBuffer.clear();
	m_StartTime = time(NULL);
	//notifyTransferStatus();
	m_State = ENone;
}

void EvaAgentDownloader::doErrorProcessing()
{
	if(m_Connecter){
		m_Connecter->DisableCallbacks();
		m_Connecter->Disconnect();
		//delete m_Connecter;
		m_Connecter=NULL;
	}
	notifyNormalStatus(ESError);
	m_ExitNow = true;
}


void EvaAgentDownloader::processAgentPacket( unsigned char * data, int len )
{
	unsigned short cmd = EvaUtil::read16(data + 5);

	switch(cmd){
	case QQ_FILE_AGENT_CMD_LOGIN:
		processLoginReply(new EvaFTAgentLoginReply(data, len));
		break;
	case QQ_FILE_AGENT_CMD_TRANSFER:{
		//unsigned short seq = EvaUtil::read16(data + 7);
		if(!m_IsSendingStart){
			processTransferInfo(new EvaFTAgentTransferReply(QQ_FILE_AGENT_TRANSFER_INFO, data, len));
		}else{
			processTransferData(new EvaFTAgentTransferReply(QQ_FILE_AGENT_TRANSFER_DATA, data, len));
		}
		}
		break;
	case QQ_FILE_AGENT_CMD_READY:
		processNotifyReady(new EvaFTAgentAskReady(data, len));
		break;
	case QQ_FILE_AGENT_CMD_START:
		processStartReply(new EvaFTAgentStartReply(data, len));
		break;
	}
}

void EvaAgentDownloader::processLoginReply( EvaFTAgentLoginReply  *packet)
{
	if(!parsePacket(packet)){
		m_State = EError;
		delete packet;
		return;
	}
	if(packet->isConnected()) m_State = ENone;
	else  m_State = EError;
	m_Sequence++;
	delete packet;
}

void EvaAgentDownloader::processNotifyReady( EvaFTAgentAskReady * packet )
{
	if(!parsePacket(packet)){
		m_State = EError;
		delete packet;
		return;
	}
	if(packet->isAskReady()){
		m_State = ENotifyReady;
	} else {
		m_State = EError;
	}
	delete packet;
}

void EvaAgentDownloader::processStartReply( EvaFTAgentStartReply * packet )
{
	if(!parsePacket(packet)){
		m_State = EError;
		delete packet;
		return;
	}
	m_State = ENone;
	delete packet;
}

void EvaAgentDownloader::processTransferInfo( EvaFTAgentTransferReply * packet )
{
	if(!parsePacket(packet)){
		m_State = EError;
		delete packet;
		return;
	}
	m_StartSequence = packet->getSequence();
	m_IsSendingStart = true;
	m_File->setCheckValues( packet->getFileNameMd5(), packet->getFileMd5());
	//QTextCodec *codec = QTextCodec::codecForName("GB18030");
	m_FileName = packet->getFileName().c_str();
	m_FileSize = packet->getFileSize();
	printf("EvaAgentDownloader:: -------------------- got info - file: %s, size: %d\n", 
				packet->getFileName().c_str(), m_FileSize);
	
	m_StartTime = time(NULL);
	if(!(m_File->setFileInfo(m_FileName, m_FileSize))){
		m_State = EError;
		delete packet;
		return;
	};
	if(m_File->loadInfoFile() && m_TransferType == QQ_TRANSFER_FILE){
		notifyNormalStatus(ESResume);
		m_State = ENone;
		delete packet;
		return;
	}
	notifyTransferStatus();
	m_State = EInfoReady;
	delete packet;
}

void EvaAgentDownloader::askResumeLastDownload( const bool rec)
{
	m_IsRecovery = rec;
	if(m_IsRecovery) m_StartOffset = m_File->getNextOffset();
	m_State = EInfoReady;
}

void EvaAgentDownloader::processTransferData( EvaFTAgentTransferReply * packet )
{
	if(!parsePacket(packet)){
		m_State = EError;
		delete packet;
		return;
	}
	processDataBuffer(packet->getSequence(), packet->getData(), packet->getDataLength());
	delete packet;
	// we don't need set m_State here
}

void EvaAgentDownloader::setBufferSize( const unsigned int size )
{
	if(size > EVA_FILE_BUFFER_MAX_FACTOR ) return;
	m_MaxBufferSize = size * 50 * EVA_FILE_BUFFER_UNIT;
}

void EvaAgentDownloader::processDataBuffer( const unsigned short seq, const unsigned char * data, 
					const unsigned int len )
{
	if(len > EVA_FILE_BUFFER_UNIT) {
		m_State = EError;
		return;
	}
	FileItem item;
	item.no = seq;
	item.len = len;
	m_BufferSize += len;
	m_BytesSent += len;
	memcpy(item.data, data, len);
	m_ItemBuffer[seq] = item;
	checkBuffer(seq);
}

void EvaAgentDownloader::checkBuffer(const unsigned short seq)
{
	if(m_BufferSize > m_MaxBufferSize || (m_BytesSent + m_StartOffset) >= m_File->getFileSize()){
		unsigned int offset = (m_ItemBuffer.begin()->second.no - m_StartSequence - 1) * EVA_FILE_BUFFER_UNIT;
		unsigned char *tmp = new unsigned char[m_BufferSize];
		unsigned int cur = 0;
		std::map<unsigned int, FileItem>::iterator iter;
		for(iter=m_ItemBuffer.begin(); iter!=m_ItemBuffer.end(); ++iter){
			memcpy(tmp+cur, iter->second.data, iter->second.len);
			cur += iter->second.len;
		}
		m_File->saveFragment(m_StartOffset + offset, m_BufferSize, tmp);
		if( m_File->isFinished()){
			if(!m_File->generateDestFile()){
				m_State = EError;
				
			}else{
				doDataReply();
				m_State = EFinished;
			}
			delete []tmp;
			m_ItemBuffer.clear();
			m_BufferSize = 0;
			return;	
		}
		delete []tmp;
		m_ItemBuffer.clear();
		m_BufferSize = 0;
		//m_State = EDataReply;
	}
	/*if( !((seq - 1) %10)) */notifyTransferStatus();
	if( !( (seq - 1) % 50 ) ){ // every 50 packets, we send a ack back to server
		m_State = EDataReply;
	}else
		m_State = ENone;
}

const bool EvaAgentDownloader::parsePacket(EvaFTAgentPacket *packet)
{
	packet->setFileAgentKey(m_FileAgentKey);
	return packet->parse();
}


/** ==================================================================== */



EvaUDPThread::EvaUDPThread(void *receiver, const int id,const list<string> &dirList,
			const list<string> &filenameList, 
			list<unsigned int> sizeList, const bool isSender)
	: EvaFileThread(receiver, id, dirList, filenameList, sizeList, isSender),
	m_State(ENone), m_Token(NULL), m_TokenLength(0),  m_ServerPort(SYN_SERVER_PORT)
{
}

EvaUDPThread::~ EvaUDPThread()
{
	if(m_Token) delete []m_Token;
}

void EvaUDPThread::setFileAgentToken(const unsigned char *token, const int len)
{
	if(!token) return;
	if(m_Token) delete [] m_Token;
	m_Token = new unsigned char[len];
	memcpy(m_Token, token, len);
	m_TokenLength = len;
}

void EvaUDPThread::setFileAgentKey(const unsigned char *key)
{
	memcpy(m_FileAgentKey, key, 16);
}


void EvaUDPThread::setServerAddress(const unsigned int ip, const unsigned short port)
{
	m_HostAddresses.clear();
	m_HostAddresses.push_back(ip);
	m_ServerPort = port;
}

void EvaUDPThread::doCreateConnection()
{
	if(m_Connecter){
		/*m_Connecter->close();
		delete m_Connecter;*/
		m_Connecter->DisableCallbacks();
		m_Connecter->Disconnect();
		m_Connecter = NULL;
	}
	//m_Connecter = new EvaNetwork(m_HostAddresses.first(), m_ServerPort, EvaNetwork::UDP);
	/*
	in_addr ina;
	ina.S_un.S_addr=m_HostAddresses.front();
	*/
	unsigned int ip=m_HostAddresses.front();
	m_Connecter=new QQConnection2(hNetlibUser,QQConnection2::CONNTYPE_UDP,inet_ntoa(/*ina*/*(in_addr*)&ip),m_ServerPort,5000,this,false);
	m_Connecter->SetThreadName("EvaUDPThread");
	//m_Connecter->SetDump(true);

	/*
	QObject::connect(m_Connecter, SIGNAL(isReady()), SLOT(slotNetworkReady()));
	QObject::connect(m_Connecter, SIGNAL(dataComming(int)), SLOT(slotDataComming(int)));
	QObject::connect(m_Connecter, SIGNAL(exceptionEvent(int)), SLOT(slotNetworkException(int)));
	*/
	
	m_State = ENone;
	//m_Connecter->connect();
	m_Connecter->Connect();
}

void EvaUDPThread::sendSynPacket(EvaFTSynPacket *packet)
{
	if(! m_Connecter ){
		util_log(0, "EvaUDPThread::send -- Network invalid!\n");
		delete packet;
		m_State = EError;
		return;
	}
	
	// set the header infomation & key
	packet->setFileAgentKey(m_FileAgentKey);
	packet->setQQ(m_MyId);
	packet->setVersion(QQ_VERSION);
	packet->setSequence(m_Sequence);
	packet->setSessionId(m_Session);

	unsigned char *buffer = new unsigned char[4096];
	int len = 0;
	packet->fill(buffer, &len);
	/*if(!m_Connecter->write((char *)buffer, len)){
		delete []buffer;
		delete packet;
		m_State = EError;
		return;
	}*/
	m_Connecter->SendData((char*)buffer,len);
	delete []buffer;
	delete packet;
}

void EvaUDPThread::processSynPacket( unsigned char * /*data*/, int /*len*/ )
{
	util_log(0, "EvaUDPThread::processSynPacket -- Not Implemented, Error!\n");
	m_State = EError;
}

void EvaUDPThread::ConnectionErrorCallback(int code, void* data) {
	util_log(0, "EvaUDPThread::slotNetworkException -- no: %d\n", code);
	if(m_State != EFinished) m_State = EError;
}

void EvaUDPThread::ConnectionDataReceiveCallback(const char* data, const int len) {
	/*
	char *rawData = new char[len+1];
	if(!m_Connecter->read(rawData, len)){
	delete []rawData;
	return;
	}
	processSynPacket((unsigned char *)rawData, len);
	delete []rawData;
	*/
	processSynPacket((unsigned char *)data, len);
}

void EvaUDPThread::ConnectionSelectTimeoutCallback() {

}

void EvaUDPThread::ConnectionCloseCallback() {
	m_Connecter=NULL;
	if(m_State != EFinished) m_State = EError;
}

void EvaUDPThread::ConnectionReadyCallback() {
	m_State = ENetworkReady;
}

/** ==================================================================== */



EvaUdpUploader::EvaUdpUploader(void *receiver, const int id, const list<string> &dirList,
			const list<string> &filenameList)
	: EvaUDPThread(receiver, id, dirList, filenameList, list<unsigned int>(), true)
{
	setThreadType(3);
	m_Sequence = 0x0000;// give it a random number anyway
}

EvaUdpUploader::~EvaUdpUploader()
{
	util_log(0,"EvaUdpUploader Destruction");
}

void EvaUdpUploader::__run() {
	m_State = EDnsQuery;
	while(!m_ExitNow){
		switch(m_State){
		case ENone:
			break;
		case EDnsQuery:
			doDnsRequest();
			break;
		case EDnsReady:
			doCreateConnection();
			break;
		case ENetworkReady:
			doCreateRequest();
			break;
		case ECreatingReady:
			doFinishProcessing();
			break;
		case EError:
			doErrorProcessing();
			break;
		default:
			break;
		}
		Sleep( 200 );
	}
}

DWORD WINAPI EvaUdpUploader::_run(void* param) {
	((EvaUdpUploader*)param)->__run();
	return 0;
}

void EvaUdpUploader::run()
{
	DWORD dwThreadID;
	CreateThread(NULL,0,_run,this,NULL,&dwThreadID);
}

void EvaUdpUploader::doErrorProcessing()
{
	util_log(0,"EvaUdpUploader::doErrorProcessing\n");
	if(m_Connecter){
		/*m_Connecter->close();
		delete m_Connecter;
		m_Connecter = NULL;*/
		m_Connecter->DisableCallbacks();
		m_Connecter->Disconnect();
		m_Connecter = NULL;
	}
	notifyNormalStatus(ESError);
	m_ExitNow = true;
}

void EvaUdpUploader::doDnsRequest()
{
	m_HostAddresses.clear();
	slotDnsReady();
	/*if(m_Dns) delete m_Dns;
	m_Dns = new QDns(SYN_SERVER_URL);
	QObject::connect(m_Dns, SIGNAL(resultsReady()), SLOT(slotDnsReady()));*/
	
// 	while(!m_HostAddresses.size()){
// 		if(m_ExitNow) break;
// 		sleep(1);
// 	}
	//m_State = ENone;
}

void EvaUdpUploader::slotDnsReady()
{
	/*
	m_HostAddresses = m_Dns->addresses();
	if(!m_HostAddresses.size()){
		QHostAddress host;
		host.setAddress(SYN_SERVER_DEFAULT_IP);
		m_HostAddresses.append(host);
	}*/
	if (!m_HostAddresses.size())
		m_HostAddresses.push_back(inet_addr(SYN_SERVER_DEFAULT_IP));
	m_State = EDnsReady;
}

void EvaUdpUploader::doCreateRequest()
{
	if(!m_Token){
		m_State = EError;
		return;
	}
	m_Sequence++;
	EvaFTSynCreate *packet = new EvaFTSynCreate();

	packet->setBuddyQQ(m_Id);
	packet->setFileAgentToken(m_Token, m_TokenLength);
	sendSynPacket(packet);
	m_State = ENone; // waiting the response from server
}

void EvaUdpUploader::doNotifyBuddy(const unsigned int session, const unsigned int ip, const unsigned short port)
{
	EvaFileNotifyAddressEvent *event = new EvaFileNotifyAddressEvent();
	event->setSession(m_Session);
	event->setSynSession(session);
	event->setIp(m_HostAddresses.front());
	event->setPort(m_ServerPort);
	event->setMyIp(ip);
	event->setMyPort(port);
	event->setBuddyQQ(m_Id);
	//QApplication::postEvent(m_Receiver, event);
	customEvent(event);
}


void EvaUdpUploader::processSynPacket( unsigned char * data, int len )
{
	unsigned short cmd = EvaUtil::read16(data + 11);

	switch(cmd){
	case QQ_FILE_SYN_CMD_CREATE:
		processCreateReply(new EvaFTSynCreateReply(data, len));
		break;
	//case QQ_FILE_SYN_CMD_REGISTER:
	//	processRegisterReply(new EvaFTAgentAskReady(data, len));
	//	break;
	default:
		break;
	}
}

void EvaUdpUploader::processCreateReply(EvaFTSynCreateReply *packet)
{
	if(!parsePacket(packet)) return;

	if(packet->isSuccessful()){
		doNotifyBuddy(packet->getSessionId(), packet->getIp(), packet->getPort());
		m_State = ECreatingReady;
	} else
		m_State = EError;
	delete packet;
}

const bool EvaUdpUploader::parsePacket( EvaFTSynPacket * packet )
{
	packet->setFileAgentKey(m_FileAgentKey);
	bool result = packet->parse();
	if(!result) { 
		m_State = EError;
		delete packet;
	}
	return result;
}

void EvaUdpUploader::doFinishProcessing()
{
	m_ExitNow = true;
}


