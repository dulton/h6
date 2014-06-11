#ifndef _H6_DEF_H_
#define _H6_DEF_H_

#include <stddef.h>
#include <stdint.h>
#include <errno.h>

#define DEFAULT_PW_WINSIZE  25

#define EUSER		255
#define EKILLED		(EUSER + 1)
#define ESESSION	(EUSER + 2)

typedef enum 
{
    H6_FALSE,
    H6_TRUE
} h6_bool_t;

#define container_of(ptr, struct_type, member) \
	(struct_type*)(((char*)(ptr)) - __offsetof__(struct_type, member))

#if defined (__GNUC__) && __GNUC__ >= 4
        # define __ATTR_WARN_UNUSED_RETSULT__ \
__attribute__((warn_unused_result))
# define __offsetof__(struct_type, member)      \
  ((long)(offsetof(struct_type, member)))
#else
        # define __ATTR_WARN_UNUSED_RETSULT__
# define __offsetof__(struct_type, member)      \
  ((long)(unsigned char*)&((struct_type*)0)->member)
#endif

#define __str(p) ((char*)(p))

	
#endif
