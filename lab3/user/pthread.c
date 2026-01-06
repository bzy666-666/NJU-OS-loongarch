#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"
#include "kernel/syscall.h"
#include "kernel/memlayout.h"
#include "kernel/loongarch.h"
#include "pthread.h"
#include <stddef.h>

/*
 * pthread lib here
 * 用户态多线程写在这
 */

ThreadTable tcb[MAX_TCB_NUM];
int current;

void pthread_initial(void)
{
    int i;
    for (i = 0; i < MAX_TCB_NUM; i++)
    {
        tcb[i].state = STATE_DEAD;
        tcb[i].joinid = -1;
        tcb[i].retval = 0;
    }
    tcb[0].state = STATE_RUNNING;
    tcb[0].pthid = 0;
    current = 0; // main thread
    return;
}

// 用户态的上下文切换，模仿内核的swtch
static void user_swtch(loongarch_context_t *old, loongarch_context_t *new) {
    __asm__ __volatile__(
        // 保存旧上下文
        "st.d $ra, %0, 0\n\t"
        "st.d $sp, %0, 8\n\t"
        "st.d $s0, %0, 16\n\t"
        "st.d $s1, %0, 24\n\t"
        "st.d $s2, %0, 32\n\t"
        "st.d $s3, %0, 40\n\t"
        "st.d $s4, %0, 48\n\t"
        "st.d $s5, %0, 56\n\t"
        "st.d $s6, %0, 64\n\t"
        "st.d $s7, %0, 72\n\t"
        "st.d $s8, %0, 80\n\t"
        "st.d $fp, %0, 88\n\t"
        
        // 恢复新上下文
        "ld.d $ra, %1, 0\n\t"
        "ld.d $sp, %1, 8\n\t"
        "ld.d $s0, %1, 16\n\t"
        "ld.d $s1, %1, 24\n\t"
        "ld.d $s2, %1, 32\n\t"
        "ld.d $s3, %1, 40\n\t"
        "ld.d $s4, %1, 48\n\t"
        "ld.d $s5, %1, 56\n\t"
        "ld.d $s6, %1, 64\n\t"
        "ld.d $s7, %1, 72\n\t"
        "ld.d $s8, %1, 80\n\t"
        "ld.d $fp, %1, 88\n\t"
        : 
        : "r"(old), "r"(new)
        : "memory"
    );
}

// 线程包装函数
 void thread_wrapper(void) {
    // 获取函数指针和参数
    void *(*func)(void *) = (void *(*)(void *))tcb[current].context.s0;
    void *arg = (void *)tcb[current].context.s1;
    
    // 调用线程函数
    void *retval = func(arg);
    
    // 线程退出
    pthread_exit(retval);
}

int pthread_create(uint32 *thread, const int *attr,
                   void *(*start_routine)(void *), void *arg) {
    int i;
    // 找一个空闲的TCB槽位
    for (i = 1; i < MAX_TCB_NUM; ++i) {
        if (tcb[i].state == STATE_DEAD) {
            break;
        }
    }
    if (i == MAX_TCB_NUM) {
        return -1; // 没有空闲槽位
    }
    
    *thread = i;
    
    // 初始化线程控制块
    tcb[i].pthid = i;
    tcb[i].pthArg = (uint64)arg;
    tcb[i].joinid = -1;
    tcb[i].retval = 0;
    
    // 设置线程栈（向下增长）
    // 注意：我们需要给栈帧预留空间
    uint64 stack_top = (uint64)&tcb[i].stack[MAX_STACK_SIZE];
    
    // 栈需要16字节对齐，并且预留一些空间给函数调用
    stack_top = (stack_top - 128) & ~0xF;
    
    // 在栈上设置返回地址，这样当线程函数返回时会调用pthread_exit
    // 但我们使用包装函数，所以不需要这个
    
    // 初始化上下文
    tcb[i].context.ra = (uint64)thread_wrapper;   // 返回地址指向包装函数
    tcb[i].context.sp = stack_top;                // 栈指针
    tcb[i].context.fp = stack_top;                // 帧指针
    
    // 保存函数指针和参数到callee-saved寄存器
    tcb[i].context.s0 = (uint64)start_routine;    // 函数指针
    tcb[i].context.s1 = (uint64)arg;              // 参数
    
    // 其他寄存器初始化为0
    tcb[i].context.s2 = 0;
    tcb[i].context.s3 = 0;
    tcb[i].context.s4 = 0;
    tcb[i].context.s5 = 0;
    tcb[i].context.s6 = 0;
    tcb[i].context.s7 = 0;
    tcb[i].context.s8 = 0;
    
    tcb[i].state = STATE_RUNNABLE;
    
    return 0;
}

int pthread_join(uint32 thread, void **retval) {
    // 检查参数有效性
    if (thread >= MAX_TCB_NUM || thread == 0) {  // 不能join主线程或无效线程
        return -1;
    }
    
    // 如果线程已经结束，直接返回
    if (tcb[thread].state == STATE_DEAD) {
        if (retval != NULL) {
            *retval = (void *)(uint64)tcb[thread].retval;
        }
        return 0;
    }
    
    // 记录当前线程正在等待这个线程
    tcb[thread].joinid = current;
    
    // 等待线程结束
    while (tcb[thread].state != STATE_DEAD) {
        pthread_yield();
    }
    
    // 获取返回值
    if (retval != NULL) {
        *retval = (void *)(uint64)tcb[thread].retval;
    }
    
    return 0;
}

void pthread_exit(void *retval) {
    // 保存返回值
    tcb[current].retval = (uint64)retval;
    
    // 如果有线程在等待join这个线程，需要唤醒它
    int joiner = tcb[current].joinid;
    
    // 标记线程为结束状态
    tcb[current].state = STATE_DEAD;
    
    // 找到下一个可运行线程
    int next = -1;
    
    // 总是先检查主线程（线程0）
    if (tcb[0].state == STATE_RUNNABLE || tcb[0].state == STATE_RUNNING) {
        next = 0;
    } else {
        // 查找其他可运行线程
        for (int i = 1; i < MAX_TCB_NUM; i++) {
            if (i != current && tcb[i].state == STATE_RUNNABLE) {
                next = i;
                break;
            }
        }
    }
    
    // 如果有线程在等待join当前线程，优先运行它
    if (joiner != -1 && tcb[joiner].state == STATE_RUNNABLE) {
        next = joiner;
    }
    
    // 如果找不到任何可运行线程，退出进程
    if (next == -1) {
        exit(0);
    }
    
    // 切换到下一个线程
    int prev = current;
    current = next;
    tcb[current].state = STATE_RUNNING;
    
    user_swtch(&tcb[prev].context, &tcb[current].context);
    
    // 永远不会到达这里
}

int pthread_yield(void) {
    // 保存当前线程的完整上下文
    __asm__ __volatile__(
        "st.d $ra, %0, 0\n\t"
        "st.d $sp, %0, 8\n\t"
        "st.d $s0, %0, 16\n\t"
        "st.d $s1, %0, 24\n\t"
        "st.d $s2, %0, 32\n\t"
        "st.d $s3, %0, 40\n\t"
        "st.d $s4, %0, 48\n\t"
        "st.d $s5, %0, 56\n\t"
        "st.d $s6, %0, 64\n\t"
        "st.d $s7, %0, 72\n\t"
        "st.d $s8, %0, 80\n\t"
        "st.d $fp, %0, 88\n\t"
        : 
        : "r"(&tcb[current].context)
        : "memory"
    );
    
    tcb[current].state = STATE_RUNNABLE;
    
    // 选择下一个可运行线程
    int next = -1;
    
    // 从当前线程的下一个开始查找
    for (int i = 1; i <= MAX_TCB_NUM; i++) {
        int idx = (current + i) % MAX_TCB_NUM;
        if (tcb[idx].state == STATE_RUNNABLE) {
            next = idx;
            break;
        }
    }
    
    // 如果没有其他可运行线程，继续运行当前线程
    if (next == -1) {
        tcb[current].state = STATE_RUNNING;
        return 0;
    }
    
    // 切换到下一个线程
    int prev = current;
    current = next;
    tcb[current].state = STATE_RUNNING;
    
    user_swtch(&tcb[prev].context, &tcb[current].context);
    
    return 0;
}
