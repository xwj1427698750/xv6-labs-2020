#include "types.h"

void *
memset(void *dst, int c, uint n)//每一个字节设置为c
{
    char *cdst = (char *) dst;
    int i;
    for (i = 0; i < n; i++) {
        cdst[i] = c;
    }
    return dst;
}

int
memcmp(const void *v1, const void *v2, uint n) {
    const uchar *s1, *s2;

    s1 = v1;
    s2 = v2;
    while (n-- > 0) {
        if (*s1 != *s2)
            return *s1 - *s2;
        s1++, s2++;
    }

    return 0;
}

void *
memmove(void *dst, const void *src, uint n)//将src指针开头的n个元素复制到以dst指针为开头的位置
{
    const char *s;
    char *d;

    s = src;
    d = dst;
    if (s < d && s + n > d) {
        s += n;
        d += n;
        while (n-- > 0)
            *--d = *--s;      //前置--和*的优先级相同，--先与d和s相结合，再被*解引用
    } else
        while (n-- > 0)
            *d++ = *s++;      //后缀++优先级高于*指针运算符,,因此该表达式等价于*d(++)，后缀++先使用值，然后将值自增1，则表达式可解释为int *pt,pt=d,d++,*pt;

    return dst;
}

// memcpy exists to placate GCC.  Use memmove.
void *
memcpy(void *dst, const void *src, uint n) {
    return memmove(dst, src, n);
}

int
strncmp(const char *p, const char *q, uint n) {
    while (n > 0 && *p && *p == *q)
        n--, p++, q++;
    if (n == 0)
        return 0;
    return (uchar) *p - (uchar) *q;
}

char *
strncpy(char *s, const char *t, int n) {
    char *os;

    os = s;
    while (n-- > 0 && (*s++ = *t++) != 0);
    while (n-- > 0)
        *s++ = 0;
    return os;
}

// Like strncpy but guaranteed to NUL-terminate.
char *
safestrcpy(char *s, const char *t, int n) {
    char *os;

    os = s;
    if (n <= 0)
        return os;
    while (--n > 0 && (*s++ = *t++) != 0);
    *s = 0;
    return os;
}

int
strlen(const char *s) {
    int n;

    for (n = 0; s[n]; n++);
    return n;
}

