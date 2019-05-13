#include "logger.h"

#include <stdarg.h>
#include <string>
#include <stdio.h>
#include <iostream>
#include <stdlib.h>

#include "configer.h"

using namespace std;

void write_log(const char *fmt, ...)
{
    va_list ap;
    char buffer[4096];
    sprintf(buffer, "/oj-home/log/log.txt");
    FILE *fp = fopen(buffer, "a+");
    if (fp == NULL)
    {
        fprintf(stderr, "open file error!\n");
        system("pwd");
    }
    va_start(ap, fmt);
    //l =
    vsprintf(buffer, fmt, ap);
    fprintf(fp, "%s\n", buffer);
    // if (DEBUG)
    // printf("%s\n", buffer);
    va_end(ap);
    fclose(fp);
}