#ifndef CODEVERIFY_H
#define CODEVERIFY_H

#define XGVC_TYPE_LOGIN 1
#define XGVC_TYPE_ADDUSER 2
#define XGVC_TYPE_ADDQUN 3
#define XGVC_TYPE_MODUSER 4

class XGraphicVerifyCode {
public:
	XGraphicVerifyCode();
	XGraphicVerifyCode(const XGraphicVerifyCode &rhs);
	virtual ~XGraphicVerifyCode();
	XGraphicVerifyCode &operator=(const XGraphicVerifyCode &rhs);
	void setSessionToken(const unsigned char *token, const unsigned short len);
	void setData(const unsigned char *data, const unsigned short len);
	void setCode(const char* code);
	void setType(const char type);

	unsigned short m_SessionTokenLen;
	unsigned char *m_SessionToken;
	unsigned short m_DataLen;
	unsigned char *m_Data;
	char m_type;
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