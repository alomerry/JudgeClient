#define BUFFER_SIZE 256
#include <sys/resource.h>
#include <sys/ptrace.h>
#include <mysql/mysql.h>
#include <algorithm>
#include <iostream>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <string>
#include <string.h>
#include <errno.h>
#include <wait.h>
using namespace std;

extern int errno;

MYSQL *STDCALL mysql_init(MYSQL *mysql);
MYSQL *STDCALL mysql_real_connect(MYSQL *mysql, const char *host, const char *user, const char *passwd,
                                  const char *db, unsigned int *port, const char *unix_socket,
                                  unsigned long *clientflag);
void testMysql();
int execute_cmd(const char *fmt, ...);
int compire();
void run_solution();
void watch_solution(pid_t pidApp);
void printf_wrongMessage();
void printf_wrongMessage(int status);
int get_proc_status(int pid, const char *mark);
int compare(const char *file1, const char *file2);
void judge_solution();

int main(int argc, char **argv)
{
    testMysql();

    int i;
    for (i = 0; i < argc; i++)
    {
        printf("Argument %d is %s \n", i, argv[i]);
    }
    // system("cd ../ && mkdir 6666");
    // execute_cmd("test %s %d", "content", 16);
    // compire();
    /*
    pid_t pidApp = fork();
    if (pidApp == 0) //子进程
    {
        run_solution();
    }
    else
    {
        cout << "pid = " << pidApp << endl;
        sleep(2);
        watch_solution(pidApp);
        judge_solution();
    }
    */

    return 0;
}
/**
 * 通过C API调用mysql
 */
void testMysql()
{
    MYSQL mysql;
    mysql_init(&mysql);
    mysql_real_connect(&mysql, "rm-bp14419zgc8077s9hjo.mysql.rds.aliyuncs.com", "mo", "WjC120211", "online_judge", 3306, NULL, 0);
    string sql = "select username from users;";

    mysql_query(&mysql, sql.c_str());

    MYSQL_RES *res = mysql_store_result(&mysql);

    int num_fields = mysql_num_fields(res);

    MYSQL_FIELD *fields = mysql_fetch_fields(res);

    while (MYSQL_ROW row = mysql_fetch_row(res)) //获取整条数据内容
    {
        for (int i = 0; i < num_fields; i++)
        {
            if (NULL == row[i])
            {
                cout << " NULL";
            }
            else
            {
                cout << " " << row[i];
            }
        }
        cout << endl;
    }
    mysql_close(&mysql);
}
/**
 * 执行cmd命令
 */
int execute_cmd(const char *fmt, ...)
{
    char cmd[BUFFER_SIZE];
    // fill(cmd, cmd + BUFFER_SIZE, '-');
    int ret = 0;
    va_list ap;

    va_start(ap, fmt);
    vsprintf(cmd, fmt, ap);
    ret = system("echo 666");
    int i = 0;
    while (cmd[i] != '\0' && i < BUFFER_SIZE)
    {
        cout << cmd[i];
        i++;
    }
    printf("\n");
    va_end(ap);
    return ret;
}
/**
 * 编译
 */
int compire()
{
    int pid;

    pid = fork();
    char *arg[] = {"g++", "test.cc", "-o", "alarm", NULL};
    if (pid < 0)
    {
        cout << "错误" << endl;
    }
    else if (pid == 0) //子进程
    {
        cout<<"子函数执行！"<<endl;
        if (execvp("g++", arg) == -1)
        {
            printf("错误代码=%d\n", errno);
            char *mesg = strerror(errno);
            printf("错误原因:%s\n", mesg);
        }
        else
        {
            cout << "无错误!" << endl;
        }
        cout << "子函数执行完毕！" << endl;
        return -1;
    }
    else //父进程
    {
        sleep(1);
        int status;
        printf_wrongMessage();
        waitpid(pid, &status, 0);
        if (WIFEXITED(status) != 0)
        {
            cout << "正常结束：返回值为：" << WEXITSTATUS(status) << endl;
        }
        else
        {
            cout << "非正常结束";
        }
    }
}
void printf_wrongMessage(int status)
{
    printf("错误代码=%d\n", status);
    char *mesg = strerror(status);
    printf("错误原因:%s\n", mesg);
}
void printf_wrongMessage()
{
    printf("错误代码=%d\n", errno);
    char *mesg = strerror(errno);
    printf("错误原因:%s\n", mesg);
}
/**
 * 执行编译结果
 */
void run_solution()
{
    freopen("user.out", "w", stdout);
    freopen("data.in", "r", stdin);
    freopen("error.out", "a+", stderr);
    //ptrace(PTRACE_TRACEME, 0, NULL, NULL);
    execl("./test", "./test", (char *)NULL);
}
/**
 * 查看运行结
 */
void watch_solution(pid_t pidApp)
{
    int status;
    struct rusage ruse;
    wait4(pidApp, &status, 0, &ruse);
    //get_proc_status(pidApp, "VmPeak:");
    if (WIFEXITED(status))
    {
        cout << "子进程正常运行结束" << endl;
    }
    else
    {
        if (WIFSIGNALED(status))
        {
            printf_wrongMessage(WTERMSIG(status));
        }
    }
    //ptrace(PTRACE_KILL, pidApp, NULL, NULL); //杀死子进程，停止执行
}
void judge_solution()
{
    int res = compare("data.in", "user.out");
    if (res)
    {
        cout << "答案正确" << endl;
    }
    else
    {
        cout << "答案错误" << endl;
    }
}
int compare(const char *file1, const char *file2)
{
    FILE *fp1, *fp2;
    char *s1, *s2, *p1, *p2;
    s1 = new char[4096], s2 = new char[4096];
    fp1 = fopen(file1, "r");
    cout << "打开文件f1" << endl;
    for (p1 = s1; EOF != fscanf(fp1, "%s", p1);)
    {
        while (*p1)
        {
            cout << *p1;
            p1++;
        }
    }
    fclose(fp1);
    cout << "打开文件f1" << endl;
    cout << "打开文件f2" << endl;
    fp2 = fopen(file2, "r");
    for (p2 = s2; EOF != fscanf(fp2, "%s", p2);)
    {
        while (*p2)
        {
            cout << *p2;
            p2++;
        }
    }
    fclose(fp2);
    cout << "打开文件f2" << endl;
    if (strcmp(s1, s2) == 0)
    {
        delete[] s1;
        delete[] s2;
        return 1;
    }
    else
    {
        delete[] s1;
        delete[] s2;
        return 0;
    }
}
int get_proc_status(pid_t pid, const char *mark)
{
    FILE *file;
    char fileName[BUFFER_SIZE], buf[BUFFER_SIZE];

    sprintf(fileName, "/proc/%d/status", pid);
    cout << "fileName" << fileName << endl;
    file = fopen(fileName, "r");
    int len = strlen(mark), res;
    cout << "文件打开成功" << file << endl;
    while (file && fgets(buf, BUFFER_SIZE - 1, file))
    {
        if (strncmp(mark, buf, len) == 0)
        {
            printf("%s", buf);
            sscanf(buf + len, "%d", &res);
            cout << (res << 10) << endl;
        }
    }

    if (file)
        fclose(file);
    return 0;
}