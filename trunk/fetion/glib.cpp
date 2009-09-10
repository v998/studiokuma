#include "stdafx.h"

LPSTR g_strndup(LPCSTR pszStr, DWORD dwLen) {
	LPSTR pszOut=(LPSTR)mir_alloc(dwLen+1);
	strncpy(pszOut,pszStr,dwLen);
	pszOut[dwLen]=0;
	return pszOut;
}

LPSTR* g_strsplit(LPCSTR pszStr, LPCSTR pszSep, DWORD dwLimit) {
	if (dwLimit<1) dwLimit=(DWORD)-1;

	int count=0, count2=1;
	int seplen=strlen(pszSep);
	LPSTR* ppszRet;

	for (LPCSTR ppszCurrent=pszStr; ppszCurrent=strstr(ppszCurrent,pszSep); ppszCurrent+=seplen)
		count++;

	ppszRet=(LPSTR*)mir_alloc(sizeof(LPSTR)*(count+2));
	*ppszRet=mir_strdup(pszStr);

	for (LPSTR ppszCurrent=*ppszRet; ppszCurrent=strstr(ppszCurrent,pszSep); ppszCurrent+=seplen) {
		*ppszCurrent=0;
		ppszRet[count2++]=ppszCurrent+seplen;
		dwLimit--;
		if (dwLimit==0) break;
	}
	ppszRet[count+1]=0;

	return ppszRet;
}

void g_strfreev(LPSTR* pszV) {
	mir_free(*pszV);
	mir_free(pszV);
};

LPSTR g_strdup_printf(LPCSTR pszFmt, ...) {
	LPSTR str=(LPSTR)mir_alloc(8192);
	LPSTR pszRet;
	va_list ap;

	va_start(ap, pszFmt);

	if (int tBytes=mir_vsnprintf(str, 8192, pszFmt, ap))
		str[tBytes]=0;

	pszRet=mir_strdup(str);
	mir_free(str);
	va_end(ap);
	return pszRet;
}

GSList* g_slist_append(GSList* gsl, LPVOID data) {
	GSList* curgsl;
	if (!gsl) {		
		curgsl=gsl=(GSList*)mir_alloc(sizeof(GSList));
	} else {
		curgsl=gsl;
		while (curgsl->next) curgsl=curgsl->next;
		curgsl->next=(GSList*)mir_alloc(sizeof(GSList));
		curgsl=curgsl->next;
	}
	curgsl->data=data;
	curgsl->next=NULL;
	return gsl;
}

GSList* g_slist_next(GSList* gsl) {
	return gsl->next;
}

GSList* g_slist_remove(GSList* gsl, LPVOID data) {
	GSList* curgsl=gsl;

	if (gsl->data==data) {
		gsl=gsl->next;
	} else {
		GSList* lastgsl=gsl;
		curgsl=curgsl->next;

		while (curgsl) {
			if (curgsl->data==data) {
				lastgsl->next=curgsl->next;
				break;
			}
			lastgsl=curgsl;
			curgsl=curgsl->next;
		}
	}

	if (curgsl) mir_free(curgsl);
	return gsl;
}

LPSTR g_strchug(LPSTR str) {
	LPSTR pszStart=str;
	while (*pszStart && *pszStart==' ') pszStart++;
	if (pszStart!=str) memmove(str,pszStart,strlen(pszStart)+1);
	return str;
}
