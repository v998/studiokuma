typedef struct _GSList {
	LPVOID data;
	_GSList *next;
} GSList;

extern LPSTR g_strndup(LPCSTR pszStr, DWORD dwLen);
extern LPSTR* g_strsplit(LPCSTR pszStr, LPCSTR pszSep, DWORD dwLimit);
extern void g_strfreev(LPSTR* pszV);
extern LPSTR g_strdup_printf(LPCSTR pszFmt, ...);
extern GSList* g_slist_append(GSList* gsl, LPVOID data);
extern GSList* g_slist_next(GSList* gsl);
extern GSList* g_slist_remove(GSList* gsl, LPVOID data);
extern LPSTR g_strchug(LPSTR str);
