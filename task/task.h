#ifndef __TASK_H
#define __TASK_H

#include <stdint.h>

/* 最大挂起任务数 */
#define TASK_MAX_SUSPEND_CNT        32



/* 互斥锁实现方式选择 */
#define TASK_MUTEX_REALIZE          2
#if (TASK_MUTEX_REALIZE == 0)
#define TASK_MUTEX_DEFAULT
#elif (TASK_MUTEX_REALIZE == 1)
#define TASK_MUTEX_PTHREAD
#elif (TASK_MUTEX_REALIZE == 2)
#define TASK_MUTEX_SPINLOCK
#endif

/* 信号量实现方式选择 */
#define TASK_SEM_REALIZE          2
#if (TASK_SEM_REALIZE == 0)
#define TASK_SEM_DEFAULT
#elif (TASK_SEM_REALIZE == 1)
#define TASK_SEM_POSIX
#elif (TASK_SEM_REALIZE == 2)
#define TASK_SEM_SYSTEMV
#endif


/* 初始化flag */
#define TASK_FLAG_INITIALIZER       0U



void task_init(void);
void task_schedule(void);
int  task_add(void(*task)(void*), void* arg, void(*cleanup)(void*), uint8_t *flag);
void task_destroy(void);
uint32_t task_getnum(void);

int task_flag_is_suspending(uint8_t *flag);
int task_flag_is_active(uint8_t *flag);
int task_flag_is_finished(uint8_t *flag);

#endif
