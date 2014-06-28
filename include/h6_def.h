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

#if defined (__GNUC__) && __GNUC__ > 2 /* since 2.9 */
	# define H6_LIKELY(expr) (__builtin_expect(!!(expr), 1))
	# define H6_UNLIKELY(expr) (__builtin_expect(!!(expr), 0))
#else
	# define H6_LIKELY(expr) (expr)
	# define H6_UNLIKELY(expr) (expr)
#endif

#ifndef REF_DEBUG_TEST
#define MAX_REF_COUNT_VALUE (10000)	/* Maybe enough */
#define REF_DEBUG_TEST(pobj)	\
    do {\
        assert((pobj) && atomic_get(&(pobj)->ref_count) > 0); \
        assert(atomic_get(&(pobj)->ref_count) < MAX_REF_COUNT_VALUE); \
    } while (0)
#endif


#define __str(p) ((char*)(p))

#define BUG_ON(x) \
do {\
	if (x) \
	{\
		fprintf(stderr, "BUG_ON(%s) at file:%s line:%d function:%s thread_id=%lu!\n", \
		#x, __FILE__, __LINE__, __FUNCTION__, pthread_self()); \
		char *_______________________p = 0; *_______________________p = 0; \
	}\
} while (0)


	
#endif
