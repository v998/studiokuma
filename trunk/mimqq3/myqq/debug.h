#ifndef _DEBUG_H
#define _DEBUG_H

#include <stdio.h>
#include <errno.h>
#include <assert.h>

// #define RELEASE

#ifndef RELEASE
/*
#define DBG(args ...) \
	print_error( __FILE__, (char*)__func__, __LINE__, ##args )
*/
#ifdef __cplusplus
extern "C" {
#endif
int util_log2(char *fmt,...);
#ifdef __cplusplus
}
#endif

#define DBG util_log2
#else
// #define DBG(args ...) 
//#define DBG printf
#endif
#define DBG util_log2
#define MSG	util_log2
void print_error(char* file, char* function, int line, const char *fmt, ...);
void hex_dump( unsigned char * buf, int len );
void debug_term_on();
void debug_term_off();
void debug_file_on();
void debug_file_off();
void debug_set_dir(char* str);

#endif //_DEBUG_H

