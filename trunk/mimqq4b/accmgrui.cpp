#include "StdAfx.h"

const char* PASSWORDMASK="\x1\x1\x1\x1\x1\x1\x1\x1";	// Masked Default Password

class CAccMgrUI {
public:
	CAccMgrUI(CProtocol* protocol, HWND hWndAccMgr) {
		m_protocol=protocol;
		m_hWnd=CreateDialogParam(CProtocol::g_hInstance,MAKEINTRESOURCE(IDD_ACCMGRUI),hWndAccMgr,DialogProc,(LPARAM)this);
	};

	~CAccMgrUI() {
		// Warning: This function call only be called in DialogProc
		EndDialog(m_hWnd,0);
		m_hWnd=NULL;
	}

	HWND GetHWnd() {
		return m_hWnd;
	}

	void ShowDialog() {
		ShowWindow(m_hWnd,SW_SHOWNORMAL);
	}

	CProtocol* GetProtocol() {
		return m_protocol;
	}
private:
	HWND m_hWnd;
	CProtocol* m_protocol;

	static INT_PTR CALLBACK DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
		switch (uMsg) {
			case WM_INITDIALOG:
				{
					CAccMgrUI* amui=(CAccMgrUI*)lParam;
					LPSTR m_szModuleName=amui->GetProtocol()->m_szModuleName;
					CHAR szTemp[MAX_PATH];
					DBVARIANT dbv;
					HANDLE hContact=NULL;

					TranslateDialogDefault(hwndDlg);

					SetWindowLong(hwndDlg,GWL_USERDATA,lParam);
					if (!READC_S2(UNIQUEIDSETTING,&dbv)) {
						itoa(dbv.dVal,szTemp,10);
						SetDlgItemTextA(hwndDlg,IDC_LOGIN,szTemp);
						DBFreeVariant(&dbv);
					}
					SetDlgItemTextA(hwndDlg,IDC_PASSWORD,PASSWORDMASK);

					CheckDlgButton(hwndDlg, IDC_FORCEINVISIBLE, READC_B2(QQ_LOGIN_INVISIBLE));

					HWND hWndControl=GetDlgItem(hwndDlg,IDC_PROTOCOL);
					SendMessage(hWndControl,CB_ADDSTRING,0,(LPARAM)TranslateT("Web2 (mimqq4b)"));
					SendMessage(hWndControl,CB_ADDSTRING,0,(LPARAM)TranslateT("WebQQ (mimqq4a)"));
					SendMessage(hWndControl,CB_SETCURSEL,0/*READC_B2("Protocol")*/,0);
					EnableWindow(hWndControl,FALSE);

					hWndControl=GetDlgItem(hwndDlg,IDC_USERHEAD);
					SendMessage(hWndControl,CB_ADDSTRING,0,(LPARAM)TranslateT("Only download for online contacts and quns"));
					SendMessage(hWndControl,CB_ADDSTRING,0,(LPARAM)TranslateT("Always download all"));
					SendMessage(hWndControl,CB_ADDSTRING,0,(LPARAM)TranslateT("Download on demand"));
					SendMessage(hWndControl,CB_SETCURSEL,READC_B2("UHDownload"),0);

					CheckRadioButton(hwndDlg,IDC_NOSIGNATURE,IDC_WITHSIGNATURE,READC_B2("EnableCustomSignature")?IDC_WITHSIGNATURE:IDC_NOSIGNATURE);
					if (!READC_TS2("CustomSignature",&dbv) && *dbv.pwszVal) {
						SetDlgItemTextW(hwndDlg,IDC_SIGNATURE,dbv.pwszVal);
						DBFreeVariant(&dbv);
					}
				}
				break;
			case WM_DESTROY:
				delete (CAccMgrUI*)GetWindowLong(hwndDlg,GWL_USERDATA);
				break;
			case WM_COMMAND:	// When a control is toggled
				switch (LOWORD(wParam)) {
					case IDC_LOGIN:
					case IDC_PASSWORD:
						if (HIWORD(wParam) != EN_CHANGE || (HWND)lParam != GetFocus()) return 0;
				}

				SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
				break;

			case WM_NOTIFY:		// When a notify is sent by Options Dialog (Property Sheet)
				if (((LPNMHDR)lParam)->code==PSN_APPLY) {
					CAccMgrUI* amui=(CAccMgrUI*)GetWindowLong(hwndDlg,GWL_USERDATA);
					LPSTR m_szModuleName=amui->GetProtocol()->m_szModuleName;
					CHAR szTemp[MAX_PATH];
					WCHAR wszTemp[MAX_PATH];
					HANDLE hContact=NULL;

					GetDlgItemTextA(hwndDlg,IDC_LOGIN,szTemp,MAX_PATH);
					WRITEC_D(UNIQUEIDSETTING,atoi(szTemp));

					GetDlgItemTextA(hwndDlg,IDC_PASSWORD,szTemp,MAX_PATH);
					if (*szTemp && !strchr(szTemp,0x1)) {
						CallService(MS_DB_CRYPT_ENCODESTRING,sizeof(szTemp),(LPARAM)szTemp);
						WRITEC_S(QQ_LOGIN_PASSWORD,szTemp);
					}

					GetDlgItemTextA(hwndDlg,IDC_SERVER,szTemp,sizeof(szTemp));

					WRITEC_B(QQ_LOGIN_INVISIBLE,(BYTE)IsDlgButtonChecked(hwndDlg,IDC_FORCEINVISIBLE));

					WRITEC_B("Protocol",SendDlgItemMessage(hwndDlg,IDC_PROTOCOL,CB_GETCURSEL,0,0));
					WRITEC_B("UHDownload",SendDlgItemMessage(hwndDlg,IDC_USERHEAD,CB_GETCURSEL,0,0));
					WRITEC_B("EnableCustomSignature",IsDlgButtonChecked(hwndDlg,IDC_WITHSIGNATURE));

					GetDlgItemTextW(hwndDlg,IDC_SIGNATURE,wszTemp,MAX_PATH);
					/*
					LPSTR pszTemp=mir_utf8encodeW(wszTemp);
					WRITEC_U8S("CustomSignature",pszTemp);
					mir_free(pszTemp);
					*/
					WRITEC_TS("CustomSignature",wszTemp);
				}
				break;
		}
		return FALSE;
	}

};

// MIM 0.8 #11+
// lParam=hWndAccMgr
int CProtocol::CreateAccMgrUI(WPARAM wParam, LPARAM lParam) {
	CAccMgrUI* amui= new CAccMgrUI(this,(HWND)lParam);
	amui->ShowDialog();

	return (int)amui->GetHWnd();
}
