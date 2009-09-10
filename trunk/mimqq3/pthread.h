#ifndef PTHREAD_H
#define PTHREAD_H
#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif
	void pthread_mutex_init(LPHANDLE hMutex,LPVOID);
	BOOL pthread_mutex_trylock(HANDLE hMutex);
	void pthread_mutex_lock(HANDLE hMutex);
	void pthread_mutex_unlock(HANDLE hMutex);
	void pthread_mutex_destroy(HANDLE hMutex);
	// int pthread_create(LPHANDLE hThread, LPVOID, void *(*start_routine)(void*), void *arg), LPVOID arg);
	#define pthread_join(x,y)
#ifdef __cplusplus
}
#endif

typedef HANDLE pthread_mutex_t;
typedef int pthread_t;
#endif // PTHREAD_H
