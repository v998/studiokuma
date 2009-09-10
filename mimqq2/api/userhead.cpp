#include "StdAfx.h"

#define MaxBlockSize      800

#define CFACE_SERVER       "cface_tms.qq.com"
#define CFACE_PORT         4000

#define UH_TIME_OUT        5
#define UH_MAX_RETRY_COUNT 4


// UH represents User Head which is used in Tencent QQ, sounds pretty @!#~*!$@# :)
class EvaUHFile {
public:
	EvaUHFile(int id);
	~EvaUHFile();
	const bool setFileInfo(const char *md5, unsigned int sid);
	const bool setFileBlock(const unsigned int numPackets, const unsigned int packetNum, 
		const unsigned int fileSize, const unsigned int partStart, 
		const unsigned int partSize, const unsigned char *buf);

	const string getMd5String() const;
	const int getQQ() const { return mId; }
	const int getSessionId() const { return mSessionId; }

	// is this file downloaded sucessfully?
	const bool isFinished();
	// return session id, and assign the start and size, return 0 means md5 wrong, 
	// need redownloading this file again, or no session available
	const unsigned int nextBlock(unsigned int *start, unsigned int *size);

	const bool isMD5Correct();
	void save(string &dir);
	void initSettings();

	const int getNumPackets() const { return mNumPackets; }
	const int getCompletedPackets();
private:
	int mId;
	char md5[16];
	bool *flags;
	unsigned int mFileSize;  // total size of this file
	unsigned int mNumPackets;   // number of blocks this file needs
	unsigned int mSessionId;   // the session id used in downloading
	unsigned char *mBuffer;  // the file data
};


EvaUHFile::EvaUHFile(int id)
: mId(id), flags(NULL), mFileSize(0), mNumPackets(0), mSessionId(0), mBuffer(NULL)
{
}

EvaUHFile::~EvaUHFile()
{
	if(flags) delete flags;
	if(mBuffer) delete mBuffer;
}

const bool EvaUHFile::setFileInfo(const char *_md5, unsigned int sid)
{
	if(mSessionId) return false;
	memcpy(md5, _md5, 16);
	mSessionId = sid;
	return true;
}

const bool EvaUHFile::setFileBlock(const unsigned int numPackets, const unsigned int packetNum, 
								   const unsigned int fileSize, const unsigned int partStart, 
								   const unsigned int partSize, const unsigned char *buf)
{
	//if(packetNum >= numPackets) return false; // impossible
	if(mFileSize && fileSize != mFileSize) return false;  // different file size
	//if(mNumPackets && numPackets != mNumPackets) return false; // num of blocks wrong
	//if(flags && flags[packetNum] ) return false; // got this block already

	if(!mFileSize) mFileSize = fileSize;
	if(!mNumPackets) mNumPackets = numPackets;
	if(!flags){ 
		flags = new bool[mNumPackets];
		for(unsigned int i=0; i<mNumPackets; i++)
			flags[i] = false;
	}

	if(!mBuffer){ 
		mBuffer = new unsigned char[mFileSize];
		memset(mBuffer, 0, mFileSize);
	}
	memcpy(mBuffer+partStart, buf, partSize);
	//flags[packetNum] = true;
	flags[partStart/MaxBlockSize] = true;
	return true;
}

const string EvaUHFile::getMd5String() const
{
	return EvaHelper::md5ToString(md5);
}

const bool EvaUHFile::isFinished()
{
	if(!flags) return false;
	bool result = true;
	for(unsigned int i=0; i< mNumPackets; i++){
		if(!flags[i]) {
			result = false;
			break;
		}
	}
	return result;
}

const int EvaUHFile::getCompletedPackets() {
	if(!flags) return 0;
	int c=0;

	for(unsigned int i=0; i< mNumPackets; i++){
		if(flags[i]) c++;
	}

	return c;
}

// return session id, and assign the start and size, return 0 means md5 wrong, 
// need redownloading this file again, no session available
const unsigned int EvaUHFile::nextBlock(unsigned int *start, unsigned int *size)
{
	if(!flags){
		*start = 0xffffffff;
		*size = 0;
		return mSessionId;
	}
	unsigned int index;
	for(index=0; index<mNumPackets; index++)
		if(!flags[index]) break;

	if(index==0 || index== mNumPackets){
		*start = 0xffffffff;
		*size = 0;
	}else{
		*start = (index * MaxBlockSize);
		if(index == mNumPackets-1)
			*size = mFileSize - (*start);
		else
			*size = MaxBlockSize;
	}

	return mSessionId;
}

const bool EvaUHFile::isMD5Correct()
{
	if(!mBuffer)
		return false;

	char *gMd5 = new char[16];
	memcpy(gMd5, EvaUtil::doMd5((char *)mBuffer, mFileSize), 16);
	if(memcmp(gMd5, md5, 16) != 0){  // which means not equal
		delete [] gMd5;
		return false;
	}
	delete []gMd5;
	return true;
}

// Given a directory, that's enough
void EvaUHFile::save(string &dir)
{
	if(!mBuffer){
		util_log(0, "EvaUHFile::save -- NULL file buffer, failed\n");
		return;
	}

	/* we don't check md5, leave the job to calling function to do, save tiem :)
	// we check md5 first
	if(!isMD5Correct()){
	util_log(0, "EvaUHFile::save -- MD5 checking, failed\n");
	initSettings(); // if md5 wrong, the data is useless, we clear all
	return;
	}
	*/
	string filePrefix = dir + "\\" + getMd5String() + ".bmp";

	ofstream file;
	file.open(filePrefix.c_str(),ios_base::out | ios_base::trunc | ios_base::binary);
	if(file.bad()){
		util_log(0, "EvaUHFile::save -- file creating, failed\n");
		return;
	}

	file.write((char*)mBuffer,mFileSize);
	file.flush();
	file.close();
	/* TODO: How to do this w/o any dll?
	QImage grayPic;
	grayPic.loadFromData(mBuffer, mFileSize);
	EvaQtUtils::convertToGrayscale(&grayPic);
	grayPic.save(filePrefix +"_off.bmp", "BMP");
	*/
}

// we only re-set the details not the basic information like id, session id and md5 value
void EvaUHFile::initSettings()
{
	if(flags) delete []flags;
	flags = NULL;
	if(mBuffer) delete []mBuffer;
	mBuffer = NULL;
	mNumPackets = 0;
	mFileSize = 0;
}

CUserHead::CUserHead(CNetwork* network):
CClientConnection("CUSERHEAD",200),
AllInfoGotCounter(0), mAskForStop(false), mCurrentFile(NULL), m_retryCount(0), m_network(network), m_hWndPopup(NULL) {
	//registerConnection(CONN_TYPE_USERHEAD,this);
	setServer(true,CFACE_SERVER,CFACE_PORT);
	char szFileName[MAX_PATH];
	// CallService(MS_UTILS_PATHTOABSOLUTE,(WPARAM)"QQ",(LPARAM)szFileName);
	FoldersGetCustomPath(network->m_avatarFolder,szFileName,MAX_PATH,"QQ");
	mUHDir=szFileName;
	mUHInfoItems.clear();

	if (ServiceExists(MS_POPUP_ADDPOPUPW) && !DBGetContactSettingByte(NULL,network->m_szModuleName,QQ_NOPROGRESSPOPUPS,0)) {
		POPUPDATAW ppd={0};
		swprintf(ppd.lpwzContactName,TranslateT("QQ User Head Operation (%s)"),network->m_tszUserName);
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

CUserHead::~CUserHead() {
	cleanUp();
	util_log(0,"[CUserHead] Wait for connection to close");
	while (isConnected()) Sleep(100);
	m_network->uhCallbackHub(-1,0,NULL,0);
	if (m_hWndPopup) PUDeletePopUp(m_hWndPopup);
	util_log(0,"[CUserHead] Instance Destruction");
}

void CUserHead::doAllInfoRequest() {
	std::list<unsigned int> toSend;
	std::list<unsigned int>::iterator it;
	int counter = 0;
	util_log(0,"[CUserHead] doAllInfoRequest -- AllInfoGotCounter: %d\n", AllInfoGotCounter);
	if(AllInfoGotCounter < mUHList.size()){
		it=mUHList.begin();
		for(unsigned int i=0; i<AllInfoGotCounter ;++it) i++;
		if (AllInfoGotCounter==0 && mUHList.size()) {
			toSend.push_back(m_network->GetMyQQ());
			counter++;
		}
		for(; it!=mUHList.end(); ++it){
			toSend.push_back(*it);
			counter++;
			if(counter>=20) break;
		}
	}
	if(!toSend.size()) return;
	if (m_hWndPopup) {
		swprintf(m_popupText,TranslateT("Requesting UH Information for %d contacts"),toSend.size());
		PUChangeTextW(m_hWndPopup,m_popupText);
	}
	util_log(0,"[CUserHead] %d Buddies sent\n", toSend.size());
	EvaUHInfoRequest *packet = new EvaUHInfoRequest();
	packet->setQQList(toSend);
	sendOut(packet);
	cmdSent = All_Info;
	timeSent = time(NULL);
}

void CUserHead::sendOut(EvaUHPacket *packet)
{
	if(!isConnected()){
		util_log(0,"[CUserHead] send -- socket error\n");
		delete packet;
		return;
	}
	UCHAR* buffer=(UCHAR*)mir_alloc(4096);
	int len=0;

	CEvaAccountSwitcher::ProcessAs(m_network->GetMyQQ());
	packet->fill(buffer, &len);
	CEvaAccountSwitcher::EndProcess();
	send((LPSTR)buffer,len);
	mir_free(buffer);
	delete packet;
}

void CUserHead::processComingData() {
	m_retryCount=0;
	switch(cmdSent){
	case No_CMD:
		break;
	case All_Info:
		processAllInfoReply();
		break;
	case Buddy_Info:
		processBuddyInfoReply();
		break;
	case Buddy_File:
		processBuddyFileReply();
		break;
	default:           //make some compilers happy :)
		break;
	}
}

void CUserHead::processAllInfoReply() {
	EvaUHInfoReply packet((unsigned char *)mBuffer, bytesRead);
	if(packet.parse()){         //  good packet 
		if(!packet.isSuccessful()){ // no UH picture available
			util_log(0,"[CUserHead] Received unsuccessful UH reply");
			return;
		}
		std::list<UHInfoItem> list= packet.getInfoItems();
		AllInfoGotCounter += list.size();
		std::list<UHInfoItem>::iterator it = list.begin();
		UHInfoItem uhii;

		while(it!=list.end()){
			if (m_network->uhCallbackHub(Buddy_Info, it->id, (char*)EvaHelper::md5ToString(it->md5).c_str(), it->sessionId)) {
				uhii=*it;
				mUHInfoItems.push_back(uhii);
				util_log(0,"[UserHead] Added, it->id=%d, it->sessionId=%d, uhii->id=%d, uhii->sessionId=%d, count=%d",it->id,it->sessionId, mUHInfoItems.back().id,mUHInfoItems.back().sessionId,mUHInfoItems.size());
			}
			++it;
		}

		if(AllInfoGotCounter < mUHList.size()){
			doAllInfoRequest();
		}else{
			AllInfoGotCounter=0;
			mUHList.clear();

			// now we start download User Head :)
			if(!doInfoRequest())    // return false means all done
				mAskForStop = true;
		}
	} else
		util_log(0,"EvaUHInfoReply parse failed");
}

bool CUserHead::doInfoRequest() {
	UHInfoItem item={0}; 
	if (!mUHInfoItems.size()) {
		util_log(0,"[UserHead] No more items to retrieve, shutting down");
		//disconnect();
		return false;
	}

	item = mUHInfoItems.front();
	util_log(0,"[UserHead] doInfoRequest, id=%d, sessionId=%x, count=%d\n",item.id,item.sessionId,mUHInfoItems.size());

	if(!(item.id))  // if id == 0, no file need download, we finish
		return false;

	if (m_hWndPopup) {
		DBVARIANT dbv={0};

		if (HANDLE hContact=m_network->FindContact(item.id)) {
			DBGetContactSettingTString(hContact,m_network->m_szModuleName,"Nick",&dbv);
		}
		
		wcscpy(m_popupText,TranslateT("Processing "));
		if (dbv.ptszVal) {
			wcscat(m_popupText,dbv.ptszVal);
			swprintf(m_popupText+wcslen(m_popupText),L"(%d)",item.id);
			DBFreeVariant(&dbv);
		} else
			_itow(item.id,m_popupText+wcslen(m_popupText),10);
		wcscat(m_popupText,L": ");
		m_popupTextP=m_popupText+wcslen(m_popupText);
		wcscpy(m_popupTextP,TranslateT("Requesting Info"));

		PUChangeTextW(m_hWndPopup,m_popupText);
	}

	std::list<unsigned int> qqList;
	qqList.push_back(item.id);
	EvaUHInfoRequest *packet = new EvaUHInfoRequest();
	packet->setQQList(qqList);
	sendOut(packet);  // send method will delete sPacket
	cmdSent = Buddy_Info;
	timeSent = time(NULL);
	return true;
}

void CUserHead::doTransferRequest(const int id, const unsigned int sid, const unsigned int start, const unsigned int end) {
	util_log(0,"[UserHead] doTransferRequest: tid: %d, sid: %8x, s: %8x, e: %8x\n", id, sid, start, end);
	if (m_hWndPopup && start==0) {
		wcscpy(m_popupTextP,TranslateT("Starting Transfer"));
		PUChangeTextW(m_hWndPopup,m_popupText);
	}
	EvaUHTransferRequest *packet = new EvaUHTransferRequest();
	packet->setUHInfo(id, sid);
	packet->setFileInfo(start, end);
	sendOut(packet);  // send method will delete sPacket
	cmdSent = Buddy_File;
	timeSent = time(NULL);
}

void CUserHead::processBuddyInfoReply() {
	util_log(0,"[CUserHead] processBuddyInfoReply\n");
	EvaUHInfoReply *packet = new EvaUHInfoReply((unsigned char *)mBuffer, bytesRead);
	if(packet->parse()){         //  good packet 
		if(!packet->isSuccessful()){ // no UH picture available
			delete packet;
			return;
		}
		std::list<UHInfoItem> list= packet->getInfoItems();
		std::list<UHInfoItem>::iterator it = list.begin(); // actually this should be at most one element
		//FIXME we should check the new information
		//if(mProfileManager) mProfileManager->updateInfo(*it);


		// now we start download User Head :)
		UHInfoItem item={0}; item = mUHInfoItems.front(); //mProfileManager->nextDownload();
		util_log(0,"[CUserHead] item.id=%d, item.sessionId=%d, count=%d",item.id,item.sessionId,mUHInfoItems.size());
		if(!(item.id)) {  // if id == 0, no file need download, we finish
			mAskForStop = true;
			delete packet;
			return;
		}

		if(mCurrentFile) delete mCurrentFile;
		mCurrentFile = new EvaUHFile(item.id);
		mCurrentFile->setFileInfo(item.md5, item.sessionId);
		unsigned int fstart, fend;
		mCurrentFile->nextBlock(&fstart, &fend);
		doTransferRequest(item.id, item.sessionId, fstart, fend);
	}
	delete packet;
}

void CUserHead::processBuddyFileReply() {
	EvaUHTransferReply *packet = new EvaUHTransferReply((unsigned char *)mBuffer, bytesRead);
	if(packet->parse()){
		if(!packet->isDataPacket()){ // this is a notification, transfer will start very soon
			util_log(0,"[CUserHead] processBuddyFileReply -- Start Soon!!!\n");
			delete packet;
			return;
		}
		if(mCurrentFile){
			util_log(0,"[CUserHead] EvaUHManager::processBuddyFileReply -- No Pkts:%d,  Pkt No:%d\n", 
				packet->getNumPackets(), packet->getPacketNum());
			mCurrentFile->setFileBlock(packet->getNumPackets(), packet->getPacketNum(),
				packet->getFileSize(), packet->getStart(), 
				packet->getPartSize(), packet->getPartData() );
			if (m_hWndPopup) {
				swprintf(m_popupTextP,TranslateT("Transferring %d/%d"),mCurrentFile->getCompletedPackets(),mCurrentFile->getNumPackets());
				PUChangeTextW(m_hWndPopup,m_popupText);
			}
			if(mCurrentFile->isFinished()){
				if(true || mCurrentFile->isMD5Correct()){
					mCurrentFile->save(mUHDir);
					//HeadFinished(packet->getQQ());
					m_network->uhCallbackHub(Buddy_File, packet->getQQ(),NULL,0);

					delete mCurrentFile;
					mCurrentFile = NULL;
					// now we download next User Head
					if (mUHInfoItems.size()) mUHInfoItems.pop_front();
					if(!doInfoRequest()){    // return false means all done
						mAskForStop = true;
						cmdSent = All_Done;
					}
				}/*else {
					util_log(0,"UH MD5 check failed");
					mCurrentFile->initSettings(); // we clear all contents of the file
				}*/
			}
		}
	}
}

void CUserHead::waitTimedOut() {
	if(mAskForStop || cmdSent == All_Done) return;

	if(m_retryCount >= UH_MAX_RETRY_COUNT){
		// we won't save the non-image user head
		//mProfileManager->saveProfile();
		if (mUHInfoItems.size()) {
			UHInfoItem item = mUHInfoItems.front();
			util_log(0, "[CUserHead] downloading user(%d) avatar error, try next one", item.id);
			m_retryCount = 0;
			cmdSent = Buddy_Info;
		} else
			disconnect();
	}

	int last = (int)(time(NULL)-timeSent);
	if(last >= UH_TIME_OUT){
		util_log(0,"[CUserHead] timeout occurred, cmdSent=%d",cmdSent);
		m_retryCount++;
		//printf("EvaUHManager::checkTimeout : %d\n", cmdSent);
		switch(cmdSent){
			case All_Info:
				doAllInfoRequest();
				break;
			case Buddy_Info:
				if(!doInfoRequest())    // return false means all done
					mAskForStop = true;
				break;
			case Buddy_File:
				{
					if(!mCurrentFile){
						UHInfoItem item = mUHInfoItems.front();
						mCurrentFile = new EvaUHFile(item.id);
						mCurrentFile->setFileInfo(item.md5, item.sessionId);
					}
					unsigned int fstart, fend, sid;
					sid = mCurrentFile->nextBlock(&fstart, &fend);
					if(!sid)  // if no session id, we ask for it
						doInfoRequest();
					else      // otherwise, download it
						doTransferRequest(mCurrentFile->getQQ(), sid, fstart, fend);
				}
				break;
			default:
				break;
		}
	}

	if (mAskForStop) disconnect();

}

void CUserHead::cleanUp() {
	util_log(0,"[CUserHead] Cleanup");
	AllInfoGotCounter = 0;
	//disconnect();

	if(mCurrentFile) delete mCurrentFile;
	mCurrentFile = NULL;
}

void CUserHead::connectionError() {

}

void CUserHead::connectionEstablished() {
	doAllInfoRequest();
}

void CUserHead::connectionClosed() {
	util_log(0,"[CUserHead] Connection closed. Delete me");
	delete this;
}

int CUserHead::dataReceived(NETLIBPACKETRECVER* nlpr) {
	bytesRead=nlpr->bytesAvailable;
	mBuffer=(LPSTR)nlpr->buffer;
	CEvaAccountSwitcher::ProcessAs(m_network->GetMyQQ());
	processComingData();
	CEvaAccountSwitcher::EndProcess();
	nlpr->bytesUsed+=nlpr->bytesAvailable;
	if (mAskForStop) disconnect();

	return 0;
}