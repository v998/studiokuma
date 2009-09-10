/*
Converted from Miranda IM MSN Plugin
Copyright (c) 2008 Studio KUMA.
Copyright (c) 2006-2008 Boris Krasnovskiy.
Copyright (c) 2003-2005 George Hazan.
Copyright (c) 2002-2003 Richard Hughes (original version).

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "stdafx.h"
#if 0
/////////////////////////////////////////////////////////////////////////////////////////
// WinInet class
/////////////////////////////////////////////////////////////////////////////////////////

#define ERROR_FLAGS (FLAGS_ERROR_UI_FILTER_FOR_ERRORS | FLAGS_ERROR_UI_FLAGS_GENERATE_DATA | FLAGS_ERROR_UI_FLAGS_CHANGE_OPTIONS )

#include "wininet.h"

typedef BOOL  ( WINAPI *ft_HttpQueryInfo )( HINTERNET, DWORD, LPVOID, LPDWORD, LPDWORD );
typedef BOOL  ( WINAPI *ft_HttpSendRequest )( HINTERNET, LPCSTR, DWORD, LPVOID, DWORD );
typedef BOOL  ( WINAPI *ft_InternetCloseHandle )( HINTERNET );
typedef DWORD ( WINAPI *ft_InternetErrorDlg )( HWND, HINTERNET, DWORD, DWORD, LPVOID* );
typedef BOOL  ( WINAPI *ft_InternetSetOption )( HINTERNET, DWORD, LPVOID, DWORD );
typedef BOOL  ( WINAPI *ft_InternetReadFile )( HINTERNET, LPVOID, DWORD, LPDWORD );
typedef BOOL  ( WINAPI *ft_InternetCrackUrl )( LPCSTR, DWORD, DWORD, LPURL_COMPONENTSA );

typedef HINTERNET ( WINAPI *ft_HttpOpenRequest )( HINTERNET, LPCSTR, LPCSTR, LPCSTR, LPCSTR, LPCSTR*, DWORD, DWORD );
typedef HINTERNET ( WINAPI *ft_InternetConnect )( HINTERNET, LPCSTR, INTERNET_PORT, LPCSTR, LPCSTR, DWORD, DWORD, DWORD );
typedef HINTERNET ( WINAPI *ft_InternetOpen )( LPCSTR, DWORD, LPCSTR, LPCSTR, DWORD );

class SSL_WinInet : public SSL_Base
{
public:
	SSL_WinInet(CNetwork* prt) : SSL_Base(prt) {}
	virtual ~SSL_WinInet();

	virtual  char* getSslResult( const char* parUrl, const char* parAuthInfo, const char* hdrs );
	virtual  int init(void);

private:
	void applyProxy( HINTERNET );
	char* readData( HINTERNET );

	//-----------------------------------------------------------------------------------
	HMODULE m_dll;

	ft_InternetCloseHandle f_InternetCloseHandle;
	ft_InternetConnect     f_InternetConnect;
	ft_InternetErrorDlg    f_InternetErrorDlg;
	ft_InternetOpen        f_InternetOpen;
	ft_InternetReadFile    f_InternetReadFile;
	ft_InternetSetOption   f_InternetSetOption;
	ft_HttpOpenRequest     f_HttpOpenRequest;
	ft_HttpQueryInfo       f_HttpQueryInfo;
	ft_HttpSendRequest     f_HttpSendRequest;
	ft_InternetCrackUrl    f_InternetCrackUrl;
};

/////////////////////////////////////////////////////////////////////////////////////////

int SSL_WinInet::init()
{
	if (( m_dll = LoadLibraryA( "WinInet.dll" )) == NULL )
		return 10;

	f_InternetCloseHandle = (ft_InternetCloseHandle)GetProcAddress( m_dll, "InternetCloseHandle" );
	f_InternetConnect = (ft_InternetConnect)GetProcAddress( m_dll, "InternetConnectA" );
	f_InternetErrorDlg = (ft_InternetErrorDlg)GetProcAddress( m_dll, "InternetErrorDlg" );
	f_InternetOpen = (ft_InternetOpen)GetProcAddress( m_dll, "InternetOpenA" );
	f_InternetReadFile = (ft_InternetReadFile)GetProcAddress( m_dll, "InternetReadFile" );
	f_InternetSetOption = (ft_InternetSetOption)GetProcAddress( m_dll, "InternetSetOptionA" );
	f_HttpOpenRequest = (ft_HttpOpenRequest)GetProcAddress( m_dll, "HttpOpenRequestA" );
	f_HttpQueryInfo = (ft_HttpQueryInfo)GetProcAddress( m_dll, "HttpQueryInfoA" );
	f_HttpSendRequest = (ft_HttpSendRequest)GetProcAddress( m_dll, "HttpSendRequestA" );
	f_InternetCrackUrl = (ft_InternetCrackUrl)GetProcAddress( m_dll, "InternetCrackUrlA" );
	return 0;
}

SSL_WinInet::~SSL_WinInet()
{
#if defined( _UNICODE ) 
	FreeLibrary( m_dll );   // we free WININET.DLL only if we're under NT
#endif
}

/////////////////////////////////////////////////////////////////////////////////////////

void SSL_WinInet::applyProxy( HINTERNET parHandle )
{
	char tBuffer[ 100 ];
	HANDLE hContact=0;
	DBVARIANT dbv;
	LPSTR m_szModuleName=proto->m_szModuleName;

	util_log( "Applying proxy parameters..." );

	if (!READC_S2("NLProxyAuthUser",&dbv)) {
		strncpy(tBuffer,dbv.pszVal,100);
		DBFreeVariant(&dbv);
		f_InternetSetOption(parHandle, INTERNET_OPTION_PROXY_USERNAME, tBuffer, strlen(tBuffer)+1);
	} else
		util_log( "Warning: proxy user name is required but missing" );

	if (!READC_S2("NLProxyAuthPassword",&dbv)) {
		strncpy(tBuffer,dbv.pszVal,100);
		DBFreeVariant(&dbv);
		CallService(MS_DB_CRYPT_DECODESTRING, strlen(tBuffer), (LPARAM)tBuffer);
		f_InternetSetOption(parHandle, INTERNET_OPTION_PROXY_PASSWORD, tBuffer, strlen( tBuffer )+1);
	}
	else util_log( "Warning: proxy user password is required but missing" );
}

/////////////////////////////////////////////////////////////////////////////////////////

char* SSL_WinInet::readData( HINTERNET hRequest )
{
	char bufQuery[32] ;
	DWORD tBufSize = sizeof( bufQuery );
	f_HttpQueryInfo( hRequest, HTTP_QUERY_CONTENT_LENGTH, bufQuery, &tBufSize, NULL );

	tBufSize = 0; 
	f_HttpQueryInfo( hRequest, HTTP_QUERY_RAW_HEADERS_CRLF, NULL, &tBufSize, NULL );

	DWORD dwSize = tBufSize + atol( bufQuery );
	char* tSslAnswer = (char*)mir_alloc( dwSize + 1 );

	if ( tSslAnswer )
	{
		f_HttpQueryInfo( hRequest, HTTP_QUERY_RAW_HEADERS_CRLF, tSslAnswer, &tBufSize, NULL );

		DWORD dwOffset = tBufSize;
		do {
			if (!f_InternetReadFile( hRequest, tSslAnswer+dwOffset, dwSize - dwOffset, &tBufSize))
			{
				mir_free( tSslAnswer );
				return NULL;
			}
			dwOffset += tBufSize;
		}
		while (tBufSize != 0 && dwOffset < dwSize);
		tSslAnswer[dwOffset] = 0;

		util_log( "SSL response:" );
		util_log(tSslAnswer);
	}

	return tSslAnswer;
}


char* SSL_WinInet::getSslResult( const char* parUrl, const char* parAuthInfo, const char* hdrs )
{
	const DWORD tFlags =
		INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTPS |
		INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTP |
		INTERNET_FLAG_IGNORE_CERT_CN_INVALID |
		INTERNET_FLAG_IGNORE_CERT_DATE_INVALID |
		INTERNET_FLAG_KEEP_CONNECTION |
		INTERNET_FLAG_NO_AUTO_REDIRECT |
		INTERNET_FLAG_NO_CACHE_WRITE |
		INTERNET_FLAG_NO_COOKIES |
		INTERNET_FLAG_RELOAD |
		INTERNET_FLAG_SECURE;

	HINTERNET tNetHandle;
	HANDLE hContact=0;
	LPSTR m_szModuleName=proto->m_szModuleName;
	bool fUseProxy=READC_B2("NLUseProxy")!=0;

	if (fUseProxy) {
		DWORD ptype = READC_B2("NLProxyType");
		if (!READC_B2("UseIeProxy") && ( ptype == PROXYTYPE_HTTP || ptype == PROXYTYPE_HTTPS)) {
			DBVARIANT dbv={0};
			if (READC_S2("NLProxyServer",&dbv)) {
				util_log( "Proxy server name should be set if proxy is used" );
				return NULL;
			}

			int tPortNumber = READC_W2("NLProxyPort");
			if (!tPortNumber) {
				util_log( "Proxy server port should be set if proxy is used" );
				return NULL;
			}

			char proxystr[1024];
			mir_snprintf( proxystr, sizeof( proxystr ), "https=http://%s:%d http=http://%s:%d", 
				dbv.pszVal, tPortNumber, dbv.pszVal, tPortNumber );

			DBFreeVariant(&dbv);

			tNetHandle = f_InternetOpen( FETION_USER_AGENT, INTERNET_OPEN_TYPE_PROXY, proxystr, NULL, 0 );
		}
		else tNetHandle = f_InternetOpen( FETION_USER_AGENT, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0 );
	}
	else 
		tNetHandle = f_InternetOpen( FETION_USER_AGENT, INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0 );

	if ( tNetHandle == NULL ) {
		util_log( "InternetOpen() failed" );
		return NULL;
	}

	util_log( "SSL request (%s): '%s'", fUseProxy ? "using proxy": "direct connection", parUrl );

	URL_COMPONENTSA urlComp = {0};
	urlComp.dwStructSize = sizeof( urlComp );
	urlComp.dwUrlPathLength = 1;
	urlComp.dwHostNameLength = 1;

	f_InternetCrackUrl( parUrl, 0, 0, &urlComp);

	char* url = ( char* )alloca( urlComp.dwHostNameLength + 1 );
	memcpy( url, urlComp.lpszHostName, urlComp.dwHostNameLength );
	url[urlComp.dwHostNameLength] = 0;

	char* tObjectName = ( char* )alloca( urlComp.dwUrlPathLength + 1 );
	memcpy( tObjectName, urlComp.lpszUrlPath, urlComp.dwUrlPathLength );
	tObjectName[urlComp.dwUrlPathLength] = 0;

	char* tSslAnswer = NULL;

	HINTERNET tUrlHandle = f_InternetConnect( tNetHandle, url, INTERNET_DEFAULT_HTTPS_PORT, "", "", INTERNET_SERVICE_HTTP, 0, 0 );
	if ( tUrlHandle != NULL ) 
	{
		HINTERNET tRequest = f_HttpOpenRequest( tUrlHandle, "POST", tObjectName, NULL, NULL, NULL, tFlags, 0 );
		if ( tRequest != NULL ) {

			unsigned tm = 6000;
			f_InternetSetOption( tRequest, INTERNET_OPTION_CONNECT_TIMEOUT, &tm, sizeof(tm));
			f_InternetSetOption( tRequest, INTERNET_OPTION_SEND_TIMEOUT, &tm, sizeof(tm));
			f_InternetSetOption( tRequest, INTERNET_OPTION_RECEIVE_TIMEOUT, &tm, sizeof(tm));

			if (fUseProxy && READC_B2("NLUseProxyAuth"))
				applyProxy( tRequest );

			char headers[2048];
			mir_snprintf(headers, sizeof( headers ), 
				"Accept: text/*\r\nContent-Type: text/xml; charset=utf-8\r\n%s", 
				hdrs ? hdrs : "");

			bool restart = false;

LBL_Restart:
			util_log( "Sending request..." );
#ifndef _DEBUG
			if (strstr(parUrl, "login") == NULL)
#endif
				util_log(parAuthInfo);

			DWORD tErrorCode = f_HttpSendRequest( tRequest, headers, strlen( headers ), 
				(void*)parAuthInfo, strlen( parAuthInfo ));
			if ( tErrorCode == 0 ) {
				TWinErrorCode errCode;
				util_log( "HttpSendRequest() failed with error %d: %s", errCode.mErrorCode, errCode.getText());

				switch( errCode.mErrorCode ) {
					case 2:
						proto->ShowNotification(TranslateT("Internet Explorer is in the 'Offline' mode. Switch IE to the 'Online' mode and then try to relogin"),NIIF_ERROR);
						break;

					case ERROR_INTERNET_INVALID_CA:
					case ERROR_INTERNET_SEC_CERT_DATE_INVALID:
					case ERROR_INTERNET_SEC_CERT_NO_REV:
					case ERROR_INTERNET_SEC_CERT_REV_FAILED:
						if (!restart)
						{
							DWORD dwFlags = SECURITY_FLAG_IGNORE_UNKNOWN_CA  |
								SECURITY_FLAG_IGNORE_REVOCATION  |   
								SECURITY_FLAG_IGNORE_CERT_DATE_INVALID;

							f_InternetSetOption( tRequest, INTERNET_OPTION_SECURITY_FLAGS, 
								&dwFlags, sizeof( dwFlags ));
							mir_free( readData( tRequest ));
							restart = true;
							goto LBL_Restart;
						}

					default:
						{
							WCHAR szTemp[MAX_PATH];
							swprintf(szTemp,TranslateT("Fetion verification failed with error %d: %S"),
								errCode.mErrorCode, errCode.getText());
							proto->ShowNotification(szTemp,NIIF_ERROR);
						}
				}
			}
			else {
				DWORD dwCode;
				DWORD tBufSize = sizeof( dwCode );
				f_HttpQueryInfo( tRequest, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &dwCode, &tBufSize, 0 );

				tSslAnswer = readData( tRequest );
			}

			f_InternetCloseHandle( tRequest );
		}

		f_InternetCloseHandle( tUrlHandle );
	}
	else util_log( "InternetOpenUrl() failed" );

	f_InternetCloseHandle( tNetHandle );
	return tSslAnswer;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Performs the MSN Passport login via SSL3 using the OpenSSL library

class SSL_OpenSsl : public SSL_Base
{
public:
	SSL_OpenSsl(CNetwork* prt) : SSL_Base(prt) {}

	virtual  char* getSslResult( const char* parUrl, const char* parAuthInfo, const char* hdrs );
	virtual  int init(void);
};

typedef int ( *PFN_SSL_int_void ) ( void );
typedef PVOID ( *PFN_SSL_pvoid_void ) ( void );
typedef PVOID ( *PFN_SSL_pvoid_pvoid ) ( PVOID );
typedef void ( *PFN_SSL_void_pvoid ) ( PVOID );
typedef int ( *PFN_SSL_int_pvoid_int ) ( PVOID, int );
typedef int ( *PFN_SSL_int_pvoid ) ( PVOID );
typedef int ( *PFN_SSL_int_pvoid_pvoid_int ) ( PVOID, PVOID, int );
typedef int ( *PFN_SSL_int_pvoid_int_pvoid ) ( PVOID, int, PVOID );

static	HMODULE hLibSSL;
static	PVOID sslCtx;

static	PFN_SSL_int_void            pfn_SSL_library_init;
static	PFN_SSL_pvoid_void          pfn_TLSv1_client_method;
static	PFN_SSL_pvoid_pvoid         pfn_SSL_CTX_new;
static	PFN_SSL_void_pvoid          pfn_SSL_CTX_free;
static	PFN_SSL_pvoid_pvoid         pfn_SSL_new;
static	PFN_SSL_void_pvoid          pfn_SSL_free;
static	PFN_SSL_int_pvoid_int       pfn_SSL_set_fd;
static	PFN_SSL_int_pvoid           pfn_SSL_connect;
static	PFN_SSL_int_pvoid_pvoid_int pfn_SSL_read;
static	PFN_SSL_int_pvoid_pvoid_int pfn_SSL_write;
static  PFN_SSL_int_pvoid_int_pvoid pfn_SSL_CTX_set_verify;

/////////////////////////////////////////////////////////////////////////////////////////

int SSL_OpenSsl::init(void)
{
	if ( sslCtx != NULL )
		return 0;

	if ( hLibSSL == NULL ) 
	{
		hLibSSL = LoadLibraryA( "WINSSL.DLL" );
		if ( hLibSSL == NULL )
			hLibSSL = LoadLibraryA( "CYASSL.DLL" );
		if ( hLibSSL == NULL )
			hLibSSL = LoadLibraryA( "SSLEAY32.DLL" );
		if ( hLibSSL == NULL )
			hLibSSL = LoadLibraryA( "LIBSSL32.DLL" );
		if ( hLibSSL == NULL ) {
			proto->ShowNotification(TranslateT("Valid SSLEAY32.DLL must be installed to perform the SSL login"), NIIF_ERROR);
			return 1;
		}

		int retVal = 0;
		if (( pfn_SSL_library_init = ( PFN_SSL_int_void )GetProcAddress( hLibSSL, "SSL_library_init" )) == NULL )
			retVal = TRUE;
		if (( pfn_TLSv1_client_method = ( PFN_SSL_pvoid_void )GetProcAddress( hLibSSL, "TLSv1_client_method" )) == NULL )
			retVal = TRUE;
		if (( pfn_SSL_CTX_new = ( PFN_SSL_pvoid_pvoid )GetProcAddress( hLibSSL, "SSL_CTX_new" )) == NULL )
			retVal = TRUE;
		if (( pfn_SSL_CTX_free = ( PFN_SSL_void_pvoid )GetProcAddress( hLibSSL, "SSL_CTX_free" )) == NULL )
			retVal = TRUE;
		if (( pfn_SSL_new = ( PFN_SSL_pvoid_pvoid )GetProcAddress( hLibSSL, "SSL_new" )) == NULL )
			retVal = TRUE;
		if (( pfn_SSL_free = ( PFN_SSL_void_pvoid )GetProcAddress( hLibSSL, "SSL_free" )) == NULL )
			retVal = TRUE;
		if (( pfn_SSL_set_fd = ( PFN_SSL_int_pvoid_int )GetProcAddress( hLibSSL, "SSL_set_fd" )) == NULL )
			retVal = TRUE;
		if (( pfn_SSL_connect = ( PFN_SSL_int_pvoid )GetProcAddress( hLibSSL, "SSL_connect" )) == NULL )
			retVal = TRUE;
		if (( pfn_SSL_read = ( PFN_SSL_int_pvoid_pvoid_int )GetProcAddress( hLibSSL, "SSL_read" )) == NULL )
			retVal = TRUE;
		if (( pfn_SSL_write = ( PFN_SSL_int_pvoid_pvoid_int )GetProcAddress( hLibSSL, "SSL_write" )) == NULL )
			retVal = TRUE;
		if (( pfn_SSL_CTX_set_verify = ( PFN_SSL_int_pvoid_int_pvoid )GetProcAddress( hLibSSL, "SSL_CTX_set_verify" )) == NULL )
			retVal = TRUE;

		if ( retVal ) {
			FreeLibrary( hLibSSL );
			proto->ShowNotification(TranslateT("Valid SSLEAY32.DLL must be installed to perform the SSL login"), NIIF_ERROR);
			return 1;
		}

		if (!pfn_SSL_library_init()) return 1;
		sslCtx = pfn_SSL_CTX_new( pfn_TLSv1_client_method());
		pfn_SSL_CTX_set_verify(sslCtx, 0, NULL);
		util_log( "OpenSSL context successully allocated" );
	}
	return 0;
}


/////////////////////////////////////////////////////////////////////////////////////////
#define NEWSTR_ALLOCA(A) (A==NULL)?NULL:strcpy((char*)alloca(strlen(A)+1),A)
#define NEWTSTR_ALLOCA(A) (A==NULL)?NULL:_tcscpy((TCHAR*)alloca(sizeof(TCHAR)*(_tcslen(A)+1)),A)

char* SSL_OpenSsl::getSslResult( const char* parUrl, const char* parAuthInfo, const char* hdrs )
{
	if ( _strnicmp( parUrl, "https://", 8 ) != 0 )
		return NULL;

	char* url = NEWSTR_ALLOCA(parUrl);
	char* path  = strchr(url+9, '/');
	char* path1 = strchr(url+9, ':');
	if (path == NULL) 
	{
		util_log( "Invalid URL passed: '%s'", parUrl );
		return NULL;
	}
	if (path < path1 || path1 == NULL)
		*path = 0;
	else
		*path1 = 0;

	++path;

	NETLIBUSERSETTINGS nls = { 0 };
	nls.cbSize = sizeof( nls );
	CallService(MS_NETLIB_GETUSERSETTINGS,WPARAM(g_hNetlibUser),LPARAM(&nls));
	int cpType = nls.proxyType;

	if (nls.useProxy && cpType == PROXYTYPE_HTTP)
	{
		nls.proxyType = PROXYTYPE_HTTPS;
		nls.szProxyServer = NEWSTR_ALLOCA(nls.szProxyServer);
		nls.szIncomingPorts = NEWSTR_ALLOCA(nls.szIncomingPorts);
		nls.szOutgoingPorts = NEWSTR_ALLOCA(nls.szOutgoingPorts);
		nls.szProxyAuthPassword = NEWSTR_ALLOCA(nls.szProxyAuthPassword);
		nls.szProxyAuthUser = NEWSTR_ALLOCA(nls.szProxyAuthUser);
		CallService(MS_NETLIB_SETUSERSETTINGS,WPARAM(g_hNetlibUser),LPARAM(&nls));
	}

	NETLIBOPENCONNECTION tConn = { 0 };
	tConn.cbSize = sizeof( tConn );
	tConn.szHost = url+8;
	tConn.wPort = 443;
	tConn.timeout = 8;
	HANDLE h = ( HANDLE )CallService( MS_NETLIB_OPENCONNECTION, ( WPARAM )g_hNetlibUser, ( LPARAM )&tConn );

	if (nls.useProxy && cpType == PROXYTYPE_HTTP)
	{
		nls.proxyType = PROXYTYPE_HTTP;
		nls.szProxyServer = NEWSTR_ALLOCA(nls.szProxyServer);
		nls.szIncomingPorts = NEWSTR_ALLOCA(nls.szIncomingPorts);
		nls.szOutgoingPorts = NEWSTR_ALLOCA(nls.szOutgoingPorts);
		nls.szProxyAuthPassword = NEWSTR_ALLOCA(nls.szProxyAuthPassword);
		nls.szProxyAuthUser = NEWSTR_ALLOCA(nls.szProxyAuthUser);
		CallService(MS_NETLIB_SETUSERSETTINGS,WPARAM(g_hNetlibUser),LPARAM(&nls));
	}

	if ( h == NULL )
		return NULL;

	char* result = NULL;
	PVOID ssl = pfn_SSL_new( sslCtx );
	if ( ssl != NULL ) {
		SOCKET s = CallService( MS_NETLIB_GETSOCKET, ( WPARAM )h, 0 );
		if ( s != INVALID_SOCKET ) {
			pfn_SSL_set_fd( ssl, s );
			if ( pfn_SSL_connect( ssl ) > 0 ) {
				util_log( "SSL connection succeeded" );

				const char* chdrs = hdrs ? hdrs : "";
				size_t hlen = strlen(chdrs) + 1024;
				char *headers = (char*)alloca(hlen);

				unsigned nBytes = mir_snprintf( headers, hlen,
					"POST /%s HTTP/1.1\r\n"
					"Accept: text/*\r\n"
					"%s"
					"User-Agent: %s\r\n"
					"Content-Length: %u\r\n"
					"Content-Type: text/xml; charset=utf-8\r\n"
					"Host: %s\r\n"
					"Connection: close\r\n"
					"Cache-Control: no-cache\r\n\r\n", path, chdrs,
					FETION_USER_AGENT, strlen( parAuthInfo ), url+8 );

				util_log( "Sending SSL query:\n%s", headers );
				pfn_SSL_write( ssl, headers, strlen( headers ));
#ifndef _DEBUG
				if (strstr(parUrl, "login") == NULL)
#endif
					util_log( "Sending SSL query:\n%s", parAuthInfo );
				pfn_SSL_write( ssl, (void*)parAuthInfo, strlen( parAuthInfo ));

				util_log( "SSL All data sent" );

				nBytes = 0;
				size_t dwTotSize = 8192;
				result = ( char* )mir_alloc( dwTotSize );

				for (;;) 
				{
					int dwSize = pfn_SSL_read( ssl, result+nBytes, dwTotSize - nBytes );
					if (dwSize  < 0) { nBytes = 0; break; }
					if (dwSize == 0) break;

					nBytes += dwSize;
					if ( nBytes >= dwTotSize ) {
						dwTotSize += 4096;
						char* rest = (char*)mir_realloc( result, dwTotSize );
						if ( rest == NULL )
							nBytes = 0;
						else 
							result = rest;
					}
				}
				result[nBytes] = 0;

				if ( nBytes > 0 ) 
				{
					util_log( "SSL read successfully read %d bytes:", nBytes );
					CallService( MS_NETLIB_LOG, ( WPARAM )g_hNetlibUser, ( LPARAM )result );

					if ( strncmp( result, "HTTP/1.1 100", 12 ) == 0 ) 
					{
						char* rest = strstr( result + 12, "HTTP/1.1" );
						if (rest) memmove(result, rest, nBytes + 1 - ( rest - result )); 
						else nBytes = 0;
					}
				}
				if (nBytes == 0)
				{
					mir_free( result );
					result = NULL;
					util_log( "SSL read failed" );
				}
			}
			else util_log( "SSL connection failed" );
		}
		else util_log( "pfn_SSL_connect failed" );

		pfn_SSL_free( ssl );
	}
	else util_log( "pfn_SSL_new failed" );

	Netlib_CloseHandle( h );
	return result;
}

SSLAgent::SSLAgent(CNetwork* proto)
{
	HANDLE hContact=NULL;
	LPSTR m_szModuleName=proto->m_szModuleName;
	unsigned useOpenSSL = READC_B2("UseOpenSSL");

	if ( useOpenSSL )
		pAgent = new SSL_OpenSsl(proto);
	else
		pAgent = new SSL_WinInet(proto);

	if ( pAgent->init() ) {
		delete pAgent;
		pAgent = NULL;
	}
}


SSLAgent::~SSLAgent()
{
	if (pAgent) delete pAgent;
}

char* httpParseHeader(char* buf, unsigned& status);

char* SSLAgent::getSslResult(char** parUrl, const char* parAuthInfo, const char* hdrs, 
							 unsigned& status, char*& htmlbody)
{
	status = 0;
	char* tResult = NULL;
	if (pAgent != NULL)
	{
		MimeHeaders httpinfo;

//lbl_retry:
		tResult = pAgent->getSslResult(*parUrl, parAuthInfo, hdrs);
		if (tResult != NULL)
		{
			status=atoi(strchr(tResult,' ')+1);
		}
		/*
		if (tResult != NULL)
		{
			char* htmlhdr = httpParseHeader( tResult, status );
			htmlbody = httpinfo.readFromBuffer( htmlhdr );
			if (status == 301 || status == 302)
			{
				const char* loc = httpinfo[ "Location" ];
				if (loc != NULL)
				{
					util_log( "Redirected to '%s'", loc );
					mir_free(*parUrl);
					*parUrl = mir_strdup(loc);
					mir_free(tResult);
					goto lbl_retry;
				}
			}
		}
		*/
	}
	return tResult;
}


void UninitSsl( void )
{
	if ( hLibSSL ) 
	{
		pfn_SSL_CTX_free( sslCtx );

		//		util_log( "Free SSL library" );
		FreeLibrary( hLibSSL );
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
// TWinErrorCode class

TWinErrorCode::TWinErrorCode() :
mErrorText( NULL )
{
	mErrorCode = ::GetLastError();
}

TWinErrorCode::~TWinErrorCode()
{
	mir_free( mErrorText );
}

char* TWinErrorCode::getText()
{
	if ( mErrorText == NULL )
	{
		int tBytes = 0;
		mErrorText = (char*)mir_alloc(256);

		if ( mErrorCode >= 12000 && mErrorCode < 12500 )
			tBytes = FormatMessageA(
			FORMAT_MESSAGE_FROM_HMODULE,
			GetModuleHandleA( "WININET.DLL" ),
			mErrorCode, LANG_NEUTRAL, mErrorText, 256, NULL );

		if ( tBytes == 0 )
			tBytes = FormatMessageA(
			FORMAT_MESSAGE_FROM_SYSTEM, NULL,
			mErrorCode, LANG_NEUTRAL, mErrorText, 256, NULL );

		if ( tBytes == 0 )
		{
			tBytes = mir_snprintf( mErrorText, 256, "unknown Windows error code %d", mErrorCode );
		}

		*mErrorText = (char)tolower( *mErrorText );

		if ( mErrorText[ tBytes-1 ] == '\n' )
			mErrorText[ --tBytes ] = 0;
		if ( mErrorText[ tBytes-1 ] == '\r' )
			mErrorText[ --tBytes ] = 0;
		if ( mErrorText[ tBytes-1 ] == '.' )
			mErrorText[ tBytes-1 ] = 0;
	}

	return mErrorText;
}

#pragma hdrstop
#endif

/////////////////////////////////////////////////////////////////////////////////////////
// constructors and destructor

MimeHeaders::MimeHeaders() :
mCount( 0 ),
mAllocCount(0),
mVals( NULL )
{
}

MimeHeaders::MimeHeaders( unsigned iInitCount ) :
mCount( 0 )
{
	mAllocCount = iInitCount;
	mVals = ( MimeHeader* )mir_alloc( iInitCount * sizeof( MimeHeader ));
}

MimeHeaders::~MimeHeaders()
{
	clear();
	mir_free( mVals );
}

void MimeHeaders::clear(void)
{
	for ( unsigned i=0; i < mCount; i++ ) 
	{
		MimeHeader& H = mVals[ i ];
		if (H.flags & 1) mir_free(( void* )H.name );
		if (H.flags & 2) mir_free(( void* )H.value );
	}
	mCount = 0;
}

unsigned MimeHeaders::allocSlot(void)
{
	if ( ++mCount >= mAllocCount ) 
	{
		mAllocCount += 10;
		mVals = ( MimeHeader* )mir_realloc( mVals, sizeof( MimeHeader ) * mAllocCount );
	}
	return mCount - 1; 
}



/////////////////////////////////////////////////////////////////////////////////////////
// add various values

void MimeHeaders::addString( const char* name, const char* szValue, unsigned flags )
{
	MimeHeader& H = mVals[ allocSlot() ];
	H.name = name;
	H.value = szValue; 
	H.flags = flags;
}

void MimeHeaders::addLong( const char* name, long lValue )
{
	MimeHeader& H = mVals[ allocSlot() ];
	H.name = name;

	char szBuffer[ 20 ];
	_ltoa( lValue, szBuffer, 10 );
	H.value = mir_strdup( szBuffer ); 
	H.flags = 2;
}

void MimeHeaders::addULong( const char* name, unsigned lValue )
{
	MimeHeader& H = mVals[ allocSlot() ];
	H.name = name;

	char szBuffer[ 20 ];
	_ultoa( lValue, szBuffer, 10 );
	H.value = mir_strdup( szBuffer ); 
	H.flags = 2;
}

void MimeHeaders::addBool( const char* name, bool lValue )
{
	MimeHeader& H = mVals[ allocSlot() ];
	H.name = name;
	H.value = lValue ? "true" : "false"; 
	H.flags = 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// write all values to a buffer

size_t MimeHeaders::getLength()
{
	size_t iResult = 0;
	for ( unsigned i=0; i < mCount; i++ ) {
		MimeHeader& H = mVals[ i ];
		iResult += strlen( H.name ) + strlen( H.value ) + 4;
	}

	return iResult;
}

char* MimeHeaders::writeToBuffer( char* pDest )
{
	for ( unsigned i=0; i < mCount; i++ ) {
		MimeHeader& H = mVals[ i ];
		pDest += sprintf( pDest, "%s: %s\r\n", H.name, H.value );
	}

	return pDest;
}

/////////////////////////////////////////////////////////////////////////////////////////
// read set of values from buffer

char* MimeHeaders::readFromBuffer( char* parString )
{
	clear();

	while ( *parString ) {
		if ( parString[0] == '\r' && parString[1] == '\n' ) {
			parString += 2;
			break;
		}

		char* peol = strchr( parString, '\r' );
		if ( peol == NULL )
			peol = parString + strlen(parString);
		*peol = '\0';

		if ( *++peol == '\n' )
			peol++;

		char* delim = strchr( parString, ':' );
		if ( delim == NULL ) {
			parString = peol;
			continue;
		}
		*delim++ = '\0';

		while ( *delim == ' ' || *delim == '\t' )
			delim++;

		MimeHeader& H = mVals[ allocSlot() ];

		H.name = parString;
		H.value = delim;
		H.flags = 0;

		parString = peol;
	}

	return parString;
}

const char* MimeHeaders::find( const char* szFieldName )
{
	for ( unsigned i=0; i < mCount; i++ ) {
		MimeHeader& MH = mVals[i];
		if ( _stricmp( MH.name, szFieldName ) == 0 )
			return MH.value;
	}

	return NULL;
}

static const struct _tag_cpltbl
{
	unsigned cp;
	char* mimecp;
} cptbl[] =
{
	{   037, "IBM037" },		  // IBM EBCDIC US-Canada 
	{   437, "IBM437" },		  // OEM United States 
	{   500, "IBM500" },          // IBM EBCDIC International 
	{   708, "ASMO-708" },        // Arabic (ASMO 708) 
	{   720, "DOS-720" },         // Arabic (Transparent ASMO); Arabic (DOS) 
	{   737, "ibm737" },          // OEM Greek (formerly 437G); Greek (DOS) 
	{   775, "ibm775" },          // OEM Baltic; Baltic (DOS) 
	{   850, "ibm850" },          // OEM Multilingual Latin 1; Western European (DOS) 
	{   852, "ibm852" },          // OEM Latin 2; Central European (DOS) 
	{   855, "IBM855" },          // OEM Cyrillic (primarily Russian) 
	{   857, "ibm857" },          // OEM Turkish; Turkish (DOS) 
	{   858, "IBM00858" },        // OEM Multilingual Latin 1 + Euro symbol 
	{   860, "IBM860" },          // OEM Portuguese; Portuguese (DOS) 
	{   861, "ibm861" },          // OEM Icelandic; Icelandic (DOS) 
	{   862, "DOS-862" },         // OEM Hebrew; Hebrew (DOS) 
	{   863, "IBM863" },          // OEM French Canadian; French Canadian (DOS) 
	{   864, "IBM864" },          // OEM Arabic; Arabic (864) 
	{   865, "IBM865" },          // OEM Nordic; Nordic (DOS) 
	{   866, "cp866" },           // OEM Russian; Cyrillic (DOS) 
	{   869, "ibm869" },		  // OEM Modern Greek; Greek, Modern (DOS) 
	{   870, "IBM870" },          // IBM EBCDIC Multilingual/ROECE (Latin 2); IBM EBCDIC Multilingual Latin 2 
	{   874, "windows-874" },     // ANSI/OEM Thai (same as 28605, ISO 8859-15); Thai (Windows) 
	{   875, "cp875" },           // IBM EBCDIC Greek Modern 
	{   932, "shift_jis" },       // ANSI/OEM Japanese; Japanese (Shift-JIS) 
	{   936, "gb2312" },          // ANSI/OEM Simplified Chinese (PRC, Singapore); Chinese Simplified (GB2312) 
	{   949, "ks_c_5601-1987" },  // ANSI/OEM Korean (Unified Hangul Code) 
	{   950, "big5" },            // ANSI/OEM Traditional Chinese (Taiwan; Hong Kong SAR, PRC); Chinese Traditional (Big5) 
	{  1026, "IBM1026" },         // IBM EBCDIC Turkish (Latin 5) 
	{  1047, "IBM01047" },        // IBM EBCDIC Latin 1/Open System 
	{  1140, "IBM01140" },        // IBM EBCDIC US-Canada (037 + Euro symbol); IBM EBCDIC (US-Canada-Euro)  
	{  1141, "IBM01141" },        // IBM EBCDIC Germany (20273 + Euro symbol); IBM EBCDIC (Germany-Euro) 
	{  1142, "IBM01142" },        // IBM EBCDIC Denmark-Norway (20277 + Euro symbol); IBM EBCDIC (Denmark-Norway-Euro) 
	{  1143, "IBM01143" },        // IBM EBCDIC Finland-Sweden (20278 + Euro symbol); IBM EBCDIC (Finland-Sweden-Euro) 
	{  1144, "IBM01144" },        // IBM EBCDIC Italy (20280 + Euro symbol); IBM EBCDIC (Italy-Euro) 
	{  1145, "IBM01145" },        // IBM EBCDIC Latin America-Spain (20284 + Euro symbol); IBM EBCDIC (Spain-Euro) 
	{  1146, "IBM01146" },        // IBM EBCDIC United Kingdom (20285 + Euro symbol); IBM EBCDIC (UK-Euro) 
	{  1147, "IBM01147" },        // IBM EBCDIC France (20297 + Euro symbol); IBM EBCDIC (France-Euro) 
	{  1148, "IBM01148" },        // IBM EBCDIC International (500 + Euro symbol); IBM EBCDIC (International-Euro) 
	{  1149, "IBM01149" },        // IBM EBCDIC Icelandic (20871 + Euro symbol); IBM EBCDIC (Icelandic-Euro) 
	{  1250, "windows-1250" },    // ANSI Central European; Central European (Windows)  
	{  1251, "windows-1251" },    // ANSI Cyrillic; Cyrillic (Windows) 
	{  1252, "windows-1252" },    // ANSI Latin 1; Western European (Windows)  
	{  1253, "windows-1253" },    // ANSI Greek; Greek (Windows) 
	{  1254, "windows-1254" },    // ANSI Turkish; Turkish (Windows) 
	{  1255, "windows-1255" },    // ANSI Hebrew; Hebrew (Windows) 
	{  1256, "windows-1256" },    // ANSI Arabic; Arabic (Windows) 
	{  1257, "windows-1257" },    // ANSI Baltic; Baltic (Windows) 
	{  1258, "windows-1258" },    // ANSI/OEM Vietnamese; Vietnamese (Windows) 
	{ 20127, "us-ascii" },        // US-ASCII (7-bit) 
	{ 20273, "IBM273" },          // IBM EBCDIC Germany 
	{ 20277, "IBM277" },          // IBM EBCDIC Denmark-Norway 
	{ 20278, "IBM278" },          // IBM EBCDIC Finland-Sweden 
	{ 20280, "IBM280" },          // IBM EBCDIC Italy 
	{ 20284, "IBM284" },          // IBM EBCDIC Latin America-Spain 
	{ 20285, "IBM285" },          // IBM EBCDIC United Kingdom 
	{ 20290, "IBM290" },          // IBM EBCDIC Japanese Katakana Extended 
	{ 20297, "IBM297" },          // IBM EBCDIC France 
	{ 20420, "IBM420" },          // IBM EBCDIC Arabic 
	{ 20423, "IBM423" },          // IBM EBCDIC Greek 
	{ 20424, "IBM424" },          // IBM EBCDIC Hebrew 
	{ 20838, "IBM-Thai" },        // IBM EBCDIC Thai 
	{ 20866, "koi8-r" },          // Russian (KOI8-R); Cyrillic (KOI8-R) 
	{ 20871, "IBM871" },          // IBM EBCDIC Icelandic 
	{ 20880, "IBM880" },          // IBM EBCDIC Cyrillic Russian 
	{ 20905, "IBM905" },          // IBM EBCDIC Turkish 
	{ 20924, "IBM00924" },        // IBM EBCDIC Latin 1/Open System (1047 + Euro symbol) 
	{ 20932, "EUC-JP" },          // Japanese (JIS 0208-1990 and 0121-1990) 
	{ 21025, "cp1025" },          // IBM EBCDIC Cyrillic Serbian-Bulgarian 
	{ 21866, "koi8-u" },          // Ukrainian (KOI8-U); Cyrillic (KOI8-U) 
	{ 28591, "iso-8859-1" },      // ISO 8859-1 Latin 1; Western European (ISO) 
	{ 28592, "iso-8859-2" },      // ISO 8859-2 Central European; Central European (ISO) 
	{ 28593, "iso-8859-3" },      // ISO 8859-3 Latin 3 
	{ 28594, "iso-8859-4" },      // ISO 8859-4 Baltic 
	{ 28595, "iso-8859-5" },      // ISO 8859-5 Cyrillic 
	{ 28596, "iso-8859-6" },      // ISO 8859-6 Arabic 
	{ 28597, "iso-8859-7" },      // ISO 8859-7 Greek 
	{ 28598, "iso-8859-8" },      // ISO 8859-8 Hebrew; Hebrew (ISO-Visual) 
	{ 28599, "iso-8859-9" },      // ISO 8859-9 Turkish 
	{ 28603, "iso-8859-13" },     // ISO 8859-13 Estonian 
	{ 28605, "iso-8859-15" },     // ISO 8859-15 Latin 9 
	{ 38598, "iso-8859-8-i" },    // ISO 8859-8 Hebrew; Hebrew (ISO-Logical) 
	{ 50220, "iso-2022-jp" },     // ISO 2022 Japanese with no halfwidth Katakana; Japanese (JIS) 
	{ 50221, "csISO2022JP" },     // ISO 2022 Japanese with halfwidth Katakana; Japanese (JIS-Allow 1 byte Kana) 
	{ 50222, "iso-2022-jp" },     // ISO 2022 Japanese JIS X 0201-1989; Japanese (JIS-Allow 1 byte Kana - SO/SI) 
	{ 50225, "iso-2022-kr" },     // ISO 2022 Korean  
	{ 50227, "ISO-2022-CN" },     // ISO 2022 Simplified Chinese; Chinese Simplified (ISO 2022) 
	{ 50229, "ISO-2022-CN-EXT" }, // ISO 2022 Traditional Chinese 
	{ 51932, "euc-jp" },          // EUC Japanese 
	{ 51936, "EUC-CN" },          // EUC Simplified Chinese; Chinese Simplified (EUC) 
	{ 51949, "euc-kr" },          // EUC Korean 
	{ 52936, "hz-gb-2312" },      // HZ-GB2312 Simplified Chinese; Chinese Simplified (HZ)  
	{ 54936, "GB18030" },         // Windows XP and later: GB18030 Simplified Chinese (4 byte); Chinese Simplified (GB18030)  
};


static unsigned FindCP( const char* mimecp )
{
	unsigned cp = CP_ACP;
	for (unsigned i = 0; i < sizeof(cptbl); ++i)
	{
		if (_stricmp(mimecp, cptbl[i].mimecp) == 0)
		{
			cp = cptbl[i].cp;
			break;
		}
	}
	return cp;
}


static int SingleHexToDecimal(char c)
{
	if ( c >= '0' && c <= '9' ) return c-'0';
	if ( c >= 'a' && c <= 'f' ) return c-'a'+10;
	if ( c >= 'A' && c <= 'F' ) return c-'A'+10;
	return -1;
}

static void  PQDecode( char* str )
{
	char* s = str, *d = str;

	while( *s )
	{
		switch (*s)
		{
		case '=': 
			{
				int digit1 = SingleHexToDecimal( s[1] );
				if ( digit1 != -1 ) 
				{
					int digit2 = SingleHexToDecimal( s[2] );
					if ( digit2 != -1 ) 
					{
						s += 3;
						*d++ = (char)(( digit1 << 4 ) | digit2);
					}	
				}
				break;
			}

		case '_':
			*d++ = ' '; ++s;
			break;

		default:
			*d++ = *s++;
			break;
		}
	}
	*d = 0;
}

static size_t utf8toutf16(char* str, wchar_t* res)
{
	wchar_t *dec;
	mir_utf8decode(str, &dec);
	wcscpy(res, dec);
	mir_free(dec);
	return wcslen(res);
}

char* Base64Decode( const char* str )
{
	if ( str == NULL ) return NULL; 

	size_t len = strlen( str );
	size_t reslen = Netlib_GetBase64DecodedBufferSize(len) + 4;
	char* res = ( char* )mir_alloc( reslen );

	char* p = const_cast< char* >( str );
	if ( len & 3 ) { // fix for stupid Kopete's base64 encoder
		char* p1 = ( char* )alloca( len+5 );
		memcpy( p1, p, len );
		p = p1;
		p1 += len; 
		for ( int i = 4 - (len & 3); i > 0; i--, p1++, len++ )
			*p1 = '=';
		*p1 = 0;
	}

	NETLIBBASE64 nlb = { p, len, ( PBYTE )res, reslen };
	CallService( MS_NETLIB_BASE64DECODE, 0, LPARAM( &nlb ));
	res[nlb.cbDecoded] = 0;

	return res;
}

wchar_t* MimeHeaders::decode(const char* val)
{
	size_t ssz = strlen(val)+1;
	char* tbuf = (char*)alloca(ssz);
	memcpy(tbuf, val, ssz);

	wchar_t* res = (wchar_t*)mir_alloc(ssz * sizeof(wchar_t));
	wchar_t* resp = res;

	char *p = tbuf;
	while (*p)
	{
		char *cp = strstr(p, "=?");
		if (cp == NULL) break;
		*cp = 0;

		size_t sz = utf8toutf16(p, resp);
		ssz -= sz; resp += sz; 
		cp += 2; 

		char *enc = strchr(cp, '?');
		if (enc == NULL) break;
		*(enc++) = 0;

		char *fld = strchr(enc, '?');
		if (fld == NULL) break;
		*(fld++) = 0;

		char *pe = strstr(fld, "?=");
		if (pe == NULL) break;
		*pe = 0;

		switch (*enc)
		{
		case 'b':
		case 'B':
			{
				char* dec = Base64Decode(fld);
				strcpy(fld, dec);
				mir_free(dec);
				break;
			}

		case 'q':
		case 'Q':
			PQDecode(fld);
			break;
		}

		if (_stricmp(cp, "UTF-8") == 0)
		{
			sz = utf8toutf16(fld, resp);
			ssz -= sz; resp += sz;
		}
		else {
			sz = MultiByteToWideChar(FindCP(cp), 0, fld, -1, resp, ssz);
			if (sz == 0)
				sz = MultiByteToWideChar(CP_ACP, 0, fld, -1, resp, ssz);
			ssz -= --sz; resp += sz;
		}
		p = pe + 2;
	}

	utf8toutf16(p, resp); 

	return res;
}


char* MimeHeaders::decodeMailBody(char* msgBody)
{
	char* res;
	const char *val = find("Content-Transfer-Encoding");
	if (val && _stricmp(val, "base64") == 0)
	{
		char* ch = msgBody;
		size_t len = strlen(msgBody) + 1;
		while (*ch != 0)
		{
			if ( *ch == '\n' || *ch == '\r' )
				memmove( ch, ch+1, len-- - ( ch - msgBody ));
			else
				++ch;
		}
		res = Base64Decode(msgBody);
	}
	else
	{
		res = mir_strdup(msgBody);
		if (val && _stricmp(val, "quoted-printable") == 0)
			PQDecode(res);
	}
	return res;
}


int sttDivideWords( char* parBuffer, int parMinItems, char** parDest )
{
	int i;
	for ( i=0; i < parMinItems; i++ ) {
		parDest[ i ] = parBuffer;

		int tWordLen = strcspn( parBuffer, " \t" );
		if ( tWordLen == 0 )
			return i;

		parBuffer += tWordLen;
		if ( *parBuffer != '\0' ) {
			int tSpaceLen = strspn( parBuffer, " \t" );
			memset( parBuffer, 0, tSpaceLen );
			parBuffer += tSpaceLen;
		}	}

	return i;
}


char* httpParseHeader(char* buf, unsigned& status)
{
	status = 0;
	char* p = strstr( buf, "\r\n" );
	if ( p != NULL ) 
	{
		*p = 0; p += 2;

		union {
			char* tWords[ 2 ];
			struct { char *method, *status; } data;
		};

		if ( sttDivideWords( buf, 2, tWords ) == 2 )
			status = strtoul(data.status, NULL, 10);
	}
	return p;
}

#define NEWSTR_ALLOCA(A) (A==NULL)?NULL:strcpy((char*)alloca(strlen(A)+1),A)

/////////////////////////////////////////////////////////////////////////////////////////
// Performs the MSN Passport login via SSL3 using the OpenSSL library

class SSL_OpenSsl : public SSL_Base
{
public:
	SSL_OpenSsl(CNetwork* prt) : SSL_Base(prt) {}

	virtual  char* getSslResult( const char* parUrl, const char* parAuthInfo, const char* hdrs );
};


/////////////////////////////////////////////////////////////////////////////////////////

char* SSL_OpenSsl::getSslResult( const char* parUrl, const char* parAuthInfo, const char* hdrs )
{
	if ( _strnicmp( parUrl, "https://", 8 ) != 0 )
		return NULL;

	char* url = NEWSTR_ALLOCA(parUrl);
	char* path  = strchr(url+9, '/');
	char* path1 = strchr(url+9, ':');
	if (path == NULL) 
	{
		util_log( "Invalid URL passed: '%s'", parUrl );
		return NULL;
	}
	if (path < path1 || path1 == NULL)
		*path = 0;
	else
		*path1 = 0;

	++path;

	NETLIBUSERSETTINGS nls = { 0 };
	nls.cbSize = sizeof( nls );
	CallService(MS_NETLIB_GETUSERSETTINGS,WPARAM(g_hNetlibUser),LPARAM(&nls));
	int cpType = nls.proxyType;

	if (nls.useProxy && cpType == PROXYTYPE_HTTP)
	{
		nls.proxyType = PROXYTYPE_HTTPS;
		nls.szProxyServer = NEWSTR_ALLOCA(nls.szProxyServer);
		nls.szIncomingPorts = NEWSTR_ALLOCA(nls.szIncomingPorts);
		nls.szOutgoingPorts = NEWSTR_ALLOCA(nls.szOutgoingPorts);
		nls.szProxyAuthPassword = NEWSTR_ALLOCA(nls.szProxyAuthPassword);
		nls.szProxyAuthUser = NEWSTR_ALLOCA(nls.szProxyAuthUser);
		CallService(MS_NETLIB_SETUSERSETTINGS,WPARAM(g_hNetlibUser),LPARAM(&nls));
	}

	NETLIBOPENCONNECTION tConn = { 0 };
	tConn.cbSize = sizeof( tConn );
	tConn.szHost = url+8;
	tConn.wPort = 443;
	tConn.timeout = 8;
	tConn.flags = NLOCF_SSL;
	HANDLE h = ( HANDLE )CallService( MS_NETLIB_OPENCONNECTION, ( WPARAM )g_hNetlibUser, ( LPARAM )&tConn );

	if (nls.useProxy && cpType == PROXYTYPE_HTTP)
	{
		nls.proxyType = PROXYTYPE_HTTP;
		nls.szProxyServer = NEWSTR_ALLOCA(nls.szProxyServer);
		nls.szIncomingPorts = NEWSTR_ALLOCA(nls.szIncomingPorts);
		nls.szOutgoingPorts = NEWSTR_ALLOCA(nls.szOutgoingPorts);
		nls.szProxyAuthPassword = NEWSTR_ALLOCA(nls.szProxyAuthPassword);
		nls.szProxyAuthUser = NEWSTR_ALLOCA(nls.szProxyAuthUser);
		CallService(MS_NETLIB_SETUSERSETTINGS,WPARAM(g_hNetlibUser),LPARAM(&nls));
	}

	if ( h == NULL ) return NULL;

	char* result = NULL;

	const char* chdrs = hdrs ? hdrs : "";
	size_t hlen = strlen(chdrs) + 1024;
	char *headers = (char*)alloca(hlen);

	unsigned nBytes = mir_snprintf( headers, hlen,
		"POST /%s HTTP/1.1\r\n"
		"Accept: text/*\r\n"
		"%s"
		"User-Agent: %s\r\n"
		"Content-Length: %u\r\n"
		"Content-Type: text/xml; charset=utf-8\r\n"
		"Host: %s\r\n"
		"Connection: close\r\n"
		"Cache-Control: no-cache\r\n\r\n", path, chdrs,
		FETION_USER_AGENT, strlen( parAuthInfo ), url+8 );

	int flags = 0;

	Netlib_Send( h, headers, strlen( headers ), flags);

#ifndef _DEBUG
	if (strstr(parUrl, "login")) flags |= MSG_NODUMP;
#endif

	Netlib_Send( h, parAuthInfo, strlen( parAuthInfo ), flags);
	util_log( "SSL All data sent" );

	nBytes = 0;
	size_t dwTotSize = 8192;
	result = ( char* )mir_alloc( dwTotSize );

	for (;;) 
	{
		int dwSize = Netlib_Recv( h, result+nBytes, dwTotSize - nBytes, 0 );
		if (dwSize  < 0) { nBytes = 0; break; }
		if (dwSize == 0) break;

		nBytes += dwSize;
		if ( nBytes >= dwTotSize ) 
		{
			dwTotSize += 4096;
			char* rest = (char*)mir_realloc( result, dwTotSize );
			if ( rest == NULL )
				nBytes = 0;
			else 
				result = rest;
		}
	}
	result[nBytes] = 0;

	if ( nBytes > 0 ) 
	{
		CallService( MS_NETLIB_LOG, ( WPARAM )g_hNetlibUser, ( LPARAM )result );

		if ( strncmp( result, "HTTP/1.1 100", 12 ) == 0 ) 
		{
			char* rest = strstr( result + 12, "HTTP/1.1" );
			if (rest) memmove(result, rest, nBytes + 1 - ( rest - result )); 
			else nBytes = 0;
		}
	}
	if (nBytes == 0)
	{
		mir_free( result );
		result = NULL;
		util_log( "SSL read failed" );
	}

	Netlib_CloseHandle( h );
	return result;
}

SSLAgent::SSLAgent(CNetwork* proto)
{
	pAgent = new SSL_OpenSsl(proto);
}


SSLAgent::~SSLAgent()
{
	if (pAgent) delete pAgent;
}


char* SSLAgent::getSslResult(char** parUrl, const char* parAuthInfo, const char* hdrs, 
							 unsigned& status, char*& htmlbody)
{
	status = 0;
	char* tResult = NULL;
	if (pAgent != NULL)
	{
		MimeHeaders httpinfo;

		//lbl_retry:
		tResult = pAgent->getSslResult(*parUrl, parAuthInfo, hdrs);
		if (tResult != NULL)
		{
			status=atoi(strchr(tResult,' ')+1);
		}
	}
	return tResult;

#if 0
	status = 0;
	char* tResult = NULL;
	if (pAgent != NULL)
	{
		MimeHeaders httpinfo;

lbl_retry:
		tResult = pAgent->getSslResult(*parUrl, parAuthInfo, hdrs);
		if (tResult != NULL)
		{
			char* htmlhdr = httpParseHeader( tResult, status );
			htmlbody = httpinfo.readFromBuffer( htmlhdr );
			if (status == 301 || status == 302)
			{
				const char* loc = httpinfo[ "Location" ];
				if (loc != NULL)
				{
					pAgent->proto->MSN_DebugLog( "Redirected to '%s'", loc );
					mir_free(*parUrl);
					*parUrl = mir_strdup(loc);
					mir_free(tResult);
					goto lbl_retry;
				}
			}
		}
	}
	return tResult;
#endif
}
