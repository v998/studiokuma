#include "StdAfx.h"

map<int,EvaAccountStatics_t*> CEvaAccountSwitcher::m_accounts;
int CEvaAccountSwitcher::m_currentaccount=0;
EvaAccountStatics_t* CEvaAccountSwitcher::m_currenteast;
CRITICAL_SECTION CEvaAccountSwitcher::m_cs={0};
volatile int CEvaAccountSwitcher::m_currentQQ=0;
volatile int CEvaAccountSwitcher::m_lockcount=0;
void (*CEvaAccountSwitcher::ProcessAs)(int)=CEvaAccountSwitcher::_NullFunc;
void (*CEvaAccountSwitcher::EndProcess)()=CEvaAccountSwitcher::_NullFunc;
void (*CEvaAccountSwitcher::FreeAccount)()=CEvaAccountSwitcher::_NullFunc;
void (*CEvaAccountSwitcher::Finalize)()=CEvaAccountSwitcher::_NullFunc;
#define ENABLE_EAS

/*
void Packet::_EAS_SwitchAccount(bool save);
void KeepAliveReplyPacket::_EAS_SwitchAccount(bool save);
void ServerDetectorPacket::_EAS_SwitchAccount(bool save);
void OutPacket::_EAS_SwitchAccount(bool save);
void QunSendIMExPacket::_EAS_SwitchAccount(bool save);
void EvaPicPacket::_EAS_SwitchAccount(bool save);
void EvaPicOutPacket::_EAS_SwitchAccount(bool save);
void EvaUHPacket::_EAS_SwitchAccount(bool save);
*/

void CEvaAccountSwitcher::_NullFunc(){};
void CEvaAccountSwitcher::_NullFunc(int){};

void CEvaAccountSwitcher::Initialize() {
	// Call this function when switching is required
	util_log(0,"Initialize CEvaAccountSwitcher");
	ProcessAs=_ProcessAs;
	EndProcess=_EndProcess;
	FreeAccount=_FreeAccout;
	Finalize=_Finalize;
}

void CEvaAccountSwitcher::_ProcessAs(int qqid) {
#ifdef ENABLE_EAS
	if (m_cs.DebugInfo==NULL) {
		// Initialize CS
		InitializeCriticalSection(&m_cs);
	}

	if (m_currentQQ!=qqid) {
		util_log(0,"CEvaAccountSwitcher(%d): Acquiring mutex lock",qqid);
		EnterCriticalSection(&m_cs);
		util_log(0,"CEvaAccountSwitcher(%d): Lock acquired",qqid);
		m_currentQQ=qqid;
	} else
		m_lockcount++;

	if (qqid!=m_currentaccount) {
		EvaAccountStatics_t* east=m_accounts[qqid];

		if (m_currentaccount!=0) {
			// save
			Packet::_EAS_SwitchAccount(true);
			ServerDetectorPacket::_EAS_SwitchAccount(true);
			KeepAliveReplyPacket::_EAS_SwitchAccount(true);
			QunSendIMExPacket::_EAS_SwitchAccount(true);
			EvaPicPacket::_EAS_SwitchAccount(true);
			EvaPicOutPacket::_EAS_SwitchAccount(true);
			EvaUHPacket::_EAS_SwitchAccount(true);
			util_log(0,"CEvaAccountSwitcher(%d): Save %d, assert=%d",qqid,m_currentaccount,m_currenteast->myQQ);
		}

		if (!east) {
			// Create account
			east=(EvaAccountStatics_t*)mir_alloc(sizeof(EvaAccountStatics_t));
			ZeroMemory(east,sizeof(EvaAccountStatics_t));
			east->startSequenceNum=5;
			m_accounts[qqid]=east;
			util_log(0,"CEvaAccountSwitcher(%d): Created",qqid);
		}

		m_currentaccount=qqid;
		m_currenteast=east;

		// assign
		Packet::_EAS_SwitchAccount(false);
		ServerDetectorPacket::_EAS_SwitchAccount(false);
		KeepAliveReplyPacket::_EAS_SwitchAccount(false);
		QunSendIMExPacket::_EAS_SwitchAccount(false);
		EvaPicPacket::_EAS_SwitchAccount(false);
		EvaPicOutPacket::_EAS_SwitchAccount(false);
		EvaUHPacket::_EAS_SwitchAccount(false);
		util_log(0,"CEvaAccountSwitcher(%d): Switched Account, QQ=%d",qqid,m_currenteast->qqNum);
	}
#endif
}

void CEvaAccountSwitcher::_EndProcess() {
#ifdef ENABLE_EAS
	/*
	if (m_currentaccount) {
		if (m_currentaccount!=0) {
			// save
			Packet::_EAS_SwitchAccount(true);
			ServerDetectorPacket::_EAS_SwitchAccount(true);
			KeepAliveReplyPacket::_EAS_SwitchAccount(true);
			QunSendIMExPacket::_EAS_SwitchAccount(true);
			EvaPicPacket::_EAS_SwitchAccount(true);
			EvaPicOutPacket::_EAS_SwitchAccount(true);
			EvaUHPacket::_EAS_SwitchAccount(true);
			util_log(0,"CEvaAccountSwitcher(%d): Save, myqq=%d, assert=%d",m_currentaccount,Packet::getQQ(),m_currenteast->qqNum);
			//if (m_currenteast->qqNum==0) DebugBreak();
		}

		m_currentaccount=0;
		m_currenteast=NULL;
	}
	*/
	if (m_lockcount==0) {
		util_log(0,"CEvaAccountSwitcher(%d): Unlock mutex",m_currentaccount);
		LeaveCriticalSection(&m_cs);
		m_currentQQ=0;
	} else
		m_lockcount--;
#endif
}

void CEvaAccountSwitcher::_FreeAccout() {
	// Free*Keys() should be called before calling this function.
#ifdef ENABLE_EAS
	EnterCriticalSection(&m_cs);
	EvaAccountStatics_t* east=m_accounts[m_currentaccount];
	mir_free(east);
	m_accounts.erase(m_currentaccount);
	LeaveCriticalSection(&m_cs);
#endif
}

void CEvaAccountSwitcher::_Finalize() {
#ifdef ENABLE_EAS
	if (m_cs.DebugInfo!=NULL) {
		EnterCriticalSection(&m_cs);
		while (m_accounts.size()) {
			m_currentaccount=m_accounts.begin()->first;
			FreeAccount();
		}
		LeaveCriticalSection(&m_cs);
		DeleteCriticalSection(&m_cs);
	}
#endif
}

#ifdef ENABLE_EAS
void KeepAliveReplyPacket::_EAS_SwitchAccount(bool save) {
	if (save) {
		CEvaAccountSwitcher::m_currenteast->onlineUsers=onlineUsers;
	} else {
		onlineUsers=CEvaAccountSwitcher::m_currenteast->onlineUsers;
	}
}

void ServerDetectorPacket::_EAS_SwitchAccount(bool save) {
	if (save) {
		CEvaAccountSwitcher::m_currenteast->m_FromIP=m_FromIP;
		CEvaAccountSwitcher::m_currenteast->m_Step=m_Step;
	} else {
		m_FromIP=CEvaAccountSwitcher::m_currenteast->m_FromIP;
		m_Step=CEvaAccountSwitcher::m_currenteast->m_Step;
	}
}

void Packet::_EAS_SwitchAccount(bool save) {
	if (save) {
		CEvaAccountSwitcher::m_currenteast->qqNum=qqNum;
		CEvaAccountSwitcher::m_currenteast->mIsUDP=mIsUDP;
		CEvaAccountSwitcher::m_currenteast->sessionKey=sessionKey;
		CEvaAccountSwitcher::m_currenteast->passwordKey=passwordKey;
		CEvaAccountSwitcher::m_currenteast->fileSessionKey=fileSessionKey;
		CEvaAccountSwitcher::m_currenteast->loginToken=loginToken;
		CEvaAccountSwitcher::m_currenteast->loginTokenLength=loginTokenLength;
		CEvaAccountSwitcher::m_currenteast->fileAgentKey=fileAgentKey;
		CEvaAccountSwitcher::m_currenteast->fileAgentToken=fileAgentToken;
		CEvaAccountSwitcher::m_currenteast->fileAgentTokenLength=fileAgentTokenLength;
		CEvaAccountSwitcher::m_currenteast->clientKey=clientKey;
		CEvaAccountSwitcher::m_currenteast->clientKeyLength=clientKeyLength;
		CEvaAccountSwitcher::m_currenteast->fileShareToken=fileShareToken;
	} else {
		qqNum=CEvaAccountSwitcher::m_currenteast->qqNum;
		mIsUDP=CEvaAccountSwitcher::m_currenteast->mIsUDP;
		sessionKey=CEvaAccountSwitcher::m_currenteast->sessionKey;
		passwordKey=CEvaAccountSwitcher::m_currenteast->passwordKey;
		fileSessionKey=CEvaAccountSwitcher::m_currenteast->fileSessionKey;
		loginToken=CEvaAccountSwitcher::m_currenteast->loginToken;
		loginTokenLength=CEvaAccountSwitcher::m_currenteast->loginTokenLength;
		fileAgentKey=CEvaAccountSwitcher::m_currenteast->fileAgentKey;
		fileAgentToken=CEvaAccountSwitcher::m_currenteast->fileAgentToken;
		fileAgentTokenLength=CEvaAccountSwitcher::m_currenteast->fileAgentTokenLength;
		clientKey=CEvaAccountSwitcher::m_currenteast->clientKey;
		clientKeyLength=CEvaAccountSwitcher::m_currenteast->clientKeyLength;
		fileShareToken=CEvaAccountSwitcher::m_currenteast->fileShareToken;
	}
}

void OutPacket::_EAS_SwitchAccount(bool save) {
	if (save) {
		CEvaAccountSwitcher::m_currenteast->startSequenceNum=startSequenceNum;
	} else {
		startSequenceNum=CEvaAccountSwitcher::m_currenteast->startSequenceNum;
	}
}

void QunSendIMExPacket::_EAS_SwitchAccount(bool save) {
	if (save) {
		CEvaAccountSwitcher::m_currenteast->messageID=messageID;
	} else {
		messageID=CEvaAccountSwitcher::m_currenteast->messageID;
	}
}

void EvaPicPacket::_EAS_SwitchAccount(bool save) {
	if (save) {
		CEvaAccountSwitcher::m_currenteast->qp_fileAgentKey=fileAgentKey;
		CEvaAccountSwitcher::m_currenteast->myQQ=myQQ;
	} else {
		fileAgentKey=CEvaAccountSwitcher::m_currenteast->qp_fileAgentKey;
		myQQ=CEvaAccountSwitcher::m_currenteast->myQQ;
	}
}

void EvaPicOutPacket::_EAS_SwitchAccount(bool save) {
	if (save) {
		CEvaAccountSwitcher::m_currenteast->sequenceStart=sequenceStart;
	} else {
		sequenceStart=CEvaAccountSwitcher::m_currenteast->sequenceStart;
	}
}

void EvaUHPacket::_EAS_SwitchAccount(bool save) {
	if (save) {
		CEvaAccountSwitcher::m_currenteast->seq_info=seq_info;
		CEvaAccountSwitcher::m_currenteast->seq_random=seq_random;
		CEvaAccountSwitcher::m_currenteast->seq_transfer=seq_transfer;
	} else {
		seq_info=CEvaAccountSwitcher::m_currenteast->seq_info;
		seq_random=CEvaAccountSwitcher::m_currenteast->seq_random;
		seq_transfer=CEvaAccountSwitcher::m_currenteast->seq_transfer;
	}
}
#endif
