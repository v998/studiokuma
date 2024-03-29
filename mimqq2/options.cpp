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
/* Options.cpp: Option page related functions
 */

#include "StdAfx.h"
#include "eva/version.cpp"

const char* PASSWORDMASK="\x1\x1\x1\x1\x1\x1\x1\x1";	// Masked Default Password
/*
typedef struct {
	HWND hwndAcc;
	HWND hwndSettings;
	HWND hwndQun;
	//HWND hwndHelper;
	bool fInit;
} LocalOptions_t;

map<CNetwork*,LocalOptions_t*> networkmap;
*/
static HWND hwndGlobal=0;
static HWND hwndGlobalFor=0;
static HWND hwndHelper=0;

//static HWND hwndAcc=0, hwndSettings=0, hwndQun=0/*, hwndBridging=0*/, hwndHelper=0;

#define CHECK_BYTE_SETTING(db,rsrc) if (DBGetContactSettingByte(NULL,network->m_szModuleName,db,0)!=IsDlgButtonChecked(hwndDlg,rsrc)) reconnectRequired=1
#define CHECK_BYTE_SETTING2(db,rsrc) if (DBGetContactSettingByte(NULL,network->m_szModuleName,db,0)!=!IsDlgButtonChecked(hwndDlg,rsrc)) reconnectRequired=1
#define WRITE_BYTE_SETTING(db,val) DBWriteContactSettingByte(NULL,network->m_szModuleName,db,(BYTE)val)
#define WRITE_CHECK_SETTING(db,rsrc) WRITE_BYTE_SETTING(db,(BYTE)IsDlgButtonChecked(hwndDlg,rsrc))
#define WRITE_CHECK_SETTING2(db,rsrc) WRITE_BYTE_SETTING(db,(BYTE)!IsDlgButtonChecked(hwndDlg,rsrc))
#define CHECK_CONTROL(rsrc,db) CheckDlgButton(hwndDlg, rsrc, DBGetContactSettingByte(NULL,network->m_szModuleName,db,0))
#define CHECK_CONTROL2(rsrc,db) CheckDlgButton(hwndDlg, rsrc, !DBGetContactSettingByte(NULL,network->m_szModuleName,db,0))
#define ADD_LIST_ITEM(s) SendMessageA(hControl,CB_ADDSTRING,0,(LPARAM)s)

// DlgProcOpts(): Dialog Callback
// hwndDlg: hWnd to options dialog
// msg: Window message received
// wParam: Depending on msg
// lParam: Depending on msg
static BOOL CALLBACK DlgProcOptsAcc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	CNetwork* network=(CNetwork*)GetWindowLong(GetParent(hwndDlg),GWL_USERDATA);

	switch (msg)
	{
		case WM_INITDIALOG:	// Options dialog is being initialized
		{
			DBVARIANT dbv;
			HWND hControl;
			char** pszServers=servers;
			TranslateDialogDefault(hwndDlg);

			// Add server to list
			hControl=GetDlgItem(hwndDlg,IDC_SERVER);
			while (*pszServers) {
				ADD_LIST_ITEM(*pszServers);
				pszServers++;
			}

			SendDlgItemMessage(hwndDlg,IDC_AVATARTYPE,CB_ADDSTRING,NULL,(LPARAM)TranslateT("Head Image and User Head"));
			SendDlgItemMessage(hwndDlg,IDC_AVATARTYPE,CB_ADDSTRING,NULL,(LPARAM)TranslateT("Head Image"));
			SendDlgItemMessage(hwndDlg,IDC_AVATARTYPE,CB_ADDSTRING,NULL,(LPARAM)TranslateT("QQ Show"));
			SendDlgItemMessage(hwndDlg,IDC_AVATARTYPE,CB_ADDSTRING,NULL,(LPARAM)TranslateT("None"));

			SendDlgItemMessage(hwndDlg,IDC_CONVERSION,CB_ADDSTRING,NULL,(LPARAM)TranslateT("No Message Conversion"));
			SendDlgItemMessage(hwndDlg,IDC_CONVERSION,CB_ADDSTRING,NULL,(LPARAM)TranslateT("Receive: No Change, Send: Convert to Simplified"));
			SendDlgItemMessage(hwndDlg,IDC_CONVERSION,CB_ADDSTRING,NULL,(LPARAM)TranslateT("Receive: Convert to Traditional, Send: No Change"));
			SendDlgItemMessage(hwndDlg,IDC_CONVERSION,CB_ADDSTRING,NULL,(LPARAM)TranslateT("Receive: Convert to Traditional, Send: Convert to Simplified"));

			if (!(hControl=GetDlgItem(hwndDlg,IDC_NOTICE))) itoa(1,(LPSTR)hControl,10);
			SetWindowText(hControl,L"Miranda IM 专用的 QQ 插件 (MirandaQQ)\n版权所有(C) 2008 小熊工作室，保留所有权利。\n此插件使用 libeva QQ 库，版权所有(C) 云帆。\nThe QQ icon is from Cristal Icons pack, courtesy of Angeli-Ka. Used with permission from author.\n\n此插件是根据 GPL 第二版的条款而授权。你可以修改并重新发布基于此产品的成果，但你「必须保留所有原有的版权宣告」，并将有关之所有代码公开。若要更多信息，请参照 http://www.gnu.org/licenses/old-licenses/gpl-2.0.html 上的 GPL 合约条款，或 http://docs.huihoo.com/open_source/GPL-chinese.html 参考该条款的中文译本。\n最后，由于太多人滥用桥接功能 (桥接20个群过分么?)，所以此功能将被永久移除。你们要加回去请自便，但不要再在支持群里询问有关问题。");
			ShowWindow(hControl,SW_SHOWNORMAL);

			if (network) {
				SendMessage(GetDlgItem(hwndDlg,IDC_CONVERSION),CB_SETCURSEL,DBGetContactSettingByte(NULL,network->m_szModuleName,QQ_MESSAGECONVERSION,0),0);

				if(!DBGetContactSetting(NULL,network->m_szModuleName,QQ_LOGINSERVER2,&dbv)) {
					SetDlgItemTextA(hwndDlg,IDC_SERVER,dbv.pszVal);
					DBFreeVariant(&dbv);
				} else {
					SetDlgItemTextA(hwndDlg,IDC_SERVER,*servers);
				}

				// Login ID and Password
				if(!DBGetContactSetting(NULL,network->m_szModuleName,UNIQUEIDSETTING,&dbv)){
					char szID[16];
					ultoa(dbv.lVal,szID,10);
					SetDlgItemTextA(hwndDlg,IDC_LOGIN,szID);
					DBFreeVariant(&dbv);
				}

				SetDlgItemTextA(hwndDlg,IDC_PASSWORD,PASSWORDMASK);

				CHECK_CONTROL(IDC_FORCEINVISIBLE,QQ_INVISIBLE);
				//CHECK_CONTROL(IDC_FORCEUNICODE,QQ_FORCEUNICODE);
				CheckDlgButton(hwndDlg,IDC_FORCEUNICODE,BST_CHECKED);
				EnableWindow(GetDlgItem(hwndDlg,IDC_FORCEUNICODE),FALSE);
				//CHECK_CONTROL2(IDC_AUTOSERVER,QQ_NOAUTOSERVER);
				ShowWindow(GetDlgItem(hwndDlg,IDC_AUTOSERVER),SW_HIDE);

				// Version and Connection Information
				/*
				pszTemp=mir_strdup(szCheckoutTime);

				SetDlgItemTextA(hwndDlg,IDC_VERSION,pszTemp);
				mir_free(pszTemp);
				*/

				WCHAR szTemp[MAX_PATH];
				swprintf(szTemp,TranslateT("This copy of MirandaQQ is based on EVA %S, Module: %S"),version,network->m_szModuleName);
				SetDlgItemText(hwndDlg,IDC_VERSION,szTemp);

				//if (qqSettings->unicode)
					SetDlgItemText(hwndDlg,IDC_UNICODEMSG,TranslateT("Unicode Messaging Support(UTF-8) is Enabled."));
				/*else
					SetDlgItemText(hwndDlg,IDC_UNICODEMSG,TranslateT("Unicode Messaging Support is Disabled."));*/
			}
			return TRUE;
		}


		case WM_COMMAND:	// When a control is toggled
			switch (LOWORD(wParam)) {
				case IDC_LOGIN:
				case IDC_PASSWORD:
				//case IDC_SERVER:
				case IDC_VERSION:
					if (HIWORD(wParam) != EN_CHANGE || (HWND)lParam != GetFocus()) return 0;
					break;
				case IDC_CONVERSION:
				case IDC_SERVER:
					if (HIWORD(wParam) != CBN_SELCHANGE) return 0;
					
			}
			SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
			break;
		
		case WM_NOTIFY:		// When a notify is sent by Options Dialog (Property Sheet)
			if (network && ((LPNMHDR)lParam)->code==PSN_APPLY) {
				int reconnectRequired=0;
				char str[128];
				char login[128];
				DBVARIANT dbv;

				// Login Information
				GetDlgItemTextA(hwndDlg,IDC_LOGIN,login,sizeof(login));
				dbv.pszVal=NULL;
				dbv.lVal=0;
				if(DBGetContactSetting(NULL,network->m_szModuleName,UNIQUEIDSETTING,&dbv) || strtoul(login,NULL,10)!=dbv.dVal)
					reconnectRequired=1;
				
				if(dbv.lVal!=0) DBFreeVariant(&dbv);
				DBWriteContactSettingDword(NULL,network->m_szModuleName,UNIQUEIDSETTING,strtoul(login,NULL,10));
				
				GetDlgItemTextA(hwndDlg,IDC_PASSWORD,str,sizeof(str));
				if (strcmp(str,PASSWORDMASK)!=0) {
					CallService(MS_DB_CRYPT_ENCODESTRING,sizeof(str),(LPARAM)str);
					dbv.pszVal=NULL;
					if(DBGetContactSetting(NULL,network->m_szModuleName,QQ_PASSWORD,&dbv) || strcmp(str,dbv.pszVal))
						reconnectRequired=1;
				
					
					if(dbv.pszVal!=NULL) DBFreeVariant(&dbv);
					DBWriteContactSettingString(NULL,network->m_szModuleName,QQ_PASSWORD,str);
				}

				// Server list
				GetDlgItemTextA(hwndDlg,IDC_SERVER,str,sizeof(str));
				if(DBGetContactSetting(NULL,network->m_szModuleName,QQ_LOGINSERVER2,&dbv) || strcmp(str,dbv.pszVal))
					reconnectRequired=1;
					
				if(dbv.pszVal!=NULL) DBFreeVariant(&dbv);
				DBWriteContactSettingString(NULL,network->m_szModuleName,QQ_LOGINSERVER2,str);

				CHECK_BYTE_SETTING(QQ_INVISIBLE,IDC_FORCEINVISIBLE);
				WRITE_CHECK_SETTING(QQ_INVISIBLE,IDC_FORCEINVISIBLE);

				CHECK_BYTE_SETTING(QQ_FORCEUNICODE,IDC_FORCEUNICODE);
				WRITE_CHECK_SETTING(QQ_FORCEUNICODE,IDC_FORCEUNICODE);

				/*
				CHECK_BYTE_SETTING2(QQ_NOAUTOSERVER,IDC_AUTOSERVER);
				WRITE_CHECK_SETTING2(QQ_NOAUTOSERVER,IDC_AUTOSERVER);
				*/

				if(Packet::isClientKeySet() && reconnectRequired) MessageBox(hwndDlg,TranslateT("The changes you have made require you to reconnect to the QQ network before they take effect"),APPNAME,MB_OK);

				DBWriteContactSettingByte(NULL,network->m_szModuleName,QQ_MESSAGECONVERSION,(BYTE)SendDlgItemMessage(hwndDlg,IDC_CONVERSION,CB_GETCURSEL,0,0));

				return TRUE;
			}
			break;
	}
	return FALSE;
}

extern "C" {
	int SelectFont(WPARAM, LPARAM);
	int SelectColor(WPARAM, LPARAM);
}

static BOOL CALLBACK DlgProcOptsSettings(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	CNetwork* network=(CNetwork*)GetWindowLong(GetParent(hwndDlg),GWL_USERDATA);

	switch (msg) {
		case WM_INITDIALOG:	// Options dialog is being initialized
			{
				TranslateDialogDefault(hwndDlg);

				// Add server to list
				SendDlgItemMessage(hwndDlg,IDC_AVATARTYPE,CB_ADDSTRING,NULL,(LPARAM)TranslateT("Head Image and User Head"));
				SendDlgItemMessage(hwndDlg,IDC_AVATARTYPE,CB_ADDSTRING,NULL,(LPARAM)TranslateT("Head Image"));
				SendDlgItemMessage(hwndDlg,IDC_AVATARTYPE,CB_ADDSTRING,NULL,(LPARAM)TranslateT("QQ Show"));
				SendDlgItemMessage(hwndDlg,IDC_AVATARTYPE,CB_ADDSTRING,NULL,(LPARAM)TranslateT("None"));

				if (network) {
					SendDlgItemMessage(hwndDlg,IDC_AVATARTYPE,CB_SETCURSEL,DBGetContactSettingByte(NULL,network->m_szModuleName,QQ_AVATARTYPE,0),0);

					// Check Options
					//CHECK_CONTROL(IDC_BBCODE,QQ_ENABLEBBCODE);
					//CHECK_CONTROL(IDC_SHOWCHAT, DBGetContactSettingByte(NULL,network->m_szModuleName,QQ_SHOWCHAT,0));
					CHECK_CONTROL(IDC_NOADDIDENTITY,QQ_DISABLEIDENTITY);
					CHECK_CONTROL(IDC_REMOVENICKCHARS,QQ_REMOVENICKCHARS);
					CHECK_CONTROL(IDC_REPLYMODEMSG,QQ_REPLYMODEMSG);
					CHECK_CONTROL(IDC_CUSTOMBLOCK2,QQ_BLOCKEMPTYREQUESTS);
					CHECK_CONTROL(IDC_STATUSASPERSONAL,QQ_STATUSASPERSONAL);
					CHECK_CONTROL(IDC_WAITACK,QQ_WAITACK);
					CHECK_CONTROL(IDC_SHOWAD,QQ_SHOWAD);
					CHECK_CONTROL(IDC_MAILNOTIFY,QQ_MAILNOTIFY);
					CHECK_CONTROL(IDC_NOPROGRESSPOPUPS,QQ_NOPROGRESSPOPUPS);
				}

				return TRUE;
			}

		case WM_COMMAND:	// When a control is toggled
			SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
			break;

		case WM_NOTIFY:		// When a notify is sent by Options Dialog (Property Sheet)
			if (network && ((LPNMHDR)lParam)->code==PSN_APPLY) {
				int reconnectRequired=0;

				// Check Options
				WRITE_BYTE_SETTING(QQ_AVATARTYPE,SendDlgItemMessage(hwndDlg,IDC_AVATARTYPE,CB_GETCURSEL,0,0));

				CHECK_BYTE_SETTING(QQ_DISABLEIDENTITY,IDC_NOADDIDENTITY);
				WRITE_CHECK_SETTING(QQ_DISABLEIDENTITY,IDC_NOADDIDENTITY);

				CHECK_BYTE_SETTING(QQ_REMOVENICKCHARS,IDC_REMOVENICKCHARS);
				WRITE_CHECK_SETTING(QQ_REMOVENICKCHARS,IDC_REMOVENICKCHARS);

				WRITE_CHECK_SETTING(QQ_REPLYMODEMSG,IDC_REPLYMODEMSG);

				WRITE_CHECK_SETTING(QQ_BLOCKEMPTYREQUESTS,IDC_CUSTOMBLOCK2);

				WRITE_CHECK_SETTING(QQ_STATUSASPERSONAL,IDC_STATUSASPERSONAL);

				//CHECK_BYTE_SETTING(QQ_WAITACK,IDC_WAITACK);
				WRITE_CHECK_SETTING(QQ_WAITACK,IDC_WAITACK);
				network->m_needAck=IsDlgButtonChecked(hwndDlg,IDC_WAITACK);

				WRITE_CHECK_SETTING(QQ_SHOWAD,IDC_SHOWAD);
				WRITE_CHECK_SETTING(QQ_MAILNOTIFY,IDC_MAILNOTIFY);

				WRITE_CHECK_SETTING(QQ_NOPROGRESSPOPUPS,IDC_NOPROGRESSPOPUPS);

				if(Packet::isClientKeySet() && reconnectRequired) MessageBox(hwndDlg,TranslateT("The changes you have made require you to reconnect to the QQ network before they take effect"),APPNAME,MB_OK);
				return TRUE;
			}
			break;
	}
	return FALSE;
}

static BOOL CALLBACK DlgProcOptsQun(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	CNetwork* network=(CNetwork*)GetWindowLong(GetParent(hwndDlg),GWL_USERDATA);

	switch (msg) {
		case WM_INITDIALOG:	// Options dialog is being initialized
			{
				TranslateDialogDefault(hwndDlg);

				if (network) {
					// Check Options
					CHECK_CONTROL(IDC_SUPPRESSQUN,QQ_SUPPRESSQUNMSG);
					//CHECK_CONTROL(IDC_NEWCHAT,QQ_NEWCHAT);
					CHECK_CONTROL(IDC_ADDQUNNUMBER,QQ_ADDQUNNUMBER);
					CHECK_CONTROL(IDC_NOLOADQUNIMAGE,QQ_NOLOADQUNIMAGE);
					CHECK_CONTROL(IDC_NOSILENTQUNHISTORY,QQ_NOSILENTQUNHISTORY);
				}

				return TRUE;
			}

		case WM_COMMAND:	// When a control is toggled
			SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
			break;

		case WM_NOTIFY:		// When a notify is sent by Options Dialog (Property Sheet)
			if (network) {
				switch (((LPNMHDR)lParam)->code)
					case PSN_APPLY:	// Apply Settings/ﾈｷ｡ｦﾉ靹ﾃ
					{
						int reconnectRequired=0;

						// Check Options/ｼ・簷｡ﾏ・
						CHECK_BYTE_SETTING(QQ_SUPPRESSQUNMSG,IDC_SUPPRESSQUN);
						WRITE_CHECK_SETTING(QQ_SUPPRESSQUNMSG,IDC_SUPPRESSQUN);
						//CHECK_BYTE_SETTING(QQ_NEWCHAT,IDC_NEWCHAT);
						//WRITE_BYTE_SETTING(QQ_NEWCHAT,(BYTE)IsDlgButtonChecked(hwndDlg,IDC_NEWCHAT));
						CHECK_BYTE_SETTING(QQ_ADDQUNNUMBER,IDC_ADDQUNNUMBER);
						WRITE_CHECK_SETTING(QQ_ADDQUNNUMBER,IDC_ADDQUNNUMBER);
						WRITE_CHECK_SETTING(QQ_NOREMOVEQUNIMAGE,IDC_NOREMOVEQUNIMAGE);
						WRITE_CHECK_SETTING(QQ_NOLOADQUNIMAGE,IDC_NOLOADQUNIMAGE);
						WRITE_CHECK_SETTING(QQ_NOSILENTQUNHISTORY,IDC_NOSILENTQUNHISTORY);

						// Apply Options
						if(Packet::isClientKeySet() && reconnectRequired) MessageBox(hwndDlg,TranslateT("The changes you have made require you to reconnect to the QQ network before they take effect"),APPNAME,MB_OK);

						return TRUE;
					}
			}
			break;
	}
	return FALSE;
}

int CALLBACK BrowseCallbackProc(HWND hwnd,UINT uMsg,LPARAM lParam,LPARAM lpData)
{
	if(uMsg==BFFM_INITIALIZED){
		LPWSTR pszPath=(LPWSTR)lpData;
		if (!*pszPath || wcschr(pszPath,'<') || wcschr(pszPath,'>')) {
			WCHAR szTemp[MAX_PATH];
			CallService(MS_UTILS_PATHTOABSOLUTEW,(WPARAM)L"QQ\\WebServer",(LPARAM)szTemp);
			CreateDirectory(szTemp,NULL);
			SendMessage(hwnd,BFFM_SETSELECTION,(WPARAM)TRUE,(LPARAM)szTemp);
		} else
			SendMessage(hwnd,BFFM_SETSELECTION,(WPARAM)TRUE,lpData);
	}
	return 0;
}

static BOOL CALLBACK DlgProcOptsGlobal(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	//CNetwork* network=(CNetwork*)GetWindowLong(GetParent(hwndDlg),GWL_USERDATA);

	switch (msg) {
		case WM_INITDIALOG:	// Options dialog is being initialized
			{
				int nTemp;
				char szTemp[MAX_PATH];
				DBVARIANT dbv;
				HANDLE hContact=NULL;
				LPSTR m_szModuleName=g_dllname;

				TranslateDialogDefault(hwndDlg);

				CheckDlgButton(hwndDlg, IDC_GLOBAL_DISABLEQUN, DBGetContactSettingByte(NULL,m_szModuleName,QQ_DISABLEHTTPD,0));
				CheckDlgButton(hwndDlg, IDC_GLOBAL_WSNONLOCAL, DBGetContactSettingByte(NULL,m_szModuleName,QQ_HTTPDALLOWEXTERNAL,0));
				CheckDlgButton(hwndDlg, IDC_NOREMOVEQUNIMAGE, DBGetContactSettingByte(NULL,m_szModuleName,QQ_NOREMOVEQUNIMAGE,0));

				nTemp=READC_W2(QQ_HTTPDPORT);
				itoa(nTemp?nTemp:170,szTemp,10);
				SetDlgItemTextA(hwndDlg,IDC_GLOBAL_WSPORT,szTemp);

				if (CNetwork::m_folders[1]) {
					WCHAR szTemp[MAX_PATH];
					FoldersGetCustomPathW(CNetwork::m_folders[1],szTemp,MAX_PATH,L"QQ\\WebServer");
					SetDlgItemText(hwndDlg,IDC_GLOBAL_WSROOT,szTemp);

					EnableWindow(GetDlgItem(hwndDlg,IDC_GLOBAL_WSROOT),FALSE);
					EnableWindow(GetDlgItem(hwndDlg,IDC_GLOBAL_WSBROWSE),FALSE);
				} else {
					if (!READC_TS2(QQ_HTTPDROOT,&dbv) && *dbv.pszVal) {
						SetDlgItemText(hwndDlg,IDC_GLOBAL_WSROOT,dbv.ptszVal);
						DBFreeVariant(&dbv);
					} else
						SetDlgItemText(hwndDlg,IDC_GLOBAL_WSROOT,TranslateT("<Default>"));
				}

				return TRUE;
			}

		case WM_COMMAND:	// When a control is toggled
			switch (LOWORD(wParam)) {
				case IDC_GLOBAL_WSPORT:
				case IDC_GLOBAL_WSROOT:
					if (HIWORD(wParam) != EN_CHANGE || (HWND)lParam != GetFocus()) return 0;
					break;
				case IDC_GLOBAL_WSBROWSE:
					{
						BROWSEINFO  binfo;
						LPITEMIDLIST idlist;
						WCHAR szTemp[MAX_PATH];

						GetDlgItemText(hwndDlg, IDC_GLOBAL_WSROOT, szTemp, MAX_PATH);
						binfo.hwndOwner=hwndDlg;
						binfo.pidlRoot=NULL;
						binfo.pszDisplayName=szTemp;
						binfo.lpszTitle=TranslateT("Select Document Root");
						binfo.ulFlags=BIF_RETURNONLYFSDIRS; 
						binfo.lpfn=&BrowseCallbackProc;             //コールバック関数を指定する
						binfo.lParam=(LPARAM)szTemp;                //コールバックに渡す引数
						binfo.iImage=(int)NULL;
						idlist=SHBrowseForFolder(&binfo);
						SHGetPathFromIDList(idlist,szTemp);         //ITEMIDLISTからパスを得る
						CoTaskMemFree(idlist);                      //ITEMIDLISTの解放     99/11/03訂正
						SetDlgItemText(hwndDlg, IDC_GLOBAL_WSROOT, szTemp);
						SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
					}
					return true;
			}

			SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
			break;
		case WM_NOTIFY:		// When a notify is sent by Options Dialog (Property Sheet)
			switch (((LPNMHDR)lParam)->code)
			case PSN_APPLY:	// Apply Settings/ﾈｷ｡ｦﾉ靹ﾃ
			{
				int reconnectRequired=0;
				HANDLE hContact=NULL;
				LPSTR m_szModuleName=g_dllname;
				union {
					WCHAR wszTemp[MAX_PATH];
					CHAR szTemp[MAX_PATH*2];
				};
				DBVARIANT dbv;
				int nTemp;

				if (DBGetContactSettingByte(NULL,m_szModuleName,QQ_DISABLEHTTPD,0)!=IsDlgButtonChecked(hwndDlg,IDC_GLOBAL_DISABLEQUN)) reconnectRequired=1;
				DBWriteContactSettingByte(NULL,m_szModuleName,QQ_DISABLEHTTPD,(BYTE)IsDlgButtonChecked(hwndDlg,IDC_GLOBAL_DISABLEQUN));
				DBWriteContactSettingByte(NULL,m_szModuleName,QQ_HTTPDALLOWEXTERNAL,(BYTE)IsDlgButtonChecked(hwndDlg,IDC_GLOBAL_WSNONLOCAL));
				DBWriteContactSettingByte(NULL,m_szModuleName,QQ_NOREMOVEQUNIMAGE,(BYTE)IsDlgButtonChecked(hwndDlg,IDC_NOREMOVEQUNIMAGE));

				GetDlgItemTextA(hwndDlg,IDC_GLOBAL_WSPORT,szTemp,MAX_PATH);
				nTemp=atoi(szTemp);
				if (nTemp==0)
					DELC(QQ_HTTPDPORT);
				else if (nTemp!=0 && nTemp!=READC_W2(QQ_HTTPDPORT)) {
					reconnectRequired=1;
					WRITEC_W(QQ_HTTPDPORT,nTemp);
				}

				if (!CNetwork::m_folders[1]) {
					GetDlgItemText(hwndDlg,IDC_GLOBAL_WSROOT,wszTemp,MAX_PATH);
					if (*wszTemp!=0 && !wcschr(wszTemp,'<') && !wcschr(wszTemp,'>')) {
						dbv.ptszVal=NULL;
						if (READC_TS2(QQ_HTTPDROOT,&dbv) || wcscmp(dbv.ptszVal,wszTemp)) {
							reconnectRequired=1;
							WRITEC_TS(QQ_HTTPDROOT,wszTemp);
						}
						if (dbv.ptszVal) DBFreeVariant(&dbv);
					} else
						DELC(QQ_HTTPDROOT);
				}

				// Apply Options
				if (reconnectRequired) MessageBox(hwndDlg,TranslateT("The changes you have made require you to restart Miranda IM before they take effect"),APPNAME,MB_OK);

				return TRUE;
			}
			break;
		case WM_DESTROY:
			hwndGlobal=hwndGlobalFor=0;
			break;
	}
	return FALSE;
}

#if 0
static BOOL CALLBACK AddQunSrcProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
			case WM_INITDIALOG: // Dialog initialization, lParam=hContact
				{
					HANDLE hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, (WPARAM)NULL, (LPARAM)NULL);
					TCHAR szQun[MAX_PATH];
					DBVARIANT dbv;

					while (hContact) {
						if (!lstrcmpA(network->m_szModuleName, (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact,(LPARAM)NULL)) && 
							READC_B2("IsQun")) {
								if (!READC_TS2("Nick",&dbv)) {
									_stprintf(szQun,_T("%s (%d)"),dbv.ptszVal,READC_D2("ExternalID"));
									DBFreeVariant(&dbv);
									SendDlgItemMessage(hwndDlg,IDC_MEMBERLIST,CB_SETITEMDATA,SendDlgItemMessage(hwndDlg,IDC_MEMBERLIST,CB_ADDSTRING,NULL,(LPARAM)szQun),(LPARAM)hContact);
								}
							}

							hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, (LPARAM)NULL);
					}

					SendDlgItemMessage(hwndDlg,IDC_MEMBERLIST,CB_SETCURSEL,0,0);
					if (!lParam) {
						ShowWindow(GetDlgItem(hwndDlg,IDC_QUNLABEL),SW_HIDE);
						ShowWindow(GetDlgItem(hwndDlg,IDC_QUNID),SW_HIDE);
						SetDlgItemText(hwndDlg,IDC_MEMBERLABEL,TranslateT("Source Qun:"));
						SetWindowText(hwndDlg,TranslateT("Select Relay Source"));
					}
				}
				break;
			case WM_COMMAND: // Button pressed
				{
					switch (LOWORD(wParam)) {
						case IDOK: // OK Button
							{
								int index=SendDlgItemMessage(hwndDlg,IDC_MEMBERLIST,CB_GETCURSEL,0,0);
								if (index!=CB_ERR) {
									HANDLE hContact=(HANDLE)SendDlgItemMessage(hwndDlg,IDC_MEMBERLIST,CB_GETITEMDATA,index,0);
									if (hContact!=(HANDLE)CB_ERR) {
										EndDialog(hwndDlg,(INT_PTR)hContact);
									}
								}
							}
							break;
						case IDCANCEL: // Cancel Button
							EndDialog(hwndDlg,0);
							break;
					}
				}
	}
	return false;
}
#endif
static void SetOptionsDlgToType(HWND hwnd, int iExpert)
{
	//LocalOptions_t* lot=networkmap[(CNetwork*)GetWindowLong(hwnd,GWL_USERDATA)];
	CNetwork* network=(CNetwork*)GetWindowLong(hwnd,GWL_USERDATA);
	TCITEM tci;
	RECT rcClient;
	HWND hwndTab = GetDlgItem(hwnd, IDC_OPTIONSTAB), hwndEnum;
	int iPages = 0;

	hwndEnum = GetWindow(network->opt_hwndAcc, GW_CHILD);


	ShowWindow(hwndEnum, SW_SHOW);
	GetClientRect(hwnd, &rcClient);
	TabCtrl_DeleteAllItems(hwndTab);

	if(!network->opt_hwndAcc) network->opt_hwndAcc = CreateDialog(hinstance,MAKEINTRESOURCE(IDD_OPT), hwnd, DlgProcOptsAcc);

	tci.mask = TCIF_PARAM|TCIF_TEXT;
	tci.lParam = (LPARAM)network->opt_hwndAcc;
	tci.pszText = TranslateT("Account/Messaging");
	TabCtrl_InsertItem(hwndTab, 0, &tci);
	MoveWindow((HWND)tci.lParam,5,26,rcClient.right-8,rcClient.bottom-29,1);
	iPages++;

	ShowWindow(network->opt_hwndAcc, SW_SHOW);

	if(iExpert) {
		if(!network->opt_hwndSettings) network->opt_hwndSettings = CreateDialog(hinstance,MAKEINTRESOURCE(IDD_OPT_SETTINGS),hwnd,DlgProcOptsSettings);
		if(!network->opt_hwndQun) network->opt_hwndQun = CreateDialog(hinstance,MAKEINTRESOURCE(IDD_OPT2),hwnd,DlgProcOptsQun);
		if(!hwndGlobalFor) {
			hwndGlobal=CreateDialog(hinstance,MAKEINTRESOURCE(IDD_OPT3),hwnd,DlgProcOptsGlobal);
			hwndGlobalFor=hwnd;
		}

#ifdef MIRANDAQQ_IPC
		if(!hwndHelper) hwndHelper = (HWND)NotifyEventHooks(hIPCEvent,QQIPCEVT_CREATE_OPTION_PAGE,(LPARAM)hwnd);
#endif

		tci.lParam = (LPARAM)network->opt_hwndSettings;
		tci.pszText = TranslateT("Settings");
		TabCtrl_InsertItem(hwndTab, iPages++, &tci);
		MoveWindow((HWND)tci.lParam,5,26,rcClient.right-8,rcClient.bottom-29,1);

		tci.lParam = (LPARAM)network->opt_hwndQun;
		tci.pszText = TranslateT("Qun Settings");
		TabCtrl_InsertItem(hwndTab, iPages++, &tci);
		MoveWindow((HWND)tci.lParam,5,26,rcClient.right-8,rcClient.bottom-29,1);

		if (hwndGlobalFor==hwnd) {
			tci.lParam = (LPARAM)hwndGlobal;
			tci.pszText = TranslateT("Global");
			TabCtrl_InsertItem(hwndTab, iPages++, &tci);
			MoveWindow((HWND)tci.lParam,5,26,rcClient.right-8,rcClient.bottom-29,1);
		}

#ifdef MIRANDAQQ_IPC
		if (hwndHelper) {
			tci.lParam = (LPARAM)hwndHelper;
			tci.pszText = TranslateT("Helper");
			TabCtrl_InsertItem(hwndTab, iPages++, &tci);
			MoveWindow((HWND)tci.lParam,5,26,rcClient.right-8,rcClient.bottom-29,1);
		}
#endif
	}
	if (network->opt_hwndSettings) ShowWindow(network->opt_hwndSettings, SW_HIDE);
	if (network->opt_hwndQun) ShowWindow(network->opt_hwndQun, SW_HIDE);
	if (hwndGlobal!=0 && hwndGlobalFor==hwnd) ShowWindow(hwndGlobal, SW_HIDE);
#ifdef MIRANDAQQ_IPC
	if (hwndHelper) ShowWindow(hwndHelper, SW_HIDE);
#endif

	TabCtrl_SetCurSel(hwndTab, 0);
}

// handle tabbed options dialog

static BOOL CALLBACK OptionsDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	//static int iInit = TRUE;
	//LocalOptions_t* lot=networkmap[(CNetwork*)GetWindowLong(hwnd,GWL_USERDATA)];
	CNetwork* network=(CNetwork*)GetWindowLong(hwnd,GWL_USERDATA);

	switch(msg) {
		case WM_INITDIALOG:
			{
				//LocalOptions_t* lot=networkmap[(CNetwork*)lParam];
				SetWindowLong(hwnd,GWL_USERDATA,lParam);
				network=(CNetwork*)lParam;
				
				//lot->fInit=true;
				network->opt_fInit=true;
				int iExpert = SendMessage(GetParent(hwnd), PSM_ISEXPERT, 0, 0);
				network->opt_hwndAcc=network->opt_hwndSettings=network->opt_hwndQun=NULL;
				//lot->hwndAcc=lot->hwndQun=lot->hwndSettings=NULL;
				SetOptionsDlgToType(hwnd, iExpert);
				//lot->fInit=false;
				network->opt_fInit=false;
				return FALSE;
			}
		case WM_DESTROY:
			//if (lot) {
			if (network) {
				if (network->opt_hwndAcc) DestroyWindow(network->opt_hwndAcc);
				if (network->opt_hwndQun) DestroyWindow(network->opt_hwndQun);
				if (network->opt_hwndSettings) DestroyWindow(network->opt_hwndSettings);
				if (network->opt_hwndAcc) DestroyWindow(network->opt_hwndAcc);
				/*
				if (lot->hwndQun) DestroyWindow(lot->hwndQun);
				if (lot->hwndSettings) DestroyWindow(lot->hwndSettings);
				delete lot;
				networkmap.erase((CNetwork*)GetWindowLong(hwnd,GWL_USERDATA));
				*/
				hwndHelper=0;
			}
			break;
		case PSM_CHANGED: // used so tabs dont have to call SendMessage(GetParent(GetParent(hwnd)), PSM_CHANGED, 0, 0);
			if(network && !network->opt_fInit) {
				SendMessage(GetParent(hwnd), PSM_CHANGED, 0, 0);
			}
			break;
		case WM_NOTIFY:
			switch(((LPNMHDR)lParam)->idFrom) {
				case 0:
					switch (((LPNMHDR)lParam)->code) {
						case PSN_APPLY:
							{
								TCITEM tci;
								int i,count;
								tci.mask = TCIF_PARAM;
								count = TabCtrl_GetItemCount(GetDlgItem(hwnd,IDC_OPTIONSTAB));
								for (i=0;i<count;i++)
								{
									TabCtrl_GetItem(GetDlgItem(hwnd,IDC_OPTIONSTAB),i,&tci);
									SendMessage((HWND)tci.lParam,WM_NOTIFY,0,lParam);
								}
							}
							break;
						case PSN_EXPERTCHANGED:
							{
								int iExpert = SendMessage(GetParent(hwnd), PSM_ISEXPERT, 0, 0);
								SetOptionsDlgToType(hwnd, iExpert);
							}
							break;
					}
					break;
				case IDC_OPTIONSTAB:
					switch (((LPNMHDR)lParam)->code) {
					case TCN_SELCHANGING:
						{
							TCITEM tci;
							tci.mask = TCIF_PARAM;
							TabCtrl_GetItem(GetDlgItem(hwnd,IDC_OPTIONSTAB),TabCtrl_GetCurSel(GetDlgItem(hwnd,IDC_OPTIONSTAB)),&tci);
							ShowWindow((HWND)tci.lParam,SW_HIDE);                     
						}
						break;
					case TCN_SELCHANGE:
						{
							TCITEM tci;
							tci.mask = TCIF_PARAM;
							TabCtrl_GetItem(GetDlgItem(hwnd,IDC_OPTIONSTAB),TabCtrl_GetCurSel(GetDlgItem(hwnd,IDC_OPTIONSTAB)),&tci);
							ShowWindow((HWND)tci.lParam,SW_SHOW);                     
						}
						break;
					}
					break;

			}
			break;
	}
	return FALSE;
}


// OptInit(): Callback Function for option dialog event
int CNetwork::OnOptionsInit(WPARAM wParam,LPARAM lParam)
{
	OPTIONSDIALOGPAGE odp={sizeof(odp)};
	//LocalOptions_t* lot=new LocalOptions_t();
	/*
	TCHAR szTitle[MAX_PATH]=L"QQ (";

	wcscat(szTitle,m_tszUserName);
	wcscat(szTitle,L")");
	*/

	odp.position=-790000000;
	odp.hInstance=hinstance;
	odp.pszTemplate=MAKEINTRESOURCEA(IDD_OPTMAIN);
	//odp.ptszTitle=szTitle;
	odp.ptszTitle=m_tszUserName;
	odp.ptszGroup = L"Network";
	odp.flags=ODPF_BOLDGROUPS|ODPF_UNICODE;
	odp.pfnDlgProc=OptionsDlgProc;
	odp.dwInitParam=(LPARAM)this;
	//networkmap[this]=lot;

	CallService(MS_OPT_ADDPAGE,wParam,(LPARAM)&odp);

	return 0;
}
