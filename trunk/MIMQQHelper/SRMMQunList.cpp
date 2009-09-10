#include "StdAfx.h"
typedef struct {
	int qunid;
	int qqid;
	LPVOID network;
} KICKUSERSTRUCT;

extern HINSTANCE hInstance;

CSRMMQunList* CSRMMQunList::getInstance() {
	if (m_inst)
		return (CSRMMQunList*) m_inst;
	else if (DBGetContactSettingByte(NULL,MIMQQ,"EnableQunList",0))
		return new CSRMMQunList();
	else
		return NULL;
}

CSRMMQunList::CSRMMQunList(): CQunListBase(), m_hWnd(NULL), hHeadImg(NULL), noheadimg(false), m_updating(false) {
	OutputDebugString(L"CSRMMQunList::CSRMMQunList\n");
	hBrushBkgnd=CreateSolidBrush(0xffb75d);
	hBrushNotice=CreateSolidBrush(0xd6ffff);
	hBrushTitle=CreateSolidBrush(0xfffff9);
	hPenTitle=CreatePen(PS_SOLID,1,0xc26724);
	hFontBold=CreateFontA(12,0,0,0,FW_BOLD,0,0,0,0,0,0,0,0,"MS Shell Dlg");
}

CSRMMQunList::~CSRMMQunList() {
	OutputDebugString(L"CSRMMQunList::~CSRMMQunList\n");
	if (m_hWnd) EndDialog(m_hWnd,0);
	m_hWnd=NULL;

	if (hHeadImg) FreeLibrary(hHeadImg);
	for (map<int,HBITMAP>::iterator iter=m_headlist.begin(); iter!=m_headlist.end(); iter++)
		DeleteObject(iter->second);

	DeleteObject(hBrushBkgnd);
	DeleteObject(hBrushNotice);
	DeleteObject(hBrushTitle);
	DeleteObject(hPenTitle);
	DeleteObject(hFontBold);
}

void CSRMMQunList::TabSwitched(CWPRETSTRUCT* cps) {
	OutputDebugString(L"CSRMMQunList::TabSwitched\n");

	hContact=(HANDLE)cps->wParam;
	LPSTR szMIMQQ=GETPROTO();

	if (READC_B2("IsQun")==1) {
		int qunid=READC_D2("UID");
		if (qunid!=m_qunid) {
			bool firsttime=quns[qunid].size()==0;
			m_qunid=qunid;
			m_hwndSRMM=cps->hwnd;

			if (!m_hWnd) 
				m_hWnd=CreateDialog(m_hInstance,MAKEINTRESOURCE(IDD_QUNLIST),NULL,DialogProc);
			else
				Show();

			PostMessage(m_hWnd,DM_UPDATETITLE,firsttime,0);

#if 0
			m_headlist.clear();			

			if (m_timerEvent) SetEvent(m_timerEvent);
			mir_forkthread(TimerThread,(LPVOID)m_qunid);

			//util_log(0,"DM_UPDATETITLE, hContact=%d, className=%s, hWnd=%d, lParam=%d",hContact,szClassName,m_hwndSRMM,msg->lParam);
			//m_inst->refresh();
			Refresh();
			if (!firsttime)
				//CallService(szIPCService,QQIPCSVC_QUN_UPDATE_ONLINE_MEMBERS,m_qunid);
				CallContactService(hContact,IPCSVC,QQIPCSVC_QUN_UPDATE_ONLINE_MEMBERS,m_qunid);
#endif
		}
	} else {
		Hide();
	}
}

void CSRMMQunList::Hide() {
	if (IsWindow(m_hWnd)) {
		OutputDebugString(L"CSRMMQunList::Hide\n");
		ShowWindow(m_hWnd,SW_HIDE);
		m_qunid=NULL;
	}
}

void CSRMMQunList::Show() {
	if (IsWindow(m_hWnd)) {
		OutputDebugString(L"CSRMMQunList::Show\n");
		ShowWindow(m_hWnd,SW_SHOWNOACTIVATE);
	}
}

void CSRMMQunList::Move() {
	if (IsWindow(m_hWnd) && IsWindow(m_hwndSRMM)) {
		RECT rect;
		OutputDebugString(L"CSRMMQunList::Move\n");
		GetWindowRect(m_hwndSRMM,&rect);
		MoveWindow(m_hWnd,rect.right,rect.top,190,rect.bottom-rect.top,TRUE);
		GetClientRect(m_hWnd,&rect);
		MoveWindow(GetDlgItem(m_hWnd,IDC_QUNMEMBERS),6,139,173,rect.bottom-145,TRUE);
		SetWindowPos(m_hWnd,m_hwndSRMM,0,0,0,0,SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE);
	}
}

void CSRMMQunList::Close() {
	if (IsWindow(m_hWnd)) {
		EndDialog(m_hWnd,0);
		DestroyWindow(m_hWnd);
		m_qunid=0;
		if (m_hWnd) OutputDebugString(L"m_hWnd not freed!");
	}
}

#define FindContact(x) (HANDLE)CallProtoService(szMIMQQ,IPCSVC,QQIPCSVC_FIND_CONTACT,(LPARAM)x)

void CSRMMQunList::_HandlePopup(void* data) {
	unsigned int* ui=(unsigned int*)data;
	LPSTR szMIMQQ=GETPROTO();

	switch (ui[0]) {
		case ID__SENDMESSAGE:
			{
				HANDLE hContact=FindContact(ui[1]);
				if (hContact)
					CallService(MS_MSG_SENDMESSAGE, (WPARAM)hContact, 0);
				else {
					char msg[16];
					hContact=FindContact(m_qunid);
					sprintf(msg,"/temp %d",ui[1]);
					CallContactService(hContact,PSS_MESSAGE,0,(LPARAM)msg);
				}
				break;
			}
		case ID__USERDETAILS:
			{
				CallProtoService(szMIMQQ,IPCSVC,QQIPCSVC_SHOW_DETAILS,(LPARAM)ui[1]);
				//Sleep(10000);
				break;
			}
		case ID__KICKMEMBER:
			{
				KICKUSERSTRUCT* kickUser=(KICKUSERSTRUCT*)mir_alloc(sizeof(KICKUSERSTRUCT));
				kickUser->qunid=m_qunid;
				kickUser->qqid=ui[1];
				kickUser->network=(LPVOID)CallProtoService(szMIMQQ,IPCSVC,QQIPCSVC_GET_NETWORK,0);
				CallProtoService(szMIMQQ,IPCSVC,QQIPCSVC_QUN_KICK_USER,(LPARAM)&kickUser);
				//mir_forkthread(KickQunUser,kickUser);
			}
	}
	free(ui);
}

INT_PTR CALLBACK CSRMMQunList::_DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
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
				DrawText(hDC,pszNotice,(int)_tcslen(pszNotice),&rect2,DT_CENTER);

				SelectObject(hDC,hBrushTitle);
				Rectangle(hDC,5,118,180,138);
				SetRect(&rect2,7,120,180,138);
				if (quns[m_qunid].size()>0) {
					_stprintf(szTemp,_T("Qun Members (%d/%d)"),onlinemembers[m_qunid],quns[m_qunid].size());
					SetBkColor(hDC,0xfffff9);
					DrawText(hDC,szTemp,(int)_tcslen(szTemp),&rect2,0);
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
		Move();
		break;
	case WM_CLOSE:
		{
		}
		break;
	case WM_DESTROY:
		{
			m_hWnd=NULL;
			break;
		}
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
			case IDC_QUNMEMBERS:
				if (m_hInstance/* &&(nmhdr->code==NM_DBLCLK || nmhdr->code==NM_RCLICK)*/ && /*(HIWORD(wParam)==WM_LBUTTONDBLCLK || HIWORD(wParam)==WM_RBUTTONUP)*/HIWORD(wParam)==LBN_DBLCLK) {
					int selected=(int)SendDlgItemMessage (hwndDlg,IDC_QUNMEMBERS,LB_GETCURSEL,0,0);
					if(selected!=LB_ERR) {
						// Right-Click
						HMENU hMenu=LoadMenu(hInstance,MAKEINTRESOURCE(IDR_QUNMEMBER));
						HMENU hMenu2=GetSubMenu(hMenu,0);
						MENUITEMINFO mii={sizeof(MENUITEMINFO),MIIM_STRING};

						if (hMenu2) {
							TCHAR szTemp[MAX_PATH];
							SendDlgItemMessage(hwndDlg,IDC_QUNMEMBERS,LB_GETTEXT,selected,(LPARAM)szTemp);
							if (*szTemp!=_T('0')) {
								int qqid=_ttoi(_tcsrchr(szTemp,_T('('))+1);
								POINT pt;

								mii.dwTypeData=szTemp+1;
								SetMenuItemInfo(hMenu2,ID__INFO,FALSE,&mii);
								unsigned int* ui=(unsigned int*)malloc(sizeof(unsigned int)*2);
								GetCursorPos(&pt);

								ui[0]=(unsigned int)TrackPopupMenu(hMenu2,TPM_RETURNCMD,pt.x,pt.y,0,hwndDlg,NULL);
								ui[1]=qqid;

								//mir_forkthread(_HandlePopup,ui);
								_HandlePopup(ui);
							}
							DestroyMenu(hMenu);
						}
					} /*else {
						util_log(0,"Notification code is 0x%x",HIWORD(wParam));
					}*/
				}
				break;
			case IDCANCEL:
				CallService("MIRANDAQQ/Helper_ToggleQunList",0,0);
				break;
		}
		break;
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


					SendDlgItemMessage(m_hWnd,IDC_QUNMEMBERS,LB_GETTEXT,lpdis->itemID,(LPARAM)tchBuffer);

					if (*tchBuffer==_T('0')) {
						SetBkColor(lpdis->hDC,RGB(255,255,255));
						SetTextColor(lpdis->hDC,0);

						SendMessage(lpdis->hwndItem, LB_GETTEXT, 
							lpdis->itemID, (LPARAM) tchBuffer); 

						TextOut(lpdis->hDC, 5, lpdis->rcItem.top+1, tchBuffer+1, (int)_tcslen(tchBuffer+1)); 						

						return TRUE;
					}

					hdcMem = CreateCompatibleDC(lpdis->hDC); 
					hBmpNew=m_headlist[_ttoi(_tcsrchr(tchBuffer,_T('('))+1)];
					if (!hBmpNew) hBmpNew=m_headlist[*headimg];
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
					hbmpOld =(HBITMAP) SelectObject(hdcMem, m_headlist[1000]); 
					BitBlt(lpdis->hDC,lpdis->rcItem.left, lpdis->rcItem.top+1,16,16,hdcMem,0,0,SRCCOPY);
					break;
				case 2: // Admins
					SelectObject(hdcMem, hbmpOld); 
					hbmpOld =(HBITMAP) SelectObject(hdcMem, m_headlist[1001]); 
					BitBlt(lpdis->hDC,lpdis->rcItem.left, lpdis->rcItem.top+1,16,16,hdcMem,0,0,SRCCOPY);
					break;
				case 3: // Investors
					SelectObject(hdcMem, hbmpOld); 
					hbmpOld =(HBITMAP) SelectObject(hdcMem, m_headlist[1002]); 
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

					TextOut(lpdis->hDC, 40, y, tchBuffer+1, (int)_tcslen(tchBuffer+1));

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
	case DM_UPDATETITLE:
		m_headlist.clear();			

		if (m_timerEvent) SetEvent(m_timerEvent);
		mir_forkthread(TimerThread,(LPVOID)m_qunid);

		//util_log(0,"DM_UPDATETITLE, hContact=%d, className=%s, hWnd=%d, lParam=%d",hContact,szClassName,m_hwndSRMM,msg->lParam);
		//m_inst->refresh();
		Refresh();
		if (!wParam)
			CallContactService(hContact,IPCSVC,QQIPCSVC_QUN_UPDATE_ONLINE_MEMBERS,m_qunid);
		break;
	}
	return FALSE;
}

INT_PTR CALLBACK CSRMMQunList::DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	return ((CSRMMQunList*)m_inst)->_DialogProc(hwndDlg,uMsg,wParam,lParam);
}

void CSRMMQunList::Refresh() {
	HWND hWnd=GetForegroundWindow();
	if (hWnd==this->m_hWnd || hWnd==this->m_hwndSRMM && IsWindow(this->m_hwndSRMM) && IsWindow(this->m_hWnd)) {
		OutputDebugString(L"CSRMMQunList::Refresh\n");
		//hContact=util_find_contact(m_qunid);
		if (hContact) {
			DBVARIANT dbv;

			if (!DBGetContactSettingTString(hContact,"CList","StatusMsg",&dbv) && quns[m_qunid].size()) {
				map<UINT,ipcmember_t> friends=quns[m_qunid];
				LPSTR szMIMQQ=GETPROTO();

				SetDlgItemText(m_hWnd,IDC_QUNNOTICE,dbv.ptszVal);
				DBFreeVariant(&dbv);
				//hide();
				Move();

				SendDlgItemMessage(m_hWnd,IDC_QUNMEMBERS,LB_RESETCONTENT,NULL,NULL);

				if (friends.size()>0) {
					TCHAR szItem[MAX_PATH];
					CHAR szKey[11];
					LONG flags;
					BYTE* flag=(BYTE*)&flags;
					short* headimg=(short*)(flag+2);
					HBITMAP hBMPHead;
					int id;
					char szPluginPath[MAX_PATH];

					m_updating=true;

					if (m_headlist.size()==0) {
						if (!hHeadImg) {
							char szPluginPath[MAX_PATH];
							CallService(MS_UTILS_PATHTOABSOLUTE,(WPARAM)"Plugins\\QQHeadImg.dll",(LPARAM)szPluginPath);
							hHeadImg=LoadLibraryA(szPluginPath);
						}

						if (hHeadImg) {
							for (int c=1000; c<1003; c++) {
								hBMPHead=LoadBitmap(hHeadImg,MAKEINTRESOURCE(c));
								if (hBMPHead) m_headlist[c]=hBMPHead;
							}
						} else
							noheadimg=true;
					}

					if (!READC_TS2("Nick",&dbv)) {
						_stprintf(szItem,_T("%s (%d)"),dbv.ptszVal,READC_D2("ExternalID"));
						DBFreeVariant(&dbv);
						SetWindowText(m_hWnd,szItem);
					}

					for (map<UINT,ipcmember_t>::iterator iter=friends.begin(); iter!=friends.end(); iter++) {
						// Byte 0: offline=5, normal=4, investor=3, admins=2, creator=1
						*szItem=_T('0');
						if (!(iter->second.flag & IPCMFLAG_ONLINE))
							*szItem+=5;

						if (iter->second.flag & IPCMFLAG_INVESTOR) 
							*szItem+=3;
						else if (iter->second.flag & IPCMFLAG_MODERATOR)
							*szItem+=2;
						else if (iter->second.flag & IPCMFLAG_CREATOR)
							*szItem+=1;
						else
							*szItem+=4;

#if 0
						*flag=iter->getCommonFlag();
						flag[1]=iter->getExtFlag();
						/*flag[2]=iter->getQunAdminValue();
						flag[3]=iter->getQunGroupIndex();*/
#endif
						*headimg=iter->second.face;
						itoa(iter->first,szKey,10);
						/*
						if (READC_S2(szKey,&dbv)) {
							// No Nick found
							_stprintf(szItem+1,_T("%d (%d)"),iter->first,iter->first);
						} else {
							// Nick found
							WCHAR szNick[MAX_PATH];
							MultiByteToWideChar(936,0,dbv.pszVal,-1,szItem+1,MAX_PATH-1);
							_stprintf(szItem+wcslen(szItem+1)+1,_T(" (%d)"),iter->first);
							DBFreeVariant(&dbv);
						}
						*/
						if (iter->second.name.size()) {
							// Nick found
							MultiByteToWideChar(936,0,iter->second.name.c_str(),-1,szItem+1,MAX_PATH-1);
							_stprintf(szItem+wcslen(szItem+1)+1,_T(" (%d)"),iter->first);
							DBFreeVariant(&dbv);
						} else {
							// No Nick found
							_stprintf(szItem+1,_T("%d (%d)"),iter->first,iter->first);
						}
						//_itot(iter->getQQ(),szItem,10);
						id=(int)SendDlgItemMessage(m_hWnd,IDC_QUNMEMBERS,LB_ADDSTRING,NULL,(LPARAM)szItem);
						SendDlgItemMessage(m_hWnd,IDC_QUNMEMBERS,LB_SETITEMDATA,id,(LPARAM)flags);

						hBMPHead=m_headlist[iter->first];
						if (!hBMPHead) {
							bool ret=true;
							if (HANDLE hContact2=util_find_contact(szMIMQQ, iter->first)) ret=DBGetContactSetting(hContact2,szMIMQQ,"UserHeadMD5",&dbv);
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
									int id=*headimg;
									if (!id)
										id=1;
									else
										id=id/3+1;

									/*
									hBMPHead=LoadBitmap(hHeadImg,MAKEINTRESOURCE(100+id));
									if (!hBMPHead) hBMPHead=LoadBitmap(hHeadImg,MAKEINTRESOURCE(101));
									*/
									hBMPHead=(HBITMAP)LoadImageA(hHeadImg,MAKEINTRESOURCEA(100+id),IMAGE_BITMAP,32,32,LR_SHARED);
									if (!hBMPHead) hBMPHead=(HBITMAP)LoadImageA(hHeadImg,MAKEINTRESOURCEA(101),IMAGE_BITMAP,32,32,LR_SHARED);
									m_headlist[*headimg]=hBMPHead;
								}
							} else
								m_headlist[iter->first]=hBMPHead;
						}
					} 

					m_updating=false;
					//m_flush=0;
					InvalidateRect(m_hWnd,NULL,TRUE);
					InvalidateRect(GetDlgItem(m_hWnd,IDC_QUNMEMBERS),NULL,FALSE);

					/*
					if (!m_timerEnabled) {
					DialogProc(m_hwnd,WM_TIMER,1,0);
					}
					*/
				} else if (READC_D2("QunVersion")==0) {
					SendDlgItemMessage(m_hWnd,IDC_QUNMEMBERS,LB_ADDSTRING,NULL,(LPARAM)TranslateT("0Please Wait..."));
					//qqNsThread->sendPacket(new QunGetInfoPacket(m_qunid));
					//CallService(szIPCService,QQIPCSVC_QUN_UPDATE_INFORMATION,m_qunid);
					CallContactService(hContact,IPCSVC,QQIPCSVC_QUN_UPDATE_INFORMATION,m_qunid);
				} else
					SendDlgItemMessage(m_hWnd,IDC_QUNMEMBERS,LB_ADDSTRING,NULL,(LPARAM)TranslateT("0Please Wait..."));

				ShowWindow(m_hWnd,SW_SHOWNA);
			} else {
				ipcmember_t ipcm={0};
				quns[m_qunid][1]=ipcm;
				//CallService(szIPCService,QQIPCSVC_QUN_UPDATE_INFORMATION,m_qunid);
				CallContactService(hContact,IPCSVC,QQIPCSVC_QUN_UPDATE_INFORMATION,m_qunid);
			}
		}
	}
}

void CSRMMQunList::NamesUpdated(ipcmembers_t* ipcms) {
	if (IsWindow(m_hWnd) && IsWindow(m_hwndSRMM)) {
		if (quns[ipcms->qunid].size()>0 && quns[ipcms->qunid].begin()->first==1) quns[ipcms->qunid].erase(1);
		for (list<ipcmember_t>::iterator iter=ipcms->members.begin(); iter!=ipcms->members.end(); iter++) {
			quns[ipcms->qunid][iter->qqid]=*iter;
		}

		//CallService(szIPCService,QQIPCSVC_QUN_UPDATE_ONLINE_MEMBERS,ipcms->qunid);
		CallContactService(hContact,IPCSVC,QQIPCSVC_QUN_UPDATE_ONLINE_MEMBERS,ipcms->qunid);
		Refresh();
	}
}

void CSRMMQunList::OnlineMembersUpdated(ipconlinemembers_t* ipcms) {
	if (IsWindow(m_hWnd) && IsWindow(m_hwndSRMM)) {
		map<UINT,ipcmember_t> oldmembers=quns[ipcms->qunid];
		map<UINT,ipcmember_t> newmembers=oldmembers;
		int currentstatus;

		if (oldmembers.size()==0) return;

		onlinemembers[ipcms->qunid]=(UCHAR)ipcms->members.size();

		// For online members
		for (list<UINT>::iterator iter=ipcms->members.begin(); iter!=ipcms->members.end(); iter++) {
			currentstatus=oldmembers[*iter].flag;
			if (currentstatus&IPCMFLAG_EXISTS) {
				if (!(currentstatus&IPCMFLAG_ONLINE)) {
					newmembers[*iter].flag|=IPCMFLAG_ONLINE;
				}
				oldmembers[*iter].flag=0;
			}
		}

		// For offline members
		for (map<UINT,ipcmember_t>::iterator iter=oldmembers.begin(); iter!=oldmembers.end(); iter++) {
			if ((iter->second.flag&IPCMFLAG_EXISTS)&&(iter->second.flag&IPCMFLAG_ONLINE)) {
				newmembers[iter->first].flag-=IPCMFLAG_ONLINE;
			}
		}

		quns[ipcms->qunid]=newmembers;

		Refresh();
	}
}
