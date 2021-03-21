#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"

int
main()
{
    if(argc>1){
        fprintf(2, "usage: pingpong\n");
        exit(1);
    }
    int pid;
    int p[2];
    char buf[10];
    pipe(p);
    pid = fork();
    if(pid == 0){
        read(p[0],buf, sizeof(buf));
        printf("%d: received ping\n",getpid());
        close(p[0]);
        write(p[1],"2", 1);
        close(p[1]);
    } else {
        write(p[1],"1",1);
        close(p[1]);
        wait((int*)0);//这一句很关键，等待子进程从管道中读取数据，否则数据会被父进程中下一句的read给读走
        read(p[0],buf, sizeof(buf));
        close(p[0]);
        printf("%d: received pong\n",getpid());
    }

    exit(0);
}

