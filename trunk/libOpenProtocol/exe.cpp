// libOpenProtocol.cpp : コンソール アプリケーションのエントリ ポイントを定義します。
//

#ifndef _MIMDLL

#include "stdafx.h"
#include <conio.h>
#include <wininet.h>

class MyOpenProtocolHandler: virtual public COpenProtocolHandler {
public:
	/*
	MyCOpenProtocolHandler() {
	}

	~MyCOpenProtocolHandler() {
	}
	*/

	LPSTR oph_strdup(LPCSTR pcszStr) {
		return _strdup(pcszStr);
	}

	LPVOID oph_malloc(int cbAllocation) {
		return LocalAlloc(LMEM_FIXED,cbAllocation);
	}

	void oph_free(void *pData) {
		LocalFree(pData);
	}

	void oph_printdebug(LPCSTR psczStr) {
		_cprintf(psczStr);
	}

	void oph_pthread_create(void(*start_routine)(LPVOID), LPVOID arg) {
		_beginthread(start_routine,0,arg);
	}

	void handler(LPCSTR pcszMsg, LPCSTR pcszStatus, LPVOID pAux) {
		m_protocol->print_debug(__FUNCTION__"(): DEFAULT! pcszMsg=%s pcszStatus=%s pAux?=%s",pcszMsg,pcszStatus,pAux==NULL?"no":"yes");
	}

	LPSTR http_get(LPCSTR pcszUrl, LPCSTR pcszReferer, LPDWORD pdwOutSize) {
		*pdwOutSize=0;
		return oph_strdup("");
	}

private:
	HINTERNET m_hInternet;
};

int _tmain(int argc, _TCHAR* argv[])
{
	MyOpenProtocolHandler handler;

	COpenProtocol protocol("lua\\qq\\init.lua",&handler);
	protocol.setLogin("1234567890","password");
	protocol.setQunImagePath("R:\\",171);
	protocol.setAvatarPath("R:\\");
	protocol.setBBCode(true);
	protocol.setInitStatus(40071);

	protocol.start();

	_getch();
	return 0;
}

#endif
