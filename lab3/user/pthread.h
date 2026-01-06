#ifndef __PTHREAD_H__
#define __PTHREAD_H__

#include "kernel/types.h"
#define MAX_TCB_NUM 4           // Why?
#define MAX_STACK_SIZE 1536     // Why? try other numbers

#define STATE_RUNNABLE 0
#define STATE_RUNNING 1
#define STATE_BLOCKED 2
#define STATE_DEAD 3



typedef struct {
    uint64 ra;    // 0: 返回地址
    uint64 sp;    // 8: 栈指针
    uint64 s0;    // 16: 保存寄存器 s0
    uint64 s1;    // 24: s1
    uint64 s2;    // 32: s2
    uint64 s3;    // 40: s3
    uint64 s4;    // 48: s4
    uint64 s5;    // 56: s5
    uint64 s6;    // 64: s6
    uint64 s7;    // 72: s7
    uint64 s8;    // 80: s8
    uint64 fp;    // 88: 帧指针
    // 注意：临时寄存器不需要保存（调用者保存）
    // a0-a7, t0-t8 由调用者负责保存
} loongarch_context_t;

typedef struct ThreadTable {
    loongarch_context_t context;
    int state;
    int pthid;
    uint64 pthArg;
    void* stack[MAX_STACK_SIZE];
    int joinid;
    int retval;
} ThreadTable;




/**
 * when you create a Process in your OS, there is no thread, you need initial these Process as a thread, so when you create a thread, they are in same structure.
**/
void thread_wrapper(void);
void pthread_initial(void); 

int pthread_create(uint32 *thread, const int *attr, void *(*start_routine)(void *), void *arg);

void pthread_exit(void *retval);

int pthread_join(uint32 thread, void **retval);

int pthread_yield(void);


#endif
