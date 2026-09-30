#ifndef DS5DONGLE_QUEUE_H
#define DS5DONGLE_QUEUE_H

#include "spinlock.h"
#include "typedef.h"

//*********************************************************************************//
//                                固定元素环形队列                                  //
//*********************************************************************************//

typedef struct {
    spinlock_t lock;
    u8 *data;
    u16 wptr;
    u16 rptr;
    u16 element_size; // 单个元素的大小
    u16 element_count; // 队列总元素个数
    u16 count; // 当前队列元素个数
} queue_t;

bool queue_init(queue_t *q, u16 element_size, u16 element_count);
void queue_reset(queue_t *q);
bool queue_push(queue_t *q, const void *element);
bool queue_peek(queue_t *q, void *element);
bool queue_pop(queue_t *q, void *element);
u16 queue_count(queue_t *q);
u16 queue_count_unsafe(queue_t *q);

#endif
