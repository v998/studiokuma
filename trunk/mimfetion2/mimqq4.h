// main.cpp
extern MM_INTERFACE   mmi;
extern UTF8_INTERFACE utfi;
//extern MD5_INTERFACE md5i;
//extern LIST_INTERFACE li;

extern PLUGINLINK* pluginLink;

#define UNIQUEIDSETTING "UID"
#define FOLDER_AVATARS	0
#define FOLDER_QUNIAGES	0
#define FOLDER_WEBROOT	0

// Database helper functions
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

// Database Entries
#define QQ_LOGIN_INVISIBLE                   "LoginInvisible"
#define QQ_LOGIN_PASSWORD                    "Password"
#define QQ_HTTPDPORT                         "HTTPDPort"
#define QQ_SILENTQUN "SilentQun"
#define QQ_INFO_EXTID "ExternalID"
#define QQ_STATUS "Status"

// Contact List Services
#define QQ_CNXTMENU_POSTIMAGE "/PostImage"
#define QQ_CNXTMENU_SILENTQUN "/SilentQun"
#define QQ_CNXTMENU_FORCESMS  "/ForceSMS"

#define QQ_MENU_DOWNLOADGROUP "/DownloadGroup"
