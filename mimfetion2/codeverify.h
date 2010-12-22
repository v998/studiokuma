class CCodeVerifyWindow {
public:
	CCodeVerifyWindow(HINSTANCE hInstance, LPWSTR pszText, LPSTR pszImagePath);
	CCodeVerifyWindow(HINSTANCE hInstance, LPWSTR pszText, LPSTR pszURL, CLibWebFetion* webqq);
	virtual ~CCodeVerifyWindow() {};
	LPCSTR GetCode() const { return m_code; }

protected:
	INT_PTR DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
	CHAR m_code[5];
	LPWSTR m_text;
	LPSTR m_imagepath;
	HBITMAP m_bitmap;
	CLibWebFetion* m_webqq;

	static INT_PTR CALLBACK _DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
};
