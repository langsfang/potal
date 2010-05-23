#ifndef _MUTEX_H
#define _MUTEX_H

#ifdef __cplusplus
extern "C" {
#endif
#ifdef HAVE_PTHREAD
#include <pthread.h>
#define MT(ptr) ((pthread_mutex_t *)ptr)
#define MUTEX_INIT(mlock) 												\
do{         														\
    if((mlock = (pthread_mutex_t *)calloc(1, sizeof(pthread_mutex_t)))) 						\
        pthread_mutex_init(MT(mlock), NULL);										\
}while(0)
#define MUTEX_LOCK(mlock) ((mlock) ? pthread_mutex_lock(MT(mlock)): -1)
#define MUTEX_UNLOCK(mlock) ((mlock) ? pthread_mutex_unlock(MT(mlock)): -1)
#define MUTEX_DESTROY(mlock) 												\
do{															\
	if(mlock)													\
	{														\
		pthread_mutex_destroy(MT(mlock)); 									\
		free(MT(mlock));											\
		mlock = NULL;												\
	}														\
}while(0)
#else
#define __MUTEX__WAIT__  100000
#define MUTEX_INIT(mlock) 
#define MUTEX_LOCK(mlock) (0)
#define MUTEX_UNLOCK(mlock) (0)
#define MUTEX_DESTROY(mlock)
#endif
#ifdef __cplusplus
 }
#endif

#endif
