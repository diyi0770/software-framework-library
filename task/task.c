#include "task.h"
#include <string.h>
#include <stdio.h>



#if (TASK_MAX_SUSPEND_CNT < 1) || (TASK_MAX_SUSPEND_CNT >= ~0U)
#error "TASK_MAX_SUSPEND_CNT is [1, 65535)"
#endif



/**
 * @brief 互斥锁实现方式
 */
#if defined(TASK_MUTEX_DEFAULT)
#define     TASK_MUTEX_INIT()           (void*)0
#define     TASK_ENTER_CRITICAL()       do {(void*)0;} while(0)
#define     TASK_EXIT_CRITICAL()        do {(void*)0;} while(0)
#define     TASK_MUTEX_DESTROY()        (void*)0
#elif defined(TASK_MUTEX_PTHREAD)
#include <pthread.h>
static pthread_mutex_t  g_mutex;
#define     TASK_MUTEX_INIT()           pthread_mutex_init(&g_mutex, NULL)
#define     TASK_ENTER_CRITICAL()       pthread_mutex_lock(&g_mutex)
#define     TASK_EXIT_CRITICAL()        pthread_mutex_unlock(&g_mutex)
#define     TASK_MUTEX_DESTROY()        pthread_mutex_destroy(&g_mutex)
#elif defined(TASK_MUTEX_SPINLOCK)
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200112L
#define __USE_XOPEN2K
#endif
#include <pthread.h>
static pthread_spinlock_t   g_lock;
#define     TASK_MUTEX_INIT()           pthread_spin_init(&g_lock, PTHREAD_PROCESS_PRIVATE)
#define     TASK_ENTER_CRITICAL()       pthread_spin_lock(&g_lock)
#define     TASK_EXIT_CRITICAL()        pthread_spin_unlock(&g_lock)
#define     TASK_MUTEX_DESTROY()        pthread_spin_destroy(&g_lock)
#endif



/**
 * @brief 信号量实现方式
 */
#if defined(TASK_SEM_DEFAULT)
#define     TASK_SEM_INIT()             (void*)0
#define     TASK_SEM_WAIT()             do {(void*)0;} while(0)
#define     TASK_SEM_POST()             do {(void*)0;} while(0)
#define     TASK_SEM_DESTROY()          (void*)0
#elif defined(TASK_SEM_POSIX)
#include <semaphore.h>
static sem_t    g_sem;
#define     TASK_SEM_INIT()             sem_init(&g_sem, 0, 0)
#define     TASK_SEM_WAIT()             sem_wait(&g_sem)
#define     TASK_SEM_POST()             sem_post(&g_sem)
#define     TASK_SEM_DESTROY()          sem_destroy(&g_sem)
#elif defined(TASK_SEM_SYSTEMV)
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
static int g_semid;
static struct sembuf g_semop_p = {.sem_op=-1, .sem_flg = SEM_UNDO};
static struct sembuf g_semop_v = {.sem_op=+1, .sem_flg = SEM_UNDO};
#define     TASK_SEM_INIT()             do {g_semid = semget(IPC_PRIVATE, 1, 0777);} while(0)
#define     TASK_SEM_WAIT()             do {semop(g_semid, &g_semop_p, 1);} while (0)
#define     TASK_SEM_POST()             do {semop(g_semid, &g_semop_v, 1);} while (0)
#define     TASK_SEM_DESTROY()          do {semctl(g_semid, 0, IPC_RMID);} while (0)
#endif



#define TASK_FLAG_SUSPENDING        (1U << 0)
#define TASK_FLAG_ACITVE            (1U << 1)
#define TASK_FLAG_FINISHED          (1U << 2)



/* typedef ********************************************************************/

/**
 * @brief 任务控制块 结构定义
 */
typedef struct {
    void        *arg;
    uint8_t     *flag;
    void(*task)(void*);
    void(*cleanup)(void*);
} task_tcb_t;



/**
 * @brief 任务队列 结构定义
 */
typedef struct {
    uint32_t    ri;
    uint32_t    wi;
    task_tcb_t  tcbpool[TASK_MAX_SUSPEND_CNT+1];
} task_queue_t;



/* global **********************************************************************/

static int _task_queue_isempty(task_queue_t *queue);
static int _task_queue_isfull(task_queue_t *queue);
static void _task_queue_add(task_queue_t *queue, task_tcb_t item);
static task_tcb_t _task_queue_get(task_queue_t *queue);
static uint32_t _task_queue_level(task_queue_t *queue);



static task_queue_t    g_task_queue;



/* function *******************************************************************/

/**
 * @brief 检查队列是否为空
 * @param queue 
 * @return int 
 */
static int _task_queue_isempty(task_queue_t *queue)
{
    return (queue->ri == queue->wi);
}



/**
 * @brief 检查队列是否已满
 * @param queue 
 * @return int 
 */
static int _task_queue_isfull(task_queue_t *queue)
{
    return ((queue->wi+1) % (TASK_MAX_SUSPEND_CNT+1) == queue->ri);
}



/**
 * @brief 入队
 * @param queue 
 * @param item 
 */
static void _task_queue_add(task_queue_t *queue, task_tcb_t item)
{
    queue->tcbpool[queue->wi] = item;
    queue->wi = (queue->wi+1) % (TASK_MAX_SUSPEND_CNT+1);
}



/**
 * @brief 出队
 * @param queue 
 * @return task_tcb_t 
 */
static task_tcb_t _task_queue_get(task_queue_t *queue)
{
    task_tcb_t  tcb;
    tcb = queue->tcbpool[queue->ri];
    queue->ri = (queue->ri+1) % (TASK_MAX_SUSPEND_CNT+1);
    return tcb;
}



/**
 * @brief 获取当前队列中的元素个数
 * @param queue 
 * @return uint32_t 
 */
static uint32_t _task_queue_level(task_queue_t *queue)
{
    return (queue->wi + (TASK_MAX_SUSPEND_CNT+1) - queue->ri) % (TASK_MAX_SUSPEND_CNT+1);
}



/**
 * @brief 初始化任务队列、互斥锁、信号量
 */
void task_init(void)
{
    g_task_queue.ri = 0;
    g_task_queue.wi = 0;
    memset(g_task_queue.tcbpool, 0, TASK_MAX_SUSPEND_CNT+1);
    
    TASK_MUTEX_INIT();
    TASK_SEM_INIT();
}



/**
 * @brief 任务调度
 */
void task_schedule(void)
{
    task_tcb_t  tcb;
    uint8_t     tmpflag;
    
    TASK_SEM_WAIT();
    
    if (_task_queue_isempty(&g_task_queue)) {
        return;
    }
    
    tcb = _task_queue_get(&g_task_queue);
    
    if (tcb.flag) {
        tmpflag = *(tcb.flag);
        *(tcb.flag) = (tmpflag | TASK_FLAG_ACITVE) & ~(TASK_FLAG_SUSPENDING);
    }
    
    if (tcb.task) {
        tcb.task(tcb.arg);
    }
    
    if (tcb.cleanup && tcb.arg) {
        tcb.cleanup(tcb.arg);
    }
    
    if (tcb.flag) {
        tmpflag = *(tcb.flag);
        *(tcb.flag) = (tmpflag | TASK_FLAG_FINISHED) & ~(TASK_FLAG_ACITVE);
    }
}



/**
 * @brief 向任务队列添加任务
 * @param task 任务函数
 * @param arg 传递给任务的数据
 * @param cleanup 数据清理函数
 * @param flag 任务状态
 * @return int 成功返回0，失败返回-1
 */
int  task_add(void(*task)(void*), void* arg, void(*cleanup)(void*), uint8_t *flag)
{
    int ret = 0;
    task_tcb_t  tcb;
    uint8_t     tmpflag;
    
    TASK_ENTER_CRITICAL();
    
    tcb.flag = flag;
    tcb.arg = arg;
    tcb.task = task;
    tcb.cleanup = cleanup;
    
    if (!_task_queue_isfull(&g_task_queue)) {
        if (tcb.flag) {
            tmpflag = *(tcb.flag);
            *(tcb.flag) = (tmpflag | TASK_FLAG_SUSPENDING) & ~TASK_FLAG_FINISHED;
        }
        _task_queue_add(&g_task_queue, tcb);
    } else {
        ret = -1;
    }
    
    TASK_SEM_POST();
    TASK_EXIT_CRITICAL();
    
    return ret;
}



/**
 * @brief 获取当前队列中的任务数
 * @return uint32_t 
 */
uint32_t task_getnum(void)
{
    return _task_queue_level(&g_task_queue);
}



/**
 * @brief 销毁任务队列、互斥锁、信号量
 */
void task_destroy(void)
{
    g_task_queue.ri = g_task_queue.wi;

    TASK_MUTEX_DESTROY();
    TASK_SEM_DESTROY();
}



/**
 * @brief 检查flag是否被设置了挂起标志
 * @param flag 
 * @return int 设置了挂起返回True，未设置挂起返回False
 */
int task_flag_is_suspending(uint8_t *flag)
{
    uint8_t tmpflag = *flag;
    return ((tmpflag & TASK_FLAG_SUSPENDING) == TASK_FLAG_SUSPENDING);
}



/**
 * @brief 检查flag是否被设置了活跃标志
 * @param flag 
 * @return int 设置了活跃返回True，未设置则返回False
 */
int task_flag_is_active(uint8_t *flag)
{
    uint8_t tmpflag = *flag;
    return ((tmpflag & TASK_FLAG_ACITVE) == TASK_FLAG_ACITVE);
}



/**
 * @brief 检查flag是否被设置了执行完成标志
 * @param flag 
 * @return int 设置了执行完成返回True，未设置则返回False
 */
int task_flag_is_finished(uint8_t *flag)
{
    uint8_t tmpflag = *flag;
    return ((tmpflag & TASK_FLAG_FINISHED) == TASK_FLAG_FINISHED);
}
