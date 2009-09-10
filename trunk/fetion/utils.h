#ifndef UTILS_H
#define UTILS_H

extern int util_log(const char *fmt,...);
extern HANDLE util_find_qun_contact(const unsigned int QQID);
extern void util_convertFromGBK(char* szGBK);
extern void util_convertToGBK(char* szGBK);
extern void util_clean_nickname(char* nickname);
extern void util_clean_nickname(LPWSTR nickname);
extern void util_remove_qun_pics();
extern void util_trimChatTags(char* szSrc, char* szTag);
//extern void util_dispatchServer(const char* url, ThreadData* td);
extern void util_fillClientKey(LPSTR pszDest);

extern int util_group_name_exists(LPCWSTR name,int skipGroup);
/*
extern void util_convertFromNative(LPSTR *szDest, LPCTSTR szSource);
extern void util_convertToNative(LPTSTR *szDest, LPCSTR szSource, bool fGBK=TRUE);
*/
#define QQ_SendBroadcast(hContact,type,result,hProcess,lParam) ProtoBroadcastAck(m_szModuleName,hContact,type,result,hProcess,lParam)

extern LRESULT CALLBACK PopupWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
#endif // UTILS_H
