#include "ringbuffer.h"

#include <stdbool.h>


static bool _is_empty(ring_buffer_t *ring)
{
    return (ring->rd_i == ring->wr_i);
}



static bool _is_full(ring_buffer_t *ring)
{
    if (ring->wr_i + 1 == ring->rd_i) {
        return true;
    }
    
    if (ring->wr_i == ring->rd_i + ring->maxsize) {
        return true;
    }
    
    return false;
}



void ring_init(ring_buffer_t *ring, unsigned char *addr, unsigned int size)
{
    ring->data = addr;
    ring->rd_i = 0;
    ring->wr_i = 0;
    ring->maxsize = size;
}



int ring_put(ring_buffer_t *ring, unsigned char *data)
{
    if (!ring || !data) {
        return -1;
    }
    
    if (!_is_full(ring)) {
        ring->data[ring->wr_i++] = *data;
        if (ring->wr_i >= ring->maxsize) {
            ring->wr_i = 0;
        }
        return 1;
    }
    
    return 0;
}



int ring_get(ring_buffer_t *ring, unsigned char *data)
{
    if (!ring || !data) {
        return -1;
    }
    
    if (!_is_empty(ring)) {
        *data = ring->data[ring->rd_i++];
        if (ring->rd_i >= ring->maxsize) {
            ring->rd_i = 0;
        }
        return 1;
    }
    
    return 0;
}
