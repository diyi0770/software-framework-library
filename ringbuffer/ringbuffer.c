#include "ringbuffer.h"


/**
 * @brief 使能发送中断
 */
#ifndef DEV_ENABLE_TXIT
#error "please define the DEV_ENABLE_TXIT(handle) used to enable dev tx interrupt!"
#endif

/**
 * @brief 关闭发送中断
 */
#ifndef DEV_DISABLE_TXIT
#error "please define the DEV_DISABLE_TXIT(handle) used to disable dev tx interrupt!"
#endif

/**
 * @brief 使能接收中断
 */
#ifndef DEV_ENABLE_RXIT
#error "please define the DEV_ENABLE_RXIT(handle) used to enable dev rx interrupt!"
#endif

/**
 * @brief 关闭接收中断
 */
#ifndef DEV_DISABLE_RXIT
#error "please define the DEV_DISABLE_RXIT(handle) used to disable dev rx interrupt!"
#endif

/**
 * @brief 写发送数据寄存器
 */
#ifndef DEV_WR_DR
#error "please define the DEV_WR_DR(handle, data) used to send data!"
#endif

/**
 * @brief 读接收数据寄存器
 */
#ifndef DEV_RD_DR
#error "please define the DEV_RD_DR(handle) used to used to read data!"
#endif

/**
 * @brief 判断设备硬件发送FIFO是否已满
 */
#ifndef IS_DEV_TXFIFO_FULL
#error "please define the IS_DEV_TXFIFO_FULL(handle) used to determine whether txfifo is full!"
#endif

/**
 * @brief 判断设备硬件接收FIFO是否为空
 */
#ifndef IS_DEV_RXFIFO_EMPTY
#error "please define the IS_DEV_RXFIFO_EMPTY(handle) used to determine whether rxfifo is empty!"
#endif


#define DEV_STAT_TXIE          (1U << 0)



static inline int _is_empty(ring_buffer_t *ring)
{
    return (ring->rd_i == ring->wr_i);
}



static inline int _is_full(ring_buffer_t *ring)
{
    /* 队列已满的情况1: 写指针之后是读指针 */
    if (ring->wr_i + 1 == ring->rd_i) {
        return 1;
    }
    
    /* 队列已满的情况2: 读指针在缓存区起始位置，写指针在缓冲区结束位置+1 */
    if (ring->wr_i == ring->rd_i + ring->maxsize) {
        return 1;
    }
    
    return 0;
}



static inline int _get_level(ring_buffer_t *ring)
{
    if (ring->wr_i >= ring->rd_i) {
        return ring->rd_i - ring->wr_i;
    }
    
    return ring->wr_i + ring->maxsize - ring->rd_i;
}



/**
 * @brief 初始化ringbuffer
 * @param ring ringbuffer地址
 * @param addr 数据缓存区起始地址
 * @param size 缓存区大小，单位字节
 * @author diyi12
 * @date 2024-01-10
 */
void ring_init(ring_buffer_t *ring, unsigned char *addr, unsigned int size)
{
    ring->data = addr;
    ring->rd_i = 0;
    ring->wr_i = 0;
    ring->maxsize = size;
}



/**
 * @brief 将数据写入ringbuffer
 * @param ring 句柄
 * @param data 写入数据
 * @return unsigned int 成功返回1，失败返回0
 * @author diyi12
 * @date 2024-01-10
 */
unsigned int ring_put(ring_buffer_t *ring, unsigned char data)
{
    if (!ring) {
        return 0;
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



/**
 * @brief 从ringbuffer中取出数据
 * @param ring 句柄
 * @param data 存放数据的地址
 * @return unsigned int 成功返回1，失败返回0
 * @author diyi12
 * @date 2024-01-10
 */
unsigned int ring_get(ring_buffer_t *ring, unsigned char *data)
{
    if (!ring || !data) {
        return 0;
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



/**
 * @brief 获取当前ringbuffer中元素的个数
 * @param ring 句柄
 * @return unsigned int 返回ringbuffer中元素的个数
 * @author diyi12
 * @date 2024-01-10
 */
unsigned int ring_level(ring_buffer_t *ring)
{
    if (!ring) {
        return 0;
    }
    
    return _get_level(ring);
}



/**
 * @brief 初始化ringbuffer设备的控制句柄
 * @param dev 句柄
 * @param handle 设备的底层控制句柄，用于开关中断、判断硬件FIFO空满以及读写设备数据寄存器
 * @param txdata 发送缓存区, 可以为NULL, 此时发送实现变为轮询
 * @param tsize 发送缓存区大小(byte), 实际的最大深度为tsize-1, 当tisze<2时, 发送实现变为轮询
 * @param rxdata 接收缓存区, 可以为NULL, 此时接收实现变为轮询
 * @param rsize 接收缓存区大小(byte), 实际的最大深度为rsize-1, 当risze<2时, 接收实现变为轮询
 * @author diyi12
 * @date 2024-01-10
 */
void ring_dev_init(ring_dev_t *dev, void *handle, unsigned char *txdata, unsigned int tsize, unsigned char *rxdata, unsigned int rsize)
{
    ring_init(&dev->tx_ring, txdata, tsize);
    ring_init(&dev->rx_ring, rxdata, rsize);
    dev->handle = handle;
    dev->status = 0;
}



/**
 * @brief 设备发送数据, 优先以轮询方式发送, 当硬件FIFO已满打开发送中断进行发送
 * @param dev 句柄
 * @param data 要发送的数据
 * @param size 数据大小(byte)
 * @return unsigned int 返回成功发送地字节数
 * @author diyi12
 * @date 2024-01-10
 */
unsigned int ring_dev_tx(ring_dev_t *dev, const unsigned char *data, unsigned int size)
{
    unsigned int    sdsize = 0;
    
    /**
     * 没有打开发送中断时，优先使用硬件FIFO发送数据
    */
    if (!(dev->status & DEV_STAT_TXIE)) {
        while (sdsize < size) {
            if (IS_DEV_TXFIFO_FULL(dev->handle)) {
                break;
            }
            
            DEV_WR_DR(dev->handle, data[sdsize]);
            sdsize++;
        }
        
        if (sdsize >= size) {
            return sdsize;
        }
    }
    
    /**
     * 硬件FIFO已填满但数据没写完时，剩余数据写入缓存区
    */
    while (sdsize < size) {
        if (ring_put(&dev->tx_ring, data[sdsize]) < 1) {
            break;
        }
        sdsize++;
    }
    
    /** 
     * 此处再次判断发送中断使能状态是为了处理以下情况:
     * 若在写入发送缓存期间进入发送中断，由于发送中断读空发送缓存后会关闭发送中断，导致
     * 退出中断后写入缓存的数据不会被中断读取并发送，所以此处需要再次判断是否打开了发送
     * 中断
     */
    if (!(dev->status & DEV_STAT_TXIE)) {
        
        /** 
         * 通过发送中断发送缓存区中的数据，若缓存区无数据也就没有开中断的必要
         */
        if (ring_level(&dev->tx_ring) < 1) {
            return sdsize;
        }
        
#if 0
        /**
         * 如果打开发送中断后出现硬件FIFO已空但未触发中断，可尝试打开此处代码进行修复
         */
        unsigned char   tmp;
        if (!IS_DEV_TXFIFO_FULL(dev->handle)) {
            ring_get(&dev->tx_ring, &tmp);
            DEV_WR_DR(dev->handle, data[sdsize]);
        }
#endif
        
        dev->status |= DEV_STAT_TXIE;
        DEV_ENABLE_TXIT(dev->handle);
    }
    
    return sdsize;
}



/**
 * @brief 设备接收数据, 接收缓存中的数据不够时会关闭中断并以轮询方式读取硬件FIFO, 然后
 *          重新打开中断。注意, 当读取完指定的字节数后缓存仍为满时不会打开中断
 * 
 * @param dev 句柄
 * @param data 存放数据的起始地址
 * @param size 需要接收的数据大小(byte)
 * @return unsigned int 返回成功接收的字节数
 * @author diyi12
 * @date 2024-01-10
 */
unsigned int ring_dev_rx(ring_dev_t *dev, unsigned char *data, unsigned int size)
{
    unsigned int    rvsize = 0;
    
    /**
     * 优先从接收缓存区中读取数据
    */
    while (rvsize < size) {
        if (ring_get(&dev->rx_ring, data+rvsize) < 1) {
            break;
        }
        rvsize++;
    }
    
    /**
     * 接收缓存区已读空但数据量仍不够时关闭中断, 以轮询方式读取
    */
    DEV_DISABLE_RXIT(dev->handle);
    
    /**
     * 再读一次接收缓存区是为了处理以下情况:
     * 若在关闭中断前又进入了中断，由于接收中断会将硬件FIFO中的数据全部写入接收缓存，所以
     * 此处需要再次读取，确保数据读取顺序正确
    */
    while (rvsize < size) {
        if (ring_get(&dev->rx_ring, data+rvsize) < 1) {
            break;
        }
        rvsize++;
    }
    
    /**
     * 此时若数据量还不够，则从硬件FIFO中直接读取
    */
    while (rvsize < size) {
        if (IS_DEV_RXFIFO_EMPTY(dev->handle)) {
            break;
        }
        data[rvsize++] = DEV_RD_DR(dev->handle);
    }
    
    /**
     * 接收缓存未满时, 将硬件FIFO中剩余的数据写入接收缓存
    */
    while (ring_level(&dev->rx_ring)+1 < dev->rx_ring.maxsize) {
        
        /**
         * 硬件FIFO读空后重新打开接收中断
         * 此处还有一个目的是保证开启中断时, 接收缓存一定有空间写入
        */
        if (IS_DEV_RXFIFO_EMPTY(dev->handle)) {
            DEV_ENABLE_RXIT(dev->handle);
            return rvsize;
        }
        
        ring_put(&dev->rx_ring, DEV_RD_DR(dev->handle));
    }
    
    return rvsize;
}



/**
 * @brief 中断里的接收实现, 读取数据并写入接收缓存, 当接收缓存已满时会关闭中断
 * @param dev 
 * @author diyi12
 * @date 2024-01-10
 */
void ring_dev_rxInISR(ring_dev_t *dev)
{
    /**
     * 接收缓存未满时将数据写入接收缓存
    */
    while (ring_level(&dev->rx_ring)+1 < dev->rx_ring.maxsize) {
        if (IS_DEV_RXFIFO_EMPTY(dev->handle)) {
            return;
        }
        
        ring_put(&dev->rx_ring, DEV_RD_DR(dev->handle));
    }
    
    /**
     * 接收缓存已满时关闭中断
    */
    DEV_DISABLE_RXIT(dev->handle);
}



/**
 * @brief 中断里的发送实现, 从发送缓存中取出数据并发送, 当发送缓存为空时会关闭发送中断
 * @param dev 句柄
 * @author diyi12
 * @date 2024-01-10
 */
void ring_dev_txInISR(ring_dev_t *dev)
{
    unsigned char data;
    
    /**
     * 取出缓存区所有数据后关闭发送中断
    */
    while (ring_level(&dev->tx_ring) > 0) {
        
        /**
         * 缓存区有数据但硬件FIFO已满时，直接退出本次写入
        */
        if (IS_DEV_TXFIFO_FULL(dev->handle)) {
            return;
        }
        
        ring_get(&dev->tx_ring, &data);
        DEV_WR_DR(dev->handle, data);
    }
    
    DEV_DISABLE_TXIT(dev->handle);
    dev->status &= ~DEV_STAT_TXIE;
}
