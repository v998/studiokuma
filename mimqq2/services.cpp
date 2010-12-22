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
/* Services.cpp: Miranda IM related operational functions
 * 
 */

#include "StdAfx.h"
#include <math.h>

#if 0
map<unsigned int,ft_t*> ftSessions;
#endif

void __cdecl _get_infothread(HANDLE hContact);
extern "C" BOOL CALLBACK ModifySignatureDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);

#define MIMPROC2(a) int a(WPARAM wParam, LPARAM lParam)
#define MIMPROC(a) int CNetwork::a(WPARAM wParam, LPARAM lParam)

LPSTR servers[]={
	"udp://sz.tencent.com:8000",
	"udp://sz2.tencent.com:8000",
	"udp://sz3.tencent.com:8000",
	"udp://sz4.tencent.com:8000",
	"udp://sz5.tencent.com:8000",
	"udp://sz6.tencent.com:8000",
	"udp://sz7.tencent.com:8000",
	"udp://202.96.170.64:8080",
	"udp://64.144.238.155:8080",
	"udp://202.104.129.254:8080",
	"tcp://tcpconn.tencent.com:8000",
	"tcp://tcpconn2.tencent.com:8000",
	"tcp://tcpconn3.tencent.com:8000",
	"tcp://tcpconn4.tencent.com:8000",
	NULL
};

unsigned char lastServer=0xff;

// GetCaps(): Get Protocol Capabilities
// wParam: Type of capability to be queried
// lParam: N/A
// Return: Corresponding capability flag
extern "C" {
	// GetName(): Get Protocol Name to be displayed in tray icon
	// wParam: Max length of lParam
	// lParam: String to receive protocol name
	MIMPROC(GetName)
	{
		char* pszResult=(char*)lParam;
		char* pszANSI=mir_u2a(m_tszUserName);
		mir_snprintf(pszResult,wParam,"%s",pszANSI);
		mir_free(pszANSI);
		//lstrcpynA((LPSTR)lParam, m_szModuleName, wParam);

		return 0;
	}

	// GetStatus(): Retrieve Prototol Status
	// wParam: N/A
	// lParam: N/A
	// Return: Current Status
	MIMPROC(GetStatus)
	{
		return m_iStatus;
	}

}

typedef struct {
	HANDLE hContact;
	CNetwork* network;
	int qunid;
} AddQunMember_t;

// AddQunMemberProcOpts(): Window Message Handler for Add Qun Member Dialog
// hwndDlg: Window handle to dialog
// msg: Window message received
// wParam: Determined by msg
// lParam: Determined by msg
// Return: true if the message has been handled and should not sent to parent, false otherwise
static BOOL CALLBACK AddQunMemberProcOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
			case WM_INITDIALOG: // Dialog initialization, lParam=hContact
				{
					AddQunMember_t* aqmt=(AddQunMember_t*)lParam;
					LPSTR m_szModuleName=aqmt->network->m_szModuleName;
					unsigned int qunid=DBGetContactSettingDword(aqmt->hContact,m_szModuleName,UNIQUEIDSETTING,0);
					SetWindowLong(hwndDlg,GWL_USERDATA,lParam);
					if (!qunid) { // Chat.dll contact
						DBVARIANT dbv;
						if (!DBGetContactSetting(aqmt->hContact,m_szModuleName,"ChatRoomID",&dbv)) {
							qunid=atoi(strchr(dbv.pszVal,'.')+1);
							DBFreeVariant(&dbv);
						}
					}

					if (qunid) { // QunID obtained, fill qun information to textbox
						Qun* qun=aqmt->network->m_qunList.getQun(qunid);
						HANDLE hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, (WPARAM)NULL, (LPARAM)NULL);
						WCHAR szQun[MAX_PATH*2];
						DBVARIANT dbv;

						if (!DBGetContactSettingTString(aqmt->hContact,aqmt->network->m_szModuleName,"Nick",&dbv)) {
							swprintf(szQun,L"%s (%d)", dbv.ptszVal, DBGetContactSettingDword(aqmt->hContact,aqmt->network->m_szModuleName,"ExternalID",0));
							DBFreeVariant(&dbv);
						} else
							swprintf(szQun,L"%d (%d)",qunid,qunid);

						SetDlgItemText(hwndDlg,IDC_QUNID,szQun);

						aqmt->qunid=qunid;

						while (hContact) {
							if (!lstrcmpA(m_szModuleName, (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact,(LPARAM)NULL)) && 
								DBGetContactSettingByte(hContact,m_szModuleName,"IsQun",0)==0 && DBGetContactSettingByte(hContact,m_szModuleName,"ChatRoom",0)==0) {
									if (!DBGetContactSettingTString(hContact,m_szModuleName,"Nick",&dbv)) {
										_stprintf(szQun,_T("%s (%d)"),dbv.ptszVal,DBGetContactSettingDword(hContact,m_szModuleName,UNIQUEIDSETTING,0));
										DBFreeVariant(&dbv);
										SendDlgItemMessage(hwndDlg,IDC_MEMBERLIST,CB_ADDSTRING,NULL,(LPARAM)szQun);
									}
								}

								hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, (LPARAM)NULL);
						}
						return NULL;

					}
				}
				break;
			case WM_COMMAND: // Button pressed
				{
					switch (LOWORD(wParam)) {
						case IDOK: // OK Button
							{
								AddQunMember_t* aqmt=(AddQunMember_t*)GetWindowLong(hwndDlg,GWL_USERDATA);
								CHAR szUID[MAX_PATH]={0};
								LPSTR pszUID;

								SendDlgItemMessageA(hwndDlg,IDC_MEMBERLIST,CB_GETLBTEXT,(LPARAM)SendDlgItemMessage(hwndDlg,IDC_MEMBERLIST,CB_GETCURSEL,NULL,NULL),(WPARAM)szUID);
								if (!*szUID)
									MessageBox(NULL,TranslateT("You did not select an contact from the list."),APPNAME,MB_ICONERROR);
								else {
									CHAR szQun[MAX_PATH]={0};
									std::list<unsigned int> list;
									QunModifyMemberPacket *out;

									pszUID=strrchr(szUID,'(')+1;
									*strchr(pszUID,')')=0;
									util_log(0,"pszUID=%s (%d)",pszUID,atoi(pszUID));

									EnableWindow(GetDlgItem(hwndDlg,IDOK),false);
									GetDlgItemTextA(hwndDlg,IDC_QUNID,szQun,MAX_PATH);
									out=new QunModifyMemberPacket(aqmt->qunid,true);
									list.push_back(atoi(pszUID));
									out->setMembers(list);
									util_log(0,"Add member %d to qun %d",list.begin(),out->getQunID());
									aqmt->network->append(out);

									EndDialog(hwndDlg,0);
								}
							}
							break;
						case IDCANCEL: // Cancel Button
							EndDialog(hwndDlg,0);
							break;
					}
				}
				break;
			case WM_DESTROY:
				delete (AddQunMember_t*)GetWindowLong(hwndDlg,GWL_USERDATA);
				break;
	}
	return false;
}

void KickQunUser(void* args) {
	KICKUSERSTRUCT* kickUser=(KICKUSERSTRUCT*) args;

	TCHAR szMsg[MAX_PATH];
	_stprintf(szMsg,TranslateT("Are you sure you want to kick user %d out of this Qun %d?"),kickUser->qqid,kickUser->qunid);
	if (MessageBox(NULL,szMsg,APPNAME,MB_ICONWARNING|MB_YESNO)==IDYES) {
		std::list<unsigned int> list;
		QunModifyMemberPacket *out=new QunModifyMemberPacket(kickUser->qunid,false);
		list.insert(list.end(),kickUser->qqid);
		out->setMembers(list);
		kickUser->network->append(out);
	}
	mir_free(kickUser);
}

// OnContactDeleted(): A buddy is about to be deleted
// wParam: (HANDLE) hContact
// lParam: N/A
// Return: 0 is the operation completed, 1 otherwise
MIMPROC(OnContactDeleted) {
	char* szProto;
	unsigned int uid;
	char is_qun;
	HANDLE hContact=(HANDLE)wParam;

	szProto=(char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, wParam, 0);
	if (!szProto || strcmp(szProto, m_szModuleName)) return 0;
	if (!Packet::isClientKeySet()) return 1;

	if (DBGetContactSettingByte(hContact,"CList","NotOnList",0)==1) return 0;

	if (READC_B2("ChatRoom")!=0) {
		GCDEST gd={m_szModuleName};
		GCEVENT ge={sizeof(ge),&gd};
		DBVARIANT dbv;
		READC_TS2("ChatRoomID",&dbv);
		gd.ptszID=dbv.pwszVal;
		gd.iType=GC_EVENT_CONTROL;
		CallService(MS_GC_EVENT,SESSION_TERMINATE,(LPARAM)&ge);
		CallService(MS_DB_CONTACT_DELETE,(WPARAM)FindContact(_wtoi(dbv.pwszVal)),0);
		DBFreeVariant(&dbv);
	} else {
		uid=READC_D2(UNIQUEIDSETTING);
		is_qun=READC_B2("IsQun");
		if (uid && !is_qun) { // Remove general contact
			util_log(0,"%s(): About to remove contact with uid=%d",__FUNCTION__,uid);
			if (MessageBox(NULL,TranslateT("Do you want to delete the contact from server?\n(If you answer 'No', the contact will appear in your list again when you online next time)"),APPNAME,MB_YESNO | MB_ICONQUESTION)==IDYES) {
				// Remove buddy from server list
				DeleteFriendPacket *packet=new DeleteFriendPacket();
				packet->setBuddyQQ(uid);
				append(packet);
			}
		} else if (is_qun) { // Remove qun contact
			util_log(0,"%s(): About to remove qun contact with uid=%d",__FUNCTION__,uid);
			if (MessageBox(NULL,TranslateT("Do you want to exit the Qun?\n(If you answer 'Yes', you will need authorization again)"),APPNAME,MB_YESNO | MB_ICONQUESTION)==IDYES) {
				// Exit from Qun
				append(new QunExitPacket(uid));
			}
		}
	}
	return 0;
}

// OnRebuildContextMenu(): Context menu is being rebuilt
// wParam: (HANDLE) hContact
// lParam: N/A
// Return: Always 0
MIMPROC(OnPrebuildContactMenu) {
	DWORD config=0;
	HANDLE hContact=(HANDLE)wParam;
	CLISTMENUITEM clmi={sizeof(clmi)};
	if (!strcmp((LPSTR)CallService(MS_PROTO_GETCONTACTBASEPROTO,(WPARAM)hContact,0),m_szModuleName)) {
		/*
		CRMI2(QQ_CNXTMENU_REMOVEME,RemoveMe,Translate("&Remove me from his/her list"));
		CRMI2(QQ_CNXTMENU_ADDQUNMEMBER,AddQunMember,Translate("&Add a member to Qun"));
		CRMI2(QQ_CNXTMENU_SILENTQUN,SilentQun,"");
		CRMI2(QQ_CNXTMENU_REAUTHORIZE,Reauthorize,Translate("Resend &authorization request"));
		CRMI2(QQ_CNXTMENU_CHANGECARDNAME,ChangeCardName,Translate("Change my Qun &Card Name"));
		CRMI2(QQ_CNXTMENU_POSTIMAGE,PostImage,Translate("Post &Image"));
		CRMI2(QQ_CNXTMENU_SELECTIMAGE,SelectImage,Translate("Select &Image"));
		CRMI2(QQ_CNXTMENU_FORCEREFRESH,QunSpace,Translate("Qun &Space"));
		*/

		if (Packet::isClientKeySet()) {
			if (READC_B2("IsQun")==1) {
				//Qun* qun=m_qunList.getQun(READC_D2(UNIQUEIDSETTING));
				config=0x74; //0x1b4;
				if (READC_D2("Creator")==m_myqq || READC_B2("IsAdmin")) { // Show "Add member to Qun"
					config+=0x2;
				}

				//clmi.flags=CMIM_FLAGS|CMIM_NAME;
				if (READC_W2("Status")!=ID_STATUS_ONLINE)
					clmi.ptszName=TranslateT("Allow &receive of Qun Message");
				else
					clmi.ptszName=TranslateT("Keep this Qun &silent");
				//CallService(MS_CLIST_MODIFYMENUITEM,(WPARAM)qqCnxtMenuItems[2],(LPARAM)&clmi);
			} else
				config=0x09;
		}
		//config=-1; // For debug only
	}

	int c=0;

	for (list<HANDLE>::iterator iter=m_contextMenuItemList.begin(); iter!=m_contextMenuItemList.end(); iter++, c++) {
		clmi.flags=CMIF_UNICODE|((config & (1<<c))?CMIM_FLAGS:CMIM_FLAGS|CMIF_HIDDEN);
		if (c==2) clmi.flags+=CMIM_NAME;
		CallService(MS_CLIST_MODIFYMENUITEM,(WPARAM)*iter,(LPARAM)&clmi);
	}

	/*
	for (int c=0; qqCnxtMenuItems[c]; c++) {
		clmi.flags=(config & (1<<c))?CMIM_FLAGS:CMIM_FLAGS|CMIF_HIDDEN;
		CallService(MS_CLIST_MODIFYMENUITEM,(WPARAM)qqCnxtMenuItems[c],(LPARAM)&clmi);
	}
	*/

	return 0;
}
#if 0 // UPGRADE_DISABLE
// _get_infothread(): Notify Details dialog for info retrieval complete
// hContact: Contact Handle
void __cdecl _get_infothread(HANDLE hContact) 
{
	// TODO: Sometimes not effective
	SleepEx(500, TRUE);
	ProtoBroadcastAck(m_szModuleName, hContact, ACKTYPE_GETINFO, ACKRESULT_SUCCESS, (HANDLE) 1, 0);
}
#endif // UPGRADE_DISABLE
std::list<OutCustomizedPic> getSendFiles(const std::list<string> &fileList)
{
	// NOTE: fileList is GBK encoded
	std::list<string> outPicList = fileList;
	std::list<OutCustomizedPic> picList;

	std::list<string>::iterator iter;

	for(iter=outPicList.begin(); iter!=outPicList.end(); ++iter){
		LPWSTR pszFilename=mir_a2u_cp((*iter).c_str(),936);
		FILE* fp=_wfopen(pszFilename,L"rb");
		if (!fp) continue;

		fseek(fp,0,SEEK_END);
		int len=ftell(fp);
		fclose(fp);

		OutCustomizedPic pic;
		pic.fileName = *iter; 
		pic.imageLength = len;
		EvaHelper::getFileMD5(pic.fileName, (char*)pic.md5);
		
		for(list<OutCustomizedPic>::iterator iter2=picList.begin(); iter2!=picList.end(); ++iter2){
			if (!memcmp(iter2->md5,pic.md5,16)) {
				pic.imageLength=0;
				break;
			}
		}

		if (pic.imageLength) {
			picList.push_back(pic);
		}
		mir_free(pszFilename);
	}
	return picList;
}
#if 0
extern "C" {

	// qq_im_sendacksuccess(): Callback Function for message send successful
	// hContact: The handle of contact that sent the message
	void __cdecl qq_im_sendacksuccess(HANDLE hContact)
	{
		ProtoBroadcastAck(m_szModuleName, hContact, ACKTYPE_MESSAGE, ACKRESULT_SUCCESS, (HANDLE) 1, 0);
	}

	// SendMessage(): Callback Function for Sending a message
	// wParam: N/A
	// lParam: Pointer to CCSDATA, which includes information of message being sent
	// Return: 0 if successful, 1 otherwise
	MIMPROC2(SendQQMessage)
	{
		CCSDATA *ccs = (CCSDATA *) lParam;
		unsigned int uid;
		bool fQun=false;
		bool fTemp=false;
	    
		char *msg = (char *) ccs->lParam;

		if (!Packet::isClientKeySet()) return 1;   
	        
		uid=DBGetContactSettingDword(ccs->hContact, m_szModuleName, UNIQUEIDSETTING, 0);
		fQun=DBGetContactSettingByte(ccs->hContact,m_szModuleName,"IsQun",0)==1;
		fTemp=uid>1000000000;
		if (fTemp && !fQun) {
			uid-=1000000000;
			if (strlen(msg)>690) return 1;
		}

		//util_log(0,"uid=%d, hContact=%d, msg=%s",uid,ccs->hContact,msg);
		if (uid) {
			char *msg_with_qq_smiley;
			SendTextIMPacket* packet=NULL; 
			QunSendIMExPacket* qpacket=NULL;
			TempSessionOpPacket* tpacket=NULL;
			GETSETTINGS();

			if (ccs->wParam & PREF_UNICODE && qqSettings->unicode) {
				int wsize=wcslen((LPWSTR)(msg+strlen(msg)+1))*sizeof(WCHAR)+16;
				msg_with_qq_smiley = (char*)mir_alloc(wsize);
				memset(msg_with_qq_smiley,0,strlen(msg)+16);

				if (READ_B2(NULL,QQ_MESSAGECONVERSION) & 2) {
					LPCWSTR pszUnicode=(LPCWSTR)(msg+strlen(msg)+1);
					LPWSTR pszSimp=(LPWSTR)mir_alloc(sizeof(WCHAR)*(wcslen(pszUnicode)+1));
					LCMapString(GetUserDefaultLCID(),LCMAP_SIMPLIFIED_CHINESE,pszUnicode,wcslen(pszUnicode)+1,pszSimp,wcslen(pszUnicode)+1);
					WideCharToMultiByte(936,0,pszSimp,-1,msg_with_qq_smiley,wsize,NULL,NULL);
					mir_free(pszSimp);
				} else
					WideCharToMultiByte(936,0,(LPCWSTR)(msg+strlen(msg)+1),-1,msg_with_qq_smiley,wsize,NULL,NULL);
			} else {
				msg_with_qq_smiley = mir_strdup(msg);
				if (!strstr(msg,"[ZDY]"))
					util_convertToGBK(msg_with_qq_smiley);
			}
			if (fQun) { // Sending Qun message
				if (strstr(msg,"[img]")) {
					if (Packet::getFileAgentKey()) {
						EvaHtmlParser parser;
						std::list<string> outPicList = parser.getCustomImages(msg); // Get list of files
						if (outPicList.size()) {
							EvaSendCustomizedPicEvent *event = new EvaSendCustomizedPicEvent(); 
							event->setPicList(getSendFiles(outPicList)); // Get MD5 and remove any inaccessible files
							event->setQunID(uid);
							event->setMessage(msg_with_qq_smiley);
							mir_free(msg_with_qq_smiley);

#if 1
							CQunImage::PostEvent(event);
#else
							if (!qqQunPicThread) {
								ThreadData* newThread = new ThreadData();

								newThread->mType = SERVER_QUNIMAGE;
								strcpy(newThread->mServer,"219.133.40.128");
								newThread->mPort = 443;
								newThread->mTCP=true;

								newThread->addQunPicEvent(event);
								newThread->startThread((pThreadFunc)QQServerThread);
							} else {
								qqQunPicThread->addQunPicEvent(event);
								delete event;
							}
#endif

							return 1;
						}
					} else {
						MessageBoxA(NULL,Translate("Error: File Agent Key not set!\n\nI will try to request key again, please try sending after 1 minute."),NULL,MB_ICONERROR);
						network->append(new EvaRequestKeyPacket(QQ_REQUEST_FILE_AGENT_KEY));
						return 0;
					}
				}
				if (!strncmp(msg,"/kick ",6)) {
					int qqid=atoi(strchr(msg,' ')+1);
					if (qqid>0) {
						KICKUSERSTRUCT* kickUser=(KICKUSERSTRUCT*)malloc(sizeof(KICKUSERSTRUCT));
						kickUser->qunid=uid;
						kickUser->qqid=qqid;
						mir_forkthread(KickQunUser,kickUser);
						mir_forkthread(qq_im_sendacksuccess, ccs->hContact);
						mir_free(msg_with_qq_smiley);
						return 1;
					}
				} else if (!strncmp(msg,"/temp ",6)) {
					int qqid=atoi(strchr(msg,' ')+1);
					HANDLE hContact=FindContact(qqid+1000000000);
					Qun* qun=qunList.getQun(uid);
					const FriendItem* fi=qun->getMemberDetails(qqid);

					if (!qun || !fi)
						MessageBox(NULL,TranslateT("The specified Qun/QQID Does not exist"),NULL,MB_ICONERROR);
					else {
						char msg2[1024];
						PROTORECVEVENT pre;
						CCSDATA ccs2;
						DBVARIANT dbv;

						if (!hContact) hContact=AddContact(qqid+1000000000,true,false);
						pre.flags=0;
						ccs2.hContact=hContact;
						ccs2.szProtoService = PSR_MESSAGE;
						ccs2.wParam = 0;
						ccs2.lParam = ( LPARAM )&pre;
						pre.timestamp = (DWORD) time(NULL);
						pre.lParam = 0;

						char* pszTemp, *pszTemp2;
						ccs2.hContact=hContact;
						pre.szMessage = msg2;

						DBGetContactSetting(ccs->hContact,m_szModuleName,"Nick",&dbv);
						pszTemp=strdup(dbv.pszVal);
						DBFreeVariant(&dbv);

						if (fi->getQunRealName().size())
							pszTemp2=strdup(fi->getQunRealName().c_str());
						else
							pszTemp2=strdup(fi->getNick().c_str());
						util_convertFromGBK(pszTemp2);

						_stprintf((LPWSTR)msg2,TranslateT("Temp Session: %s(%d) in %s"),pszTemp2,qqid,pszTemp);
						WRITEC_TS("Nick",(LPTSTR)msg2);

						util_convertToGBK(pszTemp);
						DBWriteContactSettingString(hContact,m_szModuleName,"Site",pszTemp);

						free(pszTemp);
						free(pszTemp2);

						CallService(MS_MSG_SENDMESSAGE,(WPARAM)hContact,0);
					}
					mir_forkthread(qq_im_sendacksuccess, ccs->hContact);
					return 1;
				} else {
					qpacket=new QunSendIMExPacket(uid);
					qqSettings->allowUpdateTime=true;
				}
			} else { // Send Contact message
				if (fTemp) {
					DBVARIANT dbv;
					tpacket=new SendTempSessionTextIMPacket();
					tpacket->setReceiver(uid);
					if (!DBGetContactSetting(ccs->hContact,m_szModuleName,"Site",&dbv)) {
						tpacket->setSite(dbv.pszVal);
						DBFreeVariant(&dbv);
					}
					DBGetContactSetting(NULL,m_szModuleName,"Nick",&dbv);
					tpacket->setNick(dbv.pszVal);
					DBFreeVariant(&dbv);
				} else {
					packet=new SendTextIMPacket();
					DBDeleteContactSetting(ccs->hContact,m_szModuleName,"LastAutoReply");
					packet->setReceiver(uid);
				}
			}

			if (qqSettings->enableBBCode) { // BBCode enabled
				bool withFormat=false;

				while (*msg_with_qq_smiley=='[') {
					if (*(msg_with_qq_smiley+2)==']') {
						// b or i or u
						switch (*(msg_with_qq_smiley+1)) {
							case 'b':
								if (packet) packet->setBold(true); else if (qpacket) qpacket->setBold(true); else tpacket->setBold(true);
								memmove(msg_with_qq_smiley,msg_with_qq_smiley+3,strlen(msg_with_qq_smiley+2));
								util_trimChatTags(msg_with_qq_smiley,"[/b]");
								break;
							case 'i':
								if (packet) packet->setItalic(true); else if (qpacket) qpacket->setItalic(true); else tpacket->setItalic(true);
								memmove(msg_with_qq_smiley,msg_with_qq_smiley+3,strlen(msg_with_qq_smiley+2));
								util_trimChatTags(msg_with_qq_smiley,"[/i]");
								break;
							case 'u':
								if (packet) packet->setUnderline(true); else if (qpacket) qpacket->setUnderline(true); else tpacket->setUnderline(true);
								memmove(msg_with_qq_smiley,msg_with_qq_smiley+3,strlen(msg_with_qq_smiley+2));
								util_trimChatTags(msg_with_qq_smiley,"[/u]");
								break;
						}
					} else if (!strncmp(msg_with_qq_smiley,"[color=",7)) {
						// set color
						char* color=strchr(msg_with_qq_smiley,'=')+1;

						switch (*color) {
							case 'r': // Red
								if (packet) {
									packet->setRed(255); packet->setGreen(0); packet->setBlue(0);
								} else if (qpacket) {
									qpacket->setRed(255); qpacket->setGreen(0); qpacket->setBlue(0);
								}
								memmove(msg_with_qq_smiley,msg_with_qq_smiley+11,strlen(msg_with_qq_smiley+10));
								break;
							case 'b': // Blue/Black
								if (*(color+2)=='u') { // Blue
									if (packet) {
										packet->setRed(0); packet->setGreen(0); packet->setBlue(255);
									} else if (qpacket) {
										qpacket->setRed(0); qpacket->setGreen(0); qpacket->setBlue(255);
									}
									memmove(msg_with_qq_smiley,msg_with_qq_smiley+12,strlen(msg_with_qq_smiley+11));
								} else { // Black
									if (packet) {
										packet->setRed(0); packet->setGreen(0); packet->setBlue(0);
									} else if (qpacket) {
										qpacket->setRed(0); qpacket->setGreen(0); qpacket->setBlue(0);
									}
									memmove(msg_with_qq_smiley,msg_with_qq_smiley+13,strlen(msg_with_qq_smiley+12));
								}
								break;
							case 'g': // Green
								if (packet) {
									packet->setRed(0); packet->setGreen(255); packet->setBlue(0);
								} else if (qpacket) {
									qpacket->setRed(0); qpacket->setGreen(255); qpacket->setBlue(0);
								}
								memmove(msg_with_qq_smiley,msg_with_qq_smiley+13,strlen(msg_with_qq_smiley+12));
								break;
							case 'm': // Magenta
								if (packet) {
									packet->setRed(255); packet->setGreen(0); packet->setBlue(255);
								} else if (qpacket) {
									qpacket->setRed(255); qpacket->setGreen(0); qpacket->setBlue(255);
								}
								memmove(msg_with_qq_smiley,msg_with_qq_smiley+15,strlen(msg_with_qq_smiley+14));
								break;
							case 'y': // Yellow
								if (packet) {
									packet->setRed(255); packet->setGreen(255); packet->setBlue(0);
								} else if (qpacket) {
									qpacket->setRed(255); qpacket->setGreen(255); qpacket->setBlue(0);
								}
								memmove(msg_with_qq_smiley,msg_with_qq_smiley+14,strlen(msg_with_qq_smiley+13));
								break;
							case 'w': // White
								if (packet) {
									packet->setRed(255); packet->setGreen(255); packet->setBlue(255);
								} else if (qpacket) {
									qpacket->setRed(255); qpacket->setGreen(255); qpacket->setBlue(255);
								}
								memmove(msg_with_qq_smiley,msg_with_qq_smiley+13,strlen(msg_with_qq_smiley+12));
								break;
						}
						util_trimChatTags(msg_with_qq_smiley,"[/color]");
					} else
						break;

					withFormat=true;
				}

				FontID fid = {sizeof(fid)};
				LOGFONTA font;
				strcpy(fid.name,packet?"Contact Messaging Font":"Qun Messaging Font");
				strcpy(fid.group,m_szModuleName);
				CallService(MS_FONT_GET,(WPARAM)&fid,(LPARAM)&font);
				COLORREF color=DBGetContactSettingDword(NULL,m_szModuleName,packet?"font1Col":"font2Col",0);

				if (!withFormat) { // No format specified, use from stored
					if (packet) {
						packet->setRed(GetRValue(color));
						packet->setGreen(GetGValue(color));
						packet->setBlue(GetBValue(color));
						packet->setBold(font.lfWeight>FW_NORMAL);
						packet->setItalic(font.lfItalic);
						packet->setUnderline(font.lfUnderline);
					} else if (qpacket) {
						qpacket->setRed(GetRValue(color));
						qpacket->setGreen(GetGValue(color));
						qpacket->setBlue(GetBValue(color));
						qpacket->setBold(font.lfWeight>FW_NORMAL);
						qpacket->setItalic(font.lfItalic);
						qpacket->setUnderline(font.lfUnderline);
					}
				}

				int fontsize=(int)DBGetContactSettingByte(NULL,m_szModuleName,packet?"font1Size":"font2Size",9);
				//util_convertToGBK(font.lfFaceName);

				if (packet) {
					//packet->setFontName(string(font.lfFaceName));
					packet->setFontSize(fontsize);
				} else if (qpacket) {
					//qpacket->setFontName(string(font.lfFaceName));
					qpacket->setFontSize(fontsize);
				} else {
					//tpacket->setFontName(string(font.lfFaceName));
					tpacket->setFontSize(fontsize);
				}

			}

			if (packet) {
				EvaHtmlParser parser;
				std::list<string> outPicList = parser.getCustomImages(msg_with_qq_smiley);
				packet->setAutoReply(true);

				char szTemp[701]={0};
				char* pszMsg=msg_with_qq_smiley;
				int msgCount=(int)ceil((float)strlen(msg_with_qq_smiley)/(700.0f));
				unsigned short seq;
				unsigned short messageID=HIWORD(GetTickCount());

				for (int c=0; c<msgCount; c++) {
					strncpy(szTemp,pszMsg,700);
					if (strlen(pszMsg)>700) {
						szTemp[700]=0;
						pszMsg+=700;
					}
						
					if (c>0) packet=new SendTextIMPacket(*packet);
					seq=DBGetContactSettingWord(ccs->hContact,m_szModuleName,"Sequence",0)+1;
					packet->setMsgSequence(seq);
					packet->setSequence(packet->getSequence()+1);
					packet->setNumFragments(msgCount);
					packet->setMessageID(messageID);
					packet->setSeqOfFragments(c);
					DBWriteContactSettingWord(ccs->hContact,m_szModuleName,"Sequence",seq);
					packet->setMessage(szTemp);
					if (c==0) {
						util_log(0,"Sequence of first IM packet is 0x%x",packet->getSequence());
						network->append(packet);
					} else {
						util_log(0,"Added IM packet 0x%x to queue",packet->getSequence());
						pendingImList[packet->getSequence()]=packet;
					}
				}
			} else if (tpacket) {
				tpacket->setMessage(msg_with_qq_smiley); 
				network->append(tpacket);
			} else {
				char szTemp[691]={0};
				char* pszMsg=msg_with_qq_smiley;
				int msgCount=(int)ceil((float)strlen(msg_with_qq_smiley)/(690));
				if ((strlen(msg_with_qq_smiley)%690+12)>700) msgCount++;
				unsigned short seq;
				qpacket->setNumFragments(msgCount);
				qpacket->setMessageID(HIWORD(GetTickCount()));

				for (int c=0; c<msgCount; c++) {
					strncpy(szTemp,pszMsg,690);
					if (strlen(pszMsg)>690) {
						szTemp[690]=0;
						pszMsg+=690;
					} else if (strlen(pszMsg)>678) {
					  szTemp[678]=0;
					  pszMsg+=678;
					} else
						szTemp[strlen(pszMsg)]=0;

					if (c>0) qpacket=new QunSendIMExPacket(*qpacket);
					qpacket->setSeqOfFragments(c);
					seq=DBGetContactSettingWord(ccs->hContact,m_szModuleName,"Sequence",0)+1;
					qpacket->setSequence(seq);
					DBWriteContactSettingWord(ccs->hContact,m_szModuleName,"Sequence",seq);
					qpacket->setMessage(szTemp);
					if (c==0) {
						util_log(0,"Sequence of first Qun IM is 0x%x",qpacket->getSequence());
						network->append(qpacket);
					} else {
						util_log(0,"Added Qun IM packet 0x%x to queue",qpacket->getSequence());
						pendingImList[qpacket->getSequence()]=qpacket;
					}
				}
				if (DBGetContactSettingWord(ccs->hContact,m_szModuleName,"Status",ID_STATUS_ONLINE)!=ID_STATUS_ONLINE)
					// Wake this Qun up
					DBWriteContactSettingWord(ccs->hContact,m_szModuleName,"Status",ID_STATUS_ONLINE);
			}

			if (!qqSettings->waitAck)
				mir_forkthread(qq_im_sendacksuccess, ccs->hContact);

			mir_free(msg_with_qq_smiley);

			// TODO: How to move this to qq_process_send_im_reply()?
			return 1;
		}
	    
		return 0;
	}

	// Added(): You have been added by somebody
	// wParam: N/A
	// lParam: (CCSDATA*) Chained Message
	// Return: 0 if dialog can be shown, 1 otherwise
	MIMPROC2(Added) {
		CCSDATA* ccs = (CCSDATA*)lParam;

		if (!Packet::isClientKeySet()) return 1;

		if (ccs->hContact)
		{
			DWORD dwUin;

			dwUin = DBGetContactSettingDword(ccs->hContact, m_szModuleName, UNIQUEIDSETTING, 0);

			if (dwUin)
			{
				return 0; // Success
			}
		}

		return 1; // Failure
	}
#endif // UPGRADE_DISABLE
	static PBITMAPINFO CreateBitmapInfoStruct(HBITMAP hBmp)
	{ 
		BITMAP bmp; 
		PBITMAPINFO pbmi; 
		WORD    cClrBits; 

		// Retrieve the bitmap's color format, width, and height. 
		GetObject(hBmp, sizeof(BITMAP), (LPSTR)&bmp);

		// Convert the color format to a count of bits. 
		cClrBits = (WORD)(bmp.bmPlanes * bmp.bmBitsPixel); 
		if (cClrBits == 1) 
			cClrBits = 1; 
		else if (cClrBits <= 4) 
			cClrBits = 4; 
		else if (cClrBits <= 8) 
			cClrBits = 8; 
		else if (cClrBits <= 16) 
			cClrBits = 16; 
		else if (cClrBits <= 24) 
			cClrBits = 24; 
		else cClrBits = 32; 

		// Allocate memory for the BITMAPINFO structure. (This structure 
		// contains a BITMAPINFOHEADER structure and an array of RGBQUAD 
		// data structures.) 

		if (cClrBits != 24) 
			pbmi = (PBITMAPINFO) LocalAlloc(LPTR, 
			sizeof(BITMAPINFOHEADER) + 
			sizeof(RGBQUAD) * (1<< cClrBits)); 

		// There is no RGBQUAD array for the 24-bit-per-pixel format. 

		else 
			pbmi = (PBITMAPINFO) LocalAlloc(LPTR, 
			sizeof(BITMAPINFOHEADER)); 

		// Initialize the fields in the BITMAPINFO structure. 

		pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER); 
		pbmi->bmiHeader.biWidth = bmp.bmWidth; 
		pbmi->bmiHeader.biHeight = bmp.bmHeight; 
		pbmi->bmiHeader.biPlanes = bmp.bmPlanes; 
		pbmi->bmiHeader.biBitCount = bmp.bmBitsPixel; 
		if (cClrBits < 24) 
			pbmi->bmiHeader.biClrUsed = (1<<cClrBits); 

		// If the bitmap is not compressed, set the BI_RGB flag. 
		pbmi->bmiHeader.biCompression = BI_RGB; 

		// Compute the number of bytes in the array of color 
		// indices and store the result in biSizeImage. 
		// For Windows NT/2000, the width must be DWORD aligned unless 
		// the bitmap is RLE compressed. This example shows this. 
		// For Windows 95/98, the width must be WORD aligned unless the 
		// bitmap is RLE compressed.
		pbmi->bmiHeader.biSizeImage = ((pbmi->bmiHeader.biWidth * cClrBits +31) & ~31) /8
			* pbmi->bmiHeader.biHeight; 
		// Set biClrImportant to 0, indicating that all of the 
		// device colors are important. 
		pbmi->bmiHeader.biClrImportant = 0; 
		return pbmi; 
	} 


	static int CreateBMPFile(LPSTR pszFile, PBITMAPINFO pbi, HBITMAP hBMP, HDC hDC) 
	{ 
		HANDLE hf;                 // file handle 
		BITMAPFILEHEADER hdr;       // bitmap file-header 
		PBITMAPINFOHEADER pbih;     // bitmap info-header 
		LPBYTE lpBits;              // memory pointer 
		DWORD dwTotal;              // total count of bytes 
		DWORD cb;                   // incremental count of bytes 
		BYTE *hp;                   // byte pointer 
		DWORD dwTmp; 

		pbih = (PBITMAPINFOHEADER) pbi; 
		lpBits = (LPBYTE) GlobalAlloc(GMEM_FIXED, pbih->biSizeImage);

		if (!lpBits) 
		{ 
			return -1;
		}

		// Retrieve the color table (RGBQUAD array) and the bits 
		// (array of palette indices) from the DIB. 
		if (!GetDIBits(hDC, hBMP, 0, (WORD) pbih->biHeight, lpBits, pbi, 
			DIB_RGB_COLORS)) 
		{ 
			return -1;
		}

		// Create the .BMP file. 
		hf = CreateFileA(pszFile, 
			GENERIC_READ | GENERIC_WRITE, 
			(DWORD) 0, 
			NULL, 
			CREATE_ALWAYS, 
			FILE_ATTRIBUTE_NORMAL, 
			(HANDLE) NULL); 
		if (hf == INVALID_HANDLE_VALUE) 
		{ 
			return -1;
		}

		hdr.bfType = 0x4d42;        // 0x42 = "B" 0x4d = "M" 
		// Compute the size of the entire file. 
		hdr.bfSize = (DWORD) (sizeof(BITMAPFILEHEADER) + 
			pbih->biSize + pbih->biClrUsed 
			* sizeof(RGBQUAD) + pbih->biSizeImage); 
		hdr.bfReserved1 = 0; 
		hdr.bfReserved2 = 0; 

		// Compute the offset to the array of color indices. 
		hdr.bfOffBits = (DWORD) sizeof(BITMAPFILEHEADER) + 
			pbih->biSize + pbih->biClrUsed 
			* sizeof (RGBQUAD); 

		// Copy the BITMAPFILEHEADER into the .BMP file. 
		if (!WriteFile(hf, (LPVOID) &hdr, sizeof(BITMAPFILEHEADER), (LPDWORD) &dwTmp,  NULL)) 
		{ 
			return -1;
		}

		// Copy the BITMAPINFOHEADER and RGBQUAD array into the file. 
		if (!WriteFile(hf, (LPVOID)pbih, sizeof(BITMAPINFOHEADER) + pbih->biClrUsed * sizeof(RGBQUAD), (LPDWORD)&dwTmp, NULL))
		{ 
			return -1;
		}

		// Copy the array of color indices into the .BMP file. 
		dwTotal = cb = pbih->biSizeImage; 
		hp = lpBits; 
		if (!WriteFile(hf, (LPSTR) hp, (int) cb, (LPDWORD) &dwTmp,NULL)) 
		{ 
			return -1;
		}

		// Close the .BMP file. 
		if (!CloseHandle(hf)) 
		{ 
			return -1;
		}

		// Free memory. 
		GlobalFree((HGLOBAL)lpBits);
		return 0;
	}

	// FetchAvatar(): Fetch avatar file from server (Using URLDownloadToFile() from urlmon.dll)
	// hContact: (HANDLE) Contact Handle
	void __cdecl CNetwork::FetchAvatar(void *data) {
		HANDLE hContact=(HANDLE)data;
		unsigned int uid=READC_D2(UNIQUEIDSETTING);
		PROTO_AVATAR_INFORMATION ai={sizeof(ai),hContact,PA_FORMAT_GIF};
		char szURL[MAX_PATH];

		// Zzz...
		CallService(MS_DB_GETPROFILEPATH,MAX_PATH,(LPARAM)ai.filename);
		Sleep(1000);

		if (READ_B2(NULL,QQ_AVATARTYPE)==2) { // Get QQ Show
			// QQ Show
			sprintf(ai.filename+strlen(ai.filename),"\\QQ\\%d.gif",uid);

			sprintf(szURL,"http://qqshow-user.tencent.com/%d/10/00/00.gif",uid);
			util_log(0,"%s(): Fetching %s",__FUNCTION__,szURL);

			NETLIBHTTPREQUEST nlhr={sizeof(nlhr),REQUEST_GET,NLHRF_GENERATEHOST,szURL};

			if (NETLIBHTTPREQUEST* nlhrr=(NETLIBHTTPREQUEST*)CallService(MS_NETLIB_HTTPTRANSACTION,(WPARAM)hNetlibUser,(LPARAM)&nlhr)) {
				HANDLE hFile=CreateFileA(ai.filename,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,0,NULL);
				if (hFile==INVALID_HANDLE_VALUE) {
					util_log(1,"%s(): Avatar for %d failed to save",__FUNCTION__,uid);
					ProtoBroadcastAck(m_szModuleName, hContact, ACKTYPE_AVATAR, ACKRESULT_FAILED, (HANDLE)&ai, (LPARAM)0);
				} else {
					DWORD dwWritten;
					WriteFile(hFile,nlhrr->pData,nlhrr->dataLength,&dwWritten,NULL);
					CloseHandle(hFile);
					util_log(1,"%s(): Avatar for %d downloaded and saved to %s",__FUNCTION__,uid,ai.filename);
					ProtoBroadcastAck(m_szModuleName, hContact, ACKTYPE_AVATAR, ACKRESULT_SUCCESS, (HANDLE)&ai, (LPARAM)0);
				}
				CallService(MS_NETLIB_FREEHTTPREQUESTSTRUCT,(WPARAM)nlhrr,0);
			} else {
				util_log(1,"%s(): Avatar for %d failed to download",__FUNCTION__,uid);
				ProtoBroadcastAck(m_szModuleName, hContact, ACKTYPE_AVATAR, ACKRESULT_FAILED, (HANDLE)&ai, (LPARAM)0);
			}
		} else { // Get Head Image
			int nTemp;
			/*
			HRSRC hRsrc;
			HGLOBAL hgRsc;
			char* pszRsc;
			DWORD rscSize;
			*/
			HINSTANCE hInst;
			char szPluginPath[MAX_PATH];

			// Head Image
			ai.format=PA_FORMAT_BMP;

			nTemp=READC_W2("Face");
			if (!nTemp)
				nTemp=1;
			else
				nTemp=nTemp/3 + 1;

			sprintf(ai.filename+strlen(ai.filename),"\\QQ\\head%d.bmp",nTemp);

			CallService(MS_UTILS_PATHTOABSOLUTE,(WPARAM)"Plugins\\QQHeadImg.dll",(LPARAM)szPluginPath);
			hInst=LoadLibraryA(szPluginPath);

			if (hInst) { // QQHeadImg.dll successfully loaded
#if 0
				hRsrc=FindResource(hInst,MAKEINTRESOURCE(100+nTemp),RT_BITMAP);
				if (hRsrc) { // Resource found
					BITMAPFILEHEADER bfh={'MB',2102,0,0,1078};

					rscSize=SizeofResource(hInst,hRsrc);
					hgRsc=LoadResource(hInst,hRsrc);
					pszRsc=(char*)LockResource(hgRsc);

					HANDLE hFile=CreateFileA(ai.filename,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,0,NULL);
					if (hFile==INVALID_HANDLE_VALUE)
						ProtoBroadcastAck(m_szModuleName, (HANDLE)hContact, ACKTYPE_AVATAR, ACKRESULT_FAILED, (HANDLE)&ai, (LPARAM)0);
					else {
						DWORD dwWritten;
						WriteFile(hFile,&bfh,sizeof(BITMAPFILEHEADER),&dwWritten,NULL);
						WriteFile(hFile,pszRsc,rscSize,&dwWritten,NULL);
						CloseHandle(hFile);
						ProtoBroadcastAck(m_szModuleName, hContact, ACKTYPE_AVATAR, ACKRESULT_SUCCESS, (HANDLE)&ai, (LPARAM)0);
					}
				} else // Resource not found
					ProtoBroadcastAck(m_szModuleName, hContact, ACKTYPE_AVATAR, ACKRESULT_FAILED, (HANDLE)&ai, (LPARAM)0);
#endif

				if (HBITMAP hBmp=LoadBitmap(hInst,MAKEINTRESOURCE(100+nTemp))) {
					HDC hdc = GetDC(NULL);
					PBITMAPINFO info = CreateBitmapInfoStruct(hBmp);
					CreateBMPFile(ai.filename, info, hBmp, hdc);
					ReleaseDC(NULL, hdc);
					DeleteObject(hBmp);
					ProtoBroadcastAck(m_szModuleName, hContact, ACKTYPE_AVATAR, ACKRESULT_SUCCESS, (HANDLE)&ai, (LPARAM)0);
				} else
					ProtoBroadcastAck(m_szModuleName, hContact, ACKTYPE_AVATAR, ACKRESULT_FAILED, (HANDLE)&ai, (LPARAM)0);

				FreeLibrary(hInst);
			} else // QQHeadImg.dll failed to load
				ProtoBroadcastAck(m_szModuleName, hContact, ACKTYPE_AVATAR, ACKRESULT_FAILED, (HANDLE)&ai, (LPARAM)0);
		}

		WRITEC_D("AvatarUpdateTS",READ_D2(NULL,"LoginTS"));
		DELC("QQShowFetching");
	}

#if 0
	void __cdecl CNetwork::FetchQunAvatar(LPVOID data) {
		HANDLE hContact=(HANDLE)data;

		PROTO_AVATAR_INFORMATION ai={sizeof(ai),(HANDLE)hContact,PA_FORMAT_JPEG};
		if (int extid=READC_D2("ExternalID")) {
			NETLIBHTTPREQUEST nlhr={sizeof(nlhr),REQUEST_GET,NLHRF_GENERATEHOST};

			char szUrl[MAX_PATH]="http://search.group.qq.com/cgi-bin/key_search?select=1&begin=0&num=10&key=";
			nlhr.szUrl=szUrl;

			itoa(extid,szUrl+strlen(szUrl),10);

			if (NETLIBHTTPREQUEST* nlhrr=(NETLIBHTTPREQUEST*)CallService(MS_NETLIB_HTTPTRANSACTION,(WPARAM)hNetlibUser,(LPARAM)&nlhr)) {
				if (LPSTR pszStart=strstr(nlhrr->pData,"<td width=\"60\"")) {
					pszStart=strstr(pszStart,"<img src")+9;
					*strchr(pszStart,' ')=0;
					strcpy(szUrl,pszStart);
					CallService(MS_NETLIB_FREEHTTPREQUESTSTRUCT,(WPARAM)nlhrr,0);
					nlhrr=(NETLIBHTTPREQUEST*)CallService(MS_NETLIB_HTTPTRANSACTION,(WPARAM)hNetlibUser,(LPARAM)&nlhr);

					CallService(MS_UTILS_PATHTOABSOLUTE,(WPARAM)"QQ\\",(LPARAM)szUrl);
					itoa(READC_D2(UNIQUEIDSETTING),szUrl+strlen(szUrl),10);
					strcat(szUrl,".jpg");

					HANDLE hFile=CreateFileA(szUrl,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,0,NULL);
					if (hFile==INVALID_HANDLE_VALUE) {
						ProtoBroadcastAck(m_szModuleName, (HANDLE)hContact, ACKTYPE_AVATAR, ACKRESULT_FAILED, (HANDLE)&ai, (LPARAM)0);
					} else {
						DWORD dwWritten;
						WriteFile(hFile,nlhrr->pData,nlhrr->dataLength,&dwWritten,NULL);
						CloseHandle(hFile);
						CallService(MS_NETLIB_FREEHTTPREQUESTSTRUCT,(WPARAM)nlhrr,0);
						ProtoBroadcastAck(m_szModuleName, hContact, ACKTYPE_AVATAR, ACKRESULT_SUCCESS, (HANDLE)&ai, (LPARAM)0);
					}
				} else
					ProtoBroadcastAck(m_szModuleName, hContact, ACKTYPE_AVATAR, ACKRESULT_FAILED, (HANDLE)&ai, (LPARAM)0);
				CallService(MS_NETLIB_FREEHTTPREQUESTSTRUCT,(WPARAM)nlhrr,0);
			} else {
				util_log(0,"MS_NETLIB_HTTPTRANSACTION failed. GetLastError=%d\n",GetLastError());
				ProtoBroadcastAck(m_szModuleName, hContact, ACKTYPE_AVATAR, ACKRESULT_FAILED, (HANDLE)&ai, (LPARAM)0);
			}
		} else
			ProtoBroadcastAck(m_szModuleName, hContact, ACKTYPE_AVATAR, ACKRESULT_FAILED, (HANDLE)&ai, (LPARAM)0);
	}
#endif

	// GetAvatarInfo(): Download Avatar
	// wParam: N/A
	// lParam: (PROTO_AVATAR_INFORMATION*) Avatar Information (Bi-directional)
	// Return: Always 0
	MIMPROC(GetAvatarInfo) {
		if (Packet::isClientKeySet() && READ_B2(NULL,QQ_AVATARTYPE)!=3) { // Avatar type not set to "None"
			PROTO_AVATAR_INFORMATION* AI = (PROTO_AVATAR_INFORMATION*)lParam;
			unsigned int uid=DBGetContactSettingDword(AI->hContact, m_szModuleName, UNIQUEIDSETTING, 0);
			
			util_log(0,"GetAvatarInfo(): %d",uid);

			// CallService(MS_UTILS_PATHTOABSOLUTE,(WPARAM)"QQ\\",(LPARAM)AI->filename);
			FoldersGetCustomPath(m_avatarFolder,AI->filename,MAX_PATH,"QQ");
			strcat(AI->filename,"\\");

			if (DBGetContactSettingByte(AI->hContact,m_szModuleName,"IsQun",0)) { // Qun can have avatar? :) Yes Now!
#if 0
				itoa(uid,AI->filename+strlen(AI->filename),10);
				strcat(AI->filename,".jpg");
				if (GetFileAttributesA(AI->filename)==INVALID_FILE_ATTRIBUTES) {
					// File not exists
					ForkThread((ThreadFunc)&CNetwork::FetchQunAvatar,AI->hContact);
					return GAIR_WAITFOR;
				} else {
					AI->format=PA_FORMAT_JPEG;
					return GAIR_SUCCESS;
				}
#else
				// Tencent changes the way to get qun image again :(
				return GAIR_NOAVATAR;
#endif
			} else if (uid) { // UID valid and 
				util_log(0,"%s(): For: %d", __FUNCTION__, uid);
			}else { // UID invalid
				util_log(98,"%s(): Passed with invalid hContact!",__FUNCTION__);
				return GAIR_NOAVATAR;
			}

			// Create QQ avatar directory if needed
			if (GetFileAttributesA(AI->filename)==INVALID_FILE_ATTRIBUTES) {
				util_log(0,"%s(): Creating QQ avatar directory %s",__FUNCTION__,AI->filename);
				CreateDirectoryA(AI->filename,NULL);
			}

			if (READ_B2(NULL,QQ_AVATARTYPE)==2) { // QQ Show
				sprintf(AI->filename+strlen(AI->filename),"\\%d.gif",uid);
				AI->format=PA_FORMAT_GIF;
			} else { // Head Image
				DBVARIANT dbv;
				AI->format=PA_FORMAT_BMP;

				if (!DBGetContactSetting(AI->hContact,m_szModuleName,"UserHeadMD5",&dbv)) {
					sprintf(AI->filename+strlen(AI->filename),"\\%s.bmp",dbv.pszVal);
					if (GetFileAttributesA(AI->filename)!=INVALID_FILE_ATTRIBUTES) {
						// File exists
						return GAIR_SUCCESS;
					} else
						return GAIR_WAITFOR;
				}
				sprintf(AI->filename+strlen(AI->filename),"\\~.bmp",uid); // Disable existence checking
			}

			util_log(0,"%s(): Trying to open %s",__FUNCTION__,AI->filename);
			if (DBGetContactSettingDword(AI->hContact,m_szModuleName,"AvatarUpdateTS",1)==READ_D2(NULL,"LoginTS") && GetFileAttributesA(AI->filename)!=INVALID_FILE_ATTRIBUTES) {
				// Avatar file already exists
				util_log(0,"%s(): Avatar exists, return GAIR_SUCCESS",__FUNCTION__);
				return GAIR_SUCCESS;
			} else {
				if (DBGetContactSettingByte(AI->hContact,m_szModuleName,"QQShowFetching",0)==0) {
					ForkThread((ThreadFunc)&CNetwork::FetchAvatar,AI->hContact);
				}
				util_log(0,"%s(): QQShow, return GAIR_SUCCESS",__FUNCTION__);
				return GAIR_WAITFOR;
			}		
		} else { // Avatar type set to "None"
			util_log(0,"%s(): return GAIR_NOAVATAR",__FUNCTION__);
			return GAIR_NOAVATAR;
		}
	}

	MIMPROC(GetWeather) {
		if (!Packet::isClientKeySet())
			MessageBox(NULL,TranslateT("You are not connected to QQ Network!"),NULL,MB_ICONERROR);
		else {
			EvaIPAddress eia(READ_D2(NULL,"IP"));
			append(new WeatherOpPacket(htonl(eia.IP())));
		}
		return 0;
	}

	// CopyMyIP(): Show and Copy my IP to clipboard
	// wParam: N/A
	// lParam: N/A
	// Return: Always 0
	MIMPROC(CopyMyIP) {
		// TODO: Somebody reflects that this function can lead to clipboard dies until Miranda is closed
		if (Packet::isClientKeySet())
			MessageBox(NULL,TranslateT("You are not connected to QQ Network!"),NULL,MB_ICONERROR);
		else {
			TCHAR szMsg[MAX_PATH];
			LPSTR  lptstrCopy;
			HGLOBAL hglbCopy;
			EvaIPAddress eip(READ_D2(NULL,"IP"));

			_stprintf(szMsg,TranslateT("Your public IP Address is %s. This address has been copied to your clipboard."),eip.toString().c_str());

			if (!OpenClipboard(NULL)) { // Failed opening clipboard
				MessageBox(NULL,TranslateT("Failed to open clipboard!"),NULL,MB_ICONERROR);
				return FALSE; 
			}
			EmptyClipboard(); 

			hglbCopy = GlobalAlloc(GMEM_MOVEABLE, strlen(eip.toString().c_str()) + 1); 
			if (hglbCopy == NULL) // Memory allocation failed
			{ 
				CloseClipboard(); 
				return FALSE; 
			} 

			// Lock the handle and copy the text to the buffer. 

			lptstrCopy = (LPSTR)GlobalLock(hglbCopy); 
			strcpy(lptstrCopy,eip.toString().c_str()); 
			GlobalUnlock(hglbCopy); 

			// Place the handle on the clipboard. 
			SetClipboardData(CF_TEXT, hglbCopy); 

			CloseClipboard();

			MessageBox(NULL,szMsg,TranslateT("My IP Address"),MB_ICONINFORMATION);
		}
		return 0;
	}

	// DownloadGroup(): System Menu Item to Download Group information
	// wParam: N/A
	// lParam: N/A
	// Return: Always 0
	MIMPROC(DownloadGroup) {
		if (Packet::isClientKeySet()) {
			m_downloadGroup=true;
			m_qqusers=0;
			m_hGroupList.clear();
			append(new GroupNameOpPacket(QQ_DOWNLOAD_GROUP_NAME));
		}
		return 0;
	}

	MIMPROC(UploadGroup) {
		GroupNameOpPacket* packet=new GroupNameOpPacket(QQ_UPLOAD_GROUP_NAME);
		HANDLE hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, (WPARAM)NULL, (LPARAM)NULL);
		DBVARIANT dbv;
		list<wstring> groupnames;
		list<wstring>::iterator iter;
		int index;
		m_downloadGroup=true;
		m_qqusers=0;
		m_hGroupList.clear();

		while (hContact) {
			if (!lstrcmpA(m_szModuleName, (LPSTR)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact,(LPARAM)NULL)) && READ_B2(hContact,"IsQun")==0 && READC_D2(UNIQUEIDSETTING)!=0) {
				index=0;
				if (!DBGetContactSettingTString(hContact,"CList","Group",&dbv)) {
					for (iter=groupnames.begin(); iter!=groupnames.end(); iter++) {
						index++;
						if (!wcscmp(iter->c_str(),dbv.ptszVal)) {
							DBFreeVariant(&dbv);
							break;
						}
					}
					if (iter==groupnames.end()) {
						util_log(0,"UploadGroup(): Add %S to list",dbv.ptszVal);
						groupnames.push_back(dbv.ptszVal);
						index++;
						DBFreeVariant(&dbv);
					}
				}
				m_hGroupList[READC_D2(UNIQUEIDSETTING)]=(HANDLE)index;
			}

			hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, (LPARAM)NULL);
		}

		list<string> sendnames;
		LPSTR pszTemp;
		sendnames.push_back("");
		for (iter=groupnames.begin(); iter!=groupnames.end(); iter++) {
			pszTemp=mir_u2a_cp(iter->c_str(),936);
			sendnames.push_back(pszTemp);
			mir_free(pszTemp);
		}
		packet->setGroups(sendnames);
		append(packet);
		m_qqusers=sendnames.size()-1;
		return 0;
	}

	void ChangeNick(CNetwork* network, LPTSTR szNewNick) {
		LPSTR m_szModuleName=network->m_szModuleName;
		char szSignature[MAX_PATH];
		LPSTR pszTemp;

		WRITE_TS(NULL,"Nick",szNewNick);

		ContactInfo ci;
		ModifyInfoPacket* packet;
		stringList details;
		char szID[16];
		DBVARIANT dbv;
		int value;

		itoa(network->GetMyQQ(),szID,10);
		details.push_back(szID); // 0
		pszTemp=mir_u2a_cp(szNewNick,936);
		details.push_back(pszTemp);
		mir_free(pszTemp);

#define READINFO_S(x) READ_S(NULL,x,szSignature); util_convertToGBK(szSignature); details.push_back(szSignature)
#define READINFO_W(x) READ_W(NULL,x,value); itoa(value,szSignature,10); details.push_back(szSignature)
#define READINFO_B(x) READ_B(NULL,x,value); itoa(value,szSignature,10); details.push_back(szSignature)

		READINFO_S("Country");
		READINFO_S("Province");
		READINFO_S("ZIP");
		READINFO_S("Address"); // 5
		READINFO_S("Telephone");
		READINFO_W("Age");

		READ_B(NULL,"Gender",value);
		if (value=='M')
			details.push_back("\xC4\xD0");
		else if (value=='F')
			details.push_back("\xC5\xAE");
		else
			details.push_back("");

		READINFO_S("FirstName");
		READINFO_S("Email"); // 10
		READINFO_S("PagerSN");
		READINFO_S("PagerNum");
		READINFO_S("PagerSP");
		READINFO_W("PagerBaseNum");
		READINFO_B("PagerType"); // 15
		READINFO_B("Occupation");
		READINFO_S("Homepage");
		READINFO_B("AuthType");
		READINFO_S("Unknown1");
		READINFO_S("Unknown2"); // 20
		READINFO_W("Face");
		READINFO_S("Mobile");
		READINFO_B("MobileType");
		READINFO_S("About");
		READINFO_S("City"); // 25
		READINFO_S("Unknown3");
		READINFO_S("Unknown4");
		READINFO_S("Unknown5");
		READINFO_B("OpenHP");
		READINFO_B("OpenContact"); //30
		READINFO_S("College");
		READINFO_B("Horoscope");
		READINFO_B("Zodiac");
		READINFO_B("Blood");
		READINFO_W("QQShow"); // 35
		READINFO_S("Unknown6");

		ci.setDetails(details);
		packet=new ModifyInfoPacket(ci);
		network->append(packet);

	}

	MIMPROC(SetMyNickname) {
		// Nick is at lParam
		LPTSTR pszNick=NULL;
		if (wParam==0) pszNick=mir_a2u((LPSTR)lParam);

		ChangeNick(this, pszNick?pszNick:(LPTSTR)lParam);
		if (pszNick) mir_free(pszNick);
		return 0;
	}

	//static ASKDLGPARAMS* currentAskDlgParams=NULL;

	// AddQunMemberProcOpts(): Window Message Handler for Add Qun Member Dialog
	// hwndDlg: Window handle to dialog
	// msg: Window message received
	// wParam: Determined by msg
	// lParam: Determined by msg
	// Return: true if the message has been handled and should not sent to parent, false otherwise
	BOOL CALLBACK ModifySignatureDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
		ASKDLGPARAMS* currentAskDlgParams=(ASKDLGPARAMS*)GetWindowLong(hwndDlg,GWL_USERDATA);
		switch (msg) {
			case WM_INITDIALOG: // Dialog initialization, lParam=hContact
				{
					DBVARIANT dbv;

					TranslateDialogDefault(hwndDlg);
					currentAskDlgParams=(ASKDLGPARAMS*)lParam;
					SetWindowLong(hwndDlg,GWL_USERDATA,lParam);
					currentAskDlgParams->network->m_hwndModifySignatureDlg=hwndDlg;

					switch (currentAskDlgParams->command) {
						case QQ_CMD_SIGNATURE_OP:
							SetWindowText(hwndDlg,TranslateT("Change Signature"));
							SetDlgItemText(hwndDlg,IDC_CAPTION,TranslateT("Enter new Personal Signature:"));
							if (!DBGetContactSettingTString(NULL,currentAskDlgParams->network->m_szModuleName,"PersonalSignature",&dbv)) {
								SetDlgItemText(hwndDlg,IDC_SIGNATURE,dbv.ptszVal);
								DBFreeVariant(&dbv);
							}
							break;
						case QQ_CMD_MODIFY_INFO:
							SetWindowText(hwndDlg,TranslateT("Change Nickname"));
							SetDlgItemText(hwndDlg,IDC_CAPTION,TranslateT("Enter new Nickname:"));
							if (!DBGetContactSettingTString(NULL,currentAskDlgParams->network->m_szModuleName,"Nick",&dbv)) {
								SetDlgItemText(hwndDlg,IDC_SIGNATURE,dbv.ptszVal);
								DBFreeVariant(&dbv);
							}
							break;
						case QQ_QUN_CMD_MODIFY_CARD:
							{
								char szID[16];
								itoa(currentAskDlgParams->network->GetMyQQ(),szID,10);
								SetWindowText(hwndDlg,TranslateT("Change Qun Card Name"));
								SetDlgItemText(hwndDlg,IDC_CAPTION,TranslateT("Enter new Qun Card Name:"));
								if (!DBGetContactSetting(currentAskDlgParams->hContact,currentAskDlgParams->network->m_szModuleName,szID,&dbv)) {
									LPTSTR pszUnicode=mir_a2u_cp(dbv.pszVal,936);
									//util_convertToNative(&pszUnicode,dbv.pszVal);
									SetDlgItemText(hwndDlg,IDC_SIGNATURE,pszUnicode);
									DBFreeVariant(&dbv);
									mir_free(pszUnicode);
								}
							}
							break;
						case QQ_QUN_CMD_JOIN_QUN_AUTH:
						case AUTH_INFO_SUB_CMD_QUN:
							SetWindowText(hwndDlg,TranslateT("Reauthorize"));
							SetDlgItemText(hwndDlg,IDC_CAPTION,TranslateT("The qun needs authorization, please enter your reason:"));
							break;
						case QQ_CMD_ADD_FRIEND_AUTH:
						case AUTH_INFO_SUB_CMD_USER:
							SetWindowText(hwndDlg,TranslateT("Reauthorize"));
							SetDlgItemText(hwndDlg,IDC_CAPTION,TranslateT("The user needs authorization, please enter your reason:"));
							break;
						case QQ_CMD_LOGIN:
							SetWindowText(hwndDlg,TranslateT("Enter Password"));
							SetDlgItemText(hwndDlg,IDC_CAPTION,TranslateT("Please enter your QQ login password:"));
							SendDlgItemMessage(hwndDlg,IDC_SIGNATURE,EM_SETPASSWORDCHAR,(WPARAM)_T('*'),NULL);
							break;
						default:
							MessageBox(NULL,L"ASSERT: ModifySignatureDlgProc with unknown command, please report to developer.",currentAskDlgParams->network->m_tszUserName,MB_ICONERROR|MB_APPLMODAL);
							break;
					}
				}
				break;
			case WM_COMMAND: // Button pressed
				{
					switch (LOWORD(wParam)) {
						case IDOK: // OK Button
							{
								TCHAR szSignature[MAX_PATH]={0};
								GetDlgItemText(hwndDlg,IDC_SIGNATURE,szSignature,MAX_PATH);

								switch (currentAskDlgParams->command) {
									case QQ_CMD_SIGNATURE_OP:
										{
											if (!*szSignature) {
												// Remove Signature
												currentAskDlgParams->network->append(new SignaturePacket(QQ_SIGNATURE_DELETE));
											} else {
												// Change Signature
												SignaturePacket *packet = new SignaturePacket(QQ_SIGNATURE_MODIFY);
												LPSTR pszTemp=mir_u2a_cp(szSignature,936);
												packet->setSignature(pszTemp);
												mir_free(pszTemp);
												currentAskDlgParams->network->append(packet);
											}
										}
										break;
									case QQ_CMD_MODIFY_INFO:
										{
											if (!*szSignature) {
												// Field emptied
												MessageBox(hwndDlg,TranslateT("You cannot set an empty nickname."),NULL,MB_ICONERROR);
											} else {
												// Change Nickname
												ChangeNick(currentAskDlgParams->network,szSignature);
											}
										}
										break;
									case QQ_QUN_CMD_MODIFY_CARD:
										{
											// Change Qun Card Name
											QunModifyCardPacket* packet;
											DBVARIANT dbv;
											int qunID=0;

											qunID=DBGetContactSettingDword(currentAskDlgParams->hContact,currentAskDlgParams->network->m_szModuleName,UNIQUEIDSETTING,0);

											if (qunID==0) {
												if (!DBGetContactSetting(currentAskDlgParams->hContact,currentAskDlgParams->network->m_szModuleName,"ChatRoomID",&dbv)) {
													qunID=atoi(strchr(dbv.pszVal,'.')+1);
													DBFreeVariant(&dbv);
												}
											}

											if (qunID==0) {
												MessageBox(hwndDlg,TranslateT("Unexpected Exception: Failed to obtain QunID!"),NULL,MB_ICONERROR);
											} else {
												LPSTR pszTemp=mir_u2a_cp(szSignature,936);
												packet=new QunModifyCardPacket(qunID,currentAskDlgParams->network->GetMyQQ());
												packet->setName(pszTemp);
												mir_free(pszTemp);
												currentAskDlgParams->network->append(packet);
											}
										}
										break;
									case QQ_CMD_ADD_FRIEND_AUTH:
									case AUTH_INFO_SUB_CMD_USER:
										{
											if (!*szSignature) {
												// Field emptied
												MessageBox(hwndDlg,TranslateT("You must enter a reason."),NULL,MB_ICONERROR);
											} else {
												LPSTR pszTemp=mir_u2a_cp(szSignature,936);
												int qqid=DBGetContactSettingDword(currentAskDlgParams->hContact,currentAskDlgParams->network->m_szModuleName,UNIQUEIDSETTING,0);
												AddFriendAuthPacket* packet=new AddFriendAuthPacket(qqid,QQ_MY_AUTH_REQUEST);
												packet->setMessage(pszTemp);
												mir_free(pszTemp);
												currentAskDlgParams->network->append(packet);
											}
										}
										break;
									case QQ_QUN_CMD_JOIN_QUN_AUTH:
									case AUTH_INFO_SUB_CMD_QUN:
										{
											if (!*szSignature) {
												// Field emptied
												MessageBox(hwndDlg,TranslateT("You must enter a reason."),NULL,MB_ICONERROR);
											} else {
												HANDLE hContact=currentAskDlgParams->hContact;
												DBWriteContactSettingTString(hContact,currentAskDlgParams->network->m_szModuleName,"AuthReason",szSignature);

												if (currentAskDlgParams->fAux) {
													CodeVerifyWindow* win=new CodeVerifyWindow(currentAskDlgParams);
													delete win;
												} else {
													LPSTR pszTemp=mir_u2a_cp(szSignature,936);

													QunAuthPacket* packet=new QunAuthPacket(currentAskDlgParams->network->m_addUID,QQ_QUN_AUTH_REQUEST);
													packet->setCode((unsigned char*)currentAskDlgParams->pAux,currentAskDlgParams->nAux);
													packet->setMessage(pszTemp);
													mir_free(pszTemp);
													currentAskDlgParams->network->append(packet);
												}
											}
										}
										break;
									case QQ_CMD_LOGIN:
										{
											if (!*szSignature) {
												// Field emptied
												MessageBox(hwndDlg,TranslateT("You must enter a password."),NULL,MB_ICONERROR);
											} else {
												char* szSignature2=mir_u2a_cp(szSignature,936);
												//util_convertFromNative(&szSignature2,szSignature);
												CallService(MS_DB_CRYPT_ENCODESTRING,sizeof(szSignature2),(LPARAM)szSignature2);
												DBWriteContactSettingString(NULL,currentAskDlgParams->network->m_szModuleName,QQ_PASSWORD,szSignature2);

												CHAR szTemp[MAX_PATH];
												strcpy(szTemp,currentAskDlgParams->network->m_szModuleName);
												strcat(szTemp,"/RemovePassword");
												CallService(MS_DB_SETSETTINGRESIDENT,TRUE,(LPARAM)szTemp);
												DBWriteContactSettingByte(NULL,currentAskDlgParams->network->m_szModuleName,"RemovePassword",1);
												//currentAskDlgParams->network->SetStatus((WPARAM)currentAskDlgParams->hContact);
												CallProtoService(currentAskDlgParams->network->m_szModuleName,PS_SETSTATUS,(WPARAM)currentAskDlgParams->hContact,0);
												mir_free(szSignature2);
											}
										}
								}
								EndDialog(hwndDlg,0);
							}
							break;
						case IDCANCEL: // Cancel Button
							EndDialog(hwndDlg,0);
							break;
					}
				}
				break;
			case WM_DESTROY:
				if (currentAskDlgParams->command==QQ_QUN_CMD_JOIN_QUN_AUTH) mir_free(currentAskDlgParams->pAux);
				free(currentAskDlgParams);
				break;
		}
		return false;
	}

	MIMPROC(ModifySignature) {
		ASKDLGPARAMS* adp=(ASKDLGPARAMS*)malloc(sizeof(ASKDLGPARAMS));
		adp->command=QQ_CMD_SIGNATURE_OP;
		adp->hContact=NULL;
		adp->network=this;
		DialogBoxParam(hinstance,MAKEINTRESOURCE(IDD_CHANGESIGNATURE),NULL,ModifySignatureDlgProc,(LPARAM)adp);
		return 0;
	}

	MIMPROC(ChangeNickname) {
		ASKDLGPARAMS* adp=(ASKDLGPARAMS*)malloc(sizeof(ASKDLGPARAMS));
		adp->command=QQ_CMD_MODIFY_INFO;
		adp->hContact=NULL;
		adp->network=this;
		DialogBoxParam(hinstance,MAKEINTRESOURCE(IDD_CHANGESIGNATURE),NULL,ModifySignatureDlgProc,(LPARAM)adp);
		return 0;
	}

	MIMPROC(ChangeCardName) {
		ASKDLGPARAMS* adp=(ASKDLGPARAMS*)malloc(sizeof(ASKDLGPARAMS));
		adp->command=QQ_QUN_CMD_MODIFY_CARD;
		adp->hContact=(HANDLE)wParam;
		adp->network=this;
		DialogBoxParam(hinstance,MAKEINTRESOURCE(IDD_CHANGESIGNATURE),NULL,ModifySignatureDlgProc,(LPARAM)adp);
		return 0;
	}

	MIMPROC(Reauthorize) {
		HANDLE hContact=(HANDLE)wParam;

		if (int id=READC_D2(UNIQUEIDSETTING)) {
			WRITEC_B("Reauthorize",1);
			m_addUID=id;
			append(new AddFriendPacket(id));
		}

		return 0;
	}

	// SilentQun(): System Menu Item for remove contacts not in server list
	// wParam: hContact
	// lParam: N/A
	// Return: Always 0
	MIMPROC(RemoveNonServerContacts) {
		if (Packet::isClientKeySet()) {
			HANDLE hContact;
			hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDFIRST,0,0);
			time_t now=(time_t)DBGetContactSettingDword(NULL,m_szModuleName,"LoginTS",0);

			while(hContact!=NULL) {
				if(!lstrcmpA(m_szModuleName,(char*)CallService(MS_PROTO_GETCONTACTBASEPROTO,(WPARAM)hContact,0))) {
					unsigned int uid=DBGetContactSettingDword(hContact,m_szModuleName,UNIQUEIDSETTING,0);
					// I am responsible for this contact, check last update
					if (DBGetContactSettingByte(hContact,m_szModuleName,"ChatRoom",0)==0 && 
						DBGetContactSettingByte(hContact,m_szModuleName,"IsQun",0)==0 &&
						(signed int)(now-DBGetContactSettingDword(hContact,m_szModuleName,"UpdateTS",0))>1) {
							// Buddy not received, remove
							util_log(0,"%s(): Removing non-server contact %d",__FUNCTION__,uid);
							DBDeleteContactSetting(hContact,m_szModuleName,UNIQUEIDSETTING);
							CallService(MS_DB_CONTACT_DELETE,(WPARAM)hContact,0);
						}

				}
				hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDNEXT,(WPARAM)hContact,0);
			}

			MessageBox(NULL,TranslateT("Maintenance Operation Completed."),APPNAME,MB_ICONINFORMATION);
		} else
			MessageBox(NULL,TranslateT("You must be connected to use this function."),APPNAME,MB_ICONINFORMATION);

		return 0;
	}
#if 0 // UPGRADE_DISABLE
#if 0
	// SelectFont(): System Menu Item for select chat font
	// wParam: N/A
	// lParam: N/A
	// Return: Always 0
	MIMPROC2(SelectFont) {
		CHOOSEFONTA cf;            // common dialog box structure
		static LOGFONTA lf={0};    // logical font structure
		HDC hdc;
		int lpsy;

		// Initialize CHOOSEFONT
		ZeroMemory(&cf, sizeof(cf));
		cf.lStructSize = sizeof (cf);
		cf.hwndOwner = NULL;
		cf.lpLogFont = &lf;
		cf.rgbColors = RGB(qqSettings->red,qqSettings->green,qqSettings->blue);
		cf.Flags = CF_SCREENFONTS | CF_EFFECTS | CF_INITTOLOGFONTSTRUCT | CF_FORCEFONTEXIST | CF_NOSCRIPTSEL;

		// Translate font to local encoding
		strcpy(lf.lfFaceName,qqSettings->fontFace);
		util_convertFromGBK(lf.lfFaceName);
		hdc=GetDC(NULL);

		// Translate pixel to pt
		lpsy=GetDeviceCaps(hdc, LOGPIXELSY);
		ReleaseDC(NULL,hdc);

		lf.lfHeight=(LONG)(-1*qqSettings->fontSize*lpsy/72.0);
		lf.lfWeight=qqSettings->bold?FW_BOLD:FW_NORMAL;
		lf.lfItalic=qqSettings->italic;
		lf.lfUnderline=qqSettings->underline;

		if (ChooseFontA(&cf)==TRUE) { // OK Pressed
			strcpy(qqSettings->fontFace,lf.lfFaceName);
			util_convertToGBK(qqSettings->fontFace);

			qqSettings->red=GetRValue(cf.rgbColors);
			qqSettings->green=GetGValue(cf.rgbColors);
			qqSettings->blue=GetBValue(cf.rgbColors);
			qqSettings->bold=lf.lfWeight==FW_BOLD;
			qqSettings->italic=lf.lfItalic==TRUE;
			qqSettings->underline=lf.lfUnderline==TRUE;
			qqSettings->fontSize=cf.iPointSize/10;

			DBWriteContactSettingString(NULL,m_szModuleName,"FontFace",lf.lfFaceName);
			DBWriteContactSettingByte(NULL,m_szModuleName,"FontSize",(unsigned char)qqSettings->fontSize);
			DBWriteContactSettingDword(NULL,m_szModuleName,"MessageColor",RGB(qqSettings->red,qqSettings->green,qqSettings->blue));
			DBWriteContactSettingByte(NULL,m_szModuleName,"MessageFormat",(qqSettings->bold?1:0)+(qqSettings->italic?2:0)+(qqSettings->underline?4:0));
		}
		return 0;
	}

	// SelectColor(): System Menu Item for select chat color
	// wParam: N/A
	// lParam: N/A
	// Return: Always 0
	MIMPROC2(SelectColor) {
		CHOOSECOLOR cc={sizeof(CHOOSECOLOR)};
		static COLORREF acrCustClr[16]; // array of custom colors 

		cc.Flags=CC_RGBINIT | CC_ANYCOLOR | CC_FULLOPEN;
		cc.rgbResult=RGB(qqSettings->red,qqSettings->green,qqSettings->blue);
		cc.lpCustColors = (LPDWORD) acrCustClr;

		if (ChooseColor(&cc)==TRUE) { // OK Pressed
			qqSettings->red=GetRValue(cc.rgbResult);
			qqSettings->green=GetGValue(cc.rgbResult);
			qqSettings->blue=GetBValue(cc.rgbResult);
			DBWriteContactSettingDword(NULL,m_szModuleName,"MessageColor",RGB(qqSettings->red,qqSettings->green,qqSettings->blue));
		}
		return 0;
	}
#endif
#endif // UPGRADE_DISABLE
	MIMPROC(SuppressAddRequests) {
		bool fSuppress=!(bool)DBGetContactSettingByte(NULL,m_szModuleName,QQ_BLOCKEMPTYREQUESTS,0);
		DBWriteContactSettingByte(NULL,m_szModuleName,QQ_BLOCKEMPTYREQUESTS,fSuppress);
		if (fSuppress) {
			MessageBox(NULL,TranslateT("Add Requests are now Blocked."),m_tszUserName,NULL);
		} else {
			MessageBox(NULL,TranslateT("Add Requests are now Allowed."),m_tszUserName,NULL);
		}
		return 0;
	}

	// SuppressQunMessages(): System Menu Item to stop notification by all Quns
	// wParam: N/A
	// lParam: 0 if bubble should be shown, !=0 otherwise
	// Return: Always 0
	MIMPROC(SuppressQunMessages) {
#if 0 // TODO: !
		CLISTMENUITEM mi={0};
		HANDLE hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, (WPARAM)NULL, (LPARAM)NULL);

		qqSettings->SuppressQunMessage=!qqSettings->SuppressQunMessage;

		mi.pszPopupName=m_szModuleName;
		mi.cbSize=sizeof(mi);
		mi.popupPosition=500090000;
		mi.flags=CMIM_NAME;
		mi.position=500090000;
		mi.pszName=qqSettings->SuppressQunMessage?Translate("&Enable Qun Message Receive"):Translate("&Suppress Qun Message Receive");
		CallService( MS_CLIST_MODIFYMENUITEM, 0, (LPARAM)&mi);

		if ((int)lParam==0) {
			// Set status for all Qun contacts
			while (hContact) {
				if (!lstrcmpA(m_szModuleName, (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact,(LPARAM)NULL))) {
					if (DBGetContactSettingByte(hContact,m_szModuleName,"ChatRoom",0)!=0 || DBGetContactSettingByte(hContact,m_szModuleName,"IsQun",0)!=0) {
						DBWriteContactSettingWord(hContact,m_szModuleName,"Status",qqSettings->SuppressQunMessage?ID_STATUS_DND:ID_STATUS_ONLINE);
					}
				}
				hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, (LPARAM)NULL);
			}

			ShowNotification(qqSettings->SuppressQunMessage?Translate("Qun Message Receive is now suppressed. It will be enabled when you send a Qun Message."):Translate("Qun Message Receive is now enabled."),NIIF_INFO);
		}
#endif
		return 0;
	}

	// AddQunMember(): Contact Menu Item to add qun member
	// wParam: (HANDLE) hContact
	// lParam: N/A
	// Return: Always 0
	MIMPROC(AddQunMember) {
		AddQunMember_t* aqmt=new AddQunMember_t();
		aqmt->qunid=0;
		aqmt->hContact=(HANDLE)wParam;
		aqmt->network=this;
		DialogBoxParam(hinstance,MAKEINTRESOURCE(IDD_ADDQUNMEMBER),NULL,AddQunMemberProcOpts,(LPARAM)aqmt);
		return 0;
	}

	// RemoveMe(): Contact Menu Item to remove me from this contact
	// wParam: (HANDLE) hContact
	// lParam: N/A
	// Return: Always 0
	MIMPROC(RemoveMe) {
		if (MessageBox(NULL,TranslateT("You will be removed from his/her contact list, are you sure?"),APPNAME,MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2)==IDYES) {
			unsigned int uid=DBGetContactSettingDword((HANDLE)wParam,m_szModuleName,UNIQUEIDSETTING,0);
			if (Packet::isClientKeySet() && uid) { // UID Valid
				append(new DeleteMePacket(uid));
			}
		}
		return 0;
	}

	// SilentQun(): Contact Menu Item for make this Qun silent
	// wParam: hContact
	// lParam: N/A
	// Return: Always 0
	MIMPROC(SilentQun) {
		if (DBGetContactSettingWord((HANDLE)wParam,m_szModuleName,"Status",ID_STATUS_ONLINE)==ID_STATUS_ONLINE) { // Not currently silent
			// Suppress for this Qun
			DBWriteContactSettingWord((HANDLE)wParam,m_szModuleName,"Status",ID_STATUS_DND);
			DBWriteContactSettingWord((HANDLE)wParam,m_szModuleName,"PrevStatus",ID_STATUS_DND);
		} else { // Currently silent
			// Allow for this Qun
			DBWriteContactSettingWord((HANDLE)wParam,m_szModuleName,"Status",ID_STATUS_ONLINE);
			DBDeleteContactSetting((HANDLE)wParam,m_szModuleName,"PrevStatus");
		}

		return 0;
	}

	void CNetwork::_CopyAndPost(HANDLE hContact, LPCWSTR szFile) {
		char szMD5[16];
		WCHAR szFileOut[MAX_PATH+40];
		LPSTR pszFile=mir_u2a_cp(szFile,936);

		if (!EvaHelper::getFileMD5(pszFile, szMD5)) {
			ForkThread((ThreadFunc)&CNetwork::ThreadMsgBox,mir_tstrdup(TranslateT("Failed sending qun image because the file is inaccessible")));
			mir_free(pszFile);
			return;
		} else {
			// Test file first
			char* pszExt=(char*)strrchr(pszFile,'.')+1;
			if (stricmp(pszExt,"bmp")!=0 && stricmp(pszExt,"jpg") && stricmp(pszExt,"gif")) {
				ForkThread((ThreadFunc)&CNetwork::ThreadMsgBox,mir_tstrdup(TranslateT("Warning! Sending non-GIF nor JPG files to qun can only be received by clients using MirandaQQ.")));
			} 
		}

		HANDLE hFile=CreateFileA(pszFile,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);

		if (GetFileSize(hFile,NULL)>61440) {
			CloseHandle(hFile);
			ForkThread((ThreadFunc)&CNetwork::ThreadMsgBox,mir_tstrdup(TranslateT("You can only send qun images that are less than or equals to 61440 bytes.")));
			return;
		} else {
			CloseHandle(hFile);
		}

		bool fChat=false;
		if (READC_B2("IsQun")==0) { // Chat.dll contact
			DBVARIANT dbv;
			if (!DBGetContactSetting(hContact,m_szModuleName,"ChatRoomID",&dbv)) {
				fChat=true;
				DBFreeVariant(&dbv);
			}
		}

		/*
		CallService(MS_UTILS_PATHTOABSOLUTEW,(WPARAM)L"QQ",(LPARAM)szFileOut);

		if (GetFileAttributes(szFileOut)==INVALID_FILE_ATTRIBUTES) CreateDirectory(szFileOut,NULL);
		wcscat(szFileOut,L"\\QunImages");
		if (GetFileAttributes(szFileOut)==INVALID_FILE_ATTRIBUTES) CreateDirectory(szFileOut,NULL);
		wcscat(szFileOut,L"\\");
		*/

		FoldersGetCustomPathW(CNetwork::m_folders[0],szFileOut,MAX_PATH,L"QQ\\QunImages\\");

		swprintf(szFileOut+wcslen(szFileOut),L"%S",EvaHelper::md5ToString(szMD5).c_str());
		wcscat(szFileOut,wcsrchr(szFile,L'.'));
		wcsupr(szFileOut);
		LPSTR pszFileOut=mir_u2a(szFileOut);

		util_log(0,"Copy %s -> %s",pszFile,pszFileOut);
		CopyFile(szFile,szFileOut,FALSE);

		if (fChat) {
			CallContactService(hContact, PSS_MESSAGE, 0, (LPARAM)((string)(string("[img]")+pszFileOut+"[/img]")).c_str());
		} else {
			string str=(string)(string("[img]")+pszFileOut+"[/img]");

			CallService(MS_MSG_SENDMESSAGE,(WPARAM)hContact,(LPARAM)str.c_str());
		}
		mir_free(pszFile);
		mir_free(pszFileOut);
}

	void __cdecl CNetwork::GetAwayMsgThread(void* gatt) {
		DBVARIANT dbv;
		HANDLE hContact=(HANDLE)gatt;

		if (!DBGetContactSetting(hContact, "CList", "StatusMsg", &dbv)) {
			ProtoBroadcastAck(m_szModuleName, hContact, ACKTYPE_AWAYMSG, ACKRESULT_SUCCESS, (HANDLE) 1, (LPARAM)dbv.pszVal);
			DBFreeVariant(&dbv);
		} else 
			ProtoBroadcastAck(m_szModuleName, hContact, ACKTYPE_AWAYMSG, ACKRESULT_SUCCESS, (HANDLE)1, (LPARAM)0);
	}
#if 0 // UPGRADE_DISABLE
	MIMPROC2(FileAllow) {
#if 0
		CCSDATA* ccs = (CCSDATA*)lParam;
		ft_t* ft=(ft_t*)ccs->wParam;
		unsigned int qqid=ft->qqid;
		EvaIPAddress eip(qqNsThread->qqn->getIP());

		char szPath[MAX_PATH]={0};

		strcpy(szPath,(char*)ccs->lParam);
		if (!*szPath) {
			CallService(MS_FILE_GETRECEIVEDFILESFOLDER,(WPARAM)ccs->hContact,(LPARAM)szPath);
		}
		if (szPath[strlen(szPath)-1]=='\\')
			szPath[strlen(szPath)-1]=0;

		util_log(0,"EvaMain::slotFileTransferAccept -- session: %d\tdir:%s\n", ccs->wParam, szPath);

		if (QQNetwork::fileManager)
			QQNetwork::fileManager->saveFileTo(qqid,ft->sessionid,szPath);

		SendFileExRequestPacket *packet = new SendFileExRequestPacket(QQ_IM_EX_REQUEST_ACCEPTED);
		packet->setReceiver(qqid);
		packet->setTransferType(QQ_TRANSFER_FILE);
		packet->setWanIp(htonl(eip.IP()));
		packet->setSessionId((unsigned short)ft->sessionid);
		network->append(packet);
		util_log(0,"EvaPacketManager::doAcceptFileRequest -- sent\n");
		ftSessions[ft->sessionid]=ft;

		return (int)ft;
#endif
		return NULL;
	}

	MIMPROC2(FileCancel) {
#if 0
		CCSDATA* ccs = (CCSDATA*)lParam;
		ft_t* ft=(ft_t*)ccs->wParam;
		unsigned int qqid=ft->qqid;

		SendFileExRequestPacket *packet = new SendFileExRequestPacket(QQ_IM_EX_REQUEST_CANCELLED);
		packet->setReceiver(qqid);
		packet->setTransferType(QQNetwork::fileManager->getTransferType(qqid,ft->sessionid));
		packet->setSessionId(ft->sessionid);
		network->append(packet);
		if (QQNetwork::fileManager) QQNetwork::fileManager->stopThread(qqid,ft->sessionid);
		ftSessions.erase(ft->sessionid);
		delete ft;
#endif
		return 1;
	}

	MIMPROC2(FileResume) {
		return 0;
	}
#endif // UPGRADE_DISABLE
	MIMPROC(DownloadUserHead) {
		if (Packet::isClientKeySet()) {
			if (m_userhead)
				MessageBox(NULL,TranslateT("User head download is in progress, please try again later."),NULL,MB_ICONERROR);
			else
				append(new RequestExtraInfoPacket());
		}
		return 0;
	}

	MIMPROC(PostImage) {
#if 0
		OPENFILENAMEA ofn={sizeof(OPENFILENAMEA)};
		char szFile[MAX_PATH]={0};       // buffer for file name
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = MAX_PATH;
		ofn.lpstrFilter = Translate("Image Files (*.jpg; *.gif)\0*.jpg;*.gif\0");
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

		// Display the Open dialog box. 

		if (GetOpenFileNameA(&ofn)==TRUE) {
			HANDLE hContact=(HANDLE)wParam;
			if (READC_B2("IsQun")==1 || READC_B2("ChatRoom")!=0) {
				// Qun
				_CopyAndPost((HANDLE)wParam,szFile);
			} else
				CallService(MS_MSG_SENDMESSAGE,(WPARAM)wParam,(LPARAM)((string)(string("[img]")+szFile+"[/img]")).c_str());
		}
#endif
		OPENFILENAMEW ofn={sizeof(OPENFILENAMEW)};
		WCHAR szFile[MAX_PATH]={0};       // buffer for file name
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = MAX_PATH;
		ofn.lpstrFilter = TranslateT("Image Files (*.jpg; *.gif)\0*.jpg;*.gif\0");
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

		// Display the Open dialog box. 

		if (GetOpenFileNameW(&ofn)==TRUE) {
			HANDLE hContact=(HANDLE)wParam;
			if (READC_B2("IsQun")==1 || READC_B2("ChatRoom")!=0) {
				// Qun
				_CopyAndPost((HANDLE)wParam,szFile);
			} else
				CallService(MS_MSG_SENDMESSAGE,(WPARAM)wParam,(LPARAM)((wstring)(wstring(L"[img]")+szFile+L"[/img]")).c_str());
		}
		return 0;
	}
#if 0 // UPGRADE_DISABLE
	MIMPROC2(SelectImage) {
		return 0;
	}

	TCHAR* a2tf( const TCHAR* str, BOOL unicode )
	{
		if ( str == NULL )
			return NULL;

#if defined( _UNICODE )
		if ( unicode )
			return mir_tstrdup( str );
		else {
			int codepage = CallService( MS_LANGPACK_GETCODEPAGE, 0, 0 );

			int cbLen = MultiByteToWideChar( codepage, 0, (char*)str, -1, 0, 0 );
			TCHAR* result = ( TCHAR* )mir_alloc( sizeof(TCHAR)*( cbLen+1 ));
			if ( result == NULL )
				return NULL;

			MultiByteToWideChar( codepage, 0, (char*)str, -1, result, cbLen );
			result[ cbLen ] = 0;
			return result;
		}
#else
		return mir_strdup( str );
#endif
	}

	void overrideStr( TCHAR*& dest, const TCHAR* src, BOOL unicode, const TCHAR* def=NULL )
	{
		if ( dest != NULL ) 
		{
			mir_free( dest );
			dest = NULL;
		}

		if ( src != NULL )
			dest = a2tf( src, unicode );
		else if ( def != NULL )
			dest = mir_tstrdup( def );
	}
#endif // UPGRADE DISABLE
	/////////////////////////////////////////////////////////////////////////////////////////
	//	GetCurrentMedia - get current media

	int CNetwork::GetCurrentMedia(WPARAM wParam, LPARAM lParam) 
	{
		LISTENINGTOINFO *cm=(LISTENINGTOINFO*)lParam;

		if (!cm || cm->cbSize!=sizeof(LISTENINGTOINFO)) return -1;

		cm->ptszArtist=mir_tstrdup(m_currentMedia.ptszArtist);
		cm->ptszAlbum=mir_tstrdup(m_currentMedia.ptszAlbum);
		cm->ptszTitle=mir_tstrdup(m_currentMedia.ptszTitle);
		cm->ptszTrack=mir_tstrdup(m_currentMedia.ptszTrack);
		cm->ptszYear=mir_tstrdup(m_currentMedia.ptszYear);
		cm->ptszGenre=mir_tstrdup(m_currentMedia.ptszGenre);
		cm->ptszLength=mir_tstrdup(m_currentMedia.ptszLength);
		cm->ptszPlayer=mir_tstrdup(m_currentMedia.ptszPlayer);
		cm->ptszType=mir_tstrdup(m_currentMedia.ptszType);
		cm->dwFlags=m_currentMedia.dwFlags;

		return 0;
	}


	VOID CALLBACK SetCurrentMediaTimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime) {
		CNetwork* network=((CNetwork*)idEvent);
		CHAR szService[MAX_PATH];
		KillTimer(NULL,network->m_timer);
		network->m_timer=NULL;
		strcat(strcpy(szService,network->m_szModuleName),PS_SET_LISTENINGTO);
		CallService(szService,1,0);
	}

	/////////////////////////////////////////////////////////////////////////////////////////
	//	SetCurrentMedia - set current media

	int CNetwork::SetCurrentMedia(WPARAM wParam, LPARAM lParam) 
	{
		if (!Packet::isClientKeySet()) return -1;

		if (!wParam && !lParam) {
			if (!m_timer) {
				m_timer=SetTimer(NULL,(UINT_PTR)this,5000,SetCurrentMediaTimerProc);
			}
			return 0;
		} else if (m_timer && lParam) {
			KillTimer(NULL,m_timer);
			m_timer=NULL;
		}

		// Clear old info
		if (m_currentMedia.ptszArtist) mir_free(m_currentMedia.ptszArtist);
		//if (m_currentMedia.ptszAlbum) mir_free(m_currentMedia.ptszAlbum);
		if (m_currentMedia.ptszTitle) mir_free(m_currentMedia.ptszTitle);
		/*
		if (m_currentMedia.ptszTrack) mir_free(m_currentMedia.ptszTrack);
		if (m_currentMedia.ptszYear) mir_free(m_currentMedia.ptszYear);
		if (m_currentMedia.ptszGenre) mir_free(m_currentMedia.ptszGenre);
		if (m_currentMedia.ptszLength) mir_free(m_currentMedia.ptszLength);
		if (m_currentMedia.ptszPlayer) mir_free(m_currentMedia.ptszPlayer);
		if (m_currentMedia.ptszType) mir_free(m_currentMedia.ptszType);
		*/
		ZeroMemory(&m_currentMedia, sizeof(m_currentMedia));

		// Copy new info
		LISTENINGTOINFO *cm = (LISTENINGTOINFO*)lParam;
		if (cm && cm->cbSize==sizeof(LISTENINGTOINFO) && (cm->ptszArtist || cm->ptszTitle)) 
		{
			BOOL unicode = cm->dwFlags & LTI_UNICODE;

			m_currentMedia.cbSize = sizeof(m_currentMedia);	// Marks that there is info set
			m_currentMedia.dwFlags = LTI_UNICODE;

			m_currentMedia.ptszArtist=unicode?mir_wstrdup(cm->ptszArtist):mir_a2u(cm->pszArtist);
			m_currentMedia.ptszTitle=unicode?mir_wstrdup(cm->ptszTitle):mir_a2u(cm->pszTitle);
		}

		LPSTR lpszSend=NULL;

		// Set user text
		if (m_currentMedia.cbSize == 0) {
			DBVARIANT dbv;
			DBDeleteContactSetting( NULL, m_szModuleName, "ListeningTo" );
			if (!READ_TS2(NULL,"PersonalSignature",&dbv)) {
				// With Personal Signature
				lpszSend=mir_u2a_cp(dbv.ptszVal,936);
				DBFreeVariant(&dbv);
			}
		} else {
			TCHAR *text;
			LPSTR lpszTemp;
			if (ServiceExists(MS_LISTENINGTO_GETPARSEDTEXT)) 
				text=(LPTSTR)CallService(MS_LISTENINGTO_GETPARSEDTEXT, (WPARAM)_T("%title% - %artist%"), (LPARAM)&m_currentMedia);
			else 
			{
				text=(LPTSTR)mir_alloc(MAX_PATH*sizeof(TCHAR));
				mir_sntprintf(text, MAX_PATH, _T("%s - %s"), (m_currentMedia.ptszTitle?m_currentMedia.ptszTitle:_T("")), 
					(m_currentMedia.ptszArtist?m_currentMedia.ptszArtist:_T("")));
			}
			WRITE_TS(NULL, "ListeningTo", text);
			lpszTemp=mir_u2a_cp(text,936);
			lpszSend=(LPSTR)mir_alloc(101);
			ZeroMemory(lpszSend,101);
			strncpy(lpszSend,lpszTemp,99);
			strcat(lpszSend,"\x1");
			mir_free(lpszTemp);
			mir_free(text);
		}

		// Send It
		if (lpszSend) {
			SignaturePacket* packet=new SignaturePacket(QQ_SIGNATURE_MODIFY);
			packet->setSignature(lpszSend);
			append(packet);
			mir_free(lpszSend);
		} else {
			append(new SignaturePacket(QQ_SIGNATURE_DELETE));
		}

		return 0;
	}
#if 0 // UPGRADE DISABLE
#if 0
	MIMPROC2(ToggleQunList) {
		qqSettings->enableQunList=qqSettings->enableQunList?false:true;
		WRITE_B(NULL,QQ_ENABLEQUNLIST,qqSettings->enableQunList);
		if (!qqSettings->enableQunList && CQunListV2::getInstance(false)!=NULL) {
			CQunListV2::getInstance(false)->destroy();
		}

		if (!wParam)
			MessageBoxA(NULL,qqSettings->enableQunList?Translate("Qun List is now Enabled. It will be shown when you switch tab or open new chat window."):Translate("Qun List is now Disabled."),NULL,MB_ICONINFORMATION);
		return 0;
	}
#endif

	static list<string>settingsNames;

	int DBSettingEnumProc(const char *szSetting,LPARAM lParam) {
		if (*szSetting>='0' && *szSetting<='9')
			settingsNames.push_back(szSetting);
		return 0;
	}
#endif // UPGRADE_DISABLE
	MIMPROC(QunSpace) {
		HANDLE hContact=(HANDLE)wParam;
		/*
		CHAR szIE[MAX_PATH];
		STARTUPINFOA si={sizeof(si)};
		PROCESS_INFORMATION pi;
		
		CHAR szUrl[MAX_PATH]="http://group.qq.com/group_index.shtml?groupid=";
		itoa(READC_D2("ExternalID"),szUrl+strlen(szUrl),10);
		*/
		// http://ptlogin.qq.com/group?clientuin=85379868&clientkey=7C747509EF03FBC2C38A5125F9833DF7926625ABF72B7FE2555B57A528F1B9F7&gid=17834828&type=0

		/*
		CHAR szUrl[MAX_PATH]="http://ptlogin.qq.com/group?clientuin=";
		itoa(m_myqq,szUrl+strlen(szUrl),10);
		strcat(szUrl,"&clientkey=");
		util_fillClientKey(szUrl+strlen(szUrl));
		strcat(szUrl,"&gid=");
		itoa(READC_D2("ExternalID"),szUrl+strlen(szUrl),10);
		strcat(szUrl,"&type=0");

		strcat(strcat(strcpy(szIE,"\""),getenv("ProgramFiles")),"\\Internet Explorer\\iexplore.exe\" ");
		strcat(szIE,szUrl);
		CreateProcessA(NULL,szIE,NULL,NULL,FALSE,0,NULL,NULL,&si,&pi);
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
		//ShellExecuteA(NULL,NULL,"iexplore.exe",szUrl,NULL,SW_SHOWNORMAL);
		*/
		// http://qun.qq.com/air/2571213#2571213
		CHAR szUrl[MAX_PATH];
		DWORD dwExtID=READC_D2("ExternalID");
		sprintf(szUrl,"http://qun.qq.com/air/%d#%d",dwExtID,dwExtID);
		CallService(MS_UTILS_OPENURL,0,(LPARAM)szUrl);
		return 0;
	}
#if 0 // Not Working

	MIMPROC2(ChangeHeadImage) {
		// http://face.qq.com/index.shtml?clientuin=85379868&clientkey=E357ACA2A1377164EECCBD5F68A0CE11804B3F1EA7F4573ECF538AE336D7
		CHAR szUrl[MAX_PATH]="http://face.qq.com/index.shtml?clientuin=";
		itoa(Packet::getQQ(),szUrl+strlen(szUrl),10);
		strcat(szUrl,"&clientkey=");
		util_fillClientKey(szUrl+strlen(szUrl));
		ShellExecuteA(NULL,NULL,"iexplore.exe",szUrl,NULL,SW_SHOWNORMAL);
		return 0;
	}
#endif // UPGRADE_DISABLE
	MIMPROC(QQMail) {
		// http://mail.qq.com/cgi-bin/login?Fun=clientread&Uin=xxx&K=xxx
		CHAR szUrl[MAX_PATH]="http://mail.qq.com/cgi-bin/login?Fun=clientread&Uin=";
		itoa(m_myqq,szUrl+strlen(szUrl),10);
		strcat(szUrl,"&K=");
		util_fillClientKey(szUrl+strlen(szUrl));
		CallService(MS_UTILS_OPENURL,0,(LPARAM)szUrl);
		return 0;
	}
#if 0 // UPGRADE_DISABLE
#if 0
	MIMPROC2(ChangeEIP) {
		OPENFILENAMEA ofn={sizeof(OPENFILENAMEA)};
		HANDLE hContact=NULL;
		DBVARIANT dbv;
		char szFile[MAX_PATH]={0};       // buffer for file name
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = MAX_PATH;
		ofn.lpstrFilter = "EIP FIles (*.eip)\0*.eip\0";
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

		if (!READC_S2("CurrentEIP",&dbv)) {
			strcpy(szFile,dbv.pszVal);
			DBFreeVariant(&dbv);
		}

		// Display the Open dialog box. 
		if (GetOpenFileNameA(&ofn)==TRUE) {
			COLERRNO cen;
			if (COLEFS* cfs=cole_mount(szFile,&cen)) {
				if (COLEFILE* cf=cole_fopen(cfs,"/config/Face.Xml",&cen)) {
					size_t filesize=cole_fsize(cf);
					LPSTR pszXml=(LPSTR)mir_alloc(filesize+1);
					pszXml[filesize]=0;
					if (cole_fread(cf,pszXml,filesize,&cen)==filesize && strstr(pszXml,"<FACESETTING>")==pszXml) {
						int facecount=atoi(strchr(strstr(pszXml," count="),'\"')+1);
						typedef struct {
							string filename;
							string shortcut;
							string tips;
						} faceitem_t;

						map<int,faceitem_t> facemap;
						faceitem_t curface;
						int fileindex;

						LPSTR pszCurrent=strstr(pszXml,"<FACE ");
						for (int c=0; c<facecount && pszCurrent!=NULL; c++) {
							// Shortcut first
							pszCurrent=strchr(strstr(pszCurrent," shortcut="),'\"')+1;
							*strchr(pszCurrent,'\"')=0;
							curface.shortcut=pszCurrent;
							pszCurrent+=strlen(pszCurrent)+1;

							// Tip
							pszCurrent=strchr(strstr(pszCurrent," tip="),'\"')+1;
							*strchr(pszCurrent,'\"')=0;
							curface.tips=pszCurrent;
							pszCurrent+=strlen(pszCurrent)+1;

							// FileIndex
							pszCurrent=strchr(strstr(pszCurrent," FileIndex="),'\"')+1;
							fileindex=atoi(pszCurrent);

							// File Org
							pszCurrent=strchr(strstr(pszCurrent,"<FILE ORG>"),'>')+1;
							*strchr(pszCurrent,'<')=0;
							curface.filename=pszCurrent;
							pszCurrent+=strlen(pszCurrent)+1;

							facemap[fileindex]=curface;
						}

						mir_free(pszXml);
						cole_fclose(cf,&cen);

						CHAR szOutFile[MAX_PATH];
						CHAR szAslTemp[MAX_PATH];
						CHAR szAslOri[MAX_PATH];
						LPSTR pszOutFile;
						strcpy(szOutFile,m_szModuleName);
						strcat(szOutFile,"-filename");
						if (DBGetContactSetting(NULL,"SmileyAdd",szOutFile,&dbv))
							return 0;

						CallService(MS_UTILS_PATHTOABSOLUTE,(WPARAM)dbv.pszVal,(LPARAM)szAslOri);
						CallService(MS_UTILS_PATHTOABSOLUTE,(WPARAM)"QQ\\~asl.tmp",(LPARAM)szAslTemp);
						DBFreeVariant(&dbv);
						FILE* fpAsl=fopen(szAslOri,"r");
						FILE* fpAslOut=fopen(szAslTemp,"w");
						while (fgets(szOutFile,MAX_PATH,fpAsl)) {
							if (strstr(szOutFile,"; EIPSTART **")==szOutFile) break;
							fputs(szOutFile,fpAslOut);
						}
						fclose(fpAsl);
						fputs("; EIPSTART ** DO NOT REMOVE THIS LINE! Items below this line will be overwritten when you select a new EIP file.\n",fpAslOut);
						
						//CallService(MS_UTILS_PATHTOABSOLUTE,(WPARAM)"QQ\\EIP",(LPARAM)szOutFile);
						strcpy(szOutFile,szAslOri);
						strrchr(szOutFile,'\\')[1]=0;
						strcat(szOutFile,"eip");
						CreateDirectoryA(szOutFile,NULL);
						strcat(szOutFile,"\\");
						pszOutFile=szOutFile+strlen(szOutFile);

						strcat(szOutFile,"*.*");
						WIN32_FIND_DATAA wfd;
						HANDLE hFind=FindFirstFileA(szOutFile,&wfd);
						if (hFind!=INVALID_HANDLE_VALUE) {
							do {
								strcpy(pszOutFile,wfd.cFileName);
								DeleteFileA(szOutFile);
							} while(FindNextFileA(hFind,&wfd));
							FindClose(hFind);
						}

						FILE* fpOut;
						CHAR szItemPath[MAX_PATH]="/Files/"; //\x1d\xf3\xdf\xaf/";
						LPSTR pszItemPath; //=szItemPath+strlen(szItemPath);
						
						COLEDIR* cd=cole_opendir_rootdir(cfs,&cen);
						COLEDIRENT* cde=cole_visiteddirentry(cd);
						/*
						char* pszTemp=cole_direntry_getname(cde);
						while (strcmp(pszTemp,"Files")) {
							cde=cole_nextdirentry(cd);
							pszTemp=cole_direntry_getname(cde);
						}
						*/
						COLEDIR* cd2=cole_opendir_direntry(cde,&cen); // Files
						cde=cole_visiteddirentry(cd2);

						strcat(szItemPath,cole_direntry_getname(cde));
						strcat(szItemPath,"/");
						pszItemPath=szItemPath+strlen(szItemPath);
						cole_closedir(cd2,&cen);
						cole_closedir(cd,&cen);

						// Extract files
						for (int c=0; c<facecount; c++) {
							curface=facemap[c];
							itoa(c,pszItemPath,10);
							cf=cole_fopen(cfs,szItemPath,&cen);
							filesize=cole_fsize(cf);
							pszXml=(LPSTR)mir_alloc(filesize);
							cole_fread(cf,pszXml,filesize,&cen);
							cole_fclose(cf,&cen);
							strcpy(pszOutFile,curface.filename.c_str());
							fpOut=fopen(szOutFile,"wb");
							fwrite(pszXml,filesize,1,fpOut);
							fclose(fpOut);
							mir_free(pszXml);
							fprintf(fpAslOut,"Smiley = \"%s\", -125, \"[img]%s[/img] %s\",\"%s\"\n",pszOutFile-4,szOutFile,curface.shortcut.c_str(),curface.tips.c_str());

						}

						fclose(fpAslOut);
						CopyFileA(szAslTemp,szAslOri,FALSE);
						DeleteFileA(szAslTemp);

						swprintf((LPTSTR)szItemPath,TranslateT("%d custom emoticons has been added to your smileys files. Please restart Miranda IM/reapply ASL to take effect."),facecount);
						MessageBox(NULL,(LPTSTR)szItemPath,_T("MirandaQQ"),MB_ICONINFORMATION);
					} else {
						MessageBox(NULL,TranslateT("Selected file is corrupted."),NULL,MB_ICONERROR);
						mir_free(pszXml);
						cole_fclose(cf,&cen);
					}

				} else {
					MessageBox(NULL,TranslateT("Selected file is not a valid EIP file."),NULL,MB_ICONERROR);
				}
				cole_umount(cfs,&cen);
			} else
				MessageBox(NULL,TranslateT("Failed to mount selected EIP file."),NULL,MB_ICONERROR);
		}
		return 0;
	}
#endif
#endif

#ifdef TESTSERVICE
	MIMPROC(TestService) {
		DBVARIANT dbv;
		READ_S2(NULL,QQ_PASSWORD,&dbv);
		CallService(MS_DB_CRYPT_DECODESTRING, lstrlenA(dbv.pszVal) + 1, (LPARAM)dbv.pszVal);

		CQunInfoExt::Login(this,m_myqq,dbv.pszVal,false);
		CQunInfoExt::AddOneJob(m_myqq,0,0,0);
		DBFreeVariant(&dbv);
		return 0;
	}
#endif

	MIMPROC(ManualLoginQunInfoExt) {
		if (!Packet::isClientKeySet()) return -1;

		DBVARIANT dbv;
		READ_S2(NULL,QQ_PASSWORD,&dbv);
		CallService(MS_DB_CRYPT_DECODESTRING, lstrlenA(dbv.pszVal) + 1, (LPARAM)dbv.pszVal);

		CQunInfoExt::Login(this,m_myqq,dbv.pszVal,false);
		CQunInfoExt::AddOneJob(m_myqq,0,0,0);
		DBFreeVariant(&dbv);
		return 0;
	}

#if 0
	MIMPROC2(QQHTTPDCommand) {
		// WPARAM: 0-AddQueue, 1-AckQueue, All-clear not avail
		if (CQunImage::m_imageServer) {
			switch (wParam) {
				case 0:
					CQunImage::m_imageServer->addFile((LPCSTR)wParam);
					break;
				case 1:
					CQunImage::m_imageServer->signalFile((LPCWSTR)wParam);
					break;
			}
		}
		return 0;
	}
}
#endif



//////////////////////////////////////////////////////////////////////////

HANDLE __cdecl CNetwork::AddToList(int flags, PROTOSEARCHRESULT* psr) {
	HANDLE hContact;
	char uid[16]={0};
	LPSTR is_qun=strchr(psr->nick,'(');

	if (!Packet::isClientKeySet()) return 0;

	if (is_qun) { // Adding a Qun
		strcpy(uid,is_qun+1);
		*strchr(uid,')')=0;
	} else // Adding a QQ user
		strcpy(uid,psr->nick);

	if (!(hContact=AddContact(atol(uid),flags & PALF_TEMPORARY,true))) return NULL;
	if (is_qun) {
		WRITEC_B("IsQun",1);
		WRITEC_D("ExternalID",atoi(psr->nick));
	}

	if (!(flags & PALF_TEMPORARY)) { // Not temporary contact, can send Add Friend/Join Qun request
		util_log(0,"Send Add Request, IsQun=%s",is_qun);
		m_addUID=atol(uid);
		if (is_qun)
			append(new QunJoinPacket(m_addUID));
		else 
			append(new EvaAddFriendExPacket(m_addUID));
	}
	return hContact;
}

HANDLE __cdecl CNetwork::AddToListByEvent(int flags, int iContact, HANDLE hDbEvent) {
	DBEVENTINFO dbei={sizeof(dbei)};
	char* nick;
	HANDLE hContact;
	int qqid;
	char* reason;

	if (!Packet::isClientKeySet()) return 0;

	if ((dbei.cbBlob=CallService(MS_DB_EVENT_GETBLOBSIZE, (WPARAM)hDbEvent, 0))==-1) {
		util_log(98,"%s(): ERROR: Can't get blob size.",__FUNCTION__);
		return 0;
	}

	util_log(0,"Blob size: %lu", dbei.cbBlob);
	dbei.pBlob=(PBYTE)_alloca(dbei.cbBlob);
	if (CallService(MS_DB_EVENT_GET, (WPARAM)hDbEvent, (LPARAM)&dbei)) {
		util_log(98,"%s(): ERROR: Can't get event.",__FUNCTION__);
		return 0;
	}

	if (dbei.eventType != EVENTTYPE_AUTHREQUEST) {
		util_log(98,"%s(): ERROR: Not authorization.",__FUNCTION__);
		return 0;
	}

	if (strcmp(dbei.szModule, m_szModuleName)) {
		util_log(98,"%s(): ERROR: Not for MIMQQ.",__FUNCTION__);
		return 0;
	}

	//Adds a contact to the contact list given an auth, added or contacts event
	//wParam=MAKEWPARAM(flags,iContact)
	//lParam=(LPARAM)(HANDLE)hDbEvent
	//Returns a HANDLE to the new contact, or NULL on failure
	//hDbEvent must be either EVENTTYPE_AUTHREQ or EVENTTYPE_ADDED
	//flags are the same as for PS_ADDTOLIST.
	//iContact is only used for contacts events. It is the 0-based index of the
	//contact in the event to add. There is no way to add two or more contacts at
	//once, you should just do lots of calls.

	/* TYPE ADDED
	blob is: uin(DWORD), hcontact(HANDLE), nick(ASCIIZ), first(ASCIIZ), 
	last(ASCIIZ), email(ASCIIZ) 

	TYPE AUTH REQ
	blob is: uin(DWORD), hcontact(HANDLE), nick(ASCIIZ), first(ASCIIZ), 
	last(ASCIIZ), email(ASCIIZ), reason(ASCIIZ)
	*/
	//hContact = (HANDLE) ( dbei.pBlob + sizeof( DWORD ));

	qqid=*(int*)dbei.pBlob;
	hContact=*(HANDLE*)(dbei.pBlob+sizeof(DWORD));
	nick = (char*)( dbei.pBlob + sizeof( DWORD )*2 );
	{
		char* firstName = nick + lstrlenA(nick) + 1;
		char* lastName = firstName + lstrlenA(firstName) + 1;
		char* email = lastName + lstrlenA(lastName) + 1;
		reason = email + lstrlenA(email) + 1;

		util_log(0,"buddy:%s first:%s last:%s e-mail:%s", nick,
			firstName, lastName, email);
		util_log(0,"reason:%s ", reason);
	}

	/* we need to send out a packet to request an add */

	if (*reason) DBWriteContactSettingString(hContact,m_szModuleName,"AuthReason",reason);

	m_addUID=qqid;
	append(new AddFriendPacket(m_addUID));
	return hContact;
}

int __cdecl CNetwork::Authorize(HANDLE hContact) {
	DBEVENTINFO dbei={sizeof(dbei)};
	unsigned int* uid;
	unsigned int qunUID;
	HANDLE hContact2;
	Qun* qun=NULL;
	LPSTR pszBlob;

	if (!Packet::isClientKeySet()) return 1;   
	if ((dbei.cbBlob=CallService(MS_DB_EVENT_GETBLOBSIZE, (WPARAM)hContact, 0))==-1) return 1;
	pszBlob=(LPSTR)(dbei.pBlob=(PBYTE)alloca(dbei.cbBlob));

	if (CallService(MS_DB_EVENT_GET, (WPARAM)hContact, (LPARAM)&dbei)) return 1;
	if (dbei.eventType!=EVENTTYPE_AUTHREQUEST) return 1;
	if (strcmp(dbei.szModule, m_szModuleName)) return 1;

	uid=(unsigned int*)dbei.pBlob;pszBlob+=sizeof(DWORD);
	hContact2=*(HANDLE*)(dbei.pBlob+sizeof(DWORD));pszBlob+=sizeof(HANDLE);
	pszBlob+=6; //nick,first,last
	pszBlob+=strlen(pszBlob)+1; // email
	pszBlob+=strlen(pszBlob)+1; // msg
	if (hContact2) qunUID=DBGetContactSettingDword(hContact2,m_szModuleName,UNIQUEIDSETTING,0);

	if (qunUID) qun=m_qunList.getQun(qunUID);

	if (qun) {
		util_log(0, "%s(): You allowed user %d to join Qun %d",__FUNCTION__,*uid,qunUID);
		QunAuthPacket* packet=new QunAuthPacket(qunUID,QQ_QUN_AUTH_APPROVE);
		packet->setReceiver(*uid);
		packet->setToken((unsigned char*)pszBlob+2,*(unsigned short*)pszBlob);
		append(packet);
	} else {
		util_log(0,"%s(): You allowed buddy %d to add you",__FUNCTION__,*uid);
		append(new AddFriendAuthPacket(*uid, QQ_MY_AUTH_APPROVE));
	}

	return 0;
}

int __cdecl CNetwork::AuthDeny(HANDLE hContact, const char* szReason) {
	DBEVENTINFO dbei={sizeof(dbei)};
	char* reason;
	unsigned int* uid;
	unsigned int qunUID;
	HANDLE hContact2;
	Qun* qun=NULL;
	LPSTR pszBlob;

	if (!Packet::isClientKeySet()) return 1;   

	if (MessageBox(NULL,TranslateT("Send deny message?\n(If you choose No, then the request is ignored to user)"),NULL,MB_ICONQUESTION | MB_YESNO)==IDNO)
		return 0;

	if ((dbei.cbBlob=CallService(MS_DB_EVENT_GETBLOBSIZE, (WPARAM)hContact, 0))==-1) return 1;

	pszBlob=(LPSTR)(dbei.pBlob=(PBYTE)alloca(dbei.cbBlob));
	if (CallService(MS_DB_EVENT_GET, (WPARAM)hContact, (LPARAM)&dbei)) return 1;
	if (dbei.eventType!=EVENTTYPE_AUTHREQUEST) return 1;
	if (strcmp(dbei.szModule, m_szModuleName)) return 1;

	uid=(unsigned int*) dbei.pBlob;
	reason = mir_strdup(szReason);

	uid=(unsigned int*)dbei.pBlob;pszBlob+=sizeof(DWORD);
	hContact2=*(HANDLE*)(dbei.pBlob+sizeof(DWORD));pszBlob+=sizeof(HANDLE);
	pszBlob+=6; //nick,first,last
	pszBlob+=strlen(pszBlob)+1; // email
	pszBlob+=strlen(pszBlob)+1; // msg
	if (hContact2) {
		qunUID=DBGetContactSettingDword(hContact2,m_szModuleName,UNIQUEIDSETTING,0);
	}

	if (qunUID) qun=m_qunList.getQun(qunUID);

	util_convertToGBK(reason);

	if (qun) {
		util_log(0, "%s(): You rejected user %d to join Qun %d",__FUNCTION__,*uid,qunUID);

		QunAuthPacket* packet=new QunAuthPacket(qunUID,QQ_QUN_AUTH_REJECT);
		packet->setReceiver(*uid);
		packet->setMessage(reason);
		packet->setToken((unsigned char*)pszBlob+2,*(unsigned short*)pszBlob);
		append(packet);
	} else {
		util_log(0,"%s(): You denied buddy %d to add you",__FUNCTION__,*uid);

		AddFriendAuthPacket *packet = new AddFriendAuthPacket(*uid, QQ_MY_AUTH_REJECT);
		packet->setMessage(reason);
		append(packet);

		// TODO: Do we need to remove the contact?
		//CallService( MS_DB_CONTACT_DELETE, (WPARAM) hContact, 0);
	}

	mir_free(reason);
	return 0;
}

// Temp
INT __cdecl CNetwork::RecvAuth(WPARAM,LPARAM lParam) {
	CCSDATA* ccs=(CCSDATA*)lParam;
	return AuthRecv(ccs->hContact,(PROTORECVEVENT*)ccs->lParam);
}

int __cdecl CNetwork::AuthRecv(HANDLE hContact, PROTORECVEVENT* pre) {
	if (pre) {
		DBEVENTINFO dbei;

		util_log(0,"%s(): Received authorization request",__FUNCTION__);
		// Show that guy
		DBDeleteContactSetting(hContact,"CList","Hidden");

		ZeroMemory(&dbei,sizeof(dbei));
		dbei.cbSize=sizeof(dbei);
		dbei.szModule=m_szModuleName;
		dbei.timestamp=pre->timestamp;
		dbei.flags=pre->flags & (PREF_CREATEREAD?DBEF_READ:0);
		dbei.eventType=EVENTTYPE_AUTHREQUEST;
		dbei.cbBlob=pre->lParam;
		dbei.pBlob=(PBYTE)pre->szMessage;
		CallService(MS_DB_EVENT_ADD,(WPARAM)NULL,(LPARAM)&dbei);

	} else
		util_log(0,"%s(): ERROR: Received empty authorization request!",__FUNCTION__);

	return 0;
}

// TODO: This function should no longer needed as MIMQQ2 provides its own auth dialog
int __cdecl CNetwork::AuthRequest(HANDLE hContact, const char* szMessage) {
	if (hContact) {
		DWORD dwUin=DBGetContactSettingDword(hContact, m_szModuleName, UNIQUEIDSETTING, 0);

		if (dwUin && szMessage)
		{	
			// Set this flag so that QQ will not display error if auth is required
			WRITEC_S("AuthReason",szMessage);

			// The add fail callback will generate auth request
			return 0; // Success
		}
	}

	return 1; // Failure
}

HANDLE __cdecl CNetwork::ChangeInfo(int iInfoType, void* pInfoData) {
	return 0;
}

HANDLE __cdecl CNetwork::FileAllow(HANDLE hContact, HANDLE hTransfer, const char* szPath) {
	MessageBox(NULL,TranslateW(L"This build of MirandaQQ2 does not support file transfer."),NULL,NIIF_ERROR);
	FileDeny(hContact,hTransfer,"Client Software does not support file transfer.");
	return (HANDLE)1;
}

int __cdecl CNetwork::FileCancel(HANDLE hContact, HANDLE hTransfer) {
	return 1;
}

int __cdecl CNetwork::FileDeny(HANDLE hContact, HANDLE hTransfer, const char* szReason) {
#if 0
	ft_t* ft=(ft_t*)hTransfer;

	SendFileExRequestPacket *packet = new SendFileExRequestPacket(QQ_IM_EX_REQUEST_CANCELLED);
	packet->setReceiver(ft->qqid);
	packet->setTransferType(QQ_TRANSFER_FILE/*m_FileManager->getTransferType(qqid, *sessionid)*/);
	packet->setSessionId(ft->sessionid);
	append(packet);

	/*
	ftSessions.erase(ft->sessionid);
	delete ft;
	*/

#endif
	return 0;
}

int __cdecl CNetwork::FileResume(HANDLE hTransfer, int* action, const char** szFilename) {
	return 1;
}

DWORD __cdecl CNetwork::GetCaps(int type, HANDLE hContact) {
	switch (type) {
		case PFLAGNUM_1: // Protocol Capabilities
			return PF1_IM | PF1_SERVERCLIST | PF1_ADDED | PF1_BASICSEARCH | PF1_SEARCHBYEMAIL | PF1_SEARCHBYNAME | PF1_NUMERICUSERID | PF1_ADDSEARCHRES | PF1_AUTHREQ | PF1_MODEMSG | PF1_FILE | PF1_CHAT | PF1_BASICSEARCH;
			break;
		case PFLAGNUM_2: // Possible Status
			return PF2_ONLINE | PF2_INVISIBLE | PF2_SHORTAWAY; // PF2_SHORTAWAY=Away
			break;
		case PFLAGNUM_3:
			return PF2_ONLINE | PF2_INVISIBLE | PF2_SHORTAWAY; // Status that supports mode message
			break;
		case PFLAGNUM_4: // Additional Capabilities
			return PF4_FORCEAUTH | PF4_FORCEADDED | PF4_NOCUSTOMAUTH | PF4_AVATARS | PF4_IMSENDUTF | PF4_OFFLINEFILES | PF4_IMSENDOFFLINE; // PF4_FORCEADDED="Send you were added" checkbox becomes uncheckable
			break;
		case PFLAG_UNIQUEIDTEXT: // Description for unique ID (For search use)
			return (int)Translate("QQ ID");
			break;
		case PFLAG_UNIQUEIDSETTING: // Where is my Unique ID stored in?
			return (int)UNIQUEIDSETTING;
		case PFLAG_MAXLENOFMESSAGE: // Maximum message length
			return 0;
			break;
		case 10000: // MIMQQ: IPCSupport
			return 1;
	}
	return 0;
}

HICON __cdecl CNetwork::GetIcon(int iconIndex) {
	return (iconIndex & 0xFFFF)==PLI_PROTOCOL?(HICON)LoadImage(hinstance, MAKEINTRESOURCE(IDI_TM), IMAGE_ICON, GetSystemMetrics(iconIndex&PLIF_SMALL?SM_CXSMICON:SM_CXICON), GetSystemMetrics(iconIndex&PLIF_SMALL?SM_CYSMICON:SM_CYICON), 0):0;
	return 0;
}

int __cdecl CNetwork::GetInfo(HANDLE hContact, int /*infoType*/) {
	DWORD dwUin = READC_D2(UNIQUEIDSETTING);

	if (!Packet::isClientKeySet()) return 1;

	if (READC_B2("ChatRoom")==1) {
		//append(new QunGetInfoPacket(_wtoi(DBGetStringW(hContact,m_szModuleName,"ChatRoomID"))));
		SendMessage(GetForegroundWindow(),WM_CLOSE,0,0);
		CallService(MS_USERINFO_SHOWDIALOG,(WPARAM)FindContact(_wtoi(DBGetStringW(hContact,m_szModuleName,"ChatRoomID"))),0);
	} else if (READC_B2("IsQun")==0) {
		// General Contact
		append(new GetUserInfoPacket(dwUin));
	} else { // Qun Contact (Info already stored in qqNetwork->qunInfo)
		append(new QunGetInfoPacket(dwUin));
	}

	return 0;
}

HANDLE __cdecl CNetwork::SearchBasic(const char* id) {
	if (Packet::isClientKeySet()) {
		// MirandaQQ only shows first 30 matching contacts, so setPage not used
		SearchUserPacket* packet=new SearchUserPacket();
		packet->setSearchType(QQ_SEARCH_QQ);
		packet->setQQ(id);
		packet->setMatchEntireString(false);
		m_searchUID=atol(id);
		append(packet);
		return (HANDLE)1;
	} else
		return 0;
}

HANDLE __cdecl CNetwork::SearchByEmail(const char* email) {
	if (Packet::isClientKeySet()) {
		SearchUserPacket* packet=new SearchUserPacket();
		packet->setSearchType(QQ_SEARCH_CUSTOM);
		packet->setEmail(email);
		packet->setMatchEntireString(false);
		m_searchUID=0;
		append(packet);
		return (HANDLE)1;
	} else
		return 0;
}

HANDLE __cdecl CNetwork::SearchByName(const char* nick, const char* firstName, const char* lastName) {
	if (Packet::isClientKeySet()) {
		// MirandaQQ only searches for Nick, other fields are ignored
		SearchUserPacket* packet=new SearchUserPacket();
		char* pszNick=mir_strdup(nick);

		util_convertToGBK(pszNick);
		packet->setSearchType(QQ_SEARCH_CUSTOM);
		packet->setNick(pszNick);
		packet->setMatchEntireString(false);
		m_searchUID=0;
		append(packet);
		mir_free(pszNick);
		return (HANDLE)1;
	} else
		return 0;
}

HWND __cdecl CNetwork::SearchAdvanced(HWND owner) {
	return 0;
}

HWND __cdecl CNetwork::CreateExtendedSearchUI(HWND owner) {
	return 0;
}

int __cdecl CNetwork::RecvContacts(HANDLE hContact, PROTORECVEVENT*) {
	return 1;
}

int __cdecl CNetwork::RecvFile(HANDLE hContact, PROTORECVFILE* evt) {
	CCSDATA ccs={hContact, PSR_FILE, 0, ( LPARAM )evt};
	return CallService(MS_PROTO_RECVFILE, 0, (LPARAM)&ccs);
}

int __cdecl CNetwork::RecvMsg(HANDLE hContact, PROTORECVEVENT* evt) {
	CCSDATA ccs={hContact, PSR_MESSAGE, 0, ( LPARAM )evt};
	return CallService(MS_PROTO_RECVMSG, 0, ( LPARAM )&ccs);
}

int __cdecl CNetwork::RecvUrl(HANDLE hContact, PROTORECVEVENT*) {
	return 1;
}

int __cdecl CNetwork::SendContacts(HANDLE hContact, int flags, int nContacts, HANDLE* hContactsList) {
	return 1;
}

HANDLE __cdecl CNetwork::SendFile(HANDLE hContact, const char* szDescription, char** ppszFiles) {
	char* file=*ppszFiles;
	char* afile=NULL;
	if (ppszFiles[1]!=NULL) {
		MessageBoxW(NULL,TranslateT("Only 1 file is allowed"),NULL,MB_ICONERROR);
		return 0;
	}

	if (READC_B2("IsQun")==1) {
		// Qun
		HWND hWndFT=NULL;
		if (CallService(MS_SYSTEM_GETVERSION,NULL,NULL)<0x00090000)
			SendMessage(GetForegroundWindow(),WM_CLOSE,NULL,NULL);
		else
			hWndFT=GetForegroundWindow();

		if (file[1]==0) {
			// Miranda IM 0.9: ppszFiles maybe in Unicode
			afile=mir_u2a_cp((LPWSTR)file,GetACP());
			file=afile;
		}

		// Test file first
		char* pszExt=strrchr(file,'.')+1;
		if (stricmp(pszExt,"bmp")!=0 && stricmp(pszExt,"jpg") && stricmp(pszExt,"gif")) {
			ForkThread((ThreadFunc)&CNetwork::ThreadMsgBox,mir_tstrdup(TranslateT("Warning! Sending non-GIF nor JPG files to qun can only be received by clients using MirandaQQ.")));
			//return;
		} 
		HANDLE hFile=CreateFileA(file,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);
		//if (GetFileAttributesA(file)==INVALID_FILE_ATTRIBUTES) {
		if (hFile==INVALID_HANDLE_VALUE) {
			ForkThread((ThreadFunc)&CNetwork::ThreadMsgBox,mir_tstrdup(TranslateT("Failed sending qun image because the file is inaccessible")));
			return NULL;
		} else if (GetFileSize(hFile,NULL)>61440) {
			CloseHandle(hFile);
			ForkThread((ThreadFunc)&CNetwork::ThreadMsgBox,mir_tstrdup(TranslateT("You can only send qun images that are less than or equals to 61440 bytes.")));
		} else {
			CloseHandle(hFile);
			string str="[img]";
			str.append(file);
			str.append("[/img]");
			CallService(MS_MSG_SENDMESSAGE,(WPARAM)hContact,(LPARAM)str.c_str());
		}
		if (afile) mir_free(afile);
		if (hWndFT) PostMessage(hWndFT,WM_CLOSE,NULL,NULL);
	} else {
		MessageBoxW(NULL,TranslateT("This MirandaQQ2 build does not support file transfer."),NULL,MB_ICONERROR);
#if 0
		unsigned int qq=READ_D2(ccs->hContact,UNIQUEIDSETTING);
		unsigned int seq=READ_W2(ccs->hContact,"Sequence")+1;
		char* pszFile=*(char**)ccs->lParam;
		EvaIPAddress eip(READ_D2(NULL,"IP"));

		list<unsigned int> sizeList;
		FILE* fp=fopen(pszFile,"rb");
		fseek(fp,0,SEEK_END);
		sizeList.push_back(ftell(fp));
		fclose(fp);
		*strrchr(pszFile,'\\')=0;

		char* pszFile2=strdup(pszFile+strlen(pszFile)+1);
		ft_t* ft=new ft_t();
		ft->file=pszFile;
		ft->qqid=qq;
		ft->sessionid=seq;
		ft->size=*sizeList.begin();
		ftSessions[seq]=ft;
		SendFileExRequestPacket *udpPacket = new SendFileExRequestPacket(QQ_IM_EX_UDP_REQUEST);
		udpPacket->setReceiver(qq);
		udpPacket->setTransferType(QQ_TRANSFER_FILE);
		util_convertToGBK(pszFile2);
		udpPacket->setFileName(pszFile2);
		free(pszFile2);
		udpPacket->setFileSize(sizeList.front());
		udpPacket->setSessionId(seq);
		udpPacket->setWanIp(htonl(eip.IP()));
		network->append(udpPacket);
		printf("EvaPacketManager::doSendFileUdpRequest - Sent.\n");

		list<string> dirList;
		list<string> fileList;
		dirList.push_back(pszFile);
		fileList.push_back(pszFile+strlen(pszFile)+1);
		if(QQNetwork::fileManager){
			QQNetwork::fileManager->newSession(qq, seq, dirList, fileList, sizeList, false, QQ_TRANSFER_FILE);
		}

		return (int)ft;
#endif
	}
	return NULL;
}

int __cdecl CNetwork::SendMsg(HANDLE hContact, int flags, const char* msg) {
	unsigned int uid;
	bool fQun=false;
	bool fTempQun=false;
	bool fTemp=false;

	if (!Packet::isClientKeySet()) return 1;   

	uid=DBGetContactSettingDword(hContact, m_szModuleName, UNIQUEIDSETTING, 0);
	fQun=DBGetContactSettingByte(hContact,m_szModuleName,"IsQun",0)==1;
	fTempQun=READC_B2("TempQun")==1;
	if (fTempQun) fQun=true;
	fTemp=uid>0x80000000;
	if (fTemp && !fQun) {
		
		uid-=0x80000000;
		if (strlen(msg)>690) return 0;
	}

	//util_log(0,"uid=%d, hContact=%d, msg=%s",uid,ccs->hContact,msg);
	if (uid) {
		LPSTR msg_with_qq_smiley;
		SendTextIMPacket* packet=NULL; 
		QunSendIMExPacket* qpacket=NULL;
		QunSendTempIMPacket* tqpacket=NULL;
		QunSendIMPacket* qpacket2=NULL;
		SendTempSessionTextIMPacket* tpacket=NULL;

		if (flags & PREF_UNICODE) {
			if (READ_B2(NULL,QQ_MESSAGECONVERSION) & 1) { // 1 or 3 is convert
				LPCWSTR pszUnicodeT=(LPCWSTR)(msg+strlen(msg)+1);
				LPWSTR pszUnicodeS=(LPWSTR)mir_tstrdup(pszUnicodeT);

				LCMapString(GetUserDefaultLCID(),LCMAP_SIMPLIFIED_CHINESE,pszUnicodeT,wcslen(pszUnicodeT)+1,pszUnicodeS,wcslen(pszUnicodeS)+1);
				msg_with_qq_smiley=mir_u2a_cp(pszUnicodeS,936);
				mir_free(pszUnicodeS);
			} else
				msg_with_qq_smiley=mir_u2a_cp((LPWSTR)(msg+strlen(msg)+1),936);

		} else if (flags & PREF_UTF) {
			if (READ_B2(NULL,QQ_MESSAGECONVERSION) & 1) { // 1 or 3 is convert
				LPWSTR pszUnicodeT=(LPWSTR)mir_utf8decodeW(msg);
				LPWSTR pszUnicodeS=(LPWSTR)mir_tstrdup(pszUnicodeT);

				LCMapString(GetUserDefaultLCID(),LCMAP_SIMPLIFIED_CHINESE,pszUnicodeT,wcslen(pszUnicodeT)+1,pszUnicodeS,wcslen(pszUnicodeS)+1);
				msg_with_qq_smiley=mir_u2a_cp(pszUnicodeS,936);
				mir_free(pszUnicodeS);
				mir_free(pszUnicodeT);
			} else {
				msg_with_qq_smiley=mir_strdup(msg);
				mir_utf8decodecp(msg_with_qq_smiley,936,NULL);
			}
		} else {
			msg_with_qq_smiley = mir_strdup(msg);
			if (!strstr(msg,"[ZDY]"))
				util_convertToGBK(msg_with_qq_smiley);
		}
		if (fQun) { // Sending Qun message
			if (strstr(msg,"[img]")) {
				if (Packet::getFileAgentKey()) {
					EvaHtmlParser parser;
					std::list<string> outPicList = parser.getCustomImages(msg_with_qq_smiley); // Get list of files
					if (outPicList.size()) {
						EvaSendCustomizedPicEvent *event = new EvaSendCustomizedPicEvent(); 
						event->setPicList(getSendFiles(outPicList)); // Get MD5 and remove any inaccessible files
						event->setQunID(uid);
						event->setMessage(msg_with_qq_smiley);
						mir_free(msg_with_qq_smiley);

						//CQunImage::PostEvent(event);
						if (!m_qunimage) m_qunimage=new CQunImage(this);
						m_qunimage->customEvent(event);

						return 1;
					}
				} else {
					MessageBoxA(NULL,Translate("Error: File Agent Key not set!\n\nI will try to request key again, please try sending after 1 minute."),NULL,MB_ICONERROR);
					append(new EvaRequestKeyPacket(QQ_REQUEST_FILE_AGENT_KEY));
					return 0;
				}
			}
			if (!strncmp(msg,"/kick ",6)) {
				int qqid=atoi(strchr(msg,' ')+1);
				if (qqid>0) {
					KICKUSERSTRUCT* kickUser=(KICKUSERSTRUCT*)mir_alloc(sizeof(KICKUSERSTRUCT));
					kickUser->qunid=uid;
					kickUser->qqid=qqid;
					kickUser->network=this;
					mir_forkthread(KickQunUser,kickUser);
					//mir_forkthread(qq_im_sendacksuccess, ccs->hContact);
					// TODO: !
					ProtoBroadcastAck(m_szModuleName, hContact, ACKTYPE_MESSAGE, ACKRESULT_SUCCESS, (HANDLE) 1, 0);

					mir_free(msg_with_qq_smiley);
					return 1;
				}
			} else if (!strncmp(msg,"/temp ",6)) {
				unsigned int qqid=atoi(strchr(msg,' ')+1);
				HANDLE hContact2=FindContact(qqid+0x80000000);
				Qun* qun=m_qunList.getQun(uid);
				const FriendItem* fi=qun->getMemberDetails(qqid);

				if (!qun || !fi)
					MessageBox(NULL,TranslateT("The specified Qun/QQID Does not exist"),NULL,MB_ICONERROR);
				else {
					WCHAR msg2[1024];
					/*
					PROTORECVEVENT pre;
					CCSDATA ccs2;
					*/
					DBVARIANT dbv;

					if (!hContact2) hContact2=AddContact(qqid+0x80000000,true,false);
					/*
					pre.flags=0;
					ccs2.hContact=hContact2;
					ccs2.szProtoService = PSR_MESSAGE;
					ccs2.wParam = 0;
					ccs2.lParam = ( LPARAM )&pre;
					pre.timestamp = (DWORD) time(NULL);
					pre.lParam = 0;
					*/

					//char* pszTemp, *pszTemp2;
					LPWSTR pszTemp, pszTemp2;
					//ccs2.hContact=hContact2;
					//pre.szMessage = msg2;

					DBGetContactSettingTString(hContact,m_szModuleName,"Nick",&dbv);
					//pszTemp=strdup(dbv.pszVal);
					pszTemp=mir_wstrdup(dbv.ptszVal);
					DBFreeVariant(&dbv);

					if (fi->getQunRealName().size())
						//pszTemp2=strdup(fi->getQunRealName().c_str());
						pszTemp2=mir_a2u_cp(fi->getQunRealName().c_str(),936);
					else
						//pszTemp2=strdup(fi->getNick().c_str());
						pszTemp2=mir_a2u_cp(fi->getNick().c_str(),936);
					//util_convertFromGBK(pszTemp2);

					swprintf(msg2,TranslateT("Temp Session: %s(%d) in %s"),pszTemp2,qqid,pszTemp);
					DBWriteContactSettingTString(hContact2,m_szModuleName,"Nick",msg2);

					//util_convertToGBK(pszTemp);
					DBWriteContactSettingTString(hContact2,m_szModuleName,"Site",pszTemp);

					/*
					free(pszTemp);
					free(pszTemp2);
					*/
					mir_free(pszTemp);
					mir_free(pszTemp2);

					CallService(MS_MSG_SENDMESSAGE,(WPARAM)hContact2,0);
				}
				//mir_forkthread(qq_im_sendacksuccess, hContact);
				// TODO: !
				ProtoBroadcastAck(m_szModuleName, hContact, ACKTYPE_MESSAGE, ACKRESULT_SUCCESS, (HANDLE) 1, 0);
				return 1;
			} else {
				if (fTempQun) {
					qpacket2=tqpacket=new QunSendTempIMPacket(READC_B2("QunType"),READC_D2("ParentQun"),uid);
				} else {
					qpacket2=qpacket=new QunSendIMExPacket(uid);
				}
				//qqSettings->allowUpdateTime=true;
			}
		} else { // Send Contact message
			if (fTemp) {
				DBVARIANT dbv;
				tpacket=new SendTempSessionTextIMPacket();
				tpacket->setReceiver(uid);
				if (!DBGetContactSettingTString(hContact,m_szModuleName,"Site",&dbv)) {
					LPSTR pszSite=mir_u2a_cp(dbv.ptszVal,936);
					tpacket->setSite(pszSite);
					DBFreeVariant(&dbv);
					mir_free(pszSite);
				}
				DBGetContactSetting(NULL,m_szModuleName,"Nick",&dbv);
				tpacket->setNick(dbv.pszVal);
				DBFreeVariant(&dbv);
			} else {
				packet=new SendTextIMPacket();
				DBDeleteContactSetting(hContact,m_szModuleName,"LastAutoReply");
				packet->setReceiver(uid);
			}
		}

		/*if (qqSettings->enableBBCode)*/ { // BBCode enabled
			bool withFormat=false;
			bool skipall=false;

			while (*msg_with_qq_smiley=='[' && !skipall) {
				if (*(msg_with_qq_smiley+2)==']') {
					// b or i or u
					switch (*(msg_with_qq_smiley+1)) {
							case 'b':
								if (packet) packet->setBold(true); else if (qpacket) qpacket->setBold(true); else tpacket->setBold(true);
								memmove(msg_with_qq_smiley,msg_with_qq_smiley+3,strlen(msg_with_qq_smiley+2));
								util_trimChatTags(msg_with_qq_smiley,"[/b]");
								break;
							case 'i':
								if (packet) packet->setItalic(true); else if (qpacket) qpacket->setItalic(true); else tpacket->setItalic(true);
								memmove(msg_with_qq_smiley,msg_with_qq_smiley+3,strlen(msg_with_qq_smiley+2));
								util_trimChatTags(msg_with_qq_smiley,"[/i]");
								break;
							case 'u':
								if (packet) packet->setUnderline(true); else if (qpacket) qpacket->setUnderline(true); else tpacket->setUnderline(true);
								memmove(msg_with_qq_smiley,msg_with_qq_smiley+3,strlen(msg_with_qq_smiley+2));
								util_trimChatTags(msg_with_qq_smiley,"[/u]");
								break;
							default:
								skipall=true;
								break;
					}
				} else if (!strncmp(msg_with_qq_smiley,"[color=",7)) {
					// set color
					char* color=strchr(msg_with_qq_smiley,'=')+1;

					switch (*color) {
							case 'r': // Red
								if (packet) {
									packet->setRed(255); packet->setGreen(0); packet->setBlue(0);
								} else if (qpacket2) {
									qpacket2->setRed(255); qpacket2->setGreen(0); qpacket2->setBlue(0);
								}
								memmove(msg_with_qq_smiley,msg_with_qq_smiley+11,strlen(msg_with_qq_smiley+10));
								break;
							case 'b': // Blue/Black
								if (*(color+2)=='u') { // Blue
									if (packet) {
										packet->setRed(0); packet->setGreen(0); packet->setBlue(255);
									} else if (qpacket2) {
										qpacket2->setRed(0); qpacket2->setGreen(0); qpacket2->setBlue(255);
									}
									memmove(msg_with_qq_smiley,msg_with_qq_smiley+12,strlen(msg_with_qq_smiley+11));
								} else { // Black
									if (packet) {
										packet->setRed(0); packet->setGreen(0); packet->setBlue(0);
									} else if (qpacket2) {
										qpacket2->setRed(0); qpacket2->setGreen(0); qpacket2->setBlue(0);
									}
									memmove(msg_with_qq_smiley,msg_with_qq_smiley+13,strlen(msg_with_qq_smiley+12));
								}
								break;
							case 'g': // Green
								if (packet) {
									packet->setRed(0); packet->setGreen(255); packet->setBlue(0);
								} else if (qpacket2) {
									qpacket2->setRed(0); qpacket2->setGreen(255); qpacket2->setBlue(0);
								}
								memmove(msg_with_qq_smiley,msg_with_qq_smiley+13,strlen(msg_with_qq_smiley+12));
								break;
							case 'm': // Magenta
								if (packet) {
									packet->setRed(255); packet->setGreen(0); packet->setBlue(255);
								} else if (qpacket2) {
									qpacket2->setRed(255); qpacket2->setGreen(0); qpacket2->setBlue(255);
								}
								memmove(msg_with_qq_smiley,msg_with_qq_smiley+15,strlen(msg_with_qq_smiley+14));
								break;
							case 'y': // Yellow
								if (packet) {
									packet->setRed(255); packet->setGreen(255); packet->setBlue(0);
								} else if (qpacket2) {
									qpacket2->setRed(255); qpacket2->setGreen(255); qpacket2->setBlue(0);
								}
								memmove(msg_with_qq_smiley,msg_with_qq_smiley+14,strlen(msg_with_qq_smiley+13));
								break;
							case 'w': // White
								if (packet) {
									packet->setRed(255); packet->setGreen(255); packet->setBlue(255);
								} else if (qpacket2) {
									qpacket2->setRed(255); qpacket2->setGreen(255); qpacket2->setBlue(255);
								}
								memmove(msg_with_qq_smiley,msg_with_qq_smiley+13,strlen(msg_with_qq_smiley+12));
								break;
					}
					util_trimChatTags(msg_with_qq_smiley,"[/color]");
				} else
					break;

				withFormat=true;
			}

			FontID fid = {sizeof(fid)};
			LOGFONTA font;
			strcpy(fid.name,packet?"Contact Messaging Font":"Qun Messaging Font");
			strcpy(fid.group,m_szModuleName);
			CallService(MS_FONT_GET,(WPARAM)&fid,(LPARAM)&font);
			COLORREF color=DBGetContactSettingDword(NULL,m_szModuleName,packet?"font1Col":"font2Col",0);

			if (!withFormat) { // No format specified, use from stored
				if (packet) {
					packet->setRed(GetRValue(color));
					packet->setGreen(GetGValue(color));
					packet->setBlue(GetBValue(color));
					packet->setBold(font.lfWeight>FW_NORMAL);
					packet->setItalic(font.lfItalic);
					packet->setUnderline(font.lfUnderline);
				} else if (qpacket2) {
					qpacket2->setRed(GetRValue(color));
					qpacket2->setGreen(GetGValue(color));
					qpacket2->setBlue(GetBValue(color));
					qpacket2->setBold(font.lfWeight>FW_NORMAL);
					qpacket2->setItalic(font.lfItalic);
					qpacket2->setUnderline(font.lfUnderline);
				}
			}

			int fontsize=(int)DBGetContactSettingByte(NULL,m_szModuleName,packet?"font1Size":"font2Size",9);
			//util_convertToGBK(font.lfFaceName);

			if (packet) {
				//packet->setFontName(string(font.lfFaceName));
				packet->setFontSize(fontsize);
			} else if (qpacket2) {
				//qpacket->setFontName(string(font.lfFaceName));
				qpacket2->setFontSize(fontsize);
			} else {
				//tpacket->setFontName(string(font.lfFaceName));
				tpacket->setFontSize(fontsize);
			}

		}

		int retseq=0;

		if (packet) {
			EvaHtmlParser parser;
			std::list<string> outPicList = parser.getCustomImages(msg_with_qq_smiley);
			packet->setAutoReply(true);

			char szTemp[701]={0};
			char* pszMsg=msg_with_qq_smiley;
			int msgCount=(int)ceil((float)strlen(msg_with_qq_smiley)/(700.0f));
			unsigned short seq;
			unsigned short messageID=HIWORD(GetTickCount());

			for (int c=0; c<msgCount; c++) {
				strncpy(szTemp,pszMsg,700);
				if (strlen(pszMsg)>700) {
					szTemp[700]=0;
					pszMsg+=700;
				}

				if (c>0) packet=new SendTextIMPacket(*packet);
				seq=DBGetContactSettingWord(hContact,m_szModuleName,"Sequence",0)+1;
				packet->setMsgSequence(seq);
				packet->setSequence(packet->getSequence()+1);
				packet->setNumFragments(msgCount);
				packet->setMessageID(messageID);
				packet->setSeqOfFragments(c);
				DBWriteContactSettingWord(hContact,m_szModuleName,"Sequence",seq);
				packet->setMessage(szTemp);
				if (c==0) {
					util_log(0,"Sequence of first IM packet is 0x%x",packet->getSequence());
					append(packet);
				} else {
					util_log(0,"Added IM packet 0x%x to queue",packet->getSequence());
					m_pendingImList[packet->getSequence()]=packet;
				}
			}
			retseq=packet->getSequence();
		} else if (tpacket) {
			unsigned int qqid=READC_D2(UNIQUEIDSETTING)-0x80000000;
			tpacket->setMessage(msg_with_qq_smiley); 
			//append(tpacket);
			m_savedTempSessionMsg=tpacket;
			EvaAddFriendGetAuthInfoPacket *packet = new EvaAddFriendGetAuthInfoPacket();
			packet->setSubSubCommand(AUTH_INFO_SUB_CMD_TEMP_SESSION);
			packet->setAddID(qqid);
			append(packet);
			retseq=1; //tpacket->getSequence();
		} else {
			char szTemp[691]={0};
			char* pszMsg=msg_with_qq_smiley;
			int msgCount=(int)ceil((float)strlen(msg_with_qq_smiley)/(690));
			if ((strlen(msg_with_qq_smiley)%690+12)>700) msgCount++;
			unsigned short seq;

			if (qpacket) {
				qpacket->setNumFragments(msgCount);
				qpacket->setMessageID(HIWORD(GetTickCount()));
			} else {
				tqpacket->setNumFragments(msgCount);
				tqpacket->setMessageID(HIWORD(GetTickCount()));
			}

			for (int c=0; c<msgCount; c++) {
				strncpy(szTemp,pszMsg,690);
				if (strlen(pszMsg)>690) {
					szTemp[690]=0;
					pszMsg+=690;
				} else if (strlen(pszMsg)>678) {
					szTemp[678]=0;
					pszMsg+=678;
				} else
					szTemp[strlen(pszMsg)]=0;

				if (qpacket) {
					if (c>0) qpacket=new QunSendIMExPacket(*qpacket);
					qpacket->setSeqOfFragments(c);
					seq=DBGetContactSettingWord(hContact,m_szModuleName,"Sequence",0)+1;
					qpacket->setSequence(seq);
					DBWriteContactSettingWord(hContact,m_szModuleName,"Sequence",seq);
					qpacket->setMessage(szTemp);
					if (c==0) {
						util_log(0,"Sequence of first Qun IM is 0x%x",qpacket->getSequence());
						append(qpacket);
					} else {
						util_log(0,"Added Qun IM packet 0x%x to queue",qpacket->getSequence());
						m_pendingImList[qpacket->getSequence()]=qpacket;
					}
				} else {
					if (c>0) tqpacket=new QunSendTempIMPacket(*tqpacket);
					tqpacket->setSeqOfFragments(c);
					seq=DBGetContactSettingWord(hContact,m_szModuleName,"Sequence",0)+1;
					tqpacket->setSequence(seq);
					DBWriteContactSettingWord(hContact,m_szModuleName,"Sequence",seq);
					tqpacket->setMessage(szTemp);
					if (c==0) {
						util_log(0,"Sequence of first Qun IM is 0x%x",tqpacket->getSequence());
						append(tqpacket);
					} else {
						util_log(0,"Added Qun IM packet 0x%x to queue",tqpacket->getSequence());
						m_pendingImList[tqpacket->getSequence()]=tqpacket;
					}
				}
			}
			if (DBGetContactSettingWord(hContact,m_szModuleName,"Status",ID_STATUS_ONLINE)!=ID_STATUS_ONLINE)
				// Wake this Qun up
				DBWriteContactSettingWord(hContact,m_szModuleName,"Status",ID_STATUS_ONLINE);

			retseq=qpacket2->getSequence();
		}

		/*if (!qqSettings->waitAck)*/
			//mir_forkthread(qq_im_sendacksuccess, ccs->hContact);
		// TODO: !
		//ProtoBroadcastAck(m_szModuleName, hContact, ACKTYPE_MESSAGE, ACKRESULT_SUCCESS, (HANDLE) 1, 0);


		mir_free(msg_with_qq_smiley);

		if (!m_needAck) {
			delayReport_t* dr=(delayReport_t*)mir_alloc(sizeof(delayReport_t));
			dr->hContact=hContact;
			dr->ackType=ACKTYPE_MESSAGE;
			dr->ackResult=ACKRESULT_SUCCESS;
			dr->aux=retseq;
			dr->aux2=NULL;
			ForkThread((ThreadFunc)&CNetwork::delayReport,dr);
		}
		return retseq;
	}

	return 0;
}

int __cdecl CNetwork::SendUrl(HANDLE hContact, int flags, const char* url) {
	return 1;
}

int __cdecl CNetwork::SetApparentMode(HANDLE hContact, int mode) {
	return 1;
}

int __cdecl CNetwork::SetStatus(int iNewStatus) {
	m_iDesiredStatus=iNewStatus;
	util_log(0,"PS_SETSTATUS(%d,0)", iNewStatus);

	if (m_iDesiredStatus==ID_STATUS_OFFLINE) {
		if (Packet::isClientKeySet()) {
			append(new ChangeStatusPacket(QQ_FRIEND_STATUS_OFFLINE));
			for(int i=0; i<4; i++) {
				append(new LogoutPacket());
			}
		}
		disconnect();
	}
	else if (m_iStatus==ID_STATUS_OFFLINE)
	{
		DBVARIANT dbv={0};
		HANDLE hContact=NULL;

		if (m_myqq=READC_D2(UNIQUEIDSETTING)) {
			if (READC_S2(QQ_PASSWORD,&dbv)!=NULL) {
				if (dbv.pszVal) DBFreeVariant(&dbv);
				ASKDLGPARAMS* adp=(ASKDLGPARAMS*)malloc(sizeof(ASKDLGPARAMS));
				adp->network=this;
				adp->command=QQ_CMD_LOGIN;
				adp->hContact=(HANDLE)iNewStatus;
				BroadcastStatus(ID_STATUS_CONNECTING);
				DialogBoxParam(hinstance,MAKEINTRESOURCE(IDD_CHANGESIGNATURE),NULL,ModifySignatureDlgProc,(LPARAM)adp);
			} else {
				DBFreeVariant(&dbv);

				CEvaAccountSwitcher::ProcessAs(m_myqq);
				if (READC_S2(QQ_LOGINSERVER2,&dbv)!=0 || !setConnectString(dbv.pszVal)) {
					if (dbv.pszVal) DBFreeVariant(&dbv);
					ShowNotification(TranslateW(L"Invalid connection string specified."),NIIF_ERROR);
					CEvaAccountSwitcher::EndProcess();
				} else {
					DBFreeVariant(&dbv);
					BroadcastStatus(ID_STATUS_CONNECTING);
					Packet::setQQ(m_myqq);
					m_IsDetecting=false;
					if (READC_B2(QQ_INVISIBLE)) m_iDesiredStatus=ID_STATUS_INVISIBLE;
					CEvaAccountSwitcher::EndProcess();
					connect();
				}
			}
		} else
			ShowNotification(TranslateW(L"You didn't specify the QQID to be used."),NIIF_ERROR);
	}
	else if (m_iStatus!=ID_STATUS_CONNECTING)
		SetServerStatus(m_iDesiredStatus);
	return 0;

}

HANDLE __cdecl CNetwork::GetAwayMsg(HANDLE hContact) {
	ForkThread((ThreadFunc)&CNetwork::GetAwayMsgThread,hContact);
	return (HANDLE)1;
}

int __cdecl CNetwork::RecvAwayMsg(HANDLE hContact, int mode, PROTORECVEVENT* evt) {
	return 1;
}

int __cdecl CNetwork::SendAwayMsg(HANDLE hContact, HANDLE hProcess, const char* msg) {
	return 1;
}

int __cdecl CNetwork::SetAwayMsg(int iStatus, const char* msg) {
	if (!Packet::isClientKeySet()) return 1;

	if (READ_B2(NULL,QQ_STATUSASPERSONAL)==1) {
		if (!msg || !*msg) {
			util_log(0,"%s(): Remove mode message for %d",__FUNCTION__,iStatus);
			append(new SignaturePacket(QQ_SIGNATURE_DELETE));
		} else {
			LPTSTR szSign=mir_a2u(msg);
			LPSTR pszSend=mir_u2a_cp(szSign,936);
			util_log(0,"%s(): Set mode message for %d to %S",__FUNCTION__,iStatus,szSign);
			WRITE_TS(NULL,"PersonalSignature",szSign);
			mir_free(szSign);
			SignaturePacket *packet = new SignaturePacket(QQ_SIGNATURE_MODIFY);
			packet->setSignature(pszSend);
			append(packet);
			mir_free(pszSend);
		}
	}

	return 0;
}

int __cdecl CNetwork::UserIsTyping(HANDLE hContact, int type) {
	return 1;
}

/////////////////////////////////////////////////////////////////////////////////////////
// OnEvent - maintain protocol events

int __cdecl CNetwork::OnEvent(PROTOEVENTTYPE eventType, WPARAM wParam, LPARAM lParam) {
	switch( eventType ) {
		case EV_PROTO_ONLOAD:  return OnModulesLoadedEx(0,0);
		case EV_PROTO_ONEXIT:  return OnPreShutdown(0,0);
		case EV_PROTO_ONOPTIONS: return OnOptionsInit( wParam, lParam ); // This is only called from AcctMgr
		case EV_PROTO_ONRENAME:
			{	
				CLISTMENUITEM clmi={sizeof(clmi)};
				clmi.flags=CMIM_NAME|CMIF_TCHAR;
				clmi.ptszName=m_tszUserName;
				CallService(MS_CLIST_MODIFYMENUITEM, (WPARAM)m_hMenuRoot, (LPARAM)&clmi);
			}	
		//case EV_PROTO_ONREADYTOEXIT:
		default: return 1;
	}	
}

int __cdecl CNetwork::OnPreShutdown(WPARAM wParam, LPARAM lParam) {
	util_log(0,__FUNCTION__);
	return 0;
}

INT_PTR CALLBACK ChooseAccountDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
		case WM_INITDIALOG:
			{
				HWND hWndList=GetDlgItem(hwndDlg,IDC_QQLIST);
				WCHAR szTemp[MAX_PATH];
				int id;

				TranslateDialogDefault(hwndDlg);

				for (list<CNetwork*>::iterator iter=g_networks.begin(); iter!=g_networks.end(); iter++) {
					swprintf(szTemp,L"%s (%d)",(*iter)->m_tszUserName,(*iter)->GetMyQQ());
					id=SendMessage(hWndList,LB_ADDSTRING,0,(LPARAM)szTemp);
					SendMessage(hWndList,LB_SETITEMDATA,id,(LPARAM)*iter);
				}
			}
			return TRUE;
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDOK:
					{
						HWND hWndList=GetDlgItem(hwndDlg,IDC_QQLIST);
						int id=SendMessage(hWndList,LB_GETCURSEL,0,0);
						if (id!=LB_ERR) {
							INT_PTR ptr=(INT_PTR)SendMessage(hWndList,LB_GETITEMDATA,id,0);
							EndDialog(hwndDlg,ptr);
						}
					}
					break;
				case IDCANCEL:
					EndDialog(hwndDlg,0);
					break;
			}
	}
	return FALSE;
}

extern "C" int ParseTencentURI(WPARAM wParam, LPARAM lParam) {
	// tencent://Message/?Uin=251464630&websiteName=qzone.qq.com&Menu=yes
	if (g_networks.size()==0)
		MessageBox(NULL,TranslateT("The URI shortcut cannot be handled because you did not create a MirandaQQ account."),L"MirandaQQ",NIIF_ERROR);
	else {
		int connected=0;
		list<CNetwork*>::iterator iter2;
		LPWSTR pszUrl=(LPWSTR)lParam;
		_wcslwr(pszUrl);

		if (wcsncmp(pszUrl,L"tencent://message/?",19)) return 1;

		for (list<CNetwork*>::iterator iter=g_networks.begin(); iter!=g_networks.end(); iter++) {
			if (CallProtoService((*iter)->m_szModuleName,PS_GETSTATUS,0,0)!=ID_STATUS_OFFLINE) {
				connected++;
				if (connected==1)
					iter2=iter;
				else
					break;
			}
		}

		if (connected==0) {
			MessageBox(NULL,TranslateT("The URI shortcut cannot be handled because you don't have an online MirandaQQ account."),L"MirandaQQ",NIIF_ERROR);
		} else {
			CNetwork* network;

			if (connected>1) {
				if (!(network=(CNetwork*)DialogBox(hinstance,MAKEINTRESOURCE(IDD_CHOOSEACCOUNT),NULL,ChooseAccountDialogProc))) return 1;
			} else
				network=*iter2;

			HANDLE hContact;
			int qqid=_wtoi(wcsstr(pszUrl,L"uin=")+4);
			if (!(hContact=network->FindContact(qqid))) {
				hContact=network->AddContact(qqid,true,false);
				network->append(new GetUserInfoPacket(qqid));
			}

			CallService(MS_MSG_SENDMESSAGE,(WPARAM)hContact,NULL);
		}
	}
	return 0;
}

#ifdef MIRANDAQQ_IPC
int __cdecl CNetwork::IPCService(WPARAM wParam,LPARAM lParam) {
	if (!Packet::isClientKeySet()) return 1;

	switch (wParam) {
			case QQIPCSVC_QUN_UPDATE_ONLINE_MEMBERS:
				append(new QunGetOnlineMemberPacket(lParam));
				break;
			case QQIPCSVC_QUN_SEND_MESSAGE:
				{
					ipcsendmessage_t* ism=(ipcsendmessage_t*)lParam;
					/*
					LPSTR pszMessage=(LPSTR)mir_alloc((wcslen(ism->message)+1)*sizeof(WCHAR)*3);
					WideCharToMultiByte(CP_ACP,0,ism->message,-1,pszMessage,(wcslen(ism->message)+1)*sizeof(WCHAR),NULL,NULL);
					wcscpy((LPWSTR)(pszMessage+strlen(pszMessage)+1),ism->message);
					CallContactService(FindContact(ism->qunid), PSS_MESSAGE, 0, (LPARAM)pszMessage);
					*/
					LPSTR pszMessage=mir_utf8encodeW(ism->message);
					CallContactService(FindContact(ism->qunid), PSS_MESSAGE, PREF_UTF, (LPARAM)pszMessage);
					mir_free(pszMessage);
				}
				break;
			case QQIPCSVC_QUN_UPDATE_INFORMATION:
				{
					HANDLE hContact=FindContact(lParam);
					WRITEC_D("QunCardUpdate",-1);
					append(new QunGetInfoPacket(lParam));
				}
				break;
			case QQIPCSVC_FIND_CONTACT:
				return (int)FindContact((int)lParam);
			case QQIPCSVC_SHOW_DETAILS:
				{
					HANDLE hContact=FindContact((int)lParam);
					if (!hContact) hContact=AddContact(lParam,true,true);
					CallService(MS_USERINFO_SHOWDIALOG, (WPARAM)hContact, 0);
				}
				break;
			case QQIPCSVC_QUN_KICK_USER:
				mir_forkthread(KickQunUser,(LPVOID)lParam);
				break;
			case QQIPCSVC_GET_NETWORK:
				return (int)this;
	}
	return 0;
}
#endif
