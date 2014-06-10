#ifndef __OBJ_H__
#define __OBJ_H__

#include <stdint.h>
#include <stdio.h>

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
	obj_fin		fin;
};

obj_t*  obj_new(uint32_t size, obj_fin fin, uint8_t *name);
void    obj_free(obj_t *p);

#define BUG_ON(x) \
do {\
	if (x) \
	{\
		fprintf(stderr, "BUG_ON(%s) at file:%s line:%d function:%s!\n", #x, __FILE__, __LINE__, __FUNCTION__); \
		char *_______________________p = 0; *_______________________p = 0; \
	}\
} while (0)

#ifdef __cplusplus
}
#endif

#endif	//__TINY_RAIN_OBJ_H__
