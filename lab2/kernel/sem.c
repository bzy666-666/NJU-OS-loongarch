#include "types.h"
#include "loongarch.h"
#include "param.h"
#include "stat.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "fs.h"
#include "sleeplock.h"
#include "file.h"
#include "fcntl.h"
#include "memory.h"
struct semaphore sem[MAX_SEM_NUM];
struct spinlock semslock;  // 保护信号量数组的锁

// 初始化信号量系统
void
semsinit(void)
{
    initlock(&semslock, "sems");
    for(int i = 0; i < MAX_SEM_NUM; i++) {
        initlock(&sem[i].lock, "sem");
        sem[i].valid = 0;      // 标记为无效
        sem[i].value = 0;
        sem[i].waiting = 0;
    }
}

// 分配一个信号量
int
semalloc(int value)
{
    int i=-1;
    
    acquire(&semslock);
    for(i = 0; i < MAX_SEM_NUM; i++) {
        if(sem[i].valid == 0) {
            sem[i].valid = 1;
            sem[i].value = value;
            sem[i].waiting = 0;
            release(&semslock);
            return i;
        }
    }
    release(&semslock);
    return -1;
}

// 释放信号量
int
semfree(int semid)
{
    if(semid < 0 || semid >= MAX_SEM_NUM)
        return -1;
    
    acquire(&semslock);
    if(sem[semid].valid == 0) {
        release(&semslock);
        return -1;
    }
    sem[semid].valid = 0;
    release(&semslock);
    return 0;
}

// P操作（等待）
void
semwait(int semid)
{
    struct semaphore *s;
    struct proc *p = myproc();
    
    if(semid < 0 || semid >= MAX_SEM_NUM)
        return;
    
    s = &sem[semid];
    acquire(&s->lock);
    
    s->value--;
    if(s->value < 0) {
   // printf("value:%d",s->value);
        // 需要阻塞
        p->chan = s;  // 设置睡眠通道
        p->next = s->waiting;
        s->waiting = p;
        
        // 睡眠
        sleep(s, &s->lock);
 
        // 被唤醒后，从等待队列中移除
        // 注意：这里不需要手动移除，由sempost负责
    }
    
    release(&s->lock);
}

// V操作（释放）
void
sempost(int semid)
{
    struct semaphore *s;
    struct proc *p;
    
    if(semid < 0 || semid >= MAX_SEM_NUM)
        return;
    
    s = &sem[semid];
    acquire(&s->lock);
    
    s->value++;
    if(s->value <= 0) {
        // 唤醒等待队列中的一个进程
        if(s->waiting != 0) {
            // 移除队列头部的进程
            p = s->waiting;
            s->waiting = p->next;
            p->next = 0;  // 清空next指针
           // printf("wakeup: pid=%d, state=%d, chan=%p\n",myproc()->pid, myproc()->state, myproc()->chan);
           // printf("wakeup: pid=%d, state=%d, chan=%p\n",p->pid, p->state, p->chan);
         // printf("begin\n");
            // 唤醒进程
            wakeup(s);
          //  printf("over\n");
           // printf("wakeup: pid=%d, state=%d, chan=%p\n",p->pid, p->state, p->chan);
        }
    }
    
    release(&s->lock);
}
