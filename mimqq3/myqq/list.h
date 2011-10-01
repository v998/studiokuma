#ifndef _LIST_H
#define _LIST_H

#include "commplatform.h"

typedef struct plist{
	pthread_mutex_t	mutex;
	int		size;
	int		count;
	void**		items;
}plist;

typedef int (*comp_func)(const void *, const void *);
typedef int (*search_func)(const void *, const void *);

int list_create( plist* l, int size );
int list_append( plist* l, void* data );
int list_remove( plist* l, void* data );
void* list_search( plist* l, void*, search_func search );
void list_sort( plist* l, comp_func comp );
void list_cleanup( plist* l );

#endif
