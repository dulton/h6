#ifndef _ATOMIC_H_
#define _ATOMIC_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
#if defined WIN32
typedef volatile unsigned long atomic_t;
# define atomic_set(p, val) InterlockedExchange(p, val)
# define atomic_get(p) InterlockedExchangeAdd(p, 0)
# define atomic_add(p, val) InterlockedExchangeAdd(p, val)
# define atomic_sub(p, val) InterlockedExchangeSubtract(p, val)
# define atomic_inc(p) InterlockedExchangeAdd(p, 1)
# define atomic_dec(p) InterlockedExchangeSubtract(p, 1)
inline int atomic_dec_and_test_zero(p)
{
	InterlockedExchangeSubtract(p, 1);
	
	return (*p == 0) ? 0 : 1;
}
#endif
*/

#if defined (__GNUC__) && __GNUC__ >= 4 /* since 4.1.2 */

#define ATOMIC_INIT			0

typedef volatile int atomic_t;

# define atomic_set(p, val) ((*(p)) = (val))
# define atomic_get(p) __sync_add_and_fetch(p, 0)
# define atomic_add(p, val)  __sync_add_and_fetch(p, val)
# define atomic_sub(p, val)  __sync_sub_and_fetch(p, val)
# define atomic_inc(p) __sync_add_and_fetch(p, 1)
# define atomic_dec(p) __sync_sub_and_fetch(p, 1)
# define atomic_dec_and_test_zero(p) (__sync_sub_and_fetch(p, 1) == 0)

#else

#if defined _ARM_
# include <atomic_armv6.h>
# define atomic_set(p, val) _atomic_set(p, val)
# define atomic_get(p) _atomic_add(0, p)
# define atomic_add(p, val) _atomic_add(val, p)
# define atomic_sub(p, val) _atomic_sub(val, p)
# define atomic_inc(p) _atomic_inc(p)
# define atomic_dec(p) _atomic_dec(p)
# define atomic_dec_and_test_zero(p) _atomic_sub_and_test(1, p)

#elif defined _X86_
# warning "ERROR!!, X86 Arch, Gcc Version < 4.0!!!"
#else
# warning "ERROR!!, Unknown Arch, No Atomic Operation Supported!!!"
#endif 

#endif /* (__GNUC__) && __GNUC__ >= 4 */

#ifdef __cplusplus
}
#endif

#endif
