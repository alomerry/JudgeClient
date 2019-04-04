#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
int main(void)
{
    alarm(5);
    int ret;
    sleep(3);
    ret = alarm(1);
    printf("%d\n", ret);
    pause();
    return 0;
}