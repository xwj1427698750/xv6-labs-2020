#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"

int child_process(int p_left[]){
    int prime;
    if(read(p_left[0],&prime,4)>0){
        printf("prime %d\n",prime);
    }
    int pid;
    int p_right[2];
    pipe(p_right);
    pid = fork();
    if(pid == 0){
        close(p_right[1]);//子进程只需要从管道中读取数据，关闭写端
        child_process(p_right);
    }else{
        int num;
        close(p_right[0]);//父进程只需要从管道中写数据，关闭读端
        while(read(p_left[0],&num,4)>0){
            if(num%prime != 0){
                write(p_right[1],&num,4);
            }
        }
        close(p_left[0]);
        close(p_right[1]);
        while(wait((int*)0)!=-1)
            ;
    }
    exit(0);
}

int
main()
{
    int pid;
    int p[2];
    pipe(p);
    pid = fork();
    if(pid == 0){
        close(p[1]);//子进程只需要从管道中读取数据，关闭写端
        child_process(p);
    } else {
        close(p[0]);//父进程只需要从管道中写数据，关闭读端
        int i;
        for(i = 2;i<=35;i++){
            write(p[1],&i,4);
        }
        close(p[1]);
        while(wait((int*)0)!=-1)
            ;
    }
    exit(0);
}