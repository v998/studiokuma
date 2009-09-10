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

#ifndef EVAFILEDOWNLOADER_H
#define EVAFILEDOWNLOADER_H

#include "../api/qqconnection2.h"
#include "../evaqtutil.h"
#include <map>

#define Eva_FileNotifyAgentEvent       65527
#define Eva_FileNotifyStatusEvent      65526
#define Eva_FileNotifySessionEvent     65525
#define Eva_FileNotifyRecoveryEvent    65524
#define Eva_FileNotifyFinishedEvent    65523
#define Eva_FileNotifyNormalEvent      65522
#define Eva_FileNotifyAddressEvent     65521

#define EVA_FILE_BUFFER_MAX_FACTOR     1000 
#define EVA_FILE_BUFFER_UNIT           0x800

enum EvaFileStatus { ESNone, ESError, ESResume, ESSendFinished, ESReceiveFinished };

class EvaFileNotifyAgentEvent : public QCustomEvent
{
public:
	EvaFileNotifyAgentEvent() : QCustomEvent(Eva_FileNotifyAgentEvent){}
	void setOldSession(const unsigned int s) { m_OldSession = s; }
	void setAgentSession(const unsigned int s) { m_Session = s; }
	void setAgentIp(const unsigned int ip) { m_Ip = ip; }
	void setAgentPort(const unsigned short port) { m_Port = port; }
	void setMyFileAgentKey(const unsigned char * key) { memcpy(m_Key, key, 16); }
	void setBuddyQQ(const int id) { m_Id = id; }
	void setTransferType(const unsigned char type) { m_TransferType = type; }
	
	const unsigned int getOldSession() const { return m_OldSession; }
	const unsigned int getAgentSession() const { return m_Session; }
	const unsigned int getAgentIp() const { return m_Ip; }
	const unsigned short getAgentPort() const { return m_Port; }
	const unsigned char * getMyFileAgentKey() const { return m_Key; }
	const int getBuddyQQ() const { return m_Id; }
	const unsigned char getTransferType() const { return m_TransferType; }
private:
	int m_Id;
	unsigned int m_OldSession;
	unsigned int m_Session;
	unsigned int m_Ip;
	unsigned short m_Port;
	unsigned char m_Key[16];
	unsigned char m_TransferType;
};

class EvaFileNotifyStatusEvent : public QCustomEvent
{
public:
	EvaFileNotifyStatusEvent() : QCustomEvent(Eva_FileNotifyStatusEvent){}
	void setSession(const unsigned int s) { m_Session = s; }
	void setFileSize(const unsigned int size) { m_FileSize = size; }
	void setBytesSent(const unsigned int bytes) { m_BytesSent = bytes; }
	void setTimeElapsed(const int sec) { m_TimeElapsed = sec; }
	void setBuddyQQ(const int id) { m_Id = id; }

	const unsigned int getSession() const { return m_Session; }
	const unsigned int getFileSize() const { return m_FileSize; }
	const unsigned int getBytesSent() const { return m_BytesSent; }
	const int getTimeElapsed() const { return m_TimeElapsed; }
	const int getBuddyQQ() const { return m_Id; }
private:
	int m_Id;
	unsigned int m_Session;
	unsigned int m_FileSize;
	unsigned int m_BytesSent;
	int m_TimeElapsed; // how many seconds
};

class EvaFileNotifySessionEvent : public QCustomEvent
{
public:
	EvaFileNotifySessionEvent() : QCustomEvent(Eva_FileNotifySessionEvent){}
	void setBuddyQQ(const int id) { m_Id = id; }
	void setOldSession(const unsigned int s) { m_OldSession = s; }
	void setNewSession(const unsigned int s) { m_NewSession = s; }

	const int getBuddyQQ() const { return m_Id; }
	const unsigned int getOldSession() const { return m_OldSession; }
	const unsigned int getNewSession() const { return m_NewSession; }
private:
	int m_Id;
	unsigned int m_OldSession;
	unsigned int m_NewSession;
};

class EvaFileNotifyNormalEvent : public QCustomEvent
{
public:
	EvaFileNotifyNormalEvent() : QCustomEvent(Eva_FileNotifyNormalEvent){}

	void setBuddyQQ(const int id) { m_Id = id; }
	void setSession(const unsigned int s) { m_Session = s; }
	void setStatus(EvaFileStatus status) { m_Status = status; }
	void setFileDir(const string &dir) { m_Dir = dir; }
	void setFileName(const string &file) { m_FileName = file; }
	void setFileSize(const unsigned int size) { m_Size = size; }
	void setTransferType(const unsigned char type) { m_TransferType = type; }

	const int getBuddyQQ() const { return m_Id; }
	const unsigned int getSession() const { return m_Session; }
	const EvaFileStatus getStatus() const { return m_Status; }
	const string getDir() const { return m_Dir; }
	const string getFileName() const { return m_FileName; }
	const unsigned int getFileSize() const { return m_Size; }
	const unsigned char getTransferType() const { return m_TransferType; }
	
private:
	int m_Id;
	unsigned int m_Session;
	EvaFileStatus m_Status;
	string m_Dir;
	string m_FileName;
	unsigned int m_Size;
	unsigned char m_TransferType;
};

class EvaFileNotifyAddressEvent : public QCustomEvent
{
public:
	EvaFileNotifyAddressEvent() : QCustomEvent(Eva_FileNotifyAddressEvent){}
	void setSession(const unsigned int s) { m_Session = s; }
	void setSynSession(const unsigned int s) { m_SynSession = s; }
	void setIp(const unsigned int ip) { m_Ip = ip; }
	void setPort(const unsigned short port) { m_Port = port; }
	void setMyIp(const unsigned int ip) { m_MyIp = ip; }
	void setMyPort(const unsigned short port) { m_MyPort = port; }
	void setBuddyQQ(const int id) { m_Id = id; }

	const unsigned int getSession() const { return m_Session; }
	const unsigned int getSynSession() const { return m_SynSession; }
	const unsigned int getIp() const { return m_Ip; }
	const unsigned short getPort() const { return m_Port; }
	const unsigned int getMyIp() const { return m_MyIp; }
	const unsigned short getMyPort() const { return m_MyPort; }

	const int getBuddyQQ() const { return m_Id; }
private:
	int m_Id;
	unsigned int m_Session;
	unsigned int m_SynSession;
	unsigned int m_Ip;
	unsigned short m_Port;
	unsigned int m_MyIp;
	unsigned short m_MyPort;
};


class EvaCachedFile;
//class QDns;


class EvaFileThread
{
public:
	// for sending
	EvaFileThread(void *receiver, const int id, const list<string> &dirList,
			const list<string> &filenameList,
			const list<unsigned int> sizeList, const bool isSender);

	~EvaFileThread();

	inline void setBuddyQQ(const int id) { m_Id = id; }
	inline void setQQ(const int id) { m_MyId = id; }
	inline void setSession(const unsigned int session) { m_Session = session; }
	inline void setFileName(const string &file) { m_FileName = file; }
	inline void setTransferType(const unsigned char type) { m_TransferType = type; }

	inline const int getBuddyQQ() const { return m_Id; }
	inline const int getQQ() const { return m_MyId; }
	inline const unsigned int getSession() const { return m_Session; }
	inline const string getFileName() const { return m_FileName; }
	inline const unsigned char getTransferType() const { return m_TransferType; }

	inline const bool isSender() const { return m_IsSender; }
	inline void stop() { m_ExitNow = true; 	m_running=false; }

	void setDir(const string &dir);
	const string getDir() const;
	const unsigned int getFileSize();

	const list<string> &getDirList() const { return m_DirList; }
	const list<string> &getFileNameList() const { return m_FileNameList; }
	const list<unsigned int> &getSizeList() const { return m_SizeList; }

	void (*customEvent)(QCustomEvent*);

	inline const void start(){ m_running=true; run();}
	inline const void wait(){ while(m_running) Sleep(500); if (m_Connecter) {m_Connecter->DisableCallbacks(); m_Connecter->Disconnect(); m_Connecter=NULL;} }
	inline const bool running() { return m_running; }

	inline const unsigned char getThreadType() { return m_threadType; }
	inline const void setThreadType(const unsigned char threadType) { m_threadType=threadType; }

protected:
	bool m_IsSender;
	void *m_Receiver;
	int m_Id, m_MyId;
	unsigned int m_Session;
	unsigned short m_Sequence;
	unsigned char m_TransferType;

	string m_Dir;
	string m_FileName;
	unsigned int m_StartOffset;
	int m_FileSize;
	bool m_ExitNow;

	unsigned int m_BytesSent;
	time_t m_StartTime;

	EvaCachedFile* m_File;
	list<EvaCachedFile*> m_FileList;
	list<string> m_DirList;
	list<string> m_FileNameList;
	list<unsigned int> m_SizeList;
	
	QQConnection2 *m_Connecter;
	void cleanUp();

	void notifyTransferStatus();
	void notifyNormalStatus(const EvaFileStatus status);

	unsigned char m_threadType;

private:
	bool m_running;

	virtual void run() {}
};


class EvaFTAgentPacket;
class EvaFTAgentCreateReply;
class EvaFTAgentLoginReply;
class EvaFTAgentAskReady;
class EvaFTAgentStartReply;
class EvaFTAgentTransferReply;

class EvaAgentThread : public EvaFileThread, QQConnection2Client
{
public:
	EvaAgentThread(void *receiver, const int id, const list<string> &dirList,
			const list<string> &filenameList,
			const list<unsigned int> sizeList, const bool isSender);
	virtual ~EvaAgentThread();

	// user must call following 3 methods before running the thread
	void setFileAgentToken(const unsigned char *token, const int len);
	void setFileAgentKey(const unsigned char *key);

	void setServerAddress(const unsigned int ip, const unsigned short port);

	void ConnectionErrorCallback(int code, void* data);
	void ConnectionDataReceiveCallback(const char* data, const int len);
	void ConnectionSelectTimeoutCallback();
	void ConnectionCloseCallback();
	void ConnectionReadyCallback();
protected:
	enum AgentState { ENone, EDnsQuery, EDnsReady, ENetworkReady, ECreatingReady,
			ENotifyBuddyReady, ENotifyReady, EInfoReady, EAskForStart, ETransfer, 
			ETransfering, EDataReply, EFinished, EError};
	AgentState m_State;
	unsigned char m_FileAgentKey[16];

	unsigned char *m_Token;
	int m_TokenLength;

	list<unsigned int> m_HostAddresses;
	unsigned short m_ServerPort;

	void doCreateConnection();
	void send(EvaFTAgentPacket *packet);
	virtual void processAgentPacket( unsigned char * data, int len );
private:
	// for receiving data	
	unsigned char m_Buffer[65535];
	unsigned int m_BufferLength;
	unsigned short m_PacketLength;

	/*
	void slotNetworkReady();
	void slotDataComming(int);
	void slotNetworkException(int);
	*/
};

class EvaAgentUploader : public EvaAgentThread
{
public:
	EvaAgentUploader(void *receiver, const int id,const list<string> &dirList,
			const list<string> &filenameList);
	~EvaAgentUploader();

	// user must call this method before running the thread
	void setBuddyIp(const unsigned int ip) { m_BuddyIp = ip; }

	void __run();
private:

	unsigned int m_BuddyIp;
	unsigned int m_AgentSession;
	bool m_IsSendingStart;
	unsigned char *m_OutBuffer;
	unsigned int m_OutBufferLength;
	unsigned int m_OutBytesSent;
	unsigned int m_NumPackets;
	void doDnsRequest();

	static DWORD WINAPI _run(void* param);
	void run();

	void doCreateRequest();
	void doNotifyBuddy();
	void doReadyReply();
	void doStartRequest();
	void doSendInfo();
	void doDataTransfering();
	void doFinishProcessing();
	void doErrorProcessing();

	void processAgentPacket( unsigned char * data, int len );

	void processCreateReply(EvaFTAgentCreateReply *packet);
	void processNotifyReady(EvaFTAgentAskReady *packet);
	void processStartReply(EvaFTAgentStartReply *packet);
	void processTransferStart(EvaFTAgentTransferReply *packet);
	void processTransferReply(EvaFTAgentTransferReply *packet);

	void slotDnsReady();
	void slotWriteReady();
};

typedef struct {
unsigned int no;
unsigned int len;
unsigned char data[EVA_FILE_BUFFER_UNIT];
} FileItem;

class EvaAgentDownloader : public EvaAgentThread
{
public:
	EvaAgentDownloader(void *receiver, const int id, const list<string> &dirList,
			const list<string> &filenameList,
			const list<unsigned int> sizeList);
	~EvaAgentDownloader();
	/** 
	* @param factor the times of 50 * EVA_FILE_BUFFER_UNIT, default buffer size is 1, which
	*                 is 50 * EVA_FILE_BUFFER_UNIT(0x800).
	*/
	
	void setBufferSize(const unsigned int factor);
	void askResumeLastDownload( const bool rec = false);

	void __run();
private:
	bool m_IsRecovery;
	unsigned int m_MaxBufferSize;
	unsigned int m_BufferSize;
	std::map<unsigned int, FileItem> m_ItemBuffer;
	bool m_IsSendingStart;
	unsigned short m_StartSequence;
	
	static DWORD WINAPI _run(void* param);
	void run();

	void processAgentPacket( unsigned char * data, int len );
	void doLoginRequest();
	void doReadyReply();
	void doStartRequest();
	void doDataReply();
	void doFinishProcessing();
	void doErrorProcessing();

	void processLoginReply( EvaFTAgentLoginReply  *packet);
	void processNotifyReady( EvaFTAgentAskReady *packet);
	void processStartReply(EvaFTAgentStartReply *packet);
	void processTransferInfo( EvaFTAgentTransferReply *packet);
	void processTransferData( EvaFTAgentTransferReply *packet);

	void processDataBuffer(const unsigned short seq, const unsigned char *data, const unsigned int len);
	void checkBuffer(const unsigned short seq);

	const bool parsePacket(EvaFTAgentPacket *packet);
};

class EvaFTSynPacket;
class EvaUDPThread : public EvaFileThread, QQConnection2Client
{
public:
	EvaUDPThread(void *receiver, const int id, const list<string> &dirList,
			const list<string> &filenameList,
			const list<unsigned int> sizeList, const bool isSender);
	virtual ~EvaUDPThread();

	// user must call following 3 methods before running the thread
	void setFileAgentToken(const unsigned char *token, const int len);
	void setFileAgentKey(const unsigned char *key);

	void setServerAddress(const unsigned int ip, const unsigned short port);

	void ConnectionErrorCallback(int code, void* data);
	void ConnectionDataReceiveCallback(const char* data, const int len);
	void ConnectionSelectTimeoutCallback();
	void ConnectionCloseCallback();
	void ConnectionReadyCallback();
protected:
	enum AgentState { ENone, EDnsQuery, EDnsReady, ENetworkReady, ECreatingReady, EFinished, EError};
	AgentState m_State;
	unsigned char m_FileAgentKey[16];

	unsigned char *m_Token;
	int m_TokenLength;

	list<unsigned int> m_HostAddresses;
	unsigned short m_ServerPort;

	void doCreateConnection();
	void sendSynPacket(EvaFTSynPacket *packet);
	virtual void processSynPacket( unsigned char * data, int len );

private:
	/*
	void slotNetworkReady();
	void slotDataComming(int);
	void slotNetworkException(int);
	*/
};

class EvaFTSynCreateReply;
class EvaUdpUploader : public EvaUDPThread
{
public:
	EvaUdpUploader(void *receiver, const int id, const list<string> &dirList,
			const list<string> &filenameList);
	~EvaUdpUploader();

	void __run();
private:
	void doDnsRequest();

	void run();
	static DWORD WINAPI _run(void* param);

	void doCreateRequest();
	void doNotifyBuddy(const unsigned int session, const unsigned int ip, const unsigned short port);
	void doFinishProcessing();
	void doErrorProcessing();

	void processSynPacket( unsigned char * data, int len );

	void processCreateReply(EvaFTSynCreateReply *packet);
	//void processRegisterReply(EvaFTAgentAskReady *packet);

	const bool parsePacket(EvaFTSynPacket *packet);

	void slotDnsReady();
};

class EvaUdpDownloader : public EvaUDPThread
{
public:
	EvaUdpDownloader(void *receiver, const int id, const list<string> &dirList, 
			const list<string> &filenameList, const list<unsigned int> sizeList)
		: EvaUDPThread(receiver, id, dirList, filenameList, sizeList, false) {setThreadType(4);};
	~EvaUdpDownloader() {};
private:
	void run() {};
};

#endif // EVAFILEDOWNLOADER_H


