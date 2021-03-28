//
// Created by lenovo on 2021/3/27.
//

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

#define ARG_LENTH 100
int
main(int argc, char *argv[]) {
    if (argc > MAXARG) {
        fprintf(2, "xargs: too many args \n");
        exit(1);
    }
    char *args[MAXARG];
    int args_index = 0;
    //转存xargs的参数到args中
    int i;
    for(i = 1;i<argc;i++)
        args[args_index++] = argv[i];
    args[args_index] = 0;
    //读取标准输入中的参数
    int args_index_copy = args_index;
    char* arg = (char*)malloc(ARG_LENTH);
    int arg_index = 0;
    char c;
    while(read(0, &c, sizeof(c))>0){
//        printf("c is %c;\n",c);
        if(c ==' ' || c =='\n'){
            arg[arg_index] = 0;             //收集了一个参数
            args[args_index++] = arg;       //在参数列表里添加参数
            arg = (char*)malloc(ARG_LENTH); //给arg分配新的空间
            arg_index = 0;                  //参数下标清0
        }else{
            arg[arg_index++] = c;
        }
        if(c == '\n'){//收集了一行参数
            int pid = fork();
            if(pid == 0){
//                printf("cmd is %s\n", args[0]);
                exec(args[0],args);
            }else{
                wait((int*)0);
                args_index = args_index_copy;//恢复参数数组的下标
                args[args_index] = 0;
            }
        }
    }
    //判断是否还有最后一行的参数没有被运行
    if(args_index != args_index_copy){
        int pid = fork();
        if(pid == 0){
            exec(args[0],args);
        }else{
            wait((int*)0);
        }
    }
    exit(0);
}