#ifndef _MEM_BLOCK_H__
#define _MEM_BLOCK_H__

#include "list.h"

#define MEMBLOCK_FLGS_DROPABLE	0x0001
#define MEMBLOCK_SET_SEQ(mb, _seq) ((mb)->seq = _seq)
#define MEMBLOCK_GET_SEQ(mb) ((mb)->seq)
#define MEMBLOCK_DROPABLE(mb) ((mb)->flags&MEMBLOCK_FLGS_DROPABLE)

#ifdef __cplusplus
extern "C" {
#endif

typedef struct __mem_block mb_t;
struct __mem_block
{
    struct list_head list;
    uint8_t *ptr;
    uint32_t seq;
    uint32_t flags;
    uint32_t offset;
    uint32_t size;
    uint32_t b_size;    /* mem real size */
    void *u;
    void (*finalize)(mb_t *mb);
};

void init_memblock_facility( void );
mb_t *alloc_memblock( void );
void free_memblock(mb_t *mb);

mb_t *alloc_gather_memblock(uint32_t size);
mb_t *gather_memblock(mb_t *gather, mb_t *mb);

#ifdef __cplusplus
}
#endif

#endif  //__TINY_RAIN_MEM_BLOCK_H__

