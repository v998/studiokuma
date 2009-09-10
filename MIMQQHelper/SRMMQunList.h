#ifndef SRMMQUNLIST_H
#define SRMMQUNLIST_H
class CSRMMQunList: public CQunListBase {
public:
	static CSRMMQunList* getInstance();
protected:
	virtual void TabSwitched(CWPRETSTRUCT* cps);
	virtual void Hide();
	virtual void Show();
	virtual void Move();
	virtual void Close();
	virtual void NamesUpdated(ipcmembers_t* ipcms);
	virtual void OnlineMembersUpdated(ipconlinemembers_t* ipcms);
private:
	CSRMMQunList();
	virtual ~CSRMMQunList();

	static INT_PTR CALLBACK DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	INT_PTR CALLBACK _DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	void _HandlePopup(void*);
	void Refresh();

	HWND m_hWnd;
	map<int,HBITMAP> m_headlist;
	HBRUSH hBrushBkgnd, hBrushNotice, hBrushTitle;
	HFONT hFontBold/*, hFontNormal*/;
	HPEN hPenTitle;
	HINSTANCE hHeadImg;
	bool noheadimg;
	bool m_updating;
};
#endif // SRMMQUNLIST_H
