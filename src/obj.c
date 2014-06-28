#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <trace.h>
#include "obj.h"
#include "h6_def.h"

obj_t*
obj_new(uint32_t size, obj_fin fin, const char *name)
{
	obj_t *p;
	size_t len;
    
	if (size < sizeof(*p))
		return NULL;
        
	p = (obj_t *)malloc(size);
	if (p)
	{
        len = strlen((char *)name);
        if (len >= NAME_SIZE)
            len = NAME_SIZE - 1;
            
		strncpy((char *)(p->name), (char *)name, len);
		p->name[len] = '\0';
        
        p->size = size;
        p->fin = fin;

        atomic_set(&p->ref_count, 1);

        TRACE_DETAIL("%s Create object(%s), current ref count %d\r\n",
            __FUNCTION__, p->name, atomic_get(&p->ref_count));
	}
    
	return p;    
}

static __inline__ void
obj_free(obj_t *p)
{
	if (p->fin)
	{
		(*p->fin)(p);
	}
    
    free(p);
}

void obj_ref(void *_p)
{
	obj_t *p = (obj_t*)_p;
	BUG_ON(atomic_get(&p->ref_count) <= 0);

	atomic_inc(&p->ref_count);

    TRACE_DETAIL("%s Increase object(%s) ref count, current count: %d\r\n",
        __FUNCTION__, p->name, atomic_get(&p->ref_count));
}


void obj_unref(void *_p)
{
	obj_t *p = (obj_t*)_p;
	BUG_ON(atomic_get(&p->ref_count) <= 0);

	if (atomic_dec_and_test_zero(&p->ref_count))
	{
        TRACE_DETAIL("%s Decrease object(%s) ref count, current count: %d\r\n",
            __FUNCTION__, p->name, atomic_get(&p->ref_count));
    
		obj_free(p);
	}
    else
    {
        TRACE_DETAIL("%s Decrease object(%s) ref count, current count: %d\r\n",
            __FUNCTION__, p->name, atomic_get(&p->ref_count));        
    }
    
}
