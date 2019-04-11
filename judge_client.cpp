#define BUFF_SIZE 512
#include <sys/types.h>
#include <sys/stat.h>
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

// extern int errno;

MYSQL *conn;
static char host_name[BUFF_SIZE];
static char user_name[BUFF_SIZE];
static char password[BUFF_SIZE];
static char db_name[BUFF_SIZE];
static char oj_home[BUFF_SIZE];
static int port_number;

//清除字符串前后的空白
void trim(char *c)
{
    char buf[BUFF_SIZE];
    char *start, *end;
    strcpy(buf, c);
    start = buf;
    while (isspace(*start))
        start++;
    end = start;
    while (!isspace(*end))
        end++;
    *end = '\0';
    strcpy(c, start);
}
//定位到 ‘=’ 后面
int after_equal(char *c)
{
    int i = 0;
    for (; c[i] != '\0' && c[i] != '='; i++)
        ;
    return ++i;
}
//读取config中指定key的(String)value
bool read_buf(char *buf, const char *key, char *value)
{
    if (strncmp(buf, key, strlen(key)) == 0)
    {
        strcpy(value, buf + after_equal(buf));
        trim(value);
        return 1;
    }
    return 0;
}
//读取config中指定key的(Int)value
void read_int(char *buf, const char *key, int *value)
{
    char buf2[BUFF_SIZE];
    if (read_buf(buf, key, buf2))
    {
        sscanf(buf2, "%d", value);
    }
}
//初始化mysql配置
void init_mysql_conf()
{
    FILE *fp = NULL;
    char buf[BUFF_SIZE];
    host_name[0] = 0;
    user_name[0] = 0;
    password[0] = 0;
    db_name[0] = 0;
    port_number = 3306;
    // sprintf(buf, "%s/etc/judge.conf", oj_home);
    fp = fopen("./etc/judge.conf", "r");
    if (fp != NULL)
    {
        while (fgets(buf, BUFF_SIZE - 1, fp))
        {
            read_buf(buf, "OJ_HOST_NAME", host_name);
            read_buf(buf, "OJ_USER_NAME", user_name);
            read_buf(buf, "OJ_PASSWORD", password);
            read_buf(buf, "OJ_DB_NAME", db_name);
            read_int(buf, "OJ_PORT_NUMBER", &port_number);
        }
        fclose(fp);
    }
}
//初始化mysql连接
int init_mysql_conn()
{
    conn = mysql_init(NULL);
    const char timeout = 30;
    //配置连接时间
    mysql_options(conn, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);

    if (!mysql_real_connect(conn, host_name, user_name, password, db_name, port_number, NULL, 0))
    {
        return 0;
    }
    return 1;
}
//获取solution的信息
void get_solution_info_mysql(char *solution_id, int &problem_id, int &user_id,
                             int &lang)
{
    MYSQL_RES *res;
    MYSQL_ROW row;

    char sql[BUFF_SIZE];
    sprintf(sql, "select problem_id, user_id, language from solution where solution_id=%s", solution_id);

    printf("%s\n", sql);

    mysql_real_query(conn, sql, strlen(sql));
    res = mysql_store_result(conn);
    row = mysql_fetch_row(res);
    if (mysql_num_rows(res) == 0)
    {
        //no solution
        exit(1);
    }
    problem_id = atoi(row[0]);
    user_id = atoi(row[1]);
    lang = atoi(row[2]);
    mysql_free_result(res);
}
//初始化参数
void init_parameters(int argc, char **argv, char *solution_id)
{
    if (argc < 2)
    {
        //error
        exit(1);
    }
    solution_id = argv[1];
}
//在judge.cc中调用参数
//runidstr  : solution_id 的字符串 参数1
//buf		: clientid   也就是进程id字符串
//oj_home	: /home/judge
//execl("/usr/bin/judge_client", "/usr/bin/judge_client", runidstr, buf,
//				oj_home, (char *) NULL);
//argv[0] 指向程序运行的全路径名
//argv[1] 指向在DOS命令行中执行程序名后的第一个字符串
//argv[2] 指向执行程序名后的第二个字符串
int main(int argc, char **argv)
{
    int problem_id = 0, user_id = 0, lang = 0;
    char solution_id[BUFF_SIZE];

    init_parameters(argc, argv, solution_id);

    printf("solution_id:%s\n", solution_id);
    
    init_mysql_conf();

    if (!init_mysql_conn())
    {
        exit(0);
    }
    printf("finishing!\n");

    get_solution_info_mysql(solution_id, problem_id, user_id, lang);

    printf("finished!\n");
    printf("problem_id:%d,user_id:%d\n", problem_id, user_id);
    // system("cd ../ && mkdir 6666");
    // execute_cmd("test %s %d", "content", 16);
    // int comile_flag = compile();
    // if (comile_flag != 0)
    {
        //编译错误，更新数据库并退出
    }
    // else
    {
        //更新数据库
    }
    /*
    1.读取输入输出文件（下载或者访问）
    2.设置时间限制，内存限制
    3. (1)子进程运行程序
       (2)父进程等待结束后判断程序输出和正确结果的异同
    4.更新结果
    */
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
    mysql_close(conn);
    return 0;
}
/**
 * 执行cmd命令
 */
int execute_cmd(const char *fmt, ...)
{
    char cmd[BUFF_SIZE];
    // fill(cmd, cmd + BUFF_SIZE, '-');
    int ret = 0;
    va_list ap;

    va_start(ap, fmt);
    vsprintf(cmd, fmt, ap);
    ret = system("echo 666");
    int i = 0;
    while (cmd[i] != '\0' && i < BUFF_SIZE)
    {
        cout << cmd[i];
        i++;
    }
    printf("\n");
    va_end(ap);
    return ret;
}
/**
 * 获取制定文件的大小 
 */
long get_file_size(const char *filename)
{
    struct stat f_stat;

    if (stat(filename, &f_stat) == -1)
    {
        return 0;
    }
    return (long)f_stat.st_size;
}
/**
 * 编译
 */
int compile()
{
    int pid;

    // freopen("log/error.out", "w", stdout);
    char *arg[] = {"g++", "test.cc", "-o", "test", NULL};
    // char *arg[] = {"./alarm", NULL};
    pid = fork();
    if (pid < 0)
    {
        cout << "错误" << endl;
    }
    else if (pid == 0) //子进程
    {
        cout << "子函数执行！" << endl;
        freopen("log/ce.txt", "w", stderr);
        if (execvp("g++", arg) == -1)
        {
            printf("编译错误");
            // printf("错误代码=%d\n", errno);
            // char *mesg = strerror(errno);
            // printf("错误原因:%s\n", mesg);
        }
    }
    else //父进程
    {
        int status = 0;
        waitpid(pid, &status, 0);
        status = get_file_size("log/ce.txt");
        printf("size:%d\n", status);
        // if (WIFEXITED(status) != 0)
        // {
        //     cout << "正常结束：返回值为：" << WEXITSTATUS(status) << endl;
        // }
        // else
        // {
        //     cout << "非正常结束";
        //     printf_wrongMessage();
        // }
        return status;
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
void judge_solution()
{
    int res = compare("./data/data.in", "./data/user.out");
    if (res)
    {
        cout << "答案正确" << endl;
    }
    else
    {
        cout << "答案错误" << endl;
    }
}
int get_proc_status(pid_t pid, const char *mark)
{
    FILE *file;
    char fileName[BUFF_SIZE], buf[BUFF_SIZE];

    sprintf(fileName, "/proc/%d/status", pid);
    cout << "fileName" << fileName << endl;
    file = fopen(fileName, "r");
    int len = strlen(mark), res;
    cout << "文件打开成功" << file << endl;
    while (file && fgets(buf, BUFF_SIZE - 1, file))
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