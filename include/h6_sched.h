/* *
 * This file implements event scheduler.
 *
 * Copyright (C) 2014 Shiyong Zhang <shiyong.zhang.cn@gmail.com>
 * See COPYING for more details
 */

#ifndef __H6_SCHED_H__
#define __H6_SCHED_H__

#include <stdint.h>
#include "h6_ev.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _h6_loop_scher h6_scher_t;

h6_scher_t *h6_sched_new(int32_t loops);
void h6_sched_free(h6_scher_t *sched);

int32_t h6_sched_add(h6_scher_t *sched, h6_ev_t *src, uint32_t weight);
int32_t h6_sched_remove(h6_scher_t *sched, h6_ev_t *src);

#ifdef __cplusplus
}
#endif

#endif	/* __H6_SCHED_H__ */
