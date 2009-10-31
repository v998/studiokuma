/*
 *  util.c
 *
 *  MyQQ utilities
 *
 *  Copyright (C) 2008  Huang Guan
 *
 *  2008-7-15 Created.
 *  2009-3-22 Add msleep.
 *
 */
#include "stdafx.h"

#ifdef __WIN32__
#include <io.h>
#include <windows.h>
#include <direct.h>
#else
#include <sys/stat.h>
#endif
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "debug.h"
#include "qqdef.h"
#include "util.h"
#include "memory.h"

int mkdir_recursive( char* path )
{
	char *p;
	if( access( path, 0 ) == 0 )
		return 0;
	for( p=path; *p; p++ ){
		if( p>path && *p == '/' ){
			*p = 0;
			if( access( path, 0 ) != 0 ){
#ifdef __WIN32__
				mkdir( path );
#else
				if( mkdir( path, S_IRWXU ) != 0 )
					return -1;
#endif
			}
			*p = '/';
		}
	}
#ifdef __WIN32__
	return mkdir( path );
#else
	return mkdir( path, S_IRWXU );
#endif
}

int trans_faces( char* src, char* dst, int outlen )
{
	char * p, *q;
	p = src;	q = dst;
	while( *p ){
		if( *(p++) == 0x14 ){
			if( (int)(q-dst) < outlen - 10 )
				q += sprintf( q, "[face:%u]", *(p++) );
		}else{
			if( (int)(q-dst) < outlen )
				*(q++) = *(p-1);
		}
	}
	*q = 0;
	return (int)(q-dst);
}


//2009-2-7 9:20 Huang Guan
//get middle value from a string by the left and the right. 
char* mid_value( char* str, char* left, char* right, char* out, int outlen )
{
	char* beg, * end, t;
	beg = strstr( str, left );
	if( beg ){
		beg += strlen(left);
		if( right ){
			end = strstr( beg, right );
		}else{
			end = beg + strlen(beg);
		}
		if( end ){
			t = *end; *end = 0;
			strncpy( out, beg, outlen );
			*end = t;
			return end;	//returns the end
		}
	}
	*out = '\0';
	return str;
}

//2009-2-7 9:21 Huang Guan
//Download a file
//http://verycode.qq.com/getimage?fromuin=942011793&touin=357339036&appt=1&flag=1
//GET  HTTP/1.1
int http_request( int* http_sock, char* url, char* session, char* data, int* datalen )
{

	NETLIBHTTPREQUEST nlhr={sizeof(nlhr),REQUEST_GET,NLHRF_GENERATEHOST};
	nlhr.szUrl=url;

	if (NETLIBHTTPREQUEST* nlhrr=(NETLIBHTTPREQUEST*)CallService(MS_NETLIB_HTTPTRANSACTION,(WPARAM)hNetlibUser,(LPARAM)&nlhr)) {
		for (int c=0; c<nlhrr->headersCount; c++) {
			if (!strcmp(nlhrr->headers[c].szName,"getqqsession")) {
				strcpy(session,nlhrr->headers[c].szValue);
				memcpy(data,nlhrr->pData,nlhrr->dataLength);
				*datalen=nlhrr->dataLength;
				break;
			}
		}
		CallService(MS_NETLIB_FREEHTTPREQUESTSTRUCT,(WPARAM)nlhrr,0);
	}

	return 0;
}

#ifdef __WIN32__
void msleep( unsigned int ms )
{
	Sleep( ms );
}
#endif

int get_splitable_pos( char* buf, int pos )
{
	//pos = 699
	if( (uchar)buf[pos]>=0x80 && (uchar)buf[pos]<=0xBF ){
		do	
			pos--;
		while( pos && (uchar)buf[pos]<0xC2 );
	}
	return pos;	//buf[pos]不可取
}
