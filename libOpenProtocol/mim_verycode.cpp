#include "StdAfx.h"
#include "protocol.h"

extern HINSTANCE g_hInst;

static INT_PTR CALLBACK DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
		case WM_INITDIALOG:
			{
				LPSTR pszPath=(LPSTR)lParam;

				TranslateDialogDefault(hwndDlg);
				SetWindowLong(hwndDlg,GWL_USERDATA,lParam);

				// HANDLE hBitmap=LoadImageA(NULL,pszPath,IMAGE_BITMAP,0,0,LR_LOADFROMFILE);
				HANDLE hBitmap=(HANDLE)CallService(MS_UTILS_LOADBITMAP,0,(LPARAM)pszPath);
				SendDlgItemMessage(hwndDlg,IDC_IMAGE,STM_SETIMAGE,IMAGE_BITMAP,(LPARAM)hBitmap);
			}
			break;
		case WM_COMMAND:	// When a control is toggled
			switch (LOWORD(wParam)) {
				case IDCANCEL:
					{
						LPSTR pszPath=(LPSTR)GetWindowLong(hwndDlg,GWL_USERDATA);
						*pszPath=0;
						EndDialog(hwndDlg,0);
					}
					break;
				case IDC_RELOAD:
					{
						LPSTR pszPath=(LPSTR)GetWindowLong(hwndDlg,GWL_USERDATA);
						strcpy(pszPath," ");
						EndDialog(hwndDlg,0);
					}
					break;
				case IDOK:
					{
						LPSTR pszPath=(LPSTR)GetWindowLong(hwndDlg,GWL_USERDATA);
						GetDlgItemTextA(hwndDlg,IDC_VERYCODE,pszPath,16);
						if (strlen(pszPath)>16 || strlen(pszPath)<4) {
							MessageBox(hwndDlg,TranslateT("Invalid verification code length, please try again"),NULL,MB_ICONERROR);
						} else
							EndDialog(hwndDlg,0);
					}
			}
			break;
	}
	return FALSE;
}

void ShowVeryCode(LPSTR pszPath) {
	DialogBoxParam(g_hInst,MAKEINTRESOURCE(IDD_VERYCODE),NULL,DialogProc,(LPARAM)pszPath);
}
