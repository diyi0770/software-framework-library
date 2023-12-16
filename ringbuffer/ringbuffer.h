#ifndef __RINGBUFFER_H
#define __RINGBUFFER_H

typedef struct {
    unsigned char   *data;
    unsigned int    rd_i;
    unsigned int    wr_i;
    unsigned int    maxsize;
} ring_buffer_t;


void ring_init(ring_buffer_t *ring, unsigned char *addr, unsigned int size);
int ring_put(ring_buffer_t *ring, unsigned char *data);
int ring_get(ring_buffer_t *ring, unsigned char *data);

#endif
