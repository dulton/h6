#ifndef __OBJ_H__
#define __OBJ_H__

#include <stdint.h>
#include <stdio.h>
#include "atomic.h"

#define NAME_SIZE		32			

#ifdef __cplusplus
extern "C" {
#endif

typedef struct __obj    obj_t;
typedef void (*obj_fin)(obj_t *p);

struct __obj
{
	uint8_t		name[NAME_SIZE];
	uint32_t	size;
    atomic_t	ref_count;
	obj_fin		fin;
};

obj_t*  obj_new(uint32_t size, obj_fin fin, const char *name);
void    obj_ref(void *p);
void    obj_unref(void *p);

#ifdef __cplusplus
}
#endif

#endif	//__TINY_RAIN_OBJ_H__
