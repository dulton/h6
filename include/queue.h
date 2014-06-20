/*
 * queue.h
 * Copyright by Zhang Shiyong, 2014. shiyong.zhang.cn@outlook.com
 */
#ifndef __QUEUE_H__
#define __QUEUE_H__

#ifdef __cplusplus
extern "C" {
#endif

/*
 * List definitions.
 */
typedef struct _list list_t;
struct _list
{
	void *data;
	list_t *next, *prev;
};

typedef int (*func_compare_custom)(void *data_orig, void *data_custom);
typedef void (*func_visit_custom)(void *data_orig, void *data_custom);

/*
 * List functions.
 */
void list_free(list_t *list);
list_t *list_insert_head(list_t *list, void *data);
list_t *list_insert_tail(list_t *list, void *data);
list_t *list_remove(list_t *list, void *data);
list_t *list_remove_all(list_t *list, void *data);
list_t *list_remove_link(list_t *list, list_t *link);
list_t *list_delete_link(list_t *list, list_t *link);
list_t *list_concat(list_t *list1, list_t *list2);
list_t *list_find(list_t *list, void *data);
list_t *list_find_custom(list_t *list, void *data, func_compare_custom func);

/*
 * List access methods.
 */
void list_foreach(list_t *list, func_visit_custom func, void *data);
list_t *list_first(list_t *list);
list_t *list_last(list_t *list);

#define list_data(list) (((list_t*)list)->data)
#define list_next(list) (list ? ((list_t*)list)->next : NULL)
#define list_prev(list) (list ? ((list_t*)list)->prev : NULL)


/*
 * queue definitions.
 */
typedef struct _queue queue_t;
struct _queue
{
	list_t			*head;	// head node of queue
	list_t			*tail;	// tail node of queue
	unsigned int	count;	// total nodes of queue
};

/*
 * queue operation
 */
queue_t *		queue_new( void );
void 			queue_init(queue_t *queue);
void 			queue_free(queue_t *queue);
void 			queue_clear(queue_t *queue);
unsigned int	queue_length(queue_t *queue);
void 			queue_push_tail(queue_t *queue, void *data);
void * 			queue_pop_head(queue_t *queue);

void *			queue_pop_tail(queue_t *queue);
void 			queue_push_head(queue_t *queue, void *data);
void 			queue_foreach(queue_t *queue, func_visit_custom func, void *data);

#ifdef __cplusplus
}
#endif

#endif // __QUEUE_H__