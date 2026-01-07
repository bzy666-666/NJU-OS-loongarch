# 实验目的
1. 实现一个简单的应用程序，并在其中调用两个自定义实现的系统调用。
2. 了解基于中断实现系统调用的全过程。
# 实验内容
1. 内核：实现进程切换机制，并提供系统调用 fork、sleep、exit。
2. 库：对上述系统调用进行封装；实现一个用户态的线程库，完成 pthread_create、
pthread_join、pthread_yield、pthread_exit 等接口。
3. 用户：对上述库函数进行测试。  
# QEMU运行lab2
```bash
cd qemu-loongarch-runenv
```  
通过脚本文件`./run_loongarch.sh`的-k参数，指定我们编译好的lab2内核，即可启动仿真运行  
```bash
./run_loongarch.sh -k ../kernel/kernel
```
