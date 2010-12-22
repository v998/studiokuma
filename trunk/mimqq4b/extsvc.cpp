#include "StdAfx.h"

void CProtocol::InitFontService() {
	// FontService
	FontIDW fid = {sizeof(fid)};
	wcscpy(fid.name,TranslateT("Contact Messaging Font"));
	wcscpy(fid.group,m_tszUserName);
	strcpy(fid.dbSettingsGroup,m_szModuleName);
	strcpy(fid.prefix,"font1");
	fid.flags=FIDF_NOAS|FIDF_SAVEPOINTSIZE|FIDF_DEFAULTVALID|FIDF_ALLOWEFFECTS;

	// you could register the font at this point - getting it will get either the global default or what the user has set it 
	// to - but we'll set a default font:

	fid.deffontsettings.charset = GB2312_CHARSET;
	fid.deffontsettings.colour = RGB(0, 0, 0);
	fid.deffontsettings.size = 12;
	fid.deffontsettings.style = 0;
	wcsncpy(fid.deffontsettings.szFace, L"SimSun", LF_FACESIZE);
	CallService(MS_FONT_REGISTERW, (WPARAM)&fid, 0);

	wcscpy(fid.name,TranslateT("Qun Messaging Font"));
	strcpy(fid.prefix,"font2");
	CallService(MS_FONT_REGISTERW, (WPARAM)&fid, 0);
}

void CProtocol::InitFoldersService() {
	WCHAR szTemp[MAX_PATH];
	LPWSTR pszTemp=szTemp;
	LPWSTR pszPath = Utils_ReplaceVarsT(L"%miranda_avatarcache%");
	int c=0;
	LPWSTR pszFolders[]={L"Avatars",L"QunImages"L"WebServer",NULL};
	LPSTR pszNames[]={"Avatars","Qun Images","Web Server Document Root"};
	LPSTR* ppszNames=pszNames;

	pszTemp+=mir_sntprintf(szTemp, MAX_PATH-20, L"%s\\%S\\", pszPath, m_szModuleName);

	for (LPWSTR* ppszFolders=pszFolders; *ppszFolders; ppszFolders++, ppszNames++) {
		wcscpy(pszTemp,*ppszFolders);
		m_folders[c++]=FoldersRegisterCustomPathW(m_szModuleName, *ppszNames, szTemp);
	}

	m_folders[c]=NULL; // Terminate the array

	mir_free(pszPath);
}

int ParseTencentURI(WPARAM wParam, LPARAM lParam) {
#if 0 // TODO
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
#endif
	return 0;
}

void CProtocol::InitAssocManager() {
	if (!ServiceExists("MIRANDAQQ/ParseTencentURI") && ServiceExists(MS_ASSOCMGR_ADDNEWURLTYPE)) {
		extern int ParseTencentURI(WPARAM,LPARAM);
		CreateServiceFunction("MIRANDAQQ/ParseTencentURI", ParseTencentURI);
		AssocMgr_AddNewUrlTypeT("tencent:", TranslateT("MirandaQQ Link Protocol"), CProtocol::g_hInstance, IDI_ICON1, "MIRANDAQQ/ParseTencentURI", UTDF_DEFAULTDISABLED);
	}
}
