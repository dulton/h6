#ifndef _TRACE_H_
#define _TRACE_H_

#include <stdint.h>
#include <stdarg.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>
#include "atomic.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _async_ring_buffer
{    
    uint32_t        size;
    char *          start;

    uint32_t        read_pos;
    uint32_t        read_reverse;   // identify times read_pos reversed

    uint32_t        write_pos;
    uint32_t        write_reverse;  // identify times write_pos reversed

    pthread_t       thread;
    pthread_mutex_t mutex;
    sem_t*          sem;
    atomic_t        exit;   // 0 - don't exit; 1 - thread exit
    atomic_t        level;
    
} async_ring_buf_t;

extern async_ring_buf_t* async_ring_buf_new(uint32_t size);
extern void async_ring_buf_free(async_ring_buf_t *buf);
extern uint32_t async_ring_buf_write(async_ring_buf_t *buf, char *string, size_t len);

//========================================================================

#define TRACE_LINE_MAX_SIZE  4096
#define TRACE_DETAIL_LEVEL   0
#define TRACE_DEBUG_LEVEL    1
#define TRACE_TRACE_LEVEL    2
#define TRACE_WARNING_LEVEL  3
#define TRACE_ERROR_LEVEL    6

#define TRACE_SUPPORT_INIT()    do {async_trace_init();} while(0)
#define TRACE_SUPPORT_UNINIT()      do {async_trace_destroy();} while(0)

#define TRACE_ENTER_FUNCTION \
    do {trace(TRACE_DETAIL_LEVEL, "%s '%s', %s:%d\n", \
        "Entering function", __FUNCTION__, __FILE__, __LINE__); \
    } while(0)
#define TRACE_EXIT_FUNCTION \
    do {trace(TRACE_DETAIL_LEVEL, "%s '%s', %s:%d\n", \
        "Exiting funcion", __FUNCTION__, __FILE__, __LINE__); \
    } while(0)
    
#define TRACE_DETAIL(format, ...)       do {trace(TRACE_DETAIL_LEVEL, format, ## __VA_ARGS__);} while(0)
#define TRACE_DEBUG(format, ...)        do {trace(TRACE_DEBUG_LEVEL, format, ## __VA_ARGS__);} while(0)
#define TRACE_TRACE(format, ...)        do {trace(TRACE_TRACE_LEVEL, format, ## __VA_ARGS__);} while(0) 
#define TRACE_WARNING(format, ...)      do {trace(TRACE_WARNING_LEVEL, format, ## __VA_ARGS__);} while(0)
#define TRACE_ERROR(format, ...)        do {trace(TRACE_ERROR_LEVEL, format, ## __VA_ARGS__);} while(0)
#define TRACE_DEBUG_DUMP(buf, size)     do {trace_dump(TRACE_DEBUG_LEVEL, buf, size);} while(0)
#define TRACE_TRACE_DUMP(buf, size)     do {trace_dump(TRACE_TRACE_LEVEL, buf, size);} while(0)
#define TRACE_WARNING_DUMP(buf, size)   do {trace_dump(TRACE_WARNING_LEVEL, buf, size);} while(0)
#define TRACE_ERROR_DUMP(buf, size)     do {trace_dump(TRACE_ERROR_LEVEL, buf, size);} while(0)

#define TRACE_ADJUST_LEVEL(level)       do {trace_adjust(level);} while(0)

//extern async_ring_buf_t *output;
extern int async_trace_init(void);
extern void async_trace_destroy(void);
extern void trace(int level, const char *format, ...);
extern void trace_dump(int level, char *buffer, size_t size);
extern void trace_adjust(int level);

#ifdef __cplusplus
}
#endif

#endif
