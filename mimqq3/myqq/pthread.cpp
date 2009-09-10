#include <StdAfx.h>

extern "C" {
	void pthread_mutex_init(LPHANDLE hMutex,LPVOID) {
		*hMutex=CreateMutex(NULL,TRUE,NULL);
	}

	void pthread_mutex_lock(HANDLE hMutex) {
		WaitForSingleObject(hMutex,INFINITE);
	}

	void pthread_mutex_unlock(HANDLE hMutex) {
		ReleaseMutex(hMutex);
	}

	BOOL pthread_mutex_trylock(HANDLE hMutex) {
		return WaitForSingleObject(hMutex,0)==WAIT_OBJECT_0;
	}

	void pthread_mutex_destroy(HANDLE hMutex) {
		// CloseHandle(hMutex);
	}
	/*
	int pthread_create(LPHANDLE hThread, LPVOID, void *(*start_routine)(void*), void *arg), LPVOID arg) {
		*hThread=(HANDLE)mir_forkthread((pThreadFunc)start_routine,arg);
		return 0;
	}
	*/
}
