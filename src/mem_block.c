#include <pthread.h>
#include "mem_block.h"

#define MAX_CACHE_COUNT             128
#define MAX_GATHER_MEM              65536

static struct list_head cache_heads;
static pthread_mutex_t  *cache_lock;
static int32_t cache_counts = 0;


void
init_memblock_facility( void )
{
    INIT_LIST_HEAD(&cache_heads);
    
    cache_lock = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(cache_lock, NULL);
}


static __inline__ mb_t*
__alloc_memblock( void )
{
    struct list_head *l;

    if (cache_counts > 0)
    {
        l = cache_heads.next;
        list_del(l);
        --cache_counts;
        return container_of(l, mb_t, list);
    }

    return (mb_t*)malloc(sizeof(mb_t));
}


mb_t *
alloc_memblock( void )
{
    mb_t *mb;

    pthread_mutex_lock(cache_lock);
    mb = __alloc_memblock();
    pthread_mutex_unlock(cache_lock);

    return mb;
}


static __inline__ void
__free_memblock(mb_t *mb)
{
    if (cache_counts < MAX_CACHE_COUNT)
    {
        list_add(&mb->list, &cache_heads);
        ++cache_counts;
        return;
    }

    free(mb);
}


static __inline__ void
_free_memblock(mb_t *mb)
{
    pthread_mutex_lock(cache_lock);
    __free_memblock(mb);
    pthread_mutex_unlock(cache_lock);
}


void
free_memblock(mb_t *mb)
{
    (*mb->finalize)(mb);
    _free_memblock(mb);
}


static void
fin_gather_memb_block(mb_t *mb)
{
    free(mb->ptr);
}


mb_t *
alloc_gather_memblock(uint32_t size)
{
    uint8_t *ptr;
    mb_t *mb;

    if (size > MAX_GATHER_MEM)
        return NULL;

    ptr = (uint8_t *)malloc(size);
    if (ptr)
    {
        mb = alloc_memblock();
        if (mb)
        {
            INIT_LIST_HEAD(&mb->list);
            mb->ptr = ptr;
            mb->seq = 0;
            mb->offset = 0;
            mb->size = 0;
            mb->b_size = size;
            mb->u = NULL;
            mb->finalize = fin_gather_memb_block;
            return mb;
        }

        free(ptr);
    }

    return NULL;
}


mb_t *
gather_memblock(mb_t *gather, mb_t *mb)
{
    uint32_t size, left;

    size = mb->size - mb->offset;
    left = gather->b_size - gather->size;

    if (size <= left)
    {
        memcpy(&gather->ptr[gather->size], &mb->ptr[mb->offset], size);
        gather->size += size;
        gather->seq = mb->seq;
        return gather;
    }

    return NULL;
}


//:~ End

