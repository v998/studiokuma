#ifndef QUNIMAGE_H
#define QUNIMAGE_H

class EvaPicOutPacket;
class EvaPicInPacket;

typedef struct {
	unsigned int sessionid;
	unsigned int ip;
	unsigned short port;
	std::string message;
	std::string md5;
} postqunimage_t;

class CQunImage: public CClientConnection {
public:
	CQunImage(CNetwork* network);
	virtual ~CQunImage();
	virtual void customEvent(QCustomEvent *e);
	//static void PostEvent(QCustomEvent* e);

	void connectionError();
	void connectionEstablished();
	void connectionClosed();
	void waitTimedOut();
	bool crashRecovery();
	int dataReceived(NETLIBPACKETRECVER* nlpr);

	void append(EvaPicOutPacket *packet);
	void stop();

	static void shutdownImageServer();

	static CRITICAL_SECTION m_cs;
	static CQunImageServer* m_imageServer;
private:
	static CQunImage* m_inst;

	typedef struct FileItem{
		string filename;
		int length;
		int offset;
		unsigned short lastPacketSeq;
		unsigned char *buf;
	} FileItem;

	int sendCount;

	bool isBusy;
	typedef struct Session {
		unsigned int qunID;
		std::list<CustomizedPic> list;
	} Session;

	std::list<Session> downloadList;
	std::list<CustomizedPic> picList;
	std::list<postqunimage_t> sentList;
	unsigned int qunID;
	CustomizedPic currentPic;
	int currentIndex;

	FileItem currentFile;

	typedef struct SessionOut{
		unsigned int qunID;
		std::list< OutCustomizedPic> list;
		string msg;
	} OutSession;

	OutCustomizedPic currentOutPic;
	std::list<OutSession> sendList;
	std::list<OutCustomizedPic> outList;
	string outMsg;

	unsigned short expectedSequence;
	unsigned int sessionID;
	unsigned int sendIP;
	unsigned short sendPort;

	void doRequestPic(CustomizedPic pic);
	void doRequestData(CustomizedPic pic, const bool isReply);

	void doProcessEvent();
	void initConnection(const int ip, const short port);

	void sendPacket(EvaPicOutPacket *packet);
	void parseInData(const EvaPicInPacket *in);

	/*
	unsigned char buf[65535];
	unsigned int bufLength;
	*/
	bool isSend;
	bool isAppending, isRemoving;

	list<EvaPicOutPacket*> outPool;
	void removePacket(const int hashCode ); // remove packet which needs acknowlegement
	void clearManager();

	void slotReady();
	void slotDataComming(int);
	void slotException(int);
	void packetMonitor();

	void slotProcessBuffer();

	void processRequestAgentReply(const EvaPicInPacket *in);
	void processRequestFaceReply(const EvaPicInPacket *in);
	void processTransferReply(const EvaPicInPacket *in);
	void processRequestStartReply(const EvaPicInPacket *in);

	void doSaveFile();
	void doRequestNextPic();

	void doProcessOutEvent();
	void doRequestStart();
	void doSendFileInfo();
	void doRequestAgent();
	void doSendNextFragment();

	void pictureReady(const UINT id, LPCWSTR fileName);
	void pictureSent(const unsigned int id, const unsigned int sessionID, const unsigned int ip, const unsigned short port);
	void sendErrorMessage(const UINT id, LPCSTR msg);

	CNetwork* m_network;

	HWND m_hWndPopup;
	WCHAR m_popupText[MAX_PATH];
	LPWSTR m_popupTextP;
	string m_md5;

	int m_timeoutCount;
};
#endif // QUNIMAGE_H