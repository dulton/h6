#include <stdlib.h>
#include <string.h>
#include "obj.h"

obj_t*
obj_new(uint32_t size, obj_fin fin, uint8_t *name)
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
	}
    
	return p;    
}

void
obj_free(obj_t *p)
{
	if (p->fin)
	{
		(*p->fin)(p);
	}
    
    free(p);
}