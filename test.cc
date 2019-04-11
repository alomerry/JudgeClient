#include <algorithm>
#include <iostream>
#include <stdio.h>
#include <string.h>
using namespace std;

int main()
{
    int len = 0;
    char buf[512];
    char res[10240];

    FILE *fp = fopen("log/ce.txt", "r");
    while (fgets(buf, 512, fp) != NULL)
    {
        strcat(res, buf);
    }
    cout << res << endl;
    fclose(fp);
    return 666;
}