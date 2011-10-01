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
#include "StdAfx.h"
LPCTSTR blood_types[] = {
	_T("Other"), _T("A"), _T("B"), _T("O"), _T("AB"), NULL
};

LPCTSTR country_names[] = {
	_T("China"), _T("Hong Kong SAR"), _T("Macau SAR"), _T("Taiwan"),
	_T("Singapore"), _T("Malaysia"), _T("America"), NULL
};

LPCTSTR province_names[] = {
	_T("Beijing"), _T("Tianjan"), _T("Shanghai"), _T("Chongqing"), _T("Hong Kong"),
	_T("Hebei"), _T("Shanxi"), _T("Inner Mongolia"), _T("Liaoning"), _T("Jilin"),
	_T("Heilongjiang"), _T("Jiangxi"), _T("Zhejiang"), _T("Jiangsu"), _T("Anhui"),
	_T("Fujian"), _T("Shandong"), _T("Henan"), _T("Hubei"), _T("Hunan"),
	_T("Guandong"), _T("Guangxi"), _T("Hainan"), _T("Sichuan"), _T("Guizhou"),
	_T("Yunnan"), _T("Tibet"), _T("Shaanxi"), _T("Gansu"), _T("Ninxia"),
	_T("Qinghai"), _T("Xinjian"), _T("Taiwan"), _T("Macau"), NULL
};

LPCTSTR zodiac_names[] = {
	_T("-"), _T("Mouse"), _T("Cow"), _T("Tiger"), _T("Rabbit"),
	_T("Dragon"), _T("Snake"), _T("Horse"), _T("Goat"), _T("Monkey"),
	_T("Chicken"), _T("Dog"), _T("Pig"), NULL
};

LPCTSTR horoscope_names[] = {
	_T("-"), _T("Aquarius"), _T("Pisoes"), _T("Aries"), _T("Taurus"),
	_T("Gemini"), _T("Cancer"), _T("Leo"), _T("Virgo"), _T("Libra"),
	_T("Scorpio"), _T("Sagittarus"), _T("Capricom"), NULL
};

LPCTSTR occupation_names[] = {
	_T("Full-time"), _T("Part-time"), _T("Manufacturing"), _T("Business"), _T("Unemployed"),
	_T("Student"), _T("Engineer"), _T("Government"), _T("Education"), _T("Service"),
	_T("Boss"), _T("Computing"), _T("Retired"), _T("Financial"),
	_T("Sales/AD/Marketing"), NULL
};

LPCTSTR qq_group_category[] = {
	_T("Classmates"), _T("Friends"), _T("Colleagues"), _T("Other"), NULL
};				

LPCTSTR sex_names[] = {
	_T("Male"), _T("Female"), _T("Neutral"), NULL
};

typedef struct {
	HANDLE hContact;
	CNetwork* network;
	UINT_PTR timer;
} LocalDetails_t;

map<HANDLE,LocalDetails_t*> networkmap;

void CNetwork::UpdateQunContacts(HWND hwndDlg, unsigned int qunid) {
	if (qqqun* qq=qun_get(&m_client,qunid,0)) {
		plist l=qq->member_list;
		HANDLE hContact=FindContact(qunid);
		DWORD creator=READC_D2("Creator");
		DBVARIANT dbv;
		TCHAR* pszNick;
		TCHAR szTemp[MAX_PATH];
		char szID[16];
		qunmember* qm;
		HWND hControl=GetDlgItem(hwndDlg,IDC_QUNINFO_MEMBERLIST);

		SendMessage(hControl,LB_RESETCONTENT,(WPARAM)NULL,(LPARAM)NULL);

		for (int c=0; c<l.count; c++) {
			qm=(qunmember*)l.items[c];
			ultoa(qm->number,szID,10);
			/*
			if (READC_S2(szID,&dbv)) {
				// Nick not found
				swprintf(szTemp,L" [%s] %d",(qm->status==QQ_ONLINE)?TranslateT("Online"):TranslateT("Offline"),qm->number);
			} else {
				// Nick found
				pszNick=mir_a2u_cp(dbv.pszVal,936);
				swprintf(szTemp,L" [%s] %s(%d)",(qm->status==QQ_ONLINE)?TranslateT("Online"):TranslateT("Offline"),pszNick,qm->number);
				mir_free(pszNick);
				DBFreeVariant(&dbv);
			}
			*/
			pszNick=mir_utf8decodeW(qm->nickname);
			swprintf(szTemp,L" [%s] %s(%d)",(qm->status==QQ_ONLINE)?TranslateT("Online"):TranslateT("Offline"),pszNick,qm->number);
			mir_free(pszNick);

			if (creator==qm->number)
				*szTemp=_T('*');
			else if (qm->role & 0x80)
				*szTemp=_T('+');

			SendMessage(hControl,LB_ADDSTRING,(WPARAM)NULL,(LPARAM)szTemp);
		}
	}
}

static BOOL CALLBACK QunDetailsDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg)
	{

	case WM_INITDIALOG:
		{
			TranslateDialogDefault(hwndDlg);
			SetWindowLong(hwndDlg,GWL_USERDATA,lParam);

			return TRUE;
		}

	case WM_TIMER:
#if 0
		if (LocalDetails_t* ldt=networkmap[(HANDLE)GetWindowLong(hwndDlg,GWL_USERDATA)]) {
			LPSTR m_szModuleName=ldt->network->m_szModuleName;
			HANDLE hContact=ldt->hContact;
			unsigned int qunid=READC_D2(UNIQUEIDSETTING);
			/*
			ldt->network->UpdateQunContacts(hwndDlg,qunid);
			*/
			KillTimer(hwndDlg,1);
			ldt->network->append(new QunGetOnlineMemberPacket(qunid));
		}
#endif
		return TRUE;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->idFrom) {
			case 0:
				switch (((LPNMHDR)lParam)->code) {
					case PSN_INFOCHANGED:
						if (LocalDetails_t* ldt=networkmap[(HANDLE)((LPPSHNOTIFY)lParam)->lParam]) {
							LPTSTR pszTemp;
							LPSTR m_szModuleName=ldt->network->m_szModuleName;
							char szTemp[MAX_PATH];
							TCHAR wszTemp[MAX_PATH];
							HANDLE hContact = (HANDLE)((LPPSHNOTIFY)lParam)->lParam;
							HANDLE hContact2=hContact;
							DBVARIANT dbv;
							DWORD nTemp;

							unsigned int qunid=0;

							qunid=READC_D2(UNIQUEIDSETTING);

							nTemp=READC_D2("Creator");
							ultoa(nTemp,szTemp,10);

							if (!READC_S2(szTemp,&dbv)) {
								// Qun creator info available
								pszTemp=mir_a2u_cp(dbv.pszVal,936);
								swprintf(wszTemp,L"%s (%u)", pszTemp, nTemp);
								mir_free(pszTemp);
								DBFreeVariant(&dbv);
							} else
								_ultow(nTemp,wszTemp,10);

							SetDlgItemText(hwndDlg,IDC_QUNINFO_CREATOR,wszTemp);

							_ultow(READC_D2("ExternalID"),wszTemp,10);
							SetDlgItemText(hwndDlg,IDC_QUNINFO_QID,wszTemp);

							if (!READC_TS2("Nick",&dbv)) {
								// Qun name available
								SetDlgItemText(hwndDlg,IDC_QUNINFO_NAME,wcschr(dbv.ptszVal,L')')+2);
								DBFreeVariant(&dbv);
							}

							if (!DBGetContactSettingTString(hContact,"CList","StatusMsg",&dbv)) {
								// Notice available
								SetDlgItemText(hwndDlg,IDC_QUNINFO_NOTICE,dbv.ptszVal);
								DBFreeVariant(&dbv);
							}

							if (!READC_TS2("Description",&dbv)) {
								// Description available
								SetDlgItemText(hwndDlg,IDC_QUNINFO_DESC,dbv.ptszVal);
								DBFreeVariant(&dbv);
							}

#if 0
							switch (READC_B2("GetInfoOnline")) {
								case 1: // Just retrieved info, send member online update call
									PostMessage(hwndDlg,WM_TIMER,1,0);
									{
										int myqq=ldt->network->GetMyQQ();
										Qun* qun=ldt->network->m_qunList.getQun(READC_D2(UNIQUEIDSETTING));
										if (qun==NULL || !(qun->isAdmin(myqq)||qun->getDetails().getCreator()==myqq)) {
											EnableWindow(GetDlgItem(hwndDlg,IDC_QUNINFO_DELMEMBER),FALSE);
										} else
											EnableWindow(GetDlgItem(hwndDlg,IDC_QUNINFO_DELMEMBER),TRUE);

										if (qun->getDetails().getCreator()==myqq) {
											EnableWindow(GetDlgItem(hwndDlg,IDC_QUNINFO_SETADMIN),TRUE);
											EnableWindow(GetDlgItem(hwndDlg,IDC_QUNINFO_UNSETADMIN),TRUE);
											EnableWindow(GetDlgItem(hwndDlg,IDC_QUNINFO_TRANSFER),TRUE);
										}
									}
									break;
								case 2: // Retrieved online info
									ldt->network->UpdateQunContacts(hwndDlg,qunid);
									WRITEC_B("GetInfoOnline",0);
									ldt->timer=SetTimer(hwndDlg,1,60000,NULL);
									break;
							}
#else
							int myqq=ldt->network->GetMyQQ();
							qqqun* qq=qun_get(&ldt->network->m_client,READC_D2(UNIQUEIDSETTING),0);
							qunmember* qm=NULL;

							if (qq) qm=qun_member_get(&ldt->network->m_client,qq,myqq,0);

							if (qq==NULL || qm==NULL || !(qm->role & 0x80)) {
								EnableWindow(GetDlgItem(hwndDlg,IDC_QUNINFO_DELMEMBER),FALSE);
							} else
								EnableWindow(GetDlgItem(hwndDlg,IDC_QUNINFO_DELMEMBER),TRUE);

							if (qq->owner==myqq) {
								EnableWindow(GetDlgItem(hwndDlg,IDC_QUNINFO_SETADMIN),TRUE);
								EnableWindow(GetDlgItem(hwndDlg,IDC_QUNINFO_UNSETADMIN),TRUE);
								EnableWindow(GetDlgItem(hwndDlg,IDC_QUNINFO_TRANSFER),TRUE);
							}

							ldt->network->UpdateQunContacts(hwndDlg,qunid);
#endif
						}
					break;
				}
				break;
		}
		break;

	case WM_DESTROY:
		if (LocalDetails_t* ldt=networkmap[hwndDlg]) {
			if (ldt->timer) KillTimer(hwndDlg,1);
			delete ldt;
			networkmap.erase(hwndDlg);
		}
		break;
	case WM_COMMAND:
		{
			if (LocalDetails_t* ldt=networkmap[(HANDLE)GetWindowLong(hwndDlg,GWL_USERDATA)]) {
				char szTemp[MAX_PATH]={0};
				if (SendDlgItemMessageA(hwndDlg,IDC_QUNINFO_MEMBERLIST,LB_GETTEXT,SendDlgItemMessage(hwndDlg,IDC_QUNINFO_MEMBERLIST,LB_GETCURSEL,0,0),(LPARAM)&szTemp)!=LB_ERR && *szTemp!=0) {
					DWORD qqid=atoi(strrchr(szTemp,'(')+1);
					LPSTR m_szModuleName=ldt->network->m_szModuleName;
					HANDLE hContact=ldt->hContact;
					int qunid=READC_D2(UNIQUEIDSETTING);

					switch (LOWORD(wParam)) {
						case IDC_QUNINFO_ADDTOME:
							{
								if (HANDLE hContact=ldt->network->FindContact(qqid)) {
									MessageBox(hwndDlg,TranslateT("The member is already in your contact list."),NULL,MB_ICONERROR);
								} else {
									ldt->network->AddContactWithSend(qqid);
								}
							}
							break;
						case IDC_QUNINFO_DELMEMBER:
							{
								TCHAR szMsg[MAX_PATH];
								_stprintf(szMsg,TranslateT("Are you sure you want to kick user %d out of this Qun %d?"),qqid,READC_D2("ExternalID"));
								if (MessageBox(NULL,szMsg,APPNAME,MB_ICONWARNING|MB_YESNO)==IDYES) {
									std::list<unsigned int> list;
#if 0 // TODO
									QunModifyMemberPacket *out=new QunModifyMemberPacket(READC_D2(UNIQUEIDSETTING),false);
									list.insert(list.end(),qqid);
									out->setMembers(list);
									ldt->network->append(out);
#endif
								}
							}
							break;
						case IDC_QUNINFO_SETADMIN:
						case IDC_QUNINFO_UNSETADMIN:
							{
#if 0 // TODO
								QunAdminOpPacket* out=new QunAdminOpPacket(qunid,qqid,LOWORD(wParam)==IDC_QUNINFO_SETADMIN);
								ldt->network->append(out);
#endif
							}
							break;
						case IDC_QUNINFO_TRANSFER:
							{
								TCHAR szMsg[MAX_PATH];
								_stprintf(szMsg,TranslateT("Are you sure you want to transfer Qun %d to user %d?"),READC_D2("ExternalID"),qqid);
								if (MessageBox(NULL,szMsg,APPNAME,MB_ICONWARNING|MB_YESNO)==IDYES) {
#if 0 // TODO
									std::list<unsigned int> list;
									QunTransferPacket *out=new QunTransferPacket(qunid,qqid);
									ldt->network->append(out);
#endif
								}
							}
							break;
					}
				}
			}
		}
		break;
#if 0
	case WM_COMMAND:
		/*switch(LOWORD(wParam))
		{

		case IDC_CHANGEDETAILS:
		{
		CallService(MS_UTILS_OPENURL, 1, (LPARAM)"http://www.icq.com/whitepages/user_details.php");
		}
		break;

		case IDCANCEL:
		SendMessage(GetParent(hwndDlg),msg,wParam,lParam);
		break;
		}*/
		break;
#endif
	}
	return FALSE;	
}

static void FillCBList(HWND hwndDlg, int controlID, LPCTSTR* array) {
	for (LPCTSTR* current=array; *current; current++)
		SendDlgItemMessage(hwndDlg,controlID,CB_ADDSTRING,(WPARAM)NULL,(LPARAM)TranslateW(*current));
}

static BOOL CALLBACK QQDetailsDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg)
	{

	case WM_INITDIALOG:
		TranslateDialogDefault(hwndDlg);
		SetWindowLong(hwndDlg,GWL_USERDATA,lParam);
		FillCBList(hwndDlg,IDC_INFO_SEX,sex_names);
		FillCBList(hwndDlg,IDC_INFO_AREA,country_names);
		FillCBList(hwndDlg,IDC_INFO_PROVINCE,province_names);
		//FillCBList(IDC_INFO_CITY,city_names);
		FillCBList(hwndDlg,IDC_INFO_ANIMAL,zodiac_names);
		FillCBList(hwndDlg,IDC_INFO_HOROSCOPE,horoscope_names);
		FillCBList(hwndDlg,IDC_INFO_BLOOD,blood_types);

		return TRUE;

	case WM_DESTROY:
		if (LocalDetails_t* ldt=networkmap[hwndDlg]) {
			delete ldt;
			networkmap.erase(hwndDlg);
		}
		break;
	case WM_NOTIFY:										 
		switch (((LPNMHDR)lParam)->idFrom) {
			case 0:
				switch (((LPNMHDR)lParam)->code) {
					case PSN_INFOCHANGED:
						if (LocalDetails_t* ldt=networkmap[(HANDLE)((LPPSHNOTIFY)lParam)->lParam]) {
							TCHAR szTemp[MAX_PATH];
							HANDLE hContact = (HANDLE)((LPPSHNOTIFY)lParam)->lParam;
							LPSTR m_szModuleName=ldt->network->m_szModuleName;
							DBVARIANT dbv;
							HBITMAP hBMP;
							HINSTANCE hInst;
							char szPluginPath[MAX_PATH]={0};

							// Contact
							int nTemp;

							// Overview
							// QQ ID
							_ltow(READC_D2(UNIQUEIDSETTING),szTemp,10);
							SetDlgItemText(hwndDlg,IDC_INFO_UID,szTemp);

#define DETAILS_READTS(key,ctl) if (!READC_TS2(key,&dbv)) { SetDlgItemText(hwndDlg,ctl,dbv.ptszVal); DBFreeVariant(&dbv); }
							// Nickname
							DETAILS_READTS("Nick",IDC_INFO_NICKNAME);

							// Age
							_itow(READC_W2("Age"),szTemp,10);
							SetDlgItemText(hwndDlg,IDC_INFO_AGE,szTemp);

							// Sex
							*szTemp=READC_B2("Gender");
							SendDlgItemMessage(hwndDlg,IDC_INFO_SEX,CB_SETCURSEL,(WPARAM)(*szTemp=='M'?0:*szTemp=='F'?1:2),0);

							DETAILS_READTS("Country",IDC_INFO_AREA);
							DETAILS_READTS("Province",IDC_INFO_PROVINCE);
							DETAILS_READTS("City",IDC_INFO_CITY);

							// Contact 
							DETAILS_READTS("Email",IDC_INFO_EMAIL);
							DETAILS_READTS("Address",IDC_INFO_ADDRESS);
							DETAILS_READTS("ZIP",IDC_INFO_ZIPCODE);
							DETAILS_READTS("Telephone",IDC_INFO_PHONE);
							DETAILS_READTS("Mobile",IDC_INFO_MOBILE);
							DETAILS_READTS("PersonalSignature",IDC_INFO_SIGNATURE);

							// Details
							DETAILS_READTS("FirstName",IDC_INFO_REALNAME);
							DETAILS_READTS("College",IDC_INFO_COLLEGE);
							DETAILS_READTS("Homepage",IDC_INFO_HOMEPAGE);
							DETAILS_READTS("About",IDC_INFO_DESCRIPTION);

							// Face
							nTemp=DBGetContactSettingWord(hContact,m_szModuleName,"Face",0);
							if (!nTemp)
								nTemp=101;
							else
								nTemp=nTemp/3 + 1;

							hBMP=NULL;
							if (!DBGetContactSetting(hContact,m_szModuleName,"UserHeadMD5",&dbv)) {
								// CallService(MS_UTILS_PATHTOABSOLUTE,(WPARAM)"QQ\\",(LPARAM)szPluginPath);
								FoldersGetCustomPath(ldt->network->m_avatarFolder,szPluginPath,MAX_PATH,"QQ");
								strcat(szPluginPath,"\\");
								strcat(szPluginPath,dbv.pszVal);
								strcat(szPluginPath,".bmp");

								hBMP=(HBITMAP)CallService(MS_UTILS_LOADBITMAP,0,(LPARAM)szPluginPath);
								DBFreeVariant(&dbv);
							}

							CallService(MS_UTILS_PATHTOABSOLUTE,(WPARAM)"Plugins\\QQHeadImg.dll",(LPARAM)szPluginPath);

							if (!hBMP) {
								hInst=LoadLibraryA(szPluginPath);

								if (hInst) {
									hBMP=LoadBitmap(hInst,MAKEINTRESOURCE(100+nTemp));
									SendDlgItemMessage(hwndDlg,IDC_INFO_FACE,STM_SETIMAGE,IMAGE_BITMAP,(LPARAM)hBMP);
								}
								FreeLibrary(hInst);
							}

							if (hBMP)
								SendDlgItemMessage(hwndDlg,IDC_INFO_FACE,STM_SETIMAGE,IMAGE_BITMAP,(LPARAM)hBMP);

							*strrchr(szPluginPath,'\\')=0;

							DETAILS_READTS("MirVer",IDC_INFO_CLIENTVER);
							DETAILS_READTS("Location",IDC_INFO_IP);

#define DETAILS_READB(key,ctl) SendDlgItemMessage(hwndDlg,ctl,CB_SETCURSEL,(WPARAM)READC_B2(key),0); 

							DETAILS_READB("Zodiac",IDC_INFO_ANIMAL);
							DETAILS_READB("Blood",IDC_INFO_BLOOD);
							DETAILS_READB("Horoscope",IDC_INFO_HOROSCOPE);

							// Level
#if 0
							int suns, moons, stars;

							EvaUtil::calcSuns(READC_W2("Level"),&suns,&moons,&stars);
							swprintf(szTemp,TranslateT("%d (%d-%d-%d)"),READC_W2("Level"), suns,moons,stars);
							SetDlgItemText(hwndDlg,IDC_LEVEL,szTemp);
#endif
							swprintf(szTemp,TranslateT("%d Hours (%d Hours to Next Level)"),READC_D2("OnlineMins")/3600,READC_W2("HoursToLevelUp"));
							SetDlgItemText(hwndDlg,IDC_ONLINE,szTemp);
							//SetWindowLong(hwndDlg,GWL_USERDATA,(LONG)hContact);
						}
						break;
				}
			break;
		}
		break;

#if 0
	case WM_COMMAND:
		/*switch(LOWORD(wParam))
		{

		case IDC_CHANGEDETAILS:
		{
		CallService(MS_UTILS_OPENURL, 1, (LPARAM)"http://www.icq.com/whitepages/user_details.php");
		}
		break;

		case IDCANCEL:
		SendMessage(GetParent(hwndDlg),msg,wParam,lParam);
		break;
		}*/
		break;
#endif
	}
	return FALSE;	
}

int CNetwork::OnDetailsInit(WPARAM wParam, LPARAM lParam)
{
	char* szProto;
	OPTIONSDIALOGPAGE odp={sizeof(odp),-1990000003};
	LocalDetails_t* ldt=new LocalDetails_t();
	HANDLE hContact=(HANDLE)lParam;

	szProto = (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, lParam, 0);
	if ((szProto == NULL || strcmp(szProto, m_szModuleName)) && lParam) return 0;

	odp.hInstance = hinstance;
	//odp.dwInitParam=(LPARAM)ldt;

	ldt->hContact=(HANDLE)lParam;
	ldt->network=this;
	ldt->timer=0;
	networkmap[hContact]=ldt;

#define ADDPAGE(a,b) odp.pszTitle = a; odp.pszTemplate = MAKEINTRESOURCEA(b); CallService(MS_USERINFO_ADDPAGE, wParam, (LPARAM)&odp); odp.position++
	if (READC_B2("IsQun")) {
		// Qun
		odp.pfnDlgProc = QunDetailsDlgProc;

		ADDPAGE(Translate("Qun Details"),IDD_INFO_QUN1);
		ADDPAGE(Translate("Qun Members"),IDD_INFO_QUN2);
	} else {
		// Regular contact 
		odp.pfnDlgProc = QQDetailsDlgProc;

		ADDPAGE(Translate("QQ Overview"),IDD_INFO_QQ1);
		ADDPAGE(Translate("QQ Contact"),IDD_INFO_QQ2);
		ADDPAGE(Translate("QQ Details"),IDD_INFO_QQ3);
	}

	return 0;
}

