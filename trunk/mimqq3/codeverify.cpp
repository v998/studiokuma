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
#include "stdafx.h"
/*
HWND CodeVerifyWindow::m_hWnd;
CodeVerifyWindow* CodeVerifyWindow::m_inst;
XGraphicVerifyCode* CodeVerifyWindow::m_code;
ASKDLGPARAMS* CodeVerifyWindow::m_adp;
HBITMAP CodeVerifyWindow::m_bitmap;
char* CodeVerifyWindow::m_codefile;
char* CodeVerifyWindow::m_sessionid;
*/
map<int,HWND> CodeVerifyWindow::m_windows;

CodeVerifyWindow::CodeVerifyWindow(XGraphicVerifyCode* code) {
	//m_inst=this;
	m_adp=NULL;
	m_sessionid=NULL;
	m_code=code;
	m_hWnd=NULL;
	DialogBoxParam(hinstance,MAKEINTRESOURCE(IDD_CODEVERIFY),NULL,DialogProc,(LPARAM)this);
	//delete m_code;
}

CodeVerifyWindow::CodeVerifyWindow(ASKDLGPARAMS* adp) {
	//m_inst=this;
	m_adp=adp;
	m_code=NULL;
	m_sessionid=NULL;
	m_hWnd=NULL;
	DialogBoxParam(hinstance,MAKEINTRESOURCE(IDD_CODEVERIFY),NULL,DialogProc,(LPARAM)this);
	//delete m_code;
}

INT_PTR CodeVerifyWindow::DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
#if 0 // TODO
	switch (uMsg) {
		case WM_INITDIALOG: 
			{
				char szCodeFile[MAX_PATH];
				CodeVerifyWindow* cvw=(CodeVerifyWindow*)lParam;
				CNetwork* network=cvw->m_adp?cvw->m_adp->network:cvw->m_code->m_network;
				SetWindowLong(hwndDlg,GWL_USERDATA,lParam);

				CallService(MS_DB_GETPROFILEPATH,MAX_PATH,(LPARAM)szCodeFile);
				if (cvw->m_code) {
					strcat(szCodeFile,"\\QQ\\CV-");
					strcat(szCodeFile,network->m_szModuleName);
					strcat(szCodeFile,".png");
					cvw->m_codefile=mir_strdup(szCodeFile);
					
					FILE* fp=fopen(szCodeFile,"wb");
					fwrite(cvw->m_code->m_Data,cvw->m_code->m_DataLen,1,fp);
					fclose(fp);

				} else {
					NETLIBHTTPREQUEST nlhr={sizeof(nlhr),REQUEST_GET,NLHRF_GENERATEHOST};

					nlhr.szUrl=(LPSTR)cvw->m_adp->pAux;

					if (NETLIBHTTPREQUEST* nlhrr=(NETLIBHTTPREQUEST*)CallService(MS_NETLIB_HTTPTRANSACTION,(WPARAM)hNetlibUser,(LPARAM)&nlhr)) {
						for (int c=0; c<nlhrr->headersCount; c++) {
							if (!strcmp(nlhrr->headers[c].szName,"getqqsession")) {
								LPSTR pszSession=nlhrr->headers[c].szValue;
								strcat(szCodeFile,"\\QQ\\CV-qun.jpg");
								cvw->m_codefile=mir_strdup(szCodeFile);
								HANDLE hFile=CreateFileA(cvw->m_codefile,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,0,NULL);
								if (hFile==INVALID_HANDLE_VALUE) {
									CallService(MS_NETLIB_FREEHTTPREQUESTSTRUCT,(WPARAM)nlhrr,0);
									MessageBox(NULL,TranslateT("Unable to retrieve verification image. please try again."),NULL,MB_ICONERROR);
									network->m_addUID=0;
									EndDialog(hwndDlg,1);
									return FALSE;
								} else {
									DWORD dwWritten;
									WriteFile(hFile,nlhrr->pData,nlhrr->dataLength,&dwWritten,NULL);
									CloseHandle(hFile);

									cvw->m_sessionid=mir_strdup(pszSession);
								}
								break;
							}
						}
						CallService(MS_NETLIB_FREEHTTPREQUESTSTRUCT,(WPARAM)nlhrr,0);
					}
				}
				if (cvw->m_codefile) {
					cvw->m_bitmap=(HBITMAP)CallService(MS_UTILS_LOADBITMAP,0,(LPARAM)cvw->m_codefile);
					cvw->m_hWnd=hwndDlg;
					SendDlgItemMessage(hwndDlg,IDC_CODEIMAGE,STM_SETIMAGE,(WPARAM)IMAGE_BITMAP,(LPARAM)cvw->m_bitmap);
				} else {
					MessageBox(NULL,TranslateT("verycode may have changed. Please contact developer."),NULL,MB_ICONERROR);
					network->m_addUID=0;
					EndDialog(hwndDlg,1);
					return FALSE;
				}
			}
			break;
		case WM_DESTROY:
			{
				CodeVerifyWindow* cvw=(CodeVerifyWindow*)GetWindowLong(hwndDlg,GWL_USERDATA);
				//m_hWnd=NULL;
				DeleteObject(cvw->m_bitmap);
				DeleteFileA(cvw->m_codefile);
				mir_free(cvw->m_codefile);
				if (cvw->m_sessionid) mir_free(cvw->m_sessionid);
			}
			break;
		case WM_COMMAND:
			{
				CodeVerifyWindow* cvw=(CodeVerifyWindow*)GetWindowLong(hwndDlg,GWL_USERDATA);
				CNetwork* network=cvw->m_adp?cvw->m_adp->network:cvw->m_code->m_network;

				switch (LOWORD(wParam)) {
				case IDOK:
					{
						char szCode[MAX_PATH];
						GetDlgItemTextA(hwndDlg,IDC_CODE,szCode,MAX_PATH);

						if (cvw->m_code) {

							RequestLoginTokenExPacket *packet = new RequestLoginTokenExPacket(QQ_LOGIN_TOKEN_VERIFY);
							packet->setToken(cvw->m_code->m_SessionToken,cvw->m_code->m_SessionTokenLen);
							packet->setCode(szCode);
							cvw->m_code->setCode(szCode);

							network->m_graphicVerifyCode=cvw->m_code;
							network->append(packet);
							m_windows[packet->getSequence()]=hwndDlg;
							EndDialog(hwndDlg,0);
						} else {
							EvaAddFriendGetAuthInfoPacket *packet;
							if (cvw->m_adp->command==AUTH_INFO_SUB_CMD_QUN) {
								HANDLE hContact=network->FindContact(network->m_addUID);
								packet = new EvaAddFriendGetAuthInfoPacket(DBGetContactSettingDword(hContact,network->m_szModuleName,"ExternalID",0), AUTH_INFO_CMD_CODE, true);
							} else {
								packet = new EvaAddFriendGetAuthInfoPacket();
								packet->setSubCommand(AUTH_INFO_CMD_CODE);
								packet->setSubSubCommand(cvw->m_adp->command);
								packet->setAddID(DBGetContactSettingDword(cvw->m_adp->hContact,network->m_szModuleName,UNIQUEIDSETTING,0)-0x80000000);
							}
							packet->setVerificationStr(szCode);
							packet->setSessionStr(cvw->m_sessionid);
							network->append(packet);
							m_windows[packet->getSequence()]=hwndDlg;
							EnableWindow(GetDlgItem(hwndDlg,IDOK),FALSE);
							//EnableWindow(GetDlgItem(hwndDlg,IDCANCEL),FALSE);
						}
					}
					break;
				case IDCANCEL:
					{
						if (cvw->m_code) 
							network->GoOffline();
						else switch (cvw->m_adp->command) {
							case AUTH_INFO_SUB_CMD_QUN:
							case AUTH_INFO_SUB_CMD_USER:
								network->m_addUID=0;
								break;
							case AUTH_INFO_SUB_CMD_TEMP_SESSION:
								delete network->m_savedTempSessionMsg;
								network->m_savedTempSessionMsg=NULL;
								break;
						}
						EndDialog(hwndDlg,1);
					}
					break;
				}
			}
			break;
		case WM_USER+1:
			MessageBox(hwndDlg,TranslateT("Incorrect verify code, try again please!"),NULL,MB_ICONERROR);
			SetFocus(GetDlgItem(hwndDlg,IDC_CODE));
			EnableWindow(GetDlgItem(hwndDlg,IDOK),TRUE);
			EnableWindow(GetDlgItem(hwndDlg,IDCANCEL),TRUE);
			break;
		case WM_USER+2: // WPARAM: Length, LPARAM: Code
			{
				CodeVerifyWindow* cvw=(CodeVerifyWindow*)GetWindowLong(hwndDlg,GWL_USERDATA);
				CNetwork* network=cvw->m_adp?cvw->m_adp->network:cvw->m_code->m_network;

				switch (cvw->m_adp->command) {
					case AUTH_INFO_SUB_CMD_USER:
						/*
						int qqid=DBGetContactSettingDword(cvw->m_adp->hContact,network->m_szModuleName,UNIQUEIDSETTING,0);
						HANDLE hContact=cvw->m_adp->hContact;
						DBVARIANT dbv;
						DBGetContactSettingTString(hContact,network->m_szModuleName,"AuthReason",&dbv);

						AddFriendAuthPacket* packet=new AddFriendAuthPacket(qqid,QQ_MY_AUTH_REQUEST);
						LPSTR pszTemp=mir_u2a_cp(dbv.ptszVal,936);
						packet->set
						packet->setCode((unsigned char*)lParam,wParam);
						packet->setMessage(pszTemp);
						mir_free(pszTemp);
						network->append(packet);
						DBFreeVariant(&dbv);
						*/
						break;
					case AUTH_INFO_SUB_CMD_QUN:
						{
							int qqid=DBGetContactSettingDword(cvw->m_adp->hContact,network->m_szModuleName,UNIQUEIDSETTING,0);
							HANDLE hContact=cvw->m_adp->hContact;
							DBVARIANT dbv;
							DBGetContactSettingTString(hContact,network->m_szModuleName,"AuthReason",&dbv);

							QunAuthPacket* packet=new QunAuthPacket(qqid,QQ_QUN_AUTH_REQUEST);
							LPSTR pszTemp=mir_u2a_cp(dbv.ptszVal,936);
							packet->setCode((unsigned char*)lParam,wParam);
							packet->setMessage(pszTemp);
							mir_free(pszTemp);
							network->append(packet);
							DBFreeVariant(&dbv);
						}
						break;
					case AUTH_INFO_SUB_CMD_TEMP_SESSION:
						{
							SendTempSessionTextIMPacket* packet=network->m_savedTempSessionMsg;
							packet->setAuthInfo((unsigned char*)lParam,wParam);
							network->append(packet);
							network->m_savedTempSessionMsg=NULL;
						}
						break;

				}
				EndDialog(hwndDlg,0);
			}
			break;
	}
#endif
	return FALSE;
}

HWND CodeVerifyWindow::getHwnd(int sequenceid) {
	return m_windows[sequenceid];
}

XGraphicVerifyCode::XGraphicVerifyCode():
m_SessionTokenLen(0), m_SessionToken(NULL), m_DataLen(0), m_Data(NULL), m_code(NULL)
{
};

XGraphicVerifyCode::XGraphicVerifyCode(const XGraphicVerifyCode &rhs):
m_SessionTokenLen(0), m_SessionToken(NULL), m_DataLen(0), m_Data(NULL), m_code(NULL)
{
	*this = rhs; 
}

XGraphicVerifyCode::~XGraphicVerifyCode() {
	if(m_SessionToken) delete [] m_SessionToken;
	if(m_Data) delete [] m_Data;
	if (m_code) free(m_code);
}

XGraphicVerifyCode &XGraphicVerifyCode::operator=(const XGraphicVerifyCode &rhs) {
	setSessionToken(rhs.m_SessionToken, rhs.m_SessionTokenLen);
	setData(rhs.m_Data, rhs.m_DataLen);
	return *this;
}

void XGraphicVerifyCode::setSessionToken(const unsigned char *token, const unsigned short len) {
	if(m_SessionToken) delete []m_SessionToken;
	m_SessionToken = new unsigned char [len];
	memcpy(m_SessionToken, token, len);
	m_SessionTokenLen = len;
};

void XGraphicVerifyCode::setData(const unsigned char *data, const unsigned short len) {
	if(m_Data) delete []m_Data;
	m_Data = new unsigned char [len];
	memcpy(m_Data, data, len);
	m_DataLen = len;
};

void XGraphicVerifyCode::setCode(const char* code) {
	if (m_code) free(m_code);
	m_code=strdup(code);
}