#ifndef __RINGBUFFER_H
#define __RINGBUFFER_H



typedef struct _ring_buffer_t   ring_buffer_t;
typedef struct _ring_dev_t      ring_dev_t;



#include "ringconfig.h"



void ring_dev_init(ring_dev_t *dev, void *handle, unsigned char *txdata, unsigned int tsize, unsigned char *rxdata, unsigned int rsize);
unsigned int ring_dev_tx(ring_dev_t *dev, void *data, unsigned int size);
unsigned int ring_dev_rx(ring_dev_t *dev, void *data, unsigned int size);
void ring_dev_rxInISR(ring_dev_t *dev);
void ring_dev_txInISR(ring_dev_t *dev);



void ring_init(ring_buffer_t *ring, unsigned char *addr, unsigned int size);
unsigned int ring_put(ring_buffer_t *ring, unsigned char data);
unsigned int ring_get(ring_buffer_t *ring, unsigned char *data);
unsigned int ring_level(ring_buffer_t *ring);
#endif
