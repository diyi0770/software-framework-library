#include "ringbuffer.h"



/**
 * @brief 定义handle的类型
 */
#ifndef DEV_HANDLE_T
#error "please define the DEV_HANDLE_T used to specified the type that the handle pointer points to!"
#endif



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



/**
 * @brief ring_dev_t@status
 */
#define DEV_STAT_TXIE          (1U << 0)
#define DEV_STAT_RXIE          (1U << 1)
#define DEV_STAT_TBSY          (1U << 2)
#define DEV_STAT_RBSY          (1U << 3)



static inline int _is_empty(ring_buffer_t *ring)
{
    return (ring->rd_i == ring->wr_i);
}



static inline int _is_full(ring_buffer_t *ring)
{
    return ((ring->wr_i + 1) % ring->maxsize == ring->rd_i);
}



static inline int _get_level(ring_buffer_t *ring)
{
    return (ring->wr_i + ring->maxsize - ring->rd_i) % ring->maxsize;
}



/**
 * @brief 初始化ringbuffer
 * @param ring ringbuffer地址
 * @param addr 数据缓存区起始地址
 * @param size 缓存区大小，单位字节
 */
void ring_init(ring_buffer_t *ring, unsigned char *addr, unsigned int size)
{
    ring->data = addr;
    ring->rd_i = 0;
    ring->wr_i = 0;
    ring->maxsize = size>=1? size: 1;
}



/**
 * @brief 将数据写入ringbuffer
 * @param ring 句柄
 * @param data 写入数据
 * @return unsigned int 成功返回1，失败返回0
 */
unsigned int ring_put(ring_buffer_t *ring, unsigned char data)
{
    if (!ring) {
        return 0;
    }
    
    if (!_is_full(ring)) {
        ring->data[ring->wr_i] = data;
        ring->wr_i = (ring->wr_i + 1) % ring->maxsize;
        return 1;
    }
    
    return 0;
}



/**
 * @brief 从ringbuffer中取出数据
 * @param ring 句柄
 * @param data 存放数据的地址
 * @return unsigned int 成功返回1，失败返回0
 */
unsigned int ring_get(ring_buffer_t *ring, unsigned char *data)
{
    if (!ring || !data) {
        return 0;
    }
    
    if (!_is_empty(ring)) {
        *data = ring->data[ring->rd_i];
        ring->rd_i = (ring->rd_i + 1) % ring->maxsize;
        return 1;
    }
    
    return 0;
}



/**
 * @brief 获取当前ringbuffer中元素的个数
 * @param ring 句柄
 * @return unsigned int 返回ringbuffer中元素的个数
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
 */
void ring_dev_init(ring_dev_t *dev, void *handle, void *txdata, unsigned int tsize, void *rxdata, unsigned int rsize)
{
    ring_init(&dev->tx_ring, txdata, tsize);
    ring_init(&dev->rx_ring, rxdata, rsize);
    dev->handle = handle;
    dev->status = 0;
    
    if (rxdata && rsize>=2) {
        dev->status = DEV_STAT_RXIE;
        DEV_ENABLE_RXIT(((DEV_HANDLE_T *)handle));
    }
}



/**
 * @brief 设备发送数据, 接口以轮询方式发送数据，当发送FIFO有数据但执行位置不在此接口内时以中断方式发送
 * @param dev 句柄
 * @param sdata 要发送的数据
 * @param size 数据大小(byte)
 * @return unsigned int 返回成功发送的字节数
 */
unsigned int ring_dev_tx(ring_dev_t *dev, const void *sdata, unsigned int size)
{
    unsigned int        sdsize = 0;
    const unsigned char *data = (unsigned char *)sdata;
    unsigned char       tmp, sendingflag=0;
    
    /* 阻止中断发送 */
    if (dev->status & DEV_STAT_TBSY) {
        sendingflag = 1;    //此处是为了避免中断里的此接口打断该接口的执行时清除掉BUSY标志
    } else {
        dev->status |= DEV_STAT_TBSY;
    }
    
    /**
     * 发送数据：
     * 所有FIFO都写满时直接退出
     * 硬件FIFO有空位且软件FIFO不为空时，从软件FIFO中取出数据并填入硬件FIFO，再向软件FIFO中追加数据
     * 硬件FIFO有空位且软件FIFO为空时，直接将发送数据填入硬件FIFO
     * 硬件FIFO写满但软件FIFO有空位时，发送数据填入软件FIFO
    */
    while (sdsize < size) {
        if ((!dev->tx_ring.data || _is_full(&dev->tx_ring)) &&\
            IS_DEV_TXFIFO_FULL(((DEV_HANDLE_T *)(dev->handle)))) {
            break;
        }
        
        if (!IS_DEV_TXFIFO_FULL(((DEV_HANDLE_T *)(dev->handle)))) {
            if (dev->tx_ring.data && !_is_empty(&dev->tx_ring)) {
                ring_get(&dev->tx_ring, &tmp);
                DEV_WR_DR(((DEV_HANDLE_T *)(dev->handle)), tmp);
                ring_put(&dev->tx_ring, data[sdsize++]);
            } else {
                DEV_WR_DR(((DEV_HANDLE_T *)(dev->handle)), data[sdsize++]);
            }
            continue;
        }
        
        if (dev->tx_ring.data) {
            ring_put(&dev->tx_ring, data[sdsize++]);
        }
    }
    
    /* 允许中断发送 */
    if (!sendingflag) {
        dev->status &= ~DEV_STAT_TBSY;
        
        /* 发送中断未使能且软件fifo不为空时，打开发送中断 */
        if ((dev->tx_ring.data && !_is_empty(&dev->tx_ring)) && !(dev->status & DEV_STAT_TXIE)) {
            dev->status |= DEV_STAT_TXIE;
            DEV_ENABLE_TXIT(((DEV_HANDLE_T *)(dev->handle)));
        }
    }
    
    return sdsize;
}



/**
 * @brief 设备接收数据, 接口以轮询方式接收数据，当执行位置不在此接口内但有数据被发送过来时以中断方式接收
 * @param dev 句柄
 * @param rdata 存放数据的起始地址
 * @param size 需要接收的数据大小(byte)
 * @return unsigned int 返回成功接收的字节数
 */
unsigned int ring_dev_rx(ring_dev_t *dev, void *rdata, unsigned int size)
{
    unsigned int    rvsize = 0;
    unsigned char   *data = (unsigned char *)rdata;
    unsigned char   recvingflag = 0;
    
    /* 阻止接收中断向缓存中写数据 */
    if (dev->status & DEV_STAT_RBSY) {
        recvingflag = 1;    //此处是为了避免中断里的此接口打断该接口的执行时清除掉BUSY标志
    } else {
        dev->status |= DEV_STAT_RBSY;
    }
    
    /**
     * 读取数据
     * 所有fifo都为空时直接退出
     * 软件fifo不为空时，读取软件fifo并将硬件fifo中的数据写入软件fifo
     * 软件fifo为空或不存在时，读取硬件fifo
     */
    while (rvsize < size) {
        if ((!dev->rx_ring.data || _is_empty(&dev->rx_ring)) &&\
            IS_DEV_RXFIFO_EMPTY(((DEV_HANDLE_T *)(dev->handle)))) {
            break;
        }
        
        if (dev->rx_ring.data && !_is_empty(&dev->rx_ring)) {
            ring_get(&dev->rx_ring, data + rvsize++);
            if (!IS_DEV_RXFIFO_EMPTY(((DEV_HANDLE_T *)(dev->handle)))) {
                ring_put(&dev->rx_ring, DEV_RD_DR(((DEV_HANDLE_T *)(dev->handle))));
            }
            continue;
        }
        
        data[rvsize++] = DEV_RD_DR(((DEV_HANDLE_T *)(dev->handle)));
    }
    
    /* 允许中断向缓存中写数据 */
    if (!recvingflag) {
        dev->status &= ~DEV_STAT_RBSY;
        
        /* 接收中断未使能且软件fifo有空位时，打开接收中断 */
        if ((dev->rx_ring.data && !_is_full(&dev->rx_ring)) && !(dev->status & DEV_STAT_RXIE)) {
            dev->status |= DEV_STAT_RXIE;
            DEV_ENABLE_RXIT(((DEV_HANDLE_T *)(dev->handle)));
        }
    }
    
    return rvsize;
}



/**
 * @brief 中断里的接收实现, 读取数据并写入接收缓存
 * @param dev 
 */
void ring_dev_rxInISR(ring_dev_t *dev)
{
    /* 若触发中断时处于接收接口中，则不需要由中断将数据写入缓存 */
    if (dev->status & DEV_STAT_RBSY) {
        DEV_DISABLE_RXIT(((DEV_HANDLE_T *)(dev->handle)));
        dev->status &= ~DEV_STAT_RXIE;
        return;
    }
    
    /* 接收缓存未满时将数据写入接收缓存 */
    while (dev->rx_ring.data && !_is_full(&dev->rx_ring)) {
        if (IS_DEV_RXFIFO_EMPTY(((DEV_HANDLE_T *)(dev->handle)))) {
            return;
        }
        ring_put(&dev->rx_ring, DEV_RD_DR(((DEV_HANDLE_T *)(dev->handle))));
    }
    
    /* 软件FIFO无空位时关闭中断 */
    DEV_DISABLE_RXIT(((DEV_HANDLE_T *)(dev->handle)));
    dev->status &= ~DEV_STAT_RXIE;
}



/**
 * @brief 中断里的发送实现, 从发送缓存中取出数据并发送, 当发送缓存为空时会关闭发送中断
 * @param dev 句柄
 */
void ring_dev_txInISR(ring_dev_t *dev)
{
    unsigned char data;
    
    /* 若触发中断时处于发送接口中，则不需要由中断进行发送 */
    if (dev->status & DEV_STAT_TBSY) {
        DEV_DISABLE_TXIT(((DEV_HANDLE_T *)(dev->handle)));
        dev->status &= ~DEV_STAT_TXIE;
        return;
    }
    
    /* 取出缓存区所有数据后关闭发送中断 */
    while (dev->tx_ring.data && !_is_empty(&dev->tx_ring)) {
        
        /* 缓存区有数据但硬件FIFO已满时，直接退出本次写入 */
        if (IS_DEV_TXFIFO_FULL(((DEV_HANDLE_T *)(dev->handle)))) {
            return;
        }
        
        ring_get(&dev->tx_ring, &data);
        DEV_WR_DR(((DEV_HANDLE_T *)(dev->handle)), data);
    }
    
    /* 软件FIFO已空时关闭中断 */
    DEV_DISABLE_TXIT(((DEV_HANDLE_T *)(dev->handle)));
    dev->status &= ~DEV_STAT_TXIE;
}
