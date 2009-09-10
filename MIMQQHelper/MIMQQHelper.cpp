// MIMQQHelper.cpp : 定義 DLL 應用程式的進入點。
//

#include "stdafx.h"

HINSTANCE hInstance;
static HANDLE hHookModulesLoaded;
static HANDLE hHookPreShutdown;
static HANDLE hHookIPCEvent;
static HANDLE hHookChatEvent;
MM_INTERFACE   mmi;
UTF8_INTERFACE utfi;

PLUGINLINK* pluginLink;				// Struct of functions pointers for service calls
const LPSTR szMIMQQError="MirandaQQ2 Helper Error";
HANDLE hSvcChangeEIP;
HANDLE hCnxtMenuChangeEIP;
static LPWSTR szBotMatch=NULL;
static LPWSTR szBotExe=NULL;

PLUGININFOEX pluginInfo={
	sizeof(PLUGININFOEX),
		"Nagato Helper (Unicode)",
		PLUGINVERSION,
		"Helper Plugin for Project Nagato (Beta Version - " __DATE__ ")",
		"Stark Wong",
		"starkwong@hotmail.com",
		"(C)2007 Stark Wong",
		"http://www.studiokuma.com/mimqq/",
		UNICODE_AWARE,
		0,
		{ 0x99112a39, 0x1632, 0x4315, { 0x9e, 0x44, 0x9f, 0x7c, 0x9c, 0x92, 0x73, 0xa7 } } // {99112A39-1632-4315-9E44-9F7C9C9273A7}

};

extern "C" {
	BOOL APIENTRY DllMain( HANDLE hModule, 
		DWORD  ul_reason_for_call, 
		LPVOID lpReserved
		)
	{
		if (ul_reason_for_call==DLL_PROCESS_ATTACH) {
			CHAR szModuleName[MAX_PATH];
			hInstance=(HINSTANCE)hModule;
			GetModuleFileNameA(hInstance,szModuleName,MAX_PATH);
			if (stricmp(szModuleName+strlen(szModuleName)-11,"_helper.dll")) {
				MessageBoxA(NULL,"You must rename the MIMQQ Helper to XXX_helper.dll\n(Where XXX is name of MIMQQ module)",szMIMQQError,MB_ICONERROR|MB_SYSTEMMODAL);
				return FALSE;
			} else {
				/*
				*strrchr(szModuleName,'_')=0;
				szMIMQQ=strdup(strrchr(szModuleName,'\\')+1);
				CharUpperA(szMIMQQ);
				*/
			}
		} /*else if (ul_reason_for_call==DLL_PROCESS_DETACH)
			free(szMIMQQ);*/
		
		return TRUE;
	}

	__declspec(dllexport) PLUGININFOEX* MirandaPluginInfoEx(DWORD mirandaVersion)
	{
		return &pluginInfo;
	}

	static const MUUID interfaces[] = {{ 0xf36d782, 0x231, 0x4dba, { 0xb1, 0xb1, 0xb9, 0xa2, 0x71, 0x7f, 0xf0, 0xf } },MIID_LAST};
	__declspec(dllexport) const MUUID* MirandaPluginInterfaces(void)
	{
		return interfaces;
	}

	static bool bRestartRequired=false;

	static BOOL CALLBACK DlgProcOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
		switch (msg)
		{
			case WM_INITDIALOG:	// Options dialog is being initialized/恁秏眕趕遺輛俴場宎趙
				{
					TranslateDialogDefault(hwndDlg);

					// Add server to list/蔚督昢樓?E箱ombox蹈?E
					SendDlgItemMessage(hwndDlg,IDC_QUNSUPPORT,CB_ADDSTRING,NULL,(LPARAM)TranslateT("None"));
					SendDlgItemMessage(hwndDlg,IDC_QUNSUPPORT,CB_ADDSTRING,NULL,(LPARAM)TranslateT("Qun Member List (TabSRMM only)"));
					SendDlgItemMessage(hwndDlg,IDC_QUNSUPPORT,CB_ADDSTRING,NULL,(LPARAM)TranslateT("Chatroom Support (TabSRMM only)"));

					SendDlgItemMessage(hwndDlg,IDC_QUNSUPPORT,CB_SETCURSEL,DBGetContactSettingByte(NULL,MIMQQ,"QunSupport",0),0);

					bRestartRequired=false;
					return TRUE;
				}

			case WM_COMMAND:	// When a control is toggled

				switch (LOWORD(wParam)) {
					case IDC_QUNSUPPORT:
						if (HIWORD(wParam)==CBN_SELCHANGE) {
							SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
							bRestartRequired=true;
						}
				}
				break;

			case WM_NOTIFY:		// When a notify is sent by Options Dialog (Property Sheet)
				{
					switch (((LPNMHDR)lParam)->code)
						case PSN_APPLY:
							{
								int newindex=(int)SendDlgItemMessage(hwndDlg,IDC_QUNSUPPORT,CB_GETCURSEL,0,0);
								DBWriteContactSettingByte(NULL,MIMQQ,"QunSupport",(BYTE)newindex);
								if (newindex==1) // TabSRMM
									DBWriteContactSettingByte(NULL,"Tab_SRMsg","enable_chat",1);

								if (bRestartRequired) {
									MessageBox(hwndDlg,TranslateT("The changes you have made to helper require you to restart Miranda IM before they take effect"),L"MirandaQQ",MB_OK|MB_ICONEXCLAMATION);
								}
								return TRUE;
							}
				}
				break;
		}
		return FALSE;
	}

	void BotHandler(ipcmessage_t* ipcm) {
		LPSTR pszModule=(LPSTR)CallService(MS_PROTO_GETCONTACTBASEPROTO,(WPARAM)ipcm->hContact,0);
		LPSTR pszName;
		LPSTR pszID;
		LPSTR pszMsg;
		LPWSTR pszMessage;

		if (DBGetContactSettingByte(ipcm->hContact,pszModule,"IsQun",0)==1) {
			pszMessage=wcsstr(ipcm->message,L":\n")+2;
			while (pszMessage && *pszMessage=='[') pszMessage=wcschr(pszMessage+1,']')+1;
			if (!pszMessage) return;
			
			if (wcsncmp(pszMessage,szBotMatch,wcslen(szBotMatch)) || pszMessage[wcslen(szBotMatch)]!=32) return;
			pszMessage=pszMessage+wcslen(szBotMatch)+1;
			wcsstr(ipcm->message,L":\n")[-1]=0;
			if (wcsstr(pszMessage,L"[/")) *wcsstr(pszMessage,L"[/")=0;

			if (wcschr(ipcm->message,L'(')) {
				*wcsrchr(ipcm->message,L'(')=0;
				pszName=mir_utf8encodeW(ipcm->message);
				pszID=mir_utf8encodeW(ipcm->message+wcslen(ipcm->message)+1);

			} else {
				pszName=mir_strdup("");
				pszID=mir_utf8encodeW(ipcm->message);
			}

		} else {
			CHAR szTemp[MAX_PATH];
			DBVARIANT dbv;

			pszMessage=ipcm->message;
			while (pszMessage && *pszMessage=='[') pszMessage=wcschr(pszMessage+1,']')+1;
			if (!pszMessage) return;

			if (wcsncmp(pszMessage,szBotMatch,wcslen(szBotMatch)) || pszMessage[wcslen(szBotMatch)]!=32) return;
			pszMessage=pszMessage+wcslen(szBotMatch)+1;
			if (wcsstr(pszMessage,L"[/")) *wcsstr(pszMessage,L"[/")=0;

			itoa(ipcm->qunid,szTemp,10);
			pszID=mir_strdup(szTemp);
			if (DBGetContactSettingTString(ipcm->hContact,pszModule,"Nick",&dbv))
				// No Nick
				pszName=mir_strdup("");
			else {
				pszName=mir_utf8encodeW(dbv.ptszVal);
				DBFreeVariant(&dbv);
			}
		}

		pszMsg=mir_utf8encodeW(pszMessage);

		LPSTR pszScript=(LPSTR)mir_alloc(strlen(pszName)+strlen(pszID)+strlen(pszMsg)+1024);
		sprintf(pszScript,"<?\r\n$name='%s';\r\n$id=%s;\r\n$msg='%s';\r\n$root='QQ/Bot';\r\nrequire(\"$root/main.php\");\r\n?>",pszName,pszID,pszMsg);

		WCHAR szCmdLine[MAX_PATH];
		swprintf(szCmdLine,L"\"%s\"",szBotExe);

		HANDLE hOutputReadTmp,hOutputRead,hOutputWrite;
		HANDLE hInputWriteTmp,hInputRead,hInputWrite;
		DWORD ThreadId;
		SECURITY_ATTRIBUTES sa={sizeof(sa),NULL,TRUE};


		// Create the child output pipe.
		CreatePipe(&hOutputReadTmp,&hOutputWrite,&sa,0);

		// Create the child input pipe.
		CreatePipe(&hInputRead,&hInputWriteTmp,&sa,0);


		// Create new output read handle and the input write handles. Set
		// the Properties to FALSE. Otherwise, the child inherits the
		// properties and, as a result, non-closeable handles to the pipes
		// are created.
		DuplicateHandle(GetCurrentProcess(),hOutputReadTmp,GetCurrentProcess(),&hOutputRead,0,FALSE,DUPLICATE_SAME_ACCESS);
		DuplicateHandle(GetCurrentProcess(),hInputWriteTmp,GetCurrentProcess(),&hInputWrite,0,FALSE,DUPLICATE_SAME_ACCESS);


		// Close inheritable copies of the handles you do not want to be
		// inherited.
		CloseHandle(hOutputReadTmp);
		CloseHandle(hInputWriteTmp);


		//PrepAndLaunchRedirectedChild(hOutputWrite,hInputRead);
		PROCESS_INFORMATION pi;
		STARTUPINFO si={sizeof(si)};

		// Set up the start up info struct.
		si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
		si.wShowWindow = SW_HIDE;
		si.hStdOutput = hOutputWrite;
		si.hStdInput  = hInputRead;

		if (CreateProcess(NULL,szBotExe,NULL,NULL,TRUE,CREATE_NEW_CONSOLE,NULL,NULL,&si,&pi)) {
			// Close any unnecessary handles.
			CloseHandle(pi.hThread);
			CloseHandle(pi.hProcess);

			// Close pipe handles (do not continue to modify the parent).
			// You need to make sure that no handles to the write end of the
			// output pipe are maintained in this process or else the pipe will
			// not close when the child process exits and the ReadFile will hang.
			CloseHandle(hOutputWrite);
			CloseHandle(hInputRead);

			WriteFile(hInputWrite,pszScript,(DWORD)strlen(pszScript)+1,&ThreadId,NULL);
			CloseHandle(hInputWrite);

			char szTemp[1001];
			string str;
			while (ReadFile(hOutputRead,szTemp,1000,&ThreadId,NULL)) {
				szTemp[ThreadId]=0;
				str+=szTemp;
			}

			ipcsendmessage_t ism={ ipcm->qunid, mir_utf8decodeW(str.c_str())};
			CallContactService(ipcm->hContact,IPCSVC,QQIPCSVC_QUN_SEND_MESSAGE,(LPARAM)&ism);
			mir_free(ism.message);

			CloseHandle(hOutputRead);
		} else {
			CloseHandle(hInputWrite);
			CloseHandle(hOutputRead);

			if (ServiceExists(MS_CLIST_SYSTRAY_NOTIFY)) {
				// Guest OS supporting tray notification
				MIRANDASYSTRAYNOTIFY err;
				err.szProto = NULL;
				err.cbSize = sizeof(err);
				err.tszInfoTitle = TranslateT("MirandaQQ Helper");
				err.tszInfo = TranslateT("Failed to launch PHP executable. Check path.");
				err.dwInfoFlags = NIIF_ERROR|NIIF_INTERN_UNICODE;
				err.uTimeout = 1000 * 10;
				CallService(MS_CLIST_SYSTRAY_NOTIFY, 0, (LPARAM) & err);
			}
		}

		mir_free(pszScript);

		mir_free(pszMsg);
		mir_free(pszName);
		mir_free(pszID);
	}

	int ToggleQunList(WPARAM,LPARAM) {
		bool enabled=DBGetContactSettingByte(NULL,MIMQQ,"EnableQunList",0)==1;

		DBWriteContactSettingByte(NULL,MIMQQ,"EnableQunList",!enabled);
		if (enabled && CQunListBase::getInstance()!=NULL) {
			CQunListBase::RemoveHook();
			CQunListBase::DeleteInstance();
		} else if (!enabled) {
			CSRMMQunList::getInstance();
			if (CQunListBase::getInstance()) CQunListBase::InstallHook(hInstance);
		}

		MessageBox(NULL,!enabled?TranslateT("Qun List is now Enabled. It will be shown when you switch tab or open new chat window."):TranslateT("Qun List is now Disabled."),NULL,MB_ICONINFORMATION);
		return 0;
	}

	void createMainMenuItems(CLISTMENUITEM* mi) {
		if (DBGetContactSettingByte(NULL,MIMQQ,"QunSupport",0)==1) {
			CLISTMENUITEM mi2=*mi; // I can't modify mi
			mi2.pszService="MIRANDAQQ/Helper_ToggleQunList";
			mi2.ptszName=TranslateT("&Toggle Qun List");
			CreateServiceFunction(mi2.pszService,ToggleQunList);
			CallService(MS_CLIST_ADDMAINMENUITEM,0,(LPARAM)&mi2);
		}
	}

	int mimqqEventProc(WPARAM command, LPARAM parameter) {
		switch (command) {
			case QQIPCEVT_PROTOCOL_OFFLINE:
				if (CQunListBase::getInstance()) {
					CQunListBase::RemoveHook();
					CQunListBase::DeleteInstance();
				}
				break;
			case QQIPCEVT_LOGGED_IN:
				{
					bool srmminit=false;
					switch (DBGetContactSettingByte(NULL,MIMQQ,"QunSupport",0)) {
						case 1: // Member List
							srmminit=true;
							CSRMMQunList::getInstance();
						case 2: // Chat support
							if (!srmminit) CChatQunList::getInstance();
							if (CQunListBase::getInstance()) CQunListBase::InstallHook(hInstance);
							break;
						default:
							break;
					}
				}
				break;
			case QQIPCEVT_QUN_ONLINE:
				if (CQunListBase::getInstance()) CQunListBase::getInstance()->QunOnline((HANDLE)parameter);
				break;
			case QQIPCEVT_RECV_MESSAGE:
				if (CQunListBase::getInstance()) CQunListBase::getInstance()->MessageReceived((ipcmessage_t*)parameter);
				if (szBotExe && *szBotExe && wcsstr(((ipcmessage_t*)parameter)->message,szBotMatch)) BotHandler((ipcmessage_t*)parameter);
				break;
			case QQIPCEVT_QUN_UPDATE_NAMES:
				if (CQunListBase::getInstance()) CQunListBase::getInstance()->NamesUpdated((ipcmembers_t*)parameter);
				break;
			case QQIPCEVT_QUN_UPDATE_ONLINE_MEMBERS:
				if (CQunListBase::getInstance()) CQunListBase::getInstance()->OnlineMembersUpdated((ipconlinemembers_t*)parameter);
				break;
			case QQIPCEVT_QUN_MESSAGE_SENT: // NOTE: Parameter changed to hContact!
				if (CQunListBase::getInstance()) CQunListBase::getInstance()->MessageSent((HANDLE)parameter);
				break;
			case QQIPCEVT_CREATE_OPTION_PAGE:
				return (int)CreateDialog(hInstance,MAKEINTRESOURCE(IDD_OPT),(HWND)parameter,DlgProcOpts);
				break;
			case QQIPCEVT_CREATE_MAIN_MENU:
				createMainMenuItems((CLISTMENUITEM*)parameter);
				break;
		}
		return 0;
	}

	int ChangeEIP(WPARAM wParam, LPARAM lParam);

	static int OnPreShutdown( WPARAM wParam, LPARAM lParam ) {
		CQunListBase::DeleteInstance();
		CQunListBase::RemoveHook();
		if (szBotExe) mir_free(szBotExe);
		if (szBotMatch) mir_free(szBotMatch);
		return 0;
	}

	static int OnModulesLoaded( WPARAM wParam, LPARAM lParam )
	{
		/*
		if (!ServiceExists("MirandaQQ/HTTPDCommand")) {
			MessageBoxA(NULL,"Your copy of MirandaQQ2 does not export IPCService. You may be using an old version.",szMIMQQError,MB_ICONERROR|MB_SYSTEMMODAL);
			return 1;
		} else*/ {
			if (!(hHookIPCEvent=HookEvent("MirandaQQ/IPCEvent",mimqqEventProc))) {
				MessageBoxA(NULL,"Failed to hook into IPCEvent in your copy of MirandaQQ2.",szMIMQQError,MB_ICONERROR|MB_SYSTEMMODAL);
				return 1;
			}

			CreateServiceFunction("MirandaQQ/ChangeEIP",ChangeEIP);

			CLISTMENUITEM mi={sizeof(mi)};
			mi.pszPopupName = NULL;
			mi.pszService="MirandaQQ/ChangeEIP";
			mi.position=-500040000;
			mi.hIcon = NULL; //(HICON)CallProtoService(szMIMQQ,PS_LOADICON,PLI_PROTOCOL|PLIF_SMALL,NULL);
			mi.pszName=Translate("Change EIP");
			hCnxtMenuChangeEIP=(HANDLE)CallService(MS_CLIST_ADDCONTACTMENUITEM,0,(LPARAM)&mi);

			DBVARIANT dbv;
			if (!DBGetContactSettingTString(NULL,MIMQQ,"PHPExe",&dbv)) {
				szBotExe=mir_wstrdup(dbv.ptszVal);
				DBFreeVariant(&dbv);

				if (!DBGetContactSettingTString(NULL,MIMQQ,"PHPMatch",&dbv)) {
					szBotMatch=mir_wstrdup(dbv.ptszVal);
					DBFreeVariant(&dbv);
				} else {
					mir_free(szBotExe);
					szBotExe=NULL;
				}
			}
		}

		return 0;
	}

	int __declspec(dllexport)Load(PLUGINLINK *link)
	{
		// return 1;
		PROTOCOLDESCRIPTOR pd={sizeof(PROTOCOLDESCRIPTOR)};
		pluginLink=link;
		mir_getMMI(&mmi);
		mir_getUTFI(&utfi);
		CHAR szTemp[MAX_PATH];
		int ret;

		GetModuleFileNameA(hInstance,szTemp,MAX_PATH);
		memmove(szTemp,strrchr(szTemp,'\\')+1,strlen(strrchr(szTemp,'\\')));
		*strchr(szTemp,'.')=0;

		pd.szName = szTemp;
		pd.type = PROTOTYPE_TRANSLATION;
		ret=CallService(MS_PROTO_REGISTERMODULE, 0,(LPARAM)&pd);

		hHookModulesLoaded = HookEvent( ME_SYSTEM_MODULESLOADED, OnModulesLoaded );
		hHookPreShutdown = HookEvent( ME_SYSTEM_PRESHUTDOWN, OnPreShutdown);
		return 0;
	}

	__declspec(dllexport)int Unload(void)
	{
		UnhookEvent(hHookModulesLoaded);
		UnhookEvent(hHookIPCEvent);
		DestroyServiceFunction(hSvcChangeEIP);
		CSRMMQunList::DeleteInstance();
		return 0;
	}

	int CountSpace(LPSTR psz) {
		int c=0;
		for (LPSTR ppsz=psz; *ppsz; ppsz++)
			if (*ppsz==0x20) c++;

		return c;
	}

	int ChangeEIP(WPARAM wParam, LPARAM lParam) {
		OPENFILENAMEA ofn={sizeof(OPENFILENAMEA)};
		HANDLE hContact=NULL;
		DBVARIANT dbv;
		char szFile[MAX_PATH]={0};       // buffer for file name
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = MAX_PATH;
		ofn.lpstrFilter = "EIP FIles (*.eip)\0*.eip\0";
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

		/*
		if (!READC_S2("CurrentEIP",&dbv)) {
			strcpy(szFile,dbv.pszVal);
			DBFreeVariant(&dbv);
		}
		*/

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
						//strcpy(szOutFile,"MirandaQQ");
						if (DBGetContactSettingByte(NULL,"SmileyAdd","UseOneForAll",0)==1) {
							strcpy(szOutFile,"Standard");
						} else {
							strcpy(szOutFile,(LPCSTR)CallService(MS_PROTO_GETCONTACTBASEPROTO,wParam,0));
						}
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
							LPSTR pszShortcut_utf8=mir_utf8encodecp(curface.tips.c_str(),936);
							LPSTR pszOutFile_utf8=mir_strdup(szOutFile);

							int count=CountSpace(pszShortcut_utf8);
							LPSTR pszSpace;
							if (count>0) {
								pszShortcut_utf8=(LPSTR)mir_realloc(pszShortcut_utf8,strlen(pszShortcut_utf8)+1+count*5);
								while (pszSpace=strchr(pszShortcut_utf8,' ')) {
									memmove(pszSpace+5,pszSpace,strlen(pszSpace)+1);
									strncpy(pszSpace,"%%_%%",5);
								}
							}
							if ((count=CountSpace(pszOutFile_utf8))>0) {
								pszOutFile_utf8=(LPSTR)mir_realloc(pszOutFile_utf8,strlen(pszOutFile_utf8)+1+count*5);
								while (pszSpace=strchr(pszOutFile_utf8,' ')) {
									memmove(pszSpace+5,pszSpace,strlen(pszSpace)+1);
									strncpy(pszSpace,"%%_%%",5);
								}
							}

							fprintf(fpAslOut,"Smiley = \"%s\", 0, \"[img]%s[/img]\",\"%s\"\n",pszOutFile-4,pszOutFile_utf8,curface.tips.c_str());
							mir_free(pszShortcut_utf8);
							mir_free(pszOutFile_utf8);
						}

						fclose(fpAslOut);
						CopyFileA(szAslTemp,szAslOri,FALSE);
						DeleteFileA(szAslTemp);

						OPENOPTIONSDIALOG ood={sizeof(ood)};
						ood.pszGroup="Customize";
						ood.pszPage="Smileys";
						CallService(MS_OPT_OPENOPTIONS,0,(LPARAM)&ood);
						HWND hWndOptions;
						while (!(hWndOptions=FindWindowW(NULL,TranslateT("Miranda IM Options")))) {
							Sleep(100);
						}
						PostMessage(hWndOptions,PSM_CHANGED,0,0);
						PostMessage(hWndOptions,WM_COMMAND,IDOK,(LPARAM)GetDlgItem(hWndOptions,IDOK));

						swprintf((LPTSTR)szItemPath,TranslateT("%d custom emoticons imported."),facecount);
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
};
