/*
 * queue.c
 * Copyright by Zhang Shiyong, 2014. shiyong.zhang.cn@outlook.com
 */
#include <assert.h>
#include <stdlib.h>
#include <string.h> 
#include "queue.h"
 
static list_t *
list_alloc( void )
{
	list_t *list = (list_t *)calloc(1, sizeof(list_t));
	
	return list;
}

/* 释放单个结点 */
void
list_free_1(list_t *list)
{
	if (list)
		free(list);
}

/* 释放所有结点 */
void
list_free(list_t *list)
{
	list_t *next;

	while (list)
	{
		next = list->next;
		list_free_1(list);
		list = next;
	}
}

list_t *
list_insert_head(list_t *list, void *data)
{
	list_t *new_list = list_alloc();

	if (new_list == NULL)
		return NULL;
	
	new_list->data = data;
	new_list->next = list;
	new_list->prev = NULL;

	if (list)
		list->prev = new_list;
	
	return new_list;
}


list_t *
list_insert_tail(list_t *list, void *data)
{
	list_t *last;	
	list_t *new_list = list_alloc();

	if (new_list == NULL)
		return NULL;

	new_list->data = data;
	new_list->next = NULL;

	if (list)
	{
		last = list_last(list);
		new_list->prev = last;
		last->next = new_list;
		return list;
	}
	else
	{
		new_list->prev = NULL;
		return new_list;
	}
}


list_t *list_remove(list_t *list, void *data)
{
	list_t *head = list;

	while (list)
	{
		if (list->data != data)
			list = list->next;
		else
		{
			if (list->prev)
				list->prev->next = list->next;
			if (list->next)
				list->next->prev = list->prev;

			if (head == list)
				head = head->next;

			list_free_1(list);
			break;
		}
	}

	return head;
}


list_t *list_remove_all(list_t *list, void *data)
{
	list_t *next, *head = list;

	while (list)
	{
		if (list->data != data)
			list = list->next;
		else
		{
			next = list->next;

			if (list->prev)
				list->prev->next = next;
			else
				head = next;

			if (next)
				next->prev = list->prev;

			list_free_1(list);
			list = next;
		}
	}

	return head;	
}


list_t *
list_remove_link(list_t *list, list_t *link)
{
	if (link)
	{
		if (link->next)
			link->next->prev = link->prev;
		if (link->prev)
			link->prev->next = link->next;

		if (link == list)
			list = list->next;

		link->next = NULL;
		link->prev = NULL;
	}

	return list;
}

list_t *
list_concat(list_t *list1, list_t *list2)
{
	list_t *last;

	if (list2)
	{
		last = list_last(list1);
		if (last)
			last->next = list2;
		else
			list1 = list2;
		list2->prev = last;
	}

	return list1;	
}


list_t *
list_find(list_t *list, void *data)
{
	while (list)
	{
		if (list->data == data)
			break;
		list = list->next;
	}

	return list;
}


list_t *
list_find_custom(list_t *list, void *data,
	func_compare_custom func)
{
	while (list)
	{
		if (!(*func)(list->data, data))
			break;
		list = list->next;
	}

	return list;
}


void
list_foreach(list_t *list, func_visit_custom func, void *data)
{
	list_t *next;

	while (list)
	{
		next = list->next;
		(*func)(list->data, data);
		list = next;
	}
}

list_t *
list_first(list_t *list)
{
	if (list)
	{
		while (list->prev)
			list = list->prev;
	}

	return list;
}


list_t *
list_last(list_t *list)
{
	if (list)
	{
		while (list->next)
			list = list->next;
	}

	return list;
}


void
queue_init(queue_t *queue)
{
	assert(queue != NULL);

	memset(queue, 0, sizeof(queue_t));
}

queue_t *
queue_new( void )
{
	queue_t *queue = (queue_t *)calloc(1, sizeof(queue_t));
	
	return queue;
}


void
queue_free(queue_t *queue)
{
	assert(queue != NULL);

	list_free(queue->head);
	free(queue);
}

void
queue_clear(queue_t *queue)
{
	assert(queue != NULL);

	list_free(queue->head);
	queue_init(queue);
}

unsigned int
queue_length(queue_t *queue)
{
	assert(queue != NULL);

	return queue->count;
}


void queue_push_tail(queue_t *queue, void *data)
{
	assert(queue != NULL);

	queue->tail = list_insert_tail(queue->tail, data);
	if (queue->tail->next)
		queue->tail = queue->tail->next;
	else
		queue->head = queue->tail;
	++queue->count;
}


void* queue_pop_head(queue_t *queue)
{
	list_t *node;
	void *data;
	assert(queue != NULL);

	if (queue->head)
	{
		node = queue->head;
		data = list_data(node);

		queue->head = list_next(node);
		if (queue->head)
			queue->head->prev = NULL;
		else
			queue->tail = NULL;

		--queue->count;
		list_free_1(node);
		return data;
	}

	return NULL;
}


void *queue_pop_tail(queue_t *queue)
{
	list_t *node;
	void *data;
	
	assert(queue != NULL);

	if(!queue->tail)	return NULL;

	node = queue->tail ;
	data = list_data(node);
	
	if(queue->tail == queue->tail )
	{
		queue->head  = queue->tail = NULL; 
	}
	else
	{
		queue->tail = list_prev(node);
	}
	--queue->count;
	list_free_1(node);
	return data;
}

void queue_push_head(queue_t *queue, void *data)
{
	assert(queue != NULL);

	if(queue->head)
	{
		queue->head = list_insert_head(queue->head, data);
	}
	else
	{
		queue->head = list_insert_head(queue->head, data);
		queue->tail = queue->head;
	}
	++queue->count;
}

void queue_foreach(queue_t *queue, func_visit_custom func, void *data)
{
	assert(queue != NULL);
	
	if(queue->head)
	{
		list_foreach(queue->head, func, data);
	}
}
