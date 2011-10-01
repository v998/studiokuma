#include "StdAfx.h"

#ifdef __WIN32__

#include "commplatform.h"

int pthread_create(pthread_t * thread, const void * attr, type_thread_routine start_routine, void * arg)
{
    DWORD dwThreadId; 
	
    *thread = CreateThread( 
        NULL,
        0,
        (type_thread_routine_win)start_routine,
        arg,
        0,
        &dwThreadId);

	if (*thread == NULL) 
		return -1;

	return 0;
}

int pthread_join(pthread_t thread, void ** value_ptr)
{
	WaitForSingleObject(thread, INFINITE);
	return 0;
}

int pthread_detach(pthread_t tid)
{
	return 0;
}

pthread_t pthread_self()
{
	return GetCurrentThread();
}

int pthread_equal(pthread_t t1, pthread_t t2)
{
	return((int)t1 == (int)t2);
}

int pthread_mutex_trylock(pthread_mutex_t *mutex)
{
	if(TryEnterCriticalSection(mutex))
		return 0;

	return -1;	//On success, pthread_mutex_trylock() returns 0. On error, one of the following values is returned: 
}

#endif

