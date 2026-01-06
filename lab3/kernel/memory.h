#ifndef _MEMORY_H__
#define _MEMORY_H__

#include "spinlock.h"

#define MAX_SEM_NUM 64    // 最大信号量数量
#define SEM_VALUE_MAX 32767

// 信号量结构
struct semaphore {
    struct spinlock lock;    
    int value;               
    int valid;               
    struct proc *waiting;   
};


#endif
