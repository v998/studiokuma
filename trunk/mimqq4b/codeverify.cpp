#include "stdafx.h"

CCodeVerifyWindow::CCodeVerifyWindow(HINSTANCE hInstance, LPWSTR pszText, LPSTR pszImagePath):
m_text(pszText), m_imagepath(pszImagePath), m_bitmap(NULL) {
	DialogBoxParam(hInstance,MAKEINTRESOURCE(IDD_CODEVERIFY),NULL,_DialogProc,(LPARAM)this);
}

CCodeVerifyWindow::CCodeVerifyWindow(HINSTANCE hInstance, LPWSTR pszText, LPSTR pszURL, CLibWebQQ* webqq):
m_text(pszText), m_imagepath(pszURL), m_bitmap(NULL), m_webqq(webqq) {
	DialogBoxParam(hInstance,MAKEINTRESOURCE(IDD_CODEVERIFY),NULL,_DialogProc,(LPARAM)this);
}

INT_PTR CCodeVerifyWindow::_DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	if (uMsg==WM_INITDIALOG) {
		SetWindowLong(hwndDlg,GWL_USERDATA,(LONG)lParam);
	}
	if (uMsg==WM_DESTROY)
		return FALSE;
	else
		return ((CCodeVerifyWindow*)GetWindowLong(hwndDlg,GWL_USERDATA))->DialogProc(hwndDlg, uMsg, wParam, lParam);
}

INT_PTR CCodeVerifyWindow::DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
		case WM_INITDIALOG: 
			{
				/*
				m_bitmap=(HBITMAP)CallService(MS_UTILS_LOADBITMAP,0,(LPARAM)m_imagepath);
				SendDlgItemMessage(hwndDlg,IDC_CODEIMAGE,STM_SETIMAGE,(WPARAM)IMAGE_BITMAP,(LPARAM)m_bitmap);
				*/
				if (m_text) SetDlgItemText(hwndDlg,IDC_TITLE,m_text);
				*m_code=0;
				PostMessage(hwndDlg,WM_COMMAND,MAKEWPARAM(IDC_CODEIMAGE,0),0);
			}
			break;
		case WM_DESTROY:
			DeleteObject(m_bitmap);
			break;
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDC_CODEIMAGE:
					{
						LPCSTR pszBasePath=m_webqq->GetBasePath();
						char szPath[MAX_PATH]={0};
						LPSTR pszData;
						DWORD dwSize;

						if (pszBasePath) {
							strcat(strcpy(szPath,pszBasePath),"\\");
						}
						sprintf(szPath+strlen(szPath),"verycode-%u.jpg",m_webqq->GetQQID());

						if (!(pszData=m_webqq->GetHTMLDocument(m_imagepath,m_webqq->GetReferer(CLibWebQQ::WEBQQ_REFERER_PTLOGIN),&dwSize))) {
							MessageBox(hwndDlg,TranslateT("Failed retrieving verification code,"),NULL,MB_ICONERROR);
							return false;
						}

						HANDLE hFile=CreateFileA(szPath,GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,CREATE_ALWAYS,0,NULL);
						if (hFile==INVALID_HANDLE_VALUE) {
							LocalFree(pszData);
							MessageBox(hwndDlg,TranslateT("Failed saving verification code,"),NULL,MB_ICONERROR);
						} else {
							DWORD dwWritten;
							WriteFile(hFile,pszData,dwSize,&dwWritten,NULL);
							CloseHandle(hFile);
							LocalFree(pszData);

							m_bitmap=(HBITMAP)CallService(MS_UTILS_LOADBITMAP,0,(LPARAM)szPath);
							if (HBITMAP hBmpOld=(HBITMAP)SendDlgItemMessage(hwndDlg,IDC_CODEIMAGE,BM_SETIMAGE,(WPARAM)IMAGE_BITMAP,(LPARAM)m_bitmap)) {
								DeleteObject(hBmpOld);
							}
						}
					}
					break;
				case IDOK:
					GetDlgItemTextA(hwndDlg,IDC_CODE,m_code,16);
				case IDCANCEL:
					EndDialog(hwndDlg,LOWORD(wParam));
				}
			break;
	}
	return FALSE;
}
