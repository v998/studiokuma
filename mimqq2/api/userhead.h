#ifndef USERHEAD_H
#define USERHEAD_H

class EvaUHPacket;
class EvaUHFile;
class EvaUHProfile;

class CUserHead: public CClientConnection {
public:
	CUserHead(CNetwork* network);
	~CUserHead();
	void connectionError();
	void connectionEstablished();
	void connectionClosed();
	void waitTimedOut();
	int dataReceived(NETLIBPACKETRECVER* nlpr);
	void append(OutPacket* out);
	void sendOut(OutPacket* out);

private:
	void processPacket(LPCBYTE lpData, const USHORT len);

public:
	enum COMMAND {No_CMD, All_Info, Buddy_Info, Buddy_File, All_Done};

	// the method will post UH event if profile loaded
	void setQQList(const std::list<unsigned int> list) { mUHList = list; }
	void stop();

	void doAllInfoRequest();
	void processComingData();
	void checkTimeout();
private:
	COMMAND cmdSent;
	time_t timeSent;
	unsigned int AllInfoGotCounter;
	string mUHDir;

	bool mAskForStop;
	const char* mBuffer;
	int bytesRead;
	std::list<unsigned int> mUHList;
	std::list<UHInfoItem> mUHInfoItems;

	EvaUHFile *mCurrentFile;

	list<unsigned long> mHostAddresses;
	void doDnsRequest();

	bool doInfoRequest();
	void doTransferRequest(const unsigned int id, const unsigned int sid,
		const unsigned  int start, const unsigned  int end);

	void processAllInfoReply();
	void processBuddyInfoReply();
	void processBuddyFileReply();

	void cleanUp();

	void sendOut(EvaUHPacket *packet);

	UCHAR m_retryCount;

	CNetwork* m_network;

	HWND m_hWndPopup;
	WCHAR m_popupText[MAX_PATH];
	LPWSTR m_popupTextP;
};
#endif // USERHEAD_H
