#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
int main(void)
{
    // alarm(5);
    int ret = 10;
    sleep(3);
    // ret = alarm(1);
    printf("alarm :%d\n", ret);
    return 0;
}