#include "queue.h"

#include "assert.h"

bool queue_init(queue_t* q, u16 element_size, u16 element_count)
{
    spin_lock_init(&q->lock);
    q->element_count = element_count;
    q->element_size = element_size;
    q->wptr = 0;
    q->rptr = 0;
    q->count = 0;
    q->data = (u8*)calloc(element_count, element_size);
    return q->data != NULL;
}

void queue_reset(queue_t* q)
{
    spin_lock(&q->lock);
    q->wptr = 0;
    q->rptr = 0;
    q->count = 0;
    spin_unlock(&q->lock);
}

static void* element_ptr(queue_t* q, u16 index)
{
    assert(index < q->element_count);
    return q->data + q->element_size * index;
}

bool queue_push(queue_t* q, const void* element)
{
    spin_lock(&q->lock);
    if (queue_count_unsafe(q) == q->element_count)
    {
        spin_unlock(&q->lock);
        return false;
    }
    memcpy(element_ptr(q, q->wptr), element, q->element_size);
    q->wptr = (q->wptr + 1) % q->element_count;
    q->count++;
    spin_unlock(&q->lock);
    return true;
}

bool queue_peek(queue_t* q, void* element)
{
    spin_lock(&q->lock);
    if (queue_count_unsafe(q) == 0 || !element)
    {
        spin_unlock(&q->lock);
        return false;
    }
    memcpy(element, element_ptr(q, q->rptr), q->element_size);
    spin_unlock(&q->lock);
    return true;
}

bool queue_pop(queue_t* q, void* element)
{
    spin_lock(&q->lock);
    if (queue_count_unsafe(q) == 0)
    {
        spin_unlock(&q->lock);
        return false;
    }
    if (element)
    {
        memcpy(element, element_ptr(q, q->rptr), q->element_size);
    }
    q->rptr = (q->rptr + 1) % q->element_count;
    q->count--;
    spin_unlock(&q->lock);
    return true;
}

u16 queue_count_unsafe(queue_t* q)
{
    return q->count;
}

u16 queue_count(queue_t* q)
{
    spin_lock(&q->lock);
    u16 count = queue_count_unsafe(q);
    spin_unlock(&q->lock);
    return count;
}
