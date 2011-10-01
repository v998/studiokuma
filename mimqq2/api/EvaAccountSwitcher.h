#ifndef EVAACCOUNTSWITCHER_H
#define EVAACCOUNTSWITCHER_H
typedef struct {
public:
	// evalogin.h
	int onlineUsers;

	//evalogintoken.h
	unsigned int m_FromIP;
	unsigned char m_Step;
	
	// evapacket.h
	unsigned int qqNum;
	bool mIsUDP;
	unsigned char *sessionKey;
	unsigned char *passwordKey;
	unsigned char *fileSessionKey;  
	unsigned char *loginToken;
	int loginTokenLength;

	unsigned char *fileAgentKey;
	unsigned char *fileAgentToken;
	int fileAgentTokenLength;

	unsigned char *clientKey;
	int clientKeyLength;

	unsigned char *fileShareToken; // note: always 24 bytes long

	short startSequenceNum;

	// evaqun.h
	short messageID;

	// evapicpacket.h
	unsigned char *qp_fileAgentKey;
	unsigned int myQQ;

	short sequenceStart;

	// evauhpacket.h
	unsigned short seq_random;
	unsigned short seq_info;
	unsigned short seq_transfer;
} EvaAccountStatics_t;

class CEvaAccountSwitcher {
public:
	static void _ProcessAs(unsigned int qqid);
	static void _EndProcess();
	static void _FreeAccout();
	static void _Finalize();
	static void Initialize();
	static void _NullFunc(unsigned int);
	static void _NullFunc();

	static unsigned int m_currentaccount;
	static EvaAccountStatics_t* m_currenteast;
private:
	static map<unsigned int,EvaAccountStatics_t*> m_accounts;
	static CRITICAL_SECTION m_cs;

	static volatile unsigned int m_currentQQ;
	static volatile int m_lockcount;

public:
	static void (*ProcessAs)(unsigned int qqid);
	static void (*EndProcess)();
	static void (*FreeAccount)();
	static void (*Finalize)();
};

#endif // EVAACCOUNTSWITCHER_H
