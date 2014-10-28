#if !defined(_REVBOT_QUEUE_H)
#define _REVBOT_QUEUE_H

#include <pthread.h>

typedef struct _queue_item
{
	char *action;
	char *data;
	size_t sz;
	
	struct _queue_item *next;
} QUEUE_ITEM;

typedef struct _queue
{
	size_t numitems;
	QUEUE_ITEM *items;
	
	pthread_mutex_t modify_mutex;
	pthread_mutex_t read_mutex;
} QUEUE;

extern QUEUE *Initialize_Queue(void);
extern void Add_Queue_Item(QUEUE *, char *, void *, size_t);
extern QUEUE_ITEM *Get_Queue_Item(QUEUE *);
extern void Free_Queue_Item(QUEUE_ITEM *);
extern void Free_Queue(QUEUE *queue);
extern void Print_Queue_Item(QUEUE *queue);

#endif
