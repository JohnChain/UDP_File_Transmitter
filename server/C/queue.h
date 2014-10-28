#if !defined(_REVBOT_QUEUE_H)
#define _REVBOT_QUEUE_H

#include <pthread.h>

typedef struct _queue_item
{
    char *filename;
	size_t length;
	char *data;
    int pack_no;
	
	struct _queue_item *next;
} QUEUE_ITEM;

typedef struct _queue
{
	size_t numitems;
	QUEUE_ITEM *head;
	QUEUE_ITEM *tail;
	
	pthread_mutex_t modify_mutex;
	pthread_mutex_t read_mutex;
} QUEUE;

extern QUEUE *Initialize_Queue(void);
extern void Append_Queue_Item(QUEUE *, char*, int, void *, size_t);
extern QUEUE_ITEM *Pop_Queue_Item(QUEUE *);
extern void Free_Queue_Item(QUEUE_ITEM *);
extern void Free_Queue(QUEUE *queue);
extern void Print_Queue_Item(QUEUE *queue);

#endif
