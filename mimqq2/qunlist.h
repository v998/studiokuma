#ifndef QUNLIST_H
#define QUNLIST_H

class CQunListV2 {
public:
	static CQunListV2* getInstance(bool);
	static bool isCreated() {return m_inst!=NULL;};
	static void InstallHook(HINSTANCE);
	static void UninstallHook();
	static int getQunid();
	void hide();
	void refresh();
	void destroy();
	static int QunMemberListService(WPARAM wParam, LPARAM lParam);

private:
	CQunListV2();
	static LRESULT CALLBACK MessageHookProcPost(int code, WPARAM wParam, LPARAM lParam);
	static INT_PTR CALLBACK DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	INT_PTR CALLBACK _DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	virtual ~CQunListV2();
	void move();
	void overrideTimer() {m_timerEnabled=false;};
	void _HandlePopup(void*);

	static CQunListV2* m_inst;
	static HHOOK hHookMessagePost;
	static HWND m_hwndSRMM;
	static unsigned int m_qunid;
	static HINSTANCE m_hInstance;
	HWND m_hwnd;
	bool m_timerEnabled;
	bool m_updating;
	unsigned char m_members;
	unsigned char m_online;
	unsigned char m_flush;

	HBRUSH hBrushBkgnd, hBrushNotice, hBrushTitle;
	HFONT hFontBold/*, hFontNormal*/;
	HPEN hPenTitle;
	HANDLE hContact;
	map<unsigned int,HBITMAP> headlist;
	HINSTANCE hHeadImg;
	bool noheadimg;
};
#endif