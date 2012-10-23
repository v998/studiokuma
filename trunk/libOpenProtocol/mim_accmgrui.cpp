#include "StdAfx.h"
#include "protocol.h"

const char* PASSWORDMASK="\x1\x1\x1\x1\x1\x1\x1\x1";	// Masked Default Password

extern HINSTANCE g_hInst;

static INT_PTR CALLBACK DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
		case WM_INITDIALOG:
			{
				PROTO_INTERFACE* amui=(PROTO_INTERFACE*)lParam;
				LPSTR m_szModuleName=amui->m_szModuleName;
				CHAR szTemp[MAX_PATH];
				DBVARIANT dbv;
				HANDLE hContact=NULL;

				TranslateDialogDefault(hwndDlg);

				SetWindowLong(hwndDlg,GWL_USERDATA,lParam);

				if (!READC_S2(KEY_BOOTSTRAP,&dbv)) {
					SetDlgItemTextA(hwndDlg,IDC_LUAFILE,dbv.pszVal);
					DBFreeVariant(&dbv);
				} else 
					SetDlgItemTextA(hwndDlg,IDC_LUAFILE,VAL_BOOTSTRAP);

				if (!READC_S2(KEY_UIN,&dbv)) {
					SetDlgItemTextA(hwndDlg,IDC_UIN,dbv.pszVal);
					DBFreeVariant(&dbv);
				}

				SetDlgItemTextA(hwndDlg,IDC_PW,PASSWORDMASK);

				if (!READC_S2(KEY_RECEIVEGCIMAGES,&dbv)) {
					CheckDlgButton(hwndDlg, IDC_GCIMAGES, dbv.bVal==0?BST_UNCHECKED:BST_CHECKED);
					DBFreeVariant(&dbv);
				} else
					CheckDlgButton(hwndDlg, IDC_GCIMAGES, BST_CHECKED);

				WORD wValue;
				if ((wValue=READC_W2(KEY_SERVERPORT))!=0) {
					DBFreeVariant(&dbv);
				} else
					wValue=VAL_SERVERPORT;

				itoa(wValue,szTemp,10);
				SetDlgItemTextA(hwndDlg,IDC_PORT,szTemp);

				CheckDlgButton(hwndDlg, IDC_SHOWCONSOLE, READC_B2(KEY_SHOWCONSOLE)==0?BST_UNCHECKED:BST_CHECKED);
			}
			break;
		case WM_COMMAND:	// When a control is toggled
			switch (LOWORD(wParam)) {
				case IDC_LUAFILE:
				case IDC_UIN:
				case IDC_PW:
					if (HIWORD(wParam) != EN_CHANGE || (HWND)lParam != GetFocus()) return 0;
			}

			SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
			break;

		case WM_NOTIFY:		// When a notify is sent by Options Dialog (Property Sheet)
			if (((LPNMHDR)lParam)->code==PSN_APPLY) {
				PROTO_INTERFACE* amui=(PROTO_INTERFACE*)GetWindowLong(hwndDlg,GWL_USERDATA);
				LPSTR m_szModuleName=amui->m_szModuleName;
				CHAR szTemp[MAX_PATH];
				WCHAR wszTemp[MAX_PATH];
				HANDLE hContact=NULL;
				DBVARIANT dbv;
				WORD wValue;

				GetDlgItemTextA(hwndDlg,IDC_LUAFILE,szTemp,MAX_PATH);

				if (!*szTemp) {
					MessageBox(hwndDlg,TranslateT("Bootstrap script not provided!"),NULL,MB_ICONERROR);
					// return PSNRET_INVALID;
				} else if (_access(szTemp,0)!=0) {
					MessageBox(hwndDlg,TranslateT("Bootstrap script inaccessible!"),NULL,MB_ICONERROR);
					// return PSNRET_INVALID;
				} else
					WRITEC_S(KEY_UIN,szTemp);

				GetDlgItemTextA(hwndDlg,IDC_UIN,szTemp,MAX_PATH);

				if (!*szTemp) {
					MessageBox(hwndDlg,TranslateT("UIN not provided!"),NULL,MB_ICONERROR);
					// return PSNRET_INVALID;
				} else
					WRITEC_S(KEY_UIN,szTemp);

				GetDlgItemTextA(hwndDlg,IDC_PW,szTemp,MAX_PATH);

				if (!*szTemp || strchr(szTemp,1)) {
					if (!READC_S2(KEY_PW,&dbv)) {
						DBFreeVariant(&dbv);
					} else {
						MessageBox(hwndDlg,TranslateT("Password not provided!"),NULL,MB_ICONERROR);
						// return PSNRET_INVALID;
					}
				} else {
					CallService(MS_DB_CRYPT_ENCODESTRING,sizeof(szTemp),(LPARAM)szTemp);
					WRITEC_S(KEY_PW,szTemp);
				}
				
				WRITEC_B(KEY_RECEIVEGCIMAGES, IsDlgButtonChecked(hwndDlg,IDC_GCIMAGES)?1:0);
				WRITEC_B(KEY_SHOWCONSOLE, IsDlgButtonChecked(hwndDlg,IDC_SHOWCONSOLE)?1:0);

				GetDlgItemTextA(hwndDlg,IDC_PORT,szTemp,MAX_PATH);
				wValue=atoi(szTemp);

				if (wValue==0) wValue=VAL_SERVERPORT;

				WRITEC_W(KEY_SERVERPORT, wValue);

				return PSNRET_NOERROR;
			}
			break;
	}
	return FALSE;
}

HWND CreateAccountManagerUI(PROTO_INTERFACE* protocol, HWND hWndAccMgr) {
	return CreateDialogParam(g_hInst,MAKEINTRESOURCE(IDD_ACCMGRUI),hWndAccMgr,DialogProc,(LPARAM)protocol);
}
