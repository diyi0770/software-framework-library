#ifndef __RINGCONFIG_H
#define __RINGCONFIG_H

#define DEV_HANDLE_T                    void
#define DEV_ENABLE_TXIT(handle)         (void*)0
#define DEV_DISABLE_TXIT(handle)        (void*)0
#define DEV_ENABLE_RXIT(handle)         (void*)0
#define DEV_DISABLE_RXIT(handle)        (void*)0
#define DEV_WR_DR(handle, data)         (void*)0
#define DEV_RD_DR(handle)               0
#define IS_DEV_TXFIFO_FULL(handle)      (void*)0
#define IS_DEV_RXFIFO_EMPTY(handle)     (void*)0

#endif
