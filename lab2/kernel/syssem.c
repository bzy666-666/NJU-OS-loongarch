#include "types.h"
#include "loongarch.h"
#include "defs.h"
#include "param.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "stat.h"
#include "spinlock.h"
#include "proc.h"
#include "fs.h"
#include "sleeplock.h"
#include "file.h"
#include "fcntl.h"
#include "memory.h"

uint64
sys_seminit(void)
{
    int value;
    int semid;
    
    // 获取参数
    if(argint(0, &value) < 0)
        return -1;
    
    // 检查值范围
    if(value < 0 || value > SEM_VALUE_MAX)
        return -1;
    
    semid = semalloc(value);
    return semid;
}

uint64
sys_semwait(void)
{
    int semid;
    
    if(argint(0, &semid) < 0)
        return -1;
    
    if(semid < 0 || semid >= MAX_SEM_NUM)
        return -1;
    
    semwait(semid);
    return 0;
}

uint64
sys_sempost(void)
{
    int semid;
    
    if(argint(0, &semid) < 0)
        return -1;
    
    if(semid < 0 || semid >= MAX_SEM_NUM)
        return -1;
    
    sempost(semid);
    return 0;
}

uint64
sys_semdestroy(void)
{
    int semid;
    
    if(argint(0, &semid) < 0)
        return -1;
    
    return semfree(semid);
}
