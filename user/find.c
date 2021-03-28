//
// Created by xwj on 2021/3/27.
//
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

char *
fmtname(char *path) {

    static char buf[512];
    char *p;

    // Find first character after last slash.
    for (p = path + strlen(path); p >= path && *p != '/'; p--);
    p++;
    memmove(buf, p, strlen(p));
    buf[strlen(p)] = 0;
    return buf;
}

void
find(char *path, char *name) {
    char buf[512], *p;
    int fd;
    struct dirent de;
    struct stat st;

    if ((fd = open(path, 0)) < 0) {
        fprintf(2, "find: cannot open %s\n", path);
        return;
    }

    if (fstat(fd, &st) < 0) {
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        return;
    }
//    printf("current path is %s\n", path);
    char* file_name;
    switch (st.type) {
        case T_FILE:
            file_name = fmtname(path);
//            printf("T_FILE %s\n", file_name);
            if(strcmp(file_name,name)==0)
                printf("%s\n", path);
            break;

        case T_DIR:
            if (strlen(path) + 1 + DIRSIZ + 1 > sizeof buf) {
                printf("find: path too long\n");
                break;
            }
            strcpy(buf, path);
            p = buf + strlen(buf);
            *p++ = '/';
            while (read(fd, &de, sizeof(de)) == sizeof(de)) {
                if (de.inum == 0)
                    continue;
                memmove(p, de.name, DIRSIZ);
                p[DIRSIZ] = 0;
                if(strcmp(de.name,".")==0||strcmp(de.name,"..")==0)
                    continue;
                find(buf,name);  //递归访问文件夹
            }
            break;
    }
    close(fd);
}

int
main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(2, "usage: find\n");
        exit(1);
    }
    find(argv[1],argv[2]);
    exit(0);
}


