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

#ifndef EVAFILEMANAGER_H
#define EVAFILEMANAGER_H

class QCustomEvent;
class EvaFileThread;

class EvaFileManager {
public:
	EvaFileManager(const int myId, void *parent = 0);
	~EvaFileManager();
	// set before starting a new thread
	void setMyBasicInfo(const unsigned char *key, const unsigned char *token, const unsigned int tokenLen);
	//void setMyProxyInfo(const QHostAddress addr, const short port, const QCString &param);

	const bool newSession(const int id, const unsigned int session, 
				const list<string> &dirList, 
				const list<string> &filenameList,
				const list<unsigned int> &sizeList, const bool isDownload,
				const unsigned char transferType = QQ_TRANSFER_FILE);
	const bool changeToAgent(const int id, const unsigned int session);

// 	const bool newSession(const int id, const QString &dir, const QString &file, 
// 			const unsigned int session, const unsigned int size, 
// 			const bool isDirectConnection = true, const bool usingProxy = false, 
// 			const bool isDownload = false);

	void changeSessionTo(const int id, const unsigned int oldSession, const unsigned int newSession);
	void setBuddyAgentKey(const int id, const unsigned int session, const unsigned char *key);
	void setAgentServer(const int id, const unsigned int session, const unsigned int ip, const unsigned short port);
	void saveFileTo(const int id, const unsigned int session, const string dir);

	void updateIp(const int id, const unsigned int session, const unsigned int ip);
	const bool startSession(const int id, const unsigned int session);

	const string getFileName(const int id, const unsigned int session, const bool isAbs = false);
	const unsigned int getFileSize(const int id, const unsigned int session);
	const unsigned char getTransferType(const int id, const unsigned int session);

// 	void newSendThread(const int id, const unsigned int ip, const QString &srcDir, const QString &srcFilename,
// 			const bool isDirectConnection = true, const bool usingProxy = false);
//	void newReceiveThread();

	void stopThread(const int id, const unsigned int session);
	void stopAll();
	const bool isSender(const int id, const unsigned int session, bool *isExisted);

	/*
	// buddy qq, session id, file size, bytes sent, time elapsed
	void (*notifyTransferStatus)(const int, const unsigned int, const unsigned int, const unsigned int, const int );
	// buddy qq, agent session id, agent ip, agent port
	void (*notifyAgentRequest)(const int, const unsigned int, const unsigned int, const unsigned int, const unsigned short, const unsigned char);
	void (*notifyTransferSessionChanged)(const int, const unsigned int, const unsigned int);
	void (*notifyTransferNormalInfo)(const int, const unsigned int, EvaFileStatus, const string, 
					const string, const unsigned int, const unsigned char);
	void (*notifyAddressRequest)(const int, const unsigned int, const unsigned int, const unsigned int, const unsigned short, 
				 const unsigned int, const unsigned short);
	*/
	void (*customEventRedirector)(QCustomEvent* event);

	void slotFileTransferResume(const int id, const unsigned int session, const bool isResume);

	EvaFileThread *getThread(const int id, const unsigned int session);
protected:
	static void customEvent(QCustomEvent *e);
private:
	// session are the keys
	//QPtrList<EvaFileThread> m_SendList;
	//QPtrList<EvaFileThread> m_ReceiveList;
	//QMap<unsigned int, EvaFileThread *> m_SendList;
	list<EvaFileThread*> m_ThreadList;
	EvaFileThread *m_LastThread;

	int m_MyId;
	unsigned int m_LocalAddress;

	// my encryption settings
	unsigned char m_FileAgentKey[16];
	unsigned char *m_FileAgentToken;
	unsigned int m_FileAgentTokenLength;

	//EvaFileThread *getThread(const unsigned int session);

	static EvaFileManager* m_inst;
};

#endif // EVAFILEMANAGER

