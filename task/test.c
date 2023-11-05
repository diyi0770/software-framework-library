#include "task.h"
#include <stdio.h>
#include <string.h>
#include <pthread.h>

volatile uint8_t g_running = 1;


void quit(uint8_t *p)
{
    *p = 0;
}


void task1(char *s)
{
    printf("- %s\n", s);
}


void* task2(void* arg)
{
    char s[32] = {0};
    static uint8_t flag = TASK_FLAG_INITIALIZER;
    
    while (1) {
        scanf("%31s", s);
        if (strncmp(s, "q", 2) == 0) {
            break;
        }
        
        while (!task_flag_is_finished(&flag) && flag != TASK_FLAG_INITIALIZER);
        task_add((void(*)(void*))task1, s, NULL, &flag);
        
        while (getchar()!='\n');
    }
    
    task_add((void(*)(void*))quit, (void*)&g_running, NULL, NULL);
    return NULL;
}


int main(int argc, char* argv[])
{
    pthread_t   tid2;
    
    task_init();
    
    pthread_create(&tid2, NULL, task2, NULL);
    
    while (g_running) {
        task_schedule();
    }
    printf("main quit!\n");
    
    pthread_join(tid2, NULL);
    printf("task2 quit!\n");
    
    task_destroy();
    
    return 0;
}