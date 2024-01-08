#ifndef __RINGBUFFER_H
#define __RINGBUFFER_H


#define     DEV_WRDR(handle, data)          (void)0


typedef struct {
    unsigned char   *data;
    unsigned int    rd_i;
    unsigned int    wr_i;
    unsigned int    maxsize;
} ring_buffer_t;


typedef struct {
    ring_buffer_t   tx_ring;
    ring_buffer_t   rx_ring;
    unsigned int    status;
    void            *handle;
} ring_dev_t;



void ring_init(ring_buffer_t *ring, unsigned char *addr, unsigned int size);
int ring_put(ring_buffer_t *ring, unsigned char data);
int ring_get(ring_buffer_t *ring, unsigned char *data);

void ring_dev_init(ring_dev_t *dev, void *handle, unsigned char *txdata, unsigned char *rxdata, unsigned int size);
int ring_dev_tx(ring_dev_t *dev, unsigned char *data, unsigned int size);
int ring_dev_rx(ring_dev_t *dev, unsigned char *data, unsigned int size);
int ring_dev_writetorx(ring_dev_t *dev, unsigned char data);
int ring_dev_readfromtx(ring_dev_t *dev, unsigned char *data);
#endif
