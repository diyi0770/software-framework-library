#include "ringbuffer.h"

#define     DEV_S_SF        (1U)


static int _is_empty(ring_buffer_t *ring)
{
    return (ring->rd_i == ring->wr_i);
}



static int _is_full(ring_buffer_t *ring)
{
    if (ring->wr_i + 1 == ring->rd_i) {
        return 1;
    }
    
    if (ring->wr_i == ring->rd_i + ring->maxsize) {
        return 1;
    }
    
    return 0;
}



void ring_init(ring_buffer_t *ring, unsigned char *addr, unsigned int size)
{
    ring->data = addr;
    ring->rd_i = 0;
    ring->wr_i = 0;
    ring->maxsize = size;
}



/**
 * @brief 
 * @param ring 
 * @param data 
 * @return int 
 * @author liuhai
 * @date 2024-01-02
 */
int ring_put(ring_buffer_t *ring, unsigned char data)
{
    if (!ring) {
        return -1;
    }
    
    if (!_is_full(ring)) {
        ring->data[ring->wr_i++] = data;
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



void ring_dev_init(ring_dev_t *dev, void *handle, unsigned char *txdata, unsigned char *rxdata, unsigned int size)
{
    ring_init(&dev->tx_ring, txdata, size);
    ring_init(&dev->rx_ring, rxdata, size);
    dev->handle = handle;
    dev->status = 0;
}



/**
 * @brief 发送数据
 * @param dev 
 * @param data 
 * @param size 
 * @return int 
 * @author liuhai
 * @date 2024-01-02
 */
int ring_dev_tx(ring_dev_t *dev, unsigned char *data, unsigned int size)
{
    unsigned int    sdsize = 0;
    unsigned char   sddata;
    
    while (sdsize < size) {
        if (ring_put(&dev->tx_ring, data[sdsize]) < 1) {
            break;
        }
        sdsize++;
    }
    
    if (dev->status & DEV_S_SF) {
        if (ring_get(&dev->tx_ring, &sddata) < 1) {
            return sdsize;
        }
        
        dev->status &= ~DEV_S_SF;
        DEV_WRDR(dev->handle, sddata);
    }
    
    return sdsize;
}



/**
 * @brief 接收数据
 * @param dev 
 * @param data 
 * @param size 
 * @return int 
 * @author liuhai
 * @date 2024-01-02
 */
int ring_dev_rx(ring_dev_t *dev, unsigned char *data, unsigned int size)
{
    unsigned int    rvsize = 0;
    
    while (rvsize < size) {
        if (ring_get(&dev->rx_ring, data + rvsize) < 1) {
            break;
        }
        rvsize++;
    }
    
    return rvsize;
}



/**
 * @brief 在接收中断里调用，将数据填入设备的接收缓存中
 * @param dev 
 * @param data 
 * @return int 
 * @author liuhai
 * @date 2024-01-02
 */
int ring_dev_writetorx(ring_dev_t *dev, unsigned char data)
{
    return ring_put(&dev->rx_ring, data);
}



/**
 * @brief 在发送中断里调用，从设备的发送缓存中取出数据
 * @param dev 
 * @param data 
 * @return int 
 * @author liuhai
 * @date 2024-01-02
 */
int ring_dev_readfromtx(ring_dev_t *dev, unsigned char *data)
{
    int ret;
    
    ret = ring_get(&dev->tx_ring, data);
    if (ret < 1) {
        dev->status |= DEV_S_SF;
    }
    
    return ret;
}
