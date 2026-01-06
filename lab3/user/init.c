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
char *argv[] = { "sh", 0 };
int test(void);



int
main(void)
{
    if(open("console", O_RDWR) < 0){
        mknod("console", CONSOLE, 0);
        open("console", O_RDWR);
    }
    dup(0);  // stdout
    dup(0);  // stderr

//lab3
    int i=8;
    int data = 0;
    int ret=0;
    printf("ret:%d\n",ret);
    ret = fork();
    printf("ret:%d\n",ret);
 if (ret == 0){
        data = 2;
        while(i != 0){
            i --;
            printf("child Process: %d, %d;\n", data, i);
            sleep(4);
        }
        exit(1);
    }
   else if(ret != -1){
     pthread_initial();
     test();
     printf("lab3 success\n");
    }
    
    while(1){
    }
    return 0;
}
