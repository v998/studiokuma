
#ifndef __PTHREAD__H__
#define __PTHREAD__H__

#ifdef __WIN32__

#include <windows.h>

typedef CRITICAL_SECTION pthread_mutex_t;
typedef HANDLE pthread_t;
#define pthread_mutex_lock EnterCriticalSection
#define pthread_mutex_unlock LeaveCriticalSection
#define pthread_mutex_init(__x__, __y__) InitializeCriticalSection(__x__)
#define pthread_mutex_destroy DeleteCriticalSection

typedef void *(WINAPI *type_thread_routine)(void*);
typedef unsigned long(WINAPI *type_thread_routine_win)(void*);

int pthread_create(pthread_t * thread, const void * attr, type_thread_routine start_routine, void * arg);
int pthread_join(pthread_t thread, void ** value_ptr);
pthread_t pthread_self();
int pthread_equal(pthread_t t1, pthread_t t2);
int pthread_mutex_trylock(pthread_mutex_t *mutex);
int pthread_detach(pthread_t tid);
#else

#include <pthread.h>

#define WINAPI
#define UNREFERENCED_PARAMETER(P) { (P) = (P); } 

#endif

#endif