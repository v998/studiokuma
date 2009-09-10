#include "StdAfx.h"

extern const char* PASSWORDMASK;
#define ADD_LIST_ITEM(s) SendMessageA(hControl,CB_ADDSTRING,0,(LPARAM)s)

class CAccMgrUI {
public:
	CAccMgrUI(CNetwork* network, HWND hWndAccMgr) {
		//if (m_hWnd) DebugBreak();
		m_network=network;
		m_hWnd=CreateDialogParam(hinstance,MAKEINTRESOURCE(IDD_ACCMGRUI),hWndAccMgr,DialogProc,(LPARAM)this);
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

	CNetwork* GetNetwork() {
		return m_network;
	}
private:
	HWND m_hWnd;
	CNetwork* m_network;

	static INT_PTR CALLBACK DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
		switch (uMsg) {
			case WM_INITDIALOG:
				{
					CAccMgrUI* amui=(CAccMgrUI*)lParam;
					LPSTR m_szModuleName=amui->GetNetwork()->m_szModuleName;
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

					// Add server to list
					HWND hControl=GetDlgItem(hwndDlg,IDC_SERVER);
					LPSTR* pszServers=servers;

					while (*pszServers) {
						ADD_LIST_ITEM(*pszServers);
						pszServers++;
					}

					if(!READC_S2(QQ_LOGINSERVER2,&dbv)) {
						SetDlgItemTextA(hwndDlg,IDC_SERVER,dbv.pszVal);
						DBFreeVariant(&dbv);
					} else {
						SetDlgItemTextA(hwndDlg,IDC_SERVER,*servers);
					}

					CheckDlgButton(hwndDlg, IDC_FORCEINVISIBLE, READC_B2(QQ_INVISIBLE));
				}
				break;
			case WM_DESTROY:
				util_log(0,"Destroy CAccMgrUI");
				delete (CAccMgrUI*)GetWindowLong(hwndDlg,GWL_USERDATA);
				break;
			case WM_COMMAND:	// When a control is toggled
				switch (LOWORD(wParam)) {
					case IDC_LOGIN:
					case IDC_PASSWORD:
						//case IDC_SERVER:
					case IDC_VERSION:
						if (HIWORD(wParam) != EN_CHANGE || (HWND)lParam != GetFocus()) return 0;
				}

				SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
				break;

			case WM_NOTIFY:		// When a notify is sent by Options Dialog (Property Sheet)
				if (((LPNMHDR)lParam)->code==PSN_APPLY) {
					CAccMgrUI* amui=(CAccMgrUI*)GetWindowLong(hwndDlg,GWL_USERDATA);
					LPSTR m_szModuleName=amui->GetNetwork()->m_szModuleName;
					CHAR szTemp[MAX_PATH];
					HANDLE hContact=NULL;

					GetDlgItemTextA(hwndDlg,IDC_LOGIN,szTemp,MAX_PATH);
					WRITEC_D(UNIQUEIDSETTING,atoi(szTemp));

					GetDlgItemTextA(hwndDlg,IDC_PASSWORD,szTemp,MAX_PATH);
					if (*szTemp && !strchr(szTemp,0x1)) {
						CallService(MS_DB_CRYPT_ENCODESTRING,sizeof(szTemp),(LPARAM)szTemp);
						WRITEC_S(QQ_PASSWORD,szTemp);
					}

					GetDlgItemTextA(hwndDlg,IDC_SERVER,szTemp,sizeof(szTemp));
					WRITEC_S(QQ_LOGINSERVER2,szTemp);

					WRITEC_B(QQ_INVISIBLE,(BYTE)IsDlgButtonChecked(hwndDlg,IDC_FORCEINVISIBLE));
				}
				break;
		}
		return FALSE;
	}

};

// MIM 0.8 #11+
// lParam=hWndAccMgr
int __cdecl CNetwork::CreateAccMgrUI(WPARAM, LPARAM lParam) {
	CAccMgrUI* amui= new CAccMgrUI(this,(HWND)lParam);
	amui->ShowDialog();

	return (int)amui->GetHWnd();
}
