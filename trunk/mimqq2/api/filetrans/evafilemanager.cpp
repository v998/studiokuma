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
#include "../api/qqapi.h"
#include <libeva.h>
#include "../utils.h"

#include "evafilemanager.h"
#include "evafiledownloader.h"
*/
#include "stdafx.h"

EvaFileManager* EvaFileManager::m_inst=NULL;

EvaFileManager::EvaFileManager(const int myId, void *parent)
	: m_MyId(myId), m_FileAgentToken(NULL)
{
	//m_ThreadList.setAutoDelete(true);
	m_inst=this;
}

EvaFileManager::~EvaFileManager()
{
	stopAll();
	list<EvaFileThread*>::iterator iter;
	for (iter=m_ThreadList.begin(); iter!=m_ThreadList.end(); iter++) {
		delete (*iter);
	}
	
	if(m_FileAgentToken) delete []m_FileAgentToken;
	m_inst=NULL;
}

void EvaFileManager::setMyBasicInfo(const unsigned char *key, const unsigned char *token, 
				const unsigned int tokenLen)
{
	if(!token) return;
	if(m_FileAgentToken) delete [] m_FileAgentToken;
	m_FileAgentToken = new unsigned char[tokenLen];
	memcpy(m_FileAgentToken, token, tokenLen);
	m_FileAgentTokenLength = tokenLen;

	memcpy(m_FileAgentKey, key, 16);
}

/*void EvaFileManager::setMyProxyInfo(const QHostAddress addr, const short port, const QCString &param)
{
	m_ProxyServer = addr;
	m_ProxyPort = port;
	m_ProxyAuthParam = param;
	m_IsProxySet = true;
}*/

const bool EvaFileManager::newSession(const int id, const unsigned int session, 
				const list<string> &dirList, 
				const list<string> &filenameList,
				const list<unsigned int> &sizeList,
				const bool isDownload, const unsigned char transferType)
{
	if(getThread(id, session)) return false;
	EvaFileThread *thread = NULL;
	if(isDownload){
		EvaUdpDownloader *dthread = new EvaUdpDownloader(this, id, dirList, filenameList, sizeList);
		dthread->setFileAgentToken(m_FileAgentToken, m_FileAgentTokenLength);
		dthread->setQQ(m_MyId);
		dthread->setSession(session);
		dthread->setTransferType(transferType);
		dthread->customEvent=customEvent;
		thread = dthread;
	} else {
		EvaUdpUploader *uthread = new EvaUdpUploader(this, id, dirList, filenameList);
		uthread->setFileAgentToken(m_FileAgentToken, m_FileAgentTokenLength);
		uthread->setFileAgentKey(m_FileAgentKey);
		uthread->setQQ(m_MyId);
		uthread->setSession(session);
		uthread->setTransferType(transferType);
		uthread->customEvent=customEvent;
		thread = uthread;
	}
	if(thread) m_ThreadList.push_back(thread);
	return true;
}

const bool EvaFileManager::changeToAgent(const int id, const unsigned int session)
{
	EvaFileThread *thread = getThread(id, session);
	if(!thread) return false;
	thread->stop();
	thread->wait();
	
	EvaAgentThread *newThread = NULL;
	EvaUdpUploader *upthread = (EvaUdpUploader *)(thread);
	//if(upthread){
	if (thread->getThreadType()==3) {
		EvaAgentUploader *uthread = new EvaAgentUploader(this, 
						upthread->getBuddyQQ(), 
						upthread->getDirList(), 
						upthread->getFileNameList());
		uthread->setFileAgentToken(m_FileAgentToken, m_FileAgentTokenLength);
		uthread->setFileAgentKey(m_FileAgentKey);
		uthread->setQQ(m_MyId);
		uthread->setSession(session);
		uthread->setTransferType(upthread->getTransferType());
		uthread->customEvent=customEvent;
		newThread = uthread;
	}else{
		EvaUdpDownloader *downthread = (EvaUdpDownloader *)(thread);
		//if(downthread){
		if (thread->getThreadType()==4) {
			EvaAgentDownloader *dthread = new EvaAgentDownloader(this, 
								downthread->getBuddyQQ(),
								downthread->getDirList(),
								downthread->getFileNameList(),
								downthread->getSizeList());
			dthread->setFileAgentToken(m_FileAgentToken, m_FileAgentTokenLength);
			dthread->setQQ(m_MyId);
			dthread->setSession(session);
			dthread->setTransferType(downthread->getTransferType());
			dthread->customEvent=customEvent;
			newThread = dthread;
		} else 
			return false;
	}
	delete thread;
	m_ThreadList.remove(thread);
	if(newThread){
		//if(m_IsProxySet) newThread->setProxySettings(m_ProxyServer, m_ProxyPort, m_ProxyAuthParam);
		m_ThreadList.push_back(newThread);
	}
	return true;
}
/*
const bool EvaFileManager::newSession(const int id, const QString &dir, 
				const QString &file, const unsigned int session, const unsigned int size,
				const bool isDirectConnection, const bool usingProxy, const bool isDownload)
{
	if(getThread(id, session)) return false;
	EvaFileThread *thread = NULL;
	if(isDownload){
		if(isDirectConnection){
		} else {
			EvaAgentDownloader *dthread = new EvaAgentDownloader(this, id, dir, file, size);
			dthread->setFileAgentToken(m_FileAgentToken, m_FileAgentTokenLength);
			dthread->setQQ(m_MyId);
			dthread->setSession(session);
			if(usingProxy){
				if(!m_IsProxySet) {
					delete dthread;
					return false;
				}
				dthread->setProxySettings(m_ProxyServer, m_ProxyPort, m_ProxyAuthParam);
			}
			thread = dthread;
		}
	} else {
		if(isDirectConnection){
			EvaUdpUploader *uthread = new EvaUdpUploader(this, id, dir, file);
			uthread->setFileAgentToken(m_FileAgentToken, m_FileAgentTokenLength);
			uthread->setFileAgentKey(m_FileAgentKey);
			uthread->setQQ(m_MyId);
			uthread->setSession(session);
			thread = uthread;
		} else {
			EvaAgentUploader *uthread = new EvaAgentUploader(this, id, dir, file);
			uthread->setFileAgentToken(m_FileAgentToken, m_FileAgentTokenLength);
			uthread->setFileAgentKey(m_FileAgentKey);
			uthread->setQQ(m_MyId);
			uthread->setSession(session);
			if(usingProxy){
				if(!m_IsProxySet) {
					delete uthread;
					return false;
				}
				uthread->setProxySettings(m_ProxyServer, m_ProxyPort, m_ProxyAuthParam);
			}
			thread = uthread;
		}
	}
	if(thread) m_ThreadList.append(thread);
	return true;
}*/

void EvaFileManager::updateIp(const int id, const unsigned int session, const unsigned int ip)
{
	EvaAgentUploader *thread = (EvaAgentUploader *)(getThread(id, session));
	if(/*thread*/getThread(id, session)->getThreadType()==1) thread->setBuddyIp(ip);
}

const bool EvaFileManager::startSession(const int id, const unsigned int session)
{
	EvaFileThread *thread = getThread(id, session);
	if(!thread){
		util_log(0,"EvaFileManager::startSession -- no session to run, return now.\n");
		return false;
	}
	if(thread->running()){
		util_log(0,"EvaFileManager::startSession -- session already running, return now.\n");
		return false;
	}
	thread->start();
	return true;
}

const string EvaFileManager::getFileName(const int id, const unsigned int session, const bool isAbs)
{
	EvaFileThread *thread = getThread(id, session);
	if(!thread) return "";
	string file = "";
	if(isAbs) file = thread->getDir() + "\\";
	file += thread->getFileName();
	return file;
}

const unsigned int EvaFileManager::getFileSize(const int id, const unsigned int session)
{
	EvaFileThread *thread = getThread(id, session);
	if(!thread) return 0;
	return thread->getFileSize();
}

const unsigned char EvaFileManager::getTransferType(const int id, const unsigned int session)
{
	EvaFileThread *thread = getThread(id, session);
	if(!thread) return 0;
	return thread->getTransferType();
}

/*
void EvaFileManager::newReceiveThread()
{
}*/

void EvaFileManager::stopThread(const int id, const unsigned int session)
{
	EvaFileThread * thread = getThread(id, session);
	if(!thread) return;
	thread->stop();
	thread->wait();
	m_ThreadList.remove(thread);
}

void EvaFileManager::stopAll()
{
	//EvaFileThread *thread;
	list<EvaFileThread*>::iterator iter;
	//for(thread = m_ThreadList.first(); thread; thread = m_ThreadList.next()){
	for (iter=m_ThreadList.begin(); iter!=m_ThreadList.end(); iter++) {
		(*iter)->stop();
		(*iter)->wait();
	}
	m_ThreadList.clear();
}

void EvaFileManager::customEvent(QCustomEvent *e)
{
	printf("EvaFileManager::customEvent \n");
	/*
	switch(e->type()){
	case Eva_FileNotifyAgentEvent:{
		EvaFileNotifyAgentEvent *ae = (EvaFileNotifyAgentEvent *)e;
		m_inst->notifyAgentRequest(ae->getBuddyQQ(), ae->getOldSession(), ae->getAgentSession(),
					ae->getAgentIp(), ae->getAgentPort(), ae->getTransferType());
		}
		break;
	case Eva_FileNotifyStatusEvent:{
		EvaFileNotifyStatusEvent *se = (EvaFileNotifyStatusEvent *)e;
		m_inst->notifyTransferStatus(se->getBuddyQQ(), se->getSession(),
				se->getFileSize(), se->getBytesSent(), se->getTimeElapsed());
		}
		break;
	case Eva_FileNotifySessionEvent:{
		EvaFileNotifySessionEvent *se = (EvaFileNotifySessionEvent *)e;
		m_inst->notifyTransferSessionChanged(se->getBuddyQQ(), se->getOldSession(), se->getNewSession());
		}
		break;
	case Eva_FileNotifyNormalEvent:{
		EvaFileNotifyNormalEvent *ne = (EvaFileNotifyNormalEvent *)e;
		m_inst->notifyTransferNormalInfo(ne->getBuddyQQ(), ne->getSession(), ne->getStatus(),
					ne->getDir(), ne->getFileName(), ne->getFileSize(), ne->getTransferType());
		}
		break;
	case Eva_FileNotifyAddressEvent:{
		printf("EvaFileManager::customEvent -- Eva_FileNotifyAddressEvent Got!");
		EvaFileNotifyAddressEvent *ae = (EvaFileNotifyAddressEvent *)e;
		m_inst->notifyAddressRequest(ae->getBuddyQQ(), ae->getSession(), ae->getSynSession(), 
			ae->getIp(), ae->getPort(), ae->getMyIp(), ae->getMyPort());
		}
		break;
	default:
		break;
	}
	*/
	m_inst->customEventRedirector(e);
}

EvaFileThread * EvaFileManager::getThread(const int id, const unsigned int session)
{
	/*EvaFileThread * thread;
	for(thread = m_ThreadList.first(); thread; thread = m_ThreadList.next()){
		if( (thread->getBuddyQQ() == id) && (thread->getSession() == session) ){
			return thread;
		}
	}*/
	list<EvaFileThread*>::iterator iter;
	for(iter=m_ThreadList.begin(); iter!=m_ThreadList.end(); iter++) {
		if (((*iter)->getBuddyQQ()==id)&&((*iter)->getSession()==session)) {
			return *iter;
		}
	}
	util_log(0,"EvaFileManager::getThread -- cannot find thread: %d\n", session);
	return NULL;	
}

void EvaFileManager::changeSessionTo(const int id, const unsigned int oldSession,
				const unsigned int newSession )
{
	EvaFileThread *thread = getThread(id, oldSession);
	if(!thread)	return;
	thread->setSession(newSession);
}

void EvaFileManager::setBuddyAgentKey(const int id, const unsigned int session, const unsigned char *key)
{
	EvaAgentThread *thread = (EvaAgentThread *)(getThread(id, session));
	if(thread) thread->setFileAgentKey(key);
}

void EvaFileManager::saveFileTo(const int id, const unsigned int session, const string dir)
{
	EvaFileThread *thread = getThread(id, session);
	if(!thread) return;
	if(thread->running()) return;
	thread->setDir(dir);
}

void EvaFileManager::setAgentServer(const int id, const unsigned int session, 
					const unsigned int ip, const unsigned short port)
{
	EvaAgentThread *thread = (EvaAgentThread *)(getThread(id, session));
	if(thread) thread->setServerAddress(ip, port);
}

void EvaFileManager::slotFileTransferResume( const int id, const unsigned int session, const bool isResume )
{
	EvaAgentDownloader *thread = (EvaAgentDownloader *)(getThread(id, session));
	if(thread) thread->askResumeLastDownload(isResume);
}

const bool EvaFileManager::isSender(const int id, const unsigned int session, bool *isExisted)
{
	EvaFileThread *thread = getThread(id, session);
	if(!thread){
		*isExisted = false;
		return false;
	}
	*isExisted = true;
	return thread->isSender();
}

