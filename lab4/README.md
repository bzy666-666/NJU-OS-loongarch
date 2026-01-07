# 实验目的
1. 实现一个简单的生产者消费者程序。
2. 介绍基于信号量的进程同步机制
# 实验内容
1. 内核：提供基于信号量的进程同步机制，并提供系统调用 sem_init、sem_post、
sem_wait、sem_destroy。
2. 库：对上述系统调用进行封装。
3. 用户：对上述库函数进行测试。
# QEMU运行lab2
```bash
cd qemu-loongarch-runenv
```  
通过脚本文件`./run_loongarch.sh`的-k参数，指定我们编译好的lab2内核，即可启动仿真运行  
```bash
./run_loongarch.sh -k ../kernel/kernel
```
