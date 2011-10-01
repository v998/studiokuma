#ifndef MIRANDAQQ_H
#define MIRANDAQQ_H

//=======================================================
//	Definitions
//=======================================================
#define DEBUG_LEVEL			0
#define APPNAME							_T("MirandaQQ2")

#define QQ_LOGINPORT                   "LoginPort"
#define QQ_LOGINSERVER2                "Server"
#define QQ_PASSWORD                    "Password"
#define QQ_INVISIBLE                   "LoginInvisible"

#define QQ_CHARSET						"ConversionMode"
#define QQ_AVATARTYPE					"AvatarType"
#define QQ_DISABLEIDENTITY				"DisableContactIdentity"
#define QQ_REMOVENICKCHARS				"RemoveNicknameChars"
#define QQ_SUPPRESSQUNMSG				"SuppressQunMessage"
#define QQ_BLOCKEMPTYREQUESTS			"BlockEmptyRequests"
#define QQ_REPLYMODEMSG					"ReplyModeMessage"
#define QQ_NOREMOVEQUNIMAGE				"NoRemoveQunImage"
#define QQ_NOLOADQUNIMAGE				"NoLoadQunImage"
#define QQ_ADDQUNNUMBER					"AddQunNumber"
#define QQ_STATUSASPERSONAL				"StatusMsgAsPersonalSignature"
#define QQ_WAITACK						"WaitForAck"
#define QQ_SHOWAD						"ShowAD"
#define QQ_MAILNOTIFY					"MailNotify"
#define QQ_ENABLEQUNLIST				"EnableQunList"
#define QQ_NOAUTOSERVER					"NoAutoServer"
#define QQ_FORCEUNICODE					"ForceUnicode"
#define QQ_MESSAGECONVERSION			"MessageConversion"
#define QQ_NOSILENTQUNHISTORY			"NoSlientQunHistory"
#define QQ_BRIDGETARGET					"BridgeTarget"
#define QQ_MANUALVERSION				"ManualVersion"
#define QQ_NOPROGRESSPOPUPS				"NoProgressPopups"
#define QQ_DISABLEHTTPD					"DisableHTTPD"
#define QQ_HTTPDALLOWEXTERNAL			"HTTPDAllowExternal"
#define QQ_HTTPDPORT					"HTTPDPort"
#define QQ_HTTPDROOT					"HTTPDRoot"
#define QQ_CONSERVATIVE					"ConservativeMode"
#define QQ_CONSERVATIVESTATE			"ConservativeState"

#define QQ_DEFAULT_CONNECTION			"udp://sz.tencent.com:8000"

#define UNIQUEIDSETTING					"UID"
#define QQ_MENU_SUPPRESSQUN				"/SuppressQunMessages"
#define QQ_MENU_COPYIP					"/CopyIP"
#define QQ_MENU_DOWNLOADGROUP			"/DownloadGroup"
#define QQ_MENU_UPLOADGROUP				"/UploadGroup"
#define QQ_MENU_REMOVENONSERVERCONTACTS	"/RemoveNonServerContacts"
#define QQ_MENU_SUPPRESSADDREQUESTS		"/SuppressAddRequests"
#define QQ_MENU_MODIFYSIGNATURE			"/ModifySignature"
#define QQ_MENU_CHANGENICKNAME			"/ChangeNickName"
#define QQ_MENU_DOWNLOADUSERHEAD		"/DownloadUserHead"
#define QQ_MENU_GETWEATHER				"/GetWeather"
#define QQ_MENU_TOGGLEQUNLIST			"/ToggleQunList"
#define QQ_MENU_CHANGEHEADIMAGE			"/ChangeHeadImage"
#define QQ_MENU_QQMAIL					"/QQMail"

#define QQ_CNXTMENU_REMOVEME			"/CnxtRemoveMe"
#define QQ_CNXTMENU_ADDQUNMEMBER		"/CnxtAddQunMember"
#define QQ_CNXTMENU_SILENTQUN			"/SilentQun"
#define QQ_CNXTMENU_REAUTHORIZE			"/ReAuthorize"
#define QQ_CNXTMENU_CHANGECARDNAME		"/ChangeCardName"
#define QQ_CNXTMENU_POSTIMAGE			"/PostImage"
#define QQ_CNXTMENU_SELECTIMAGE			"/SelectImage"
#define QQ_CNXTMENU_QUNSPACE			"/QunSpace"
#define QQ_CNXTMENU_FORCEREFRESH		"/ForceRefresh"
#define QQ_CNXTMENU_CHANGEEIP			"/ChangeEIP"

#define WRITE_S(c,k,v) DBWriteContactSettingString(c,m_szModuleName,k,v)
#define WRITE_TS(c,k,v) DBWriteContactSettingTString(c,m_szModuleName,k,v)
#define WRITE_U8S(c,k,v) DBWriteContactSettingUTF8String(c,m_szModuleName,k,v)
#define WRITE_B(c,k,v) DBWriteContactSettingByte(c,m_szModuleName,k,v)
#define WRITE_W(c,k,v) DBWriteContactSettingWord(c,m_szModuleName,k,v)
#define WRITE_D(c,k,v) DBWriteContactSettingDword(c,m_szModuleName,k,v)
#define WRITEC_S(k,v) WRITE_S(hContact,k,v)
#define WRITEC_TS(k,v) WRITE_TS(hContact,k,v)
#define WRITEC_U8S(k,v) WRITE_U8S(hContact,k,v)
#define WRITEC_B(k,v) WRITE_B(hContact,k,v)
#define WRITEC_W(k,v) WRITE_W(hContact,k,v)
#define WRITEC_D(k,v) WRITE_D(hContact,k,v)
#define WRITEINFO_S(k,i) WRITEC_S(k,info.at(i).c_str())
//#define WRITEINFO_TS(k,i) pszTemp=mir_a2u_cp(info.at(i).c_str(),936); WRITEC_TS(k,pszTemp); mir_free(pszTemp)
#define WRITEINFO_U8S(k,i) if (info.at(i)) WRITEC_U8S(k,info.at(i)+2)
#define WRITEINFO_B(k,i) if (info.at(i)) WRITEC_B(k,*(unsigned char*)(info.at(i)+2))
#define WRITEINFO_W(k,i) if (info.at(i)) WRITEC_W(k,htons(*(unsigned short*)(info.at(i)+2)))
#define WRITEINFO_D(k,i) if (info.at(i)) WRITEC_D(k,htonl(*(unsigned int*)(info.at(i)+2)))

#define READ_S(c,k,v) if (!DBGetContactSetting(c,m_szModuleName,k,&dbv)) {strcpy(v,dbv.pszVal);DBFreeVariant(&dbv);} else *v=0
#define READ_S2(c,k,v) DBGetContactSetting(c,m_szModuleName,k,v)
#define READ_TS2(c,k,v) DBGetContactSettingTString(c,m_szModuleName,k,v)
#define READ_U8S2(c,k,v) DBGetContactSettingUTF8String(c,m_szModuleName,k,v)
#define READ_B2(c,k) DBGetContactSettingByte(c,m_szModuleName,k,0)
#define READ_B(c,k,v) v=DBGetContactSettingByte(c,m_szModuleName,k,0)
#define READ_W2(c,k) DBGetContactSettingWord(c,m_szModuleName,k,0)
#define READ_W(c,k,v) v=DBGetContactSettingWord(c,m_szModuleName,k,0)
#define READ_D2(c,k) DBGetContactSettingDword(c,m_szModuleName,k,0)
#define READ_D(c,k,v) v=DBGetContactSettingDword(c,m_szModuleName,k,0)
#define READ_2(c,k,v) DBGetContactSetting(c,m_szModuleName,k,v)
#define READC_2(c,k,v) READ_2(hContact,k,v)
#define READC_S2(k,v) READ_S2(hContact,k,v)
#define READC_TS2(k,v) READ_TS2(hContact,k,v)
#define READC_U8S2(k,v) READ_U8S2(hContact,k,v)
#define READC_B2(k) READ_B2(hContact,k)
#define READC_B(k,v) READ_B(hContact,k,v)
#define READC_W(k,v) READ_W(hContact,k,v)
#define READC_D(k,v) READ_D(hContact,k,v)
#define READC_D2(k) READ_D2(hContact,k)
#define READC_W2(k) READ_W2(hContact,k)

#define DELC(k) DBDeleteContactSetting(hContact,m_szModuleName,k)

#define PS_SETMYAVATAR "/SetMyAvatar"
#define PS_GETMYAVATAR "/GetMyAvatar"
#define PS_GETMYAVATARMAXSIZE "/GetMyAvatarMaxSize"

#define PS_SETMYNICKNAME "/SetNickname"

#define PS_GETMYNICKNAMEMAXLENGTH "/GetMyNicknameMaxLength"

//=======================================================
//	Defines
//=======================================================
//General
extern HINSTANCE hinstance;
extern HANDLE hNetlibUser;
extern char g_dllname[MAX_PATH];

typedef struct {
	CNetwork* network;
	unsigned int qunid;
	unsigned int qqid;
} KICKUSERSTRUCT;

typedef struct {
	CNetwork* network;
	unsigned int command;
	HANDLE hContact;
	LPVOID pAux;
	int nAux;
	bool fAux;
} ASKDLGPARAMS;

#if 0 // Disabled
class ft_t {
public:
	int qqid;
	int sessionid;
	string file;
	int size;
	HWND hWndPopup;
};

extern map<unsigned int,ft_t*> ftSessions;
#endif

#ifdef MIRANDAQQ_IPC
extern HANDLE hIPCEvent;
#endif

extern char* servers[];

extern list<CNetwork*> g_networks;
extern bool g_enableBBCode;

#endif

