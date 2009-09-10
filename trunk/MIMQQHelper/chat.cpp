#include "stdafx.h"


map<UINT,map<UINT,ipcmember_t>> quns;
map<UINT,UCHAR> onlinemembers;

HANDLE util_find_contact(const LPSTR szMIMQQ, const unsigned int QQID) {
	HANDLE hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, (WPARAM)NULL, (LPARAM)NULL);
	while (hContact) {
		if (!lstrcmpA(szMIMQQ, (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact,(LPARAM)NULL)) && 
			DBGetContactSettingDword(hContact,szMIMQQ,"UID",0)==QQID)
			return hContact;

		hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, (LPARAM)NULL);
	}
	return NULL;
}

#define DM_CONTAINERSELECTED (WM_USER+39)

//////////////////////////////////////////////////////////////////////////
CQunListBase* CQunListBase::m_inst=NULL;
HHOOK CQunListBase::hHookMessagePost=NULL;
HINSTANCE CQunListBase::m_hInstance=NULL;
int CQunListBase::m_qunid=0;
HANDLE CQunListBase::m_timerEvent=NULL;
HWND CQunListBase::m_hwndSRMM=NULL;
HANDLE CQunListBase::hContact=NULL;

CQunListBase::CQunListBase() {
	OutputDebugString(L"CQunListBase::CQunListBase\n");
	m_inst=this;
}

CQunListBase::~CQunListBase() {
	OutputDebugString(L"CQunListBase::~CQunListBase\n");
	m_inst=NULL;
	m_hInstance=NULL;
	m_qunid=0;
}

void CQunListBase::InstallHook(HINSTANCE hInstance) {
	OutputDebugString(L"CQunListBase::InstallHook\n");
	m_hInstance=hInstance;
	hHookMessagePost=SetWindowsHookEx(WH_CALLWNDPROCRET, (HOOKPROC)MessageHookProcPost, NULL, GetCurrentThreadId());
}

void CQunListBase::RemoveHook() {
	OutputDebugString(L"CQunListBase::RemoveHook\n");
	if (hHookMessagePost) UnhookWindowsHookEx(hHookMessagePost);
	hHookMessagePost=NULL;
}

#define DM_SELECTTAB		 (WM_USER+23)

LRESULT CALLBACK CQunListBase::MessageHookProcPost(int code, WPARAM wParam, LPARAM lParam) {
	if (code == HC_ACTION) {
		CWPRETSTRUCT *msg = (CWPRETSTRUCT*)lParam;

		switch(msg->message) 
		{
			case DM_UPDATETITLE:
				{
					TCHAR szClassName[8];

					GetClassName(msg->hwnd, szClassName, 8);
					if (!_tcscmp(szClassName,_T("#32770")) && msg->wParam>0)
						m_inst->TabSwitched(msg);
				}
				break;
			case WM_SIZING:
			case WM_MOVING:
			case WM_EXITSIZEMOVE:
				if (m_inst && msg->hwnd==m_hwndSRMM) {
					//util_log(0,"WM_MOVING");
					if (IsIconic(m_hwndSRMM))
						m_inst->Hide();
					else
						m_inst->Move();
				}
				break;
			case WM_SIZE:
				if (m_inst && msg->hwnd==m_hwndSRMM && wParam==SIZE_MINIMIZED) {
					OutputDebugString(L"CQunListBase: SIZE_MINIMIZED\n");
					m_inst->Hide();
				}
				break;
			case WM_CLOSE:
				{
					if (m_inst && msg->hwnd==m_hwndSRMM) {
						//util_log(0,"WM_CLOSE");
						m_inst->Close();
					}
				}
				break;
			case WM_ACTIVATE:
				//util_log(0,"WM_ACTIVATE, wParam=%d",wParam);
				if (m_inst && msg->hwnd==m_hwndSRMM && GetForegroundWindow()==m_hwndSRMM /*&& (wParam==WA_ACTIVE || wParam==WA_CLICKACTIVE)*/) {
					OutputDebugString(L"CQunListBase: WM_ACTIVATE");
					//qunList->SwitchQun(qunList->GetCurrentQun());
					//SetWindowPos(m_hWnd,m_hwndSRMM,0,0,0,0,SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE);
					m_inst->Move();
				}
				break;
			/*
			default:	
				if (msg->message>=0x400 && msg->message<0x500) {
					char szTemp[MAX_PATH];
					sprintf(szTemp,"TabSRMM message: WM_USER+%d\n",msg->message-0x400);
					OutputDebugStringA(szTemp);
				}
			*/
				
		}
	}
	return CallNextHookEx(hHookMessagePost, code, wParam, lParam);
}

void CQunListBase::DeleteInstance() {
	OutputDebugString(L"CQunListBase::DeleteInstance\n");
	if (m_inst) delete m_inst;
	if (m_inst) OutputDebugString(L"Instance still in memory!\n");
	if (m_timerEvent) SetEvent(m_timerEvent);
}

void CQunListBase::TimerThread(LPVOID data) {
	OutputDebugString(L"CQunListBase::TimerThread Start\n");
	m_timerEvent=CreateEvent(NULL,TRUE,FALSE,NULL);

	while (WaitForSingleObject(m_timerEvent,60000)==WAIT_TIMEOUT) {
		// Refresh qun online members
		//CallService(szIPCService,QQIPCSVC_QUN_UPDATE_ONLINE_MEMBERS,m_qunid);
		CallContactService(hContact,IPCSVC,QQIPCSVC_QUN_UPDATE_ONLINE_MEMBERS,m_qunid);
	}
	CloseHandle(m_timerEvent);
	m_timerEvent=NULL;
	OutputDebugString(L"CQunListBase::TimerThread End\n");
}

CQunListBase* CQunListBase::getInstance() {
	return m_inst;
}