#ifndef CODEVERIFY_H
#define CODEVERIFY_H

class XGraphicVerifyCode {
public:
	XGraphicVerifyCode();
	XGraphicVerifyCode(const XGraphicVerifyCode &rhs);
	virtual ~XGraphicVerifyCode();
	XGraphicVerifyCode &operator=(const XGraphicVerifyCode &rhs);
	void setSessionToken(const unsigned char *token, const unsigned short len);
	void setData(const unsigned char *data, const unsigned short len);
	void setCode(const char* code);

	unsigned short m_SessionTokenLen;
	unsigned char *m_SessionToken;
	unsigned short m_DataLen;
	unsigned char *m_Data;
	char* m_code;

	CNetwork* m_network;
};

class CodeVerifyWindow {
public:
	CodeVerifyWindow(XGraphicVerifyCode* code);
	CodeVerifyWindow(ASKDLGPARAMS* adp);
	static HWND getHwnd(int sequenceid);

	virtual ~CodeVerifyWindow() {};

private:
	XGraphicVerifyCode* m_code;
	ASKDLGPARAMS* m_adp;
	HBITMAP m_bitmap;
	HWND m_hWnd;
	//CodeVerifyWindow* m_inst;
	char* m_codefile;
	char* m_sessionid;
	static map<int,HWND> m_windows;

	static INT_PTR CALLBACK DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

#endif