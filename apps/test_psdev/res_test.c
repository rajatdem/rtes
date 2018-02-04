#include<stdio.h>
#include<time.h>
#include<sys/syscall.h>
void main(int argc, char *argv[])
{
    struct timespec A;
    struct timespec B;

    A.tv_sec = 1;
    A.tv_nsec = 0;
    B.tv_sec = 5;
    B.tv_nsec = 0;

    printf("Return = %d\n", syscall(379, (int)atoi(argv[1]), &A, &B, 0));
}
