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

#define DM_CLOSEME		(WM_USER+1)
#define DM_UPDATETITLE	(WM_USER+16)

extern void KickQunUser(void* args);

CQunListV2* CQunListV2::m_inst=NULL;
HHOOK CQunListV2::hHookMessagePost=NULL;
HWND CQunListV2::m_hwndSRMM=NULL;
int CQunListV2::m_qunid=NULL;
HINSTANCE CQunListV2::m_hInstance=NULL;

int CQunListV2::QunMemberListService(WPARAM wParam, LPARAM lParam) {
	const CWPRETSTRUCT *msg = (CWPRETSTRUCT*)lParam;

	switch(msg->message) 
	{
	case DM_UPDATETITLE:
		{
			if (qqNsThread) {
				TCHAR szClassName[32] = {0};

				GetClassName(msg->hwnd, szClassName, sizeof(szClassName));
				if (!_tcscmp(szClassName,_T("#32770")) && msg->wParam>0) {
					HANDLE hContact=(HANDLE)msg->wParam;
					if (READC_B2("IsQun")==1) {
						int qunid=DBGetContactSettingDword(hContact,qqProtocolName,UNIQUEIDSETTING,0);
						if (qunid!=m_qunid) {
							m_qunid=qunid;
							CQunListV2::getInstance(true);

							m_hwndSRMM=msg->hwnd;
							m_inst->overrideTimer();

							util_log(0,"DM_UPDATETITLE, hContact=%d, className=%s, hWnd=%d, lParam=%d",hContact,szClassName,m_hwndSRMM,msg->lParam);
							m_inst->refresh();
						}
					} else if (m_inst)
						m_inst->hide();
				}
			}
		}
		break;
	case WM_SIZING:
	case WM_MOVING:
	case WM_EXITSIZEMOVE:
		if (m_inst && msg->hwnd==m_hwndSRMM) {
			//util_log(0,"WM_MOVING");
			if (IsIconic(m_hwndSRMM))
				m_inst->hide();
			else
				m_inst->move();
		}
		break;
	case WM_SIZE:
		if (m_inst && msg->hwnd==m_hwndSRMM && wParam==SIZE_MINIMIZED) {
			util_log(0,"Minimized\n");
			m_inst->hide();
		}
		break;
	case WM_CLOSE:
		{
			if (m_inst && msg->hwnd==m_hwndSRMM) {
				//util_log(0,"WM_CLOSE");
				delete m_inst;
			}
		}
		break;
	case WM_ACTIVATE:
		//util_log(0,"WM_ACTIVATE, wParam=%d",wParam);
		if (m_inst && msg->hwnd==m_hwndSRMM && GetForegroundWindow()==m_hwndSRMM /*&& (wParam==WA_ACTIVE || wParam==WA_CLICKACTIVE)*/) {
			util_log(0,"WM_ACTIVATE");
			//qunList->SwitchQun(qunList->GetCurrentQun());
			//SetWindowPos(m_hwnd,m_hwndSRMM,0,0,0,0,SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE);
			m_inst->move();
		}
		break;
	}

	return 0;
}

void CQunListV2::InstallHook(HINSTANCE hInst) {
	m_hInstance=hInst;
	hHookMessagePost = SetWindowsHookEx(WH_CALLWNDPROCRET, (HOOKPROC)CQunListV2::MessageHookProcPost, NULL, GetCurrentThreadId());
}

void CQunListV2::UninstallHook() {
	if (hHookMessagePost) UnhookWindowsHookEx(hHookMessagePost);
}

LRESULT CALLBACK CQunListV2::MessageHookProcPost(int code, WPARAM wParam, LPARAM lParam) {
	GETSETTINGS();
	if (qqSettings->enableQunList) {
		if (code == HC_ACTION)
		{
			const CWPRETSTRUCT *msg = (CWPRETSTRUCT*)lParam;

			switch(msg->message) 
			{
			case DM_UPDATETITLE:
			case WM_SIZING:
			case WM_MOVING:
			case WM_EXITSIZEMOVE:
			case WM_SIZE:
			case WM_CLOSE:
			case WM_ACTIVATE:
				{
					char szService[MAX_PATH];
					strcpy(szService,qqProtocolName);
					strcat(szService,"/QMLService");
					CallService(szService,(WPARAM)msg->message,lParam);
				}
				break;
			}
#if 0
			switch(msg->message) 
			{
			case DM_UPDATETITLE:
				{
					if (qqNsThread) {
						TCHAR szClassName[32] = {0};

						GetClassName(msg->hwnd, szClassName, sizeof(szClassName));
						if (!_tcscmp(szClassName,_T("#32770")) && msg->wParam>0) {
							HANDLE hContact=(HANDLE)msg->wParam;
							if (READC_B2("IsQun")==1) {
								int qunid=DBGetContactSettingDword(hContact,qqProtocolName,UNIQUEIDSETTING,0);
								if (qunid!=m_qunid) {
									m_qunid=qunid;
									CQunListV2::getInstance(true);

									m_hwndSRMM=msg->hwnd;
									m_inst->overrideTimer();

									util_log(0,"DM_UPDATETITLE, hContact=%d, className=%s, hWnd=%d, lParam=%d",hContact,szClassName,m_hwndSRMM,msg->lParam);
									m_inst->refresh();
								}
							} else if (m_inst)
								m_inst->hide();
						}
					}
				}
				break;
			case WM_SIZING:
			case WM_MOVING:
			case WM_EXITSIZEMOVE:
				if (m_inst && msg->hwnd==m_hwndSRMM) {
					//util_log(0,"WM_MOVING");
					if (IsIconic(m_hwndSRMM))
						m_inst->hide();
					else
						m_inst->move();
				}
				break;
			case WM_SIZE:
				if (m_inst && msg->hwnd==m_hwndSRMM && wParam==SIZE_MINIMIZED) {
					util_log(0,"Minimized\n");
					m_inst->hide();
				}
				break;
			case WM_CLOSE:
				{
					if (m_inst && msg->hwnd==m_hwndSRMM) {
						//util_log(0,"WM_CLOSE");
						delete m_inst;
					}
				}
				break;
			case WM_ACTIVATE:
				//util_log(0,"WM_ACTIVATE, wParam=%d",wParam);
				if (m_inst && msg->hwnd==m_hwndSRMM && GetForegroundWindow()==m_hwndSRMM /*&& (wParam==WA_ACTIVE || wParam==WA_CLICKACTIVE)*/) {
					util_log(0,"WM_ACTIVATE");
					//qunList->SwitchQun(qunList->GetCurrentQun());
					//SetWindowPos(m_hwnd,m_hwndSRMM,0,0,0,0,SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE);
					m_inst->move();
				}
				break;
			}
#endif
		}

	}
	return CallNextHookEx(hHookMessagePost, code, wParam, lParam);
}

CQunListV2* CQunListV2::getInstance(bool create) {
	if (m_inst || !create)
		return m_inst;
	else {
		m_inst=new CQunListV2();
		return m_inst;
	}
}

CQunListV2::CQunListV2(): m_timerEnabled(false), m_updating(false), hHeadImg(NULL), noheadimg(false), m_members(0), m_online(0) {
	hBrushBkgnd=CreateSolidBrush(0xffb75d);
	hBrushNotice=CreateSolidBrush(0xd6ffff);
	hBrushTitle=CreateSolidBrush(0xfffff9);
	hPenTitle=CreatePen(PS_SOLID,1,0xc26724);
	hFontBold=CreateFontA(12,0,0,0,FW_BOLD,0,0,0,0,0,0,0,0,"MS Shell Dlg");

	headlist.clear();
	m_hwnd=CreateDialog(m_hInstance,MAKEINTRESOURCE(IDD_QUNLIST),NULL,DialogProc);

}

CQunListV2::~CQunListV2() {
	DestroyWindow(m_hwnd);
	if (hHeadImg) FreeLibrary(hHeadImg);
	for (map<int,HBITMAP>::iterator iter=headlist.begin(); iter!=headlist.end(); iter++)
		DeleteObject(iter->second);

	m_inst=NULL;
	m_qunid=NULL;

	DeleteObject(hBrushBkgnd);
	DeleteObject(hBrushNotice);
	DeleteObject(hBrushTitle);
	DeleteObject(hPenTitle);
	DeleteObject(hFontBold);
	//DeleteObject(hFontNormal);
}

void CQunListV2::hide() {
	ShowWindow(m_hwnd,SW_HIDE);
	m_qunid=NULL;
}

INT_PTR CALLBACK CQunListV2::_DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_ERASEBKGND:
		{
			HDC hDC=(HDC)wParam;
			if (hDC != 0) {
				RECT rect, rect2;
				TCHAR szTemp[MAX_PATH];
				LPTSTR pszNotice;

				GetClientRect(hwndDlg,&rect);
				FillRect(hDC,&rect,hBrushBkgnd);

				SelectObject(hDC,hPenTitle);
				SelectObject(hDC,hBrushNotice);
				Rectangle(hDC,5,5,180,119);
				SetBkColor(hDC,0xd6ffff);

				SetRect(&rect2,6,6,179,118);
				pszNotice=TranslateT("Notice");
				DrawText(hDC,pszNotice,_tcslen(pszNotice),&rect2,DT_CENTER);

				SelectObject(hDC,hBrushTitle);
				Rectangle(hDC,5,118,180,138);
				SetRect(&rect2,7,120,180,138);
				if (m_members>0) {
					_stprintf(szTemp,_T("Qun Members (%d/%d)"),m_online,m_members);
					SetBkColor(hDC,0xfffff9);
					DrawText(hDC,szTemp,_tcslen(szTemp),&rect2,0);
				}

				Rectangle(hDC,5,137,180,rect.bottom-5);

			}
			return TRUE;
		}
	case WM_CTLCOLORSTATIC:
		switch (GetDlgCtrlID((HWND)lParam)) {
		case IDC_QUNNOTICE:
			SetBkColor((HDC)wParam,0xd6ffff);
			return (LRESULT)hBrushNotice;
		default:
			return FALSE;
		}
		break;
	case WM_INITDIALOG:
		MoveWindow(GetDlgItem(hwndDlg,IDC_QUNNOTICE),10,20,160,94,FALSE);
		return TRUE;
	case WM_MEASUREITEM:
		((LPMEASUREITEMSTRUCT)lParam)->itemHeight=18; 
		return TRUE; 

		/*case WM_INITDIALOG:
		((CQunList*)lParam)->_InitDialog(hwndDlg);
		break;
		case DM_CLOSEME:
		EndDialog(hwndDlg,0);
		break;
		case WM_DESTROY:
		m_instance->_OnDestroy();
		break;*/
	case WM_SIZE:
		move();
		break;
	case WM_CLOSE:
		{
			char tModule[ 100 ];

			strcpy(tModule,qqProtocolName);
			strcat(tModule,QQ_MENU_TOGGLEQUNLIST);

			CallService(tModule,1,0);
		}
		break;
	case WM_DESTROY:
		{
			m_hwnd=NULL;
			break;
		}
		/*case WM_TIMER:
		{
		KillTimer(hwndDlg,1);
		if (m_instance) {
		Qun* qun=m_instance->GetCurrentQun();
		if (qun) {
		int qunid=qun->getQunID();
		//m_instance->ForceUpdate();
		qqNetwork->send(new QunGetOnlineMemberPacket(qunid));
		SetTimer(m_instance->GetHWND(),1,60000,NULL);
		}
		}
		}
		break;*/
		/*case WM_MOVING:
		return TRUE;
		case WM_ACTIVATE:
		if (wParam==WM_ACTIVATE || wParam==WM_MOUSEACTIVATE) {
		ShowWindow(m_instance->GetSRMMHWND(),SW_SHOW);
		}
		break;*/
		/*case WM_NOTIFY:
		{
			LPNMHDR nmhdr=(LPNMHDR)lParam;*/
	case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDC_QUNMEMBERS:
					if (m_hInstance/* &&(nmhdr->code==NM_DBLCLK || nmhdr->code==NM_RCLICK)*/ && /*(HIWORD(wParam)==WM_LBUTTONDBLCLK || HIWORD(wParam)==WM_RBUTTONUP)*/HIWORD(wParam)==LBN_DBLCLK) {
						int selected=SendDlgItemMessage (m_hwnd,IDC_QUNMEMBERS,LB_GETCURSEL,0,0);
						if(selected!=LB_ERR)
						{
							//if (nmhdr->code==NM_DBLCLK) {
							/*if (HIWORD(wParam)==WM_LBUTTONDBLCLK) {
								// Send Message
								TCHAR szTemp[MAX_PATH];
								SendDlgItemMessage(m_hwnd,IDC_QUNMEMBERS,LB_GETTEXT,selected,(LPARAM)szTemp);
								if (*szTemp!=_T('0')) {
									int qqid=_ttoi(_tcsrchr(szTemp,_T('('))+1);
									HANDLE hContact=FindContact(qqid);
									if (hContact)
										CallService(MS_MSG_SENDMESSAGE, (WPARAM)hContact, 0);
									else {
										char msg[16];
										hContact=FindContact(m_inst->getQunid());
										sprintf(msg,"/temp %d",qqid);
										CallContactService(hContact,PSS_MESSAGE,0,(LPARAM)msg);
									}
								}

							//} else if (nmhdr->code==NM_RCLICK) {
							} else if (HIWORD(wParam)==LBN_DBLCLK)*/ {
								// Right-Click
								HMENU hMenu=LoadMenu(m_hInstance,MAKEINTRESOURCE(IDR_QUNMEMBER));
								HMENU hMenu2=GetSubMenu(hMenu,0);
								MENUITEMINFO mii={sizeof(MENUITEMINFO),MIIM_STRING};

								if (hMenu2) {
									TCHAR szTemp[MAX_PATH];
									SendDlgItemMessage(m_hwnd,IDC_QUNMEMBERS,LB_GETTEXT,selected,(LPARAM)szTemp);
									if (*szTemp!=_T('0')) {
										int qqid=_ttoi(_tcsrchr(szTemp,_T('('))+1);
										POINT pt;

										mii.dwTypeData=szTemp+1;
										SetMenuItemInfo(hMenu2,ID__INFO,FALSE,&mii);
										uint* ui=(uint*)malloc(sizeof(uint)*2);
										GetCursorPos(&pt);

										ui[0]=(uint)TrackPopupMenu(hMenu2,TPM_RETURNCMD,pt.x,pt.y,0,m_hwnd,NULL);
										ui[1]=qqid;

										//mir_forkthread(_HandlePopup,ui);
										_HandlePopup(ui);
									}
								}
								DestroyMenu(hMenu);
							}
						}
					} else {
						util_log(0,"Notification code is 0x%x",HIWORD(wParam));
					}
					break;
				case IDCANCEL:
					{
						char szName[MAX_PATH];
						strcpy(szName,qqProtocolName);
						strcat(szName,QQ_MENU_TOGGLEQUNLIST);
						CallService(QQ_MENU_TOGGLEQUNLIST,1,0);
					}
					break;
			}
			break;
	case WM_TIMER:
		if (m_timerEnabled=true) KillTimer(hwndDlg,1);
		if (qqNsThread) qqNsThread->sendPacket(new QunGetOnlineMemberPacket(m_qunid));
		m_timerEnabled=true;
		SetTimer(hwndDlg,1,60000,NULL);
		return 0;
	case WM_DRAWITEM: 
		{
			if (wParam==IDC_QUNMEMBERS && !m_updating) {
				LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT) lParam; 
				TCHAR tchBuffer[MAX_PATH]; 
				TEXTMETRIC tm; 
				int y; 
				HDC hdcMem; 
				HFONT hFontNormal=NULL;
				HBITMAP hbmpOld, hBmpNew;
				BYTE* flag=(BYTE*)&lpdis->itemData;
				short* headimg=(short*)(flag+2);

				// If there are no list box items, skip this message. 

				if (lpdis->itemID == -1) 
				{ 
					break; 
				} 

				// Draw the bitmap and text for the list box item. Draw a 
				// rectangle around the bitmap if it is selected. 

				switch (lpdis->itemAction) 
				{ 
				case ODA_SELECT: 
				case ODA_DRAWENTIRE: 

					// Display the bitmap associated with the item. 
					
					/*hbmpPicture =(HBITMAP)SendMessage(lpdis->hwndItem, 
					LB_GETITEMDATA, lpdis->itemID, (LPARAM) 0); */

					
					SendDlgItemMessage(m_hwnd,IDC_QUNMEMBERS,LB_GETTEXT,lpdis->itemID,(LPARAM)tchBuffer);

					if (*tchBuffer==_T('0')) {
						SetBkColor(lpdis->hDC,RGB(255,255,255));
						SetTextColor(lpdis->hDC,0);

						SendMessage(lpdis->hwndItem, LB_GETTEXT, 
							lpdis->itemID, (LPARAM) tchBuffer); 

						TextOut(lpdis->hDC, 5, lpdis->rcItem.top+1, tchBuffer+1, _tcslen(tchBuffer+1)); 						

						return TRUE;
					}

					hdcMem = CreateCompatibleDC(lpdis->hDC); 
					hBmpNew=headlist[_ttoi(_tcsrchr(tchBuffer,_T('('))+1)];
					if (!hBmpNew) hBmpNew=headlist[*headimg];
					hbmpOld =(HBITMAP) SelectObject(hdcMem, hBmpNew); 

					// Display the text associated with the item. 

					/*RECT rcBitmap;
					rcBitmap.left = lpdis->rcItem.left; 
					rcBitmap.top = lpdis->rcItem.top; 
					rcBitmap.right = lpdis->rcItem.left + XBITMAP; 
					rcBitmap.bottom = lpdis->rcItem.top + YBITMAP; */

					FillRect(lpdis->hDC,&lpdis->rcItem,(lpdis->itemState & ODS_SELECTED)?GetSysColorBrush(COLOR_HIGHLIGHT):WHITE_BRUSH);
					/*BitBlt(lpdis->hDC, 
						lpdis->rcItem.left, lpdis->rcItem.top, 
						lpdis->rcItem.right - lpdis->rcItem.left, 
						lpdis->rcItem.bottom - lpdis->rcItem.top, 
						hdcMem, 0, 0, SRCCOPY); */

					StretchBlt(lpdis->hDC,lpdis->rcItem.left+16, lpdis->rcItem.top+1,16,16,hdcMem,0,0,32,32,SRCCOPY);

					switch ((*tchBuffer-_T('0'))%5) {
						case 1: // Qun Creator
							SelectObject(hdcMem, hbmpOld); 
							hbmpOld =(HBITMAP) SelectObject(hdcMem, headlist[1000]); 
							BitBlt(lpdis->hDC,lpdis->rcItem.left, lpdis->rcItem.top+1,16,16,hdcMem,0,0,SRCCOPY);
							break;
						case 2: // Admins
							SelectObject(hdcMem, hbmpOld); 
							hbmpOld =(HBITMAP) SelectObject(hdcMem, headlist[1001]); 
							BitBlt(lpdis->hDC,lpdis->rcItem.left, lpdis->rcItem.top+1,16,16,hdcMem,0,0,SRCCOPY);
							break;
						case 3: // Investors
							SelectObject(hdcMem, hbmpOld); 
							hbmpOld =(HBITMAP) SelectObject(hdcMem, headlist[1002]); 
							BitBlt(lpdis->hDC,lpdis->rcItem.left, lpdis->rcItem.top+1,16,16,hdcMem,0,0,SRCCOPY);
							break;
					}

					SelectObject(hdcMem, hbmpOld); 
					DeleteDC(hdcMem); 

					SetBkColor(lpdis->hDC,(lpdis->itemState & ODS_SELECTED)?GetSysColor(COLOR_HIGHLIGHT):RGB(255,255,255));
					SetTextColor(lpdis->hDC,(lpdis->itemState & ODS_SELECTED)?GetSysColor(COLOR_HIGHLIGHTTEXT):0);

					if (lpdis->itemState & ODS_SELECTED) 
					{ 
						// Set RECT coordinates to surround only the 
						// bitmap. 
						// Draw a rectangle around bitmap to indicate 
						// the selection. 

						//FillRect(lpdis->hDC,&rcBitmap,GetSysColorBrush(COLOR_HIGHLIGHT));
						DrawFocusRect(lpdis->hDC, &lpdis->rcItem/* &rcBitmap*/); 
					}

					SendMessage(lpdis->hwndItem, LB_GETTEXT, 
						lpdis->itemID, (LPARAM) tchBuffer); 

					GetTextMetrics(lpdis->hDC, &tm); 

					y = (lpdis->rcItem.bottom + lpdis->rcItem.top - 
						tm.tmHeight) / 2+1;


					if (*tchBuffer<=_T('5')) {
						hFontNormal=(HFONT)SelectObject(lpdis->hDC,hFontBold);
					}

					TextOut(lpdis->hDC, 
						40, 
						y, 
						tchBuffer+1, 
						_tcslen(tchBuffer+1)); 						
					

					if (hFontNormal) {
						SelectObject(lpdis->hDC,hFontNormal);
					}

					GdiFlush();

					break; 

				case ODA_FOCUS: 

					// Do not process focus changes. The focus caret 
					// (outline rectangle) indicates the selection. 
					// The IDOK button indicates the final 
					// selection. 

					break; 
				}
			}
			return TRUE; 
		}
	}
	return FALSE;
}

INT_PTR CALLBACK CQunListV2::DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	return m_inst->_DialogProc(hwndDlg,uMsg,wParam,lParam);
}

extern QunList qunList;

void CQunListV2::refresh() {
	HWND hWnd=GetForegroundWindow();
	if (hWnd==this->m_hwnd || hWnd==this->m_hwndSRMM) {
		hContact=FindContact(m_qunid);
		if (hContact) {
			DBVARIANT dbv;
			Qun* qun=qunList.getQun(m_qunid);

			if (!DBGetContactSettingTString(hContact,"CList","StatusMsg",&dbv) && qun) {
				list<FriendItem> members=qun->getMembers();

				SetDlgItemText(m_hwnd,IDC_QUNNOTICE,dbv.ptszVal);
				DBFreeVariant(&dbv);
				//hide();
				move();

				SendDlgItemMessage(m_hwnd,IDC_QUNMEMBERS,LB_RESETCONTENT,NULL,NULL);
				m_members=members.size();
				m_online=0;

				if (m_members>0) {
					TCHAR szItem[MAX_PATH];
					CHAR szKey[11];
					LONG flags;
					BYTE* flag=(BYTE*)&flags;
					short* headimg=(short*)(flag+2);
					HBITMAP hBMPHead;
					int id;
					char szPluginPath[MAX_PATH];
					LPTSTR pszTemp;
					QunInfo info=qun->getDetails();

					m_updating=true;

					if (headlist.size()==0) {
						if (!hHeadImg) {
							char szPluginPath[MAX_PATH];
							CallService(MS_UTILS_PATHTOABSOLUTE,(WPARAM)"Plugins\\QQHeadImg.dll",(LPARAM)szPluginPath);
							hHeadImg=LoadLibraryA(szPluginPath);
						}

						if (hHeadImg) {
							for (int c=1000; c<1003; c++) {
								hBMPHead=LoadBitmap(hHeadImg,MAKEINTRESOURCE(c));
								if (hBMPHead) headlist[c]=hBMPHead;
							}
						} else
							noheadimg=true;
					}

					util_convertToNative(&pszTemp,info.getName().c_str());
					_stprintf(szItem,_T("%s (%d)"),pszTemp,info.getExtID());
					free(pszTemp);
					SetWindowText(m_hwnd,szItem);

					for (list<FriendItem>::iterator iter=members.begin(); iter!=members.end(); iter++) {
						// Byte 0: offline=5, normal=4, investor=3, admins=2, creator=1
						*szItem=_T('0');
						if (iter->isOnline()) 
							m_online++;
						else
							*szItem+=5;

						if (iter->isShareHolder()) 
							*szItem+=3;
						else if (iter->isAdmin())
							*szItem+=2;
						else if (READC_D2("Creator")==iter->getQQ())
							*szItem+=1;
						else
							*szItem+=4;

						*flag=iter->getCommonFlag();
						flag[1]=iter->getExtFlag();
						/*flag[2]=iter->getQunAdminValue();
						flag[3]=iter->getQunGroupIndex();*/
						*headimg=iter->getFace();

						itoa(iter->getQQ(),szKey,10);
						if (READC_S2(szKey,&dbv)) {
							// No Nick found
							_stprintf(szItem+1,_T("%d (%d)"),iter->getQQ(),iter->getQQ());
						} else {
							// Nick found
							LPTSTR pszNick;
							util_convertToNative(&pszNick,dbv.pszVal);
							_stprintf(szItem+1,_T("%s (%d)"),pszNick,iter->getQQ());
							free(pszNick);
						}
						//_itot(iter->getQQ(),szItem,10);
						id=SendDlgItemMessage(m_hwnd,IDC_QUNMEMBERS,LB_ADDSTRING,NULL,(LPARAM)szItem);
						SendDlgItemMessage(m_hwnd,IDC_QUNMEMBERS,LB_SETITEMDATA,id,(LPARAM)flags);

						hBMPHead=headlist[iter->getQQ()];
						if (!hBMPHead) {
							bool ret=true;
							HANDLE hContact2=FindContact(iter->getQQ());
							if (hContact2) ret=DBGetContactSetting(hContact2,qqProtocolName,"UserHeadMD5",&dbv);
							if (!ret) {
								CallService(MS_UTILS_PATHTOABSOLUTE,(WPARAM)"QQ\\",(LPARAM)szPluginPath);
								strcat(szPluginPath,dbv.pszVal);
								strcat(szPluginPath,".bmp");

								hBMPHead=(HBITMAP)LoadImageA(NULL,szPluginPath,IMAGE_BITMAP,32,32,LR_LOADFROMFILE);
								DBFreeVariant(&dbv);
							}

							//if ((hBMPHead=(HBITMAP)LoadImageA(NULL,szPluginPath,IMAGE_BITMAP,LR_DEFAULTSIZE,LR_DEFAULTSIZE,LR_DEFAULTCOLOR))==NULL) {
							if (!hBMPHead) {
								if (!noheadimg) {
									//if (*headimg<=2) *headimg=300;
									hBMPHead=LoadBitmap(hHeadImg,MAKEINTRESOURCE(100+(*headimg<=2?300:(*headimg/3))+1));
									if (!hBMPHead) hBMPHead=LoadBitmap(hHeadImg,MAKEINTRESOURCE(201));
									headlist[*headimg]=hBMPHead;
								}
							} else
								headlist[iter->getQQ()]=hBMPHead;
						}
					} 

					m_updating=false;
					m_flush=0;
					InvalidateRect(m_hwnd,NULL,TRUE);
					InvalidateRect(GetDlgItem(m_hwnd,IDC_QUNMEMBERS),NULL,FALSE);

					if (!m_timerEnabled) {
						DialogProc(m_hwnd,WM_TIMER,1,0);
					}
				} else if (READC_W2("QunVersion")==0) {
					SendDlgItemMessage(m_hwnd,IDC_QUNMEMBERS,LB_ADDSTRING,NULL,(LPARAM)TranslateT("0Please Wait..."));
					qqNsThread->sendPacket(new QunGetInfoPacket(m_qunid));
				} else
					SendDlgItemMessage(m_hwnd,IDC_QUNMEMBERS,LB_ADDSTRING,NULL,(LPARAM)TranslateT("0Please Wait..."));

				ShowWindow(m_hwnd,SW_SHOWNA);
			} else {
				if (!qun) {
					// Qun not init, I init
					qunList.add(Qun(m_qunid));
					qqNsThread->sendPacket(new QunGetInfoPacket(m_qunid));
				}
			}
		}
	}
}

void CQunListV2::move() {
	RECT rect;
	GetWindowRect(m_hwndSRMM,&rect);
	MoveWindow(m_hwnd,rect.right,rect.top,190,rect.bottom-rect.top,TRUE);
	GetClientRect(m_hwnd,&rect);
	MoveWindow(GetDlgItem(m_hwnd,IDC_QUNMEMBERS),6,139,173,rect.bottom-145,TRUE);
	SetWindowPos(m_hwnd,m_hwndSRMM,0,0,0,0,SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE);
}

int CQunListV2::getQunid() {
	return m_qunid;
}

void CQunListV2::_HandlePopup(void* data) {
	uint* ui=(uint*)data;

	switch (ui[0]) {
		case ID__SENDMESSAGE:
			{
				HANDLE hContact=FindContact(ui[1]);
				if (hContact)
					CallService(MS_MSG_SENDMESSAGE, (WPARAM)hContact, 0);
				else {
					char msg[16];
					hContact=FindContact(m_inst->getQunid());
					sprintf(msg,"/temp %d",ui[1]);
					CallContactService(hContact,PSS_MESSAGE,0,(LPARAM)msg);
				}
				break;
			}
		case ID__USERDETAILS:
			{
				HANDLE hContact=FindContact((int)ui[1]);
				if (!hContact) hContact=AddContact(ui[1],true,true);
				CallService(MS_USERINFO_SHOWDIALOG, (WPARAM)hContact, 0);
				//Sleep(10000);
				break;
			}
		case ID__KICKMEMBER:
			{
				KICKUSERSTRUCT* kickUser=(KICKUSERSTRUCT*)malloc(sizeof(KICKUSERSTRUCT));
				kickUser->qunid=m_inst->getQunid();
				kickUser->qqid=ui[1];
				kickUser->network=this;
				mir_forkthread(KickQunUser,kickUser);
			}
	}
	free(ui);
}

void CQunListV2::destroy() {
	delete m_inst;
}