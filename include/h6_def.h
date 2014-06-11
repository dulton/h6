#ifndef _H6_DEF_H_
#define _H6_DEF_H_

#include <stddef.h>
#include <stdint.h>

#define DEFAULT_PW_WINSIZE  25

typedef enum 
{
    H6_FALSE,
    H6_TRUE
} h6_bool_t;

#define container_of(ptr, struct_type, member) \
	(struct_type*)(((char*)(ptr)) - __offsetof__(struct_type, member))
	
#endif