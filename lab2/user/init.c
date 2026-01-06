// init: The initial user-level program

#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/spinlock.h"
#include "kernel/sleeplock.h"
#include "kernel/fs.h"
#include "kernel/file.h"
#include "user/user.h"
#include "kernel/fcntl.h"



int
main(void)
{
    if(open("console", O_RDWR) < 0){
        mknod("console", CONSOLE, 0);
        open("console", O_RDWR);
    }
    dup(0);  // stdout
    dup(0);  // stderr
//lab2
    int num=0;
    printf("write a num\n");
    scanf("%d", &num);  
    printf("what u write num is: %d\n", num);
    printf("lab2 success\n");



   
    while(1){
    }
    return 0;
}
