#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
int main(void)
{
    // alarm(5);
    int ret[2];
    ret[3] = 10;
    // ret = alarm(1);
    printf("alarm:hello！\n");
    return 666;
}