#include <iostream>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
using namespace std;

int main()
{
    // open DIRs
    DIR *dp;
    dirent *dirp;
    char tmp[4096] = {'\0'};
    if ((dp = opendir("./input")) == NULL)
    {
        printf("文件夹打开失败\n");
    }
    for (; (dirp = readdir(dp)) != NULL;)
    {
        strcpy(tmp, dirp->d_name);
        printf("%s\t", strtok(tmp, "."));
        // printf("%s\n", dirp->d_name);
    }
    return 666;
}