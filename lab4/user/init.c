// init: The initial user-level program

#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/spinlock.h"
#include "kernel/sleeplock.h"
#include "kernel/fs.h"
#include "kernel/file.h"
#include "user/user.h"
#include "kernel/fcntl.h"
#include "pthread.h"
#include "pthreadtest.h"

int mutex=-1;
int buffer=-1;
void producer(int arg) {

    int pid = getpid();
    for (int k = 1; k <= 8; ++k) {
        sleep(64);
        printf("pid %d, producer %d, produce, product %d\n", pid, arg, k);
        printf("pid %d, producer %d, try lock, product %d\n", pid, arg, k);
        semwait(mutex);
        printf("pid %d, producer %d, locked\n", pid, arg);
        sempost(mutex);
        printf("pid %d, producer %d, unlock\n", pid, arg);
       sempost(buffer);
    }
}

void consumer(int arg) {

    int pid = getpid();
    for (int k = 1; k <= 4; ++k) {
        printf("pid %d, consumer %d, try consume, product %d\n", pid, arg, k);
        semwait(buffer);
        printf("pid %d, consumer %d, try lock, product %d\n", pid, arg, k);
        semwait(mutex);
        
        printf("pid %d, consumer %d, locked\n", pid, arg);
        sempost(mutex);
        printf("pid %d, consumer %d, unlock\n", pid, arg);
        sleep(64);
        printf("pid %d, consumer %d, consumed, product %d\n", pid, arg, k);
    }

 
}

int
main(void)
{
    if(open("console", O_RDWR) < 0){
        mknod("console", CONSOLE, 0);
        open("console", O_RDWR);
    }
    dup(0);  // stdout
    dup(0);  // stderr
   
    int i = 4;
    int semid = 0;

    int pid=0;
    //sem_t sem;
printf("Father Process: Semaphore Initializing.\n");
    semid = seminit(2);
    //printf("%d\n",semid);
    pid = fork();
    if (pid == 0) {
        while( i != 0) {
            i --;
            printf("Child Process: Semaphore Waiting.\n");
            semwait(semid);
            printf("Child Process: In Critical Area.\n");
            
        }
        printf("Child Process: Semaphore Destroying.\n");
        semdestroy(semid);
        exit(1);
    }
else if (pid != -1) {
        while( i != 0) {
            i --;
            printf("Father Process: Sleeping.\n");
            sleep(128);
            printf("Father Process: Semaphore Posting.\n");
            sempost(semid);
        }
        printf("Father Process: Semaphore Destroying.\n");
        semdestroy(semid);
        //exit();
    }




int ret=-1;
mutex=seminit(1);
buffer=seminit(0);
   for (int i = 0; i < 6; ++i) {
  // printf("in re\n");
      ret=fork();
      //test1();     
     // printf("i=%d\n",i);
     
        if (ret == 0) {
            if (i < 2){
                producer(i + 1);
                }
            else{
                consumer(i - 1);
                }
                
            //exit(1);
            break;
            //while(1);
        }
    }
        while (1);
    semdestroy(mutex);
    semdestroy(buffer);



   
    while(1){
    }
    return 0;
}
