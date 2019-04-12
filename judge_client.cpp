#define BUFF_SIZE 512
#define STD_MB 1048576
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/resource.h>
// #include <sys/ptrace.h>
#include <mysql/mysql.h>
// #include <algorithm>
#include <iostream>
#include <stdio.h>
// #include <stdarg.h>
#include <unistd.h>
#include <string>
#include <string.h>
// #include <errno.h>
#include <wait.h>
using namespace std;

// extern int errno;
static const char lang_txt[4][8] = {"java", "c", "cpp", "py"};

MYSQL *conn;
static char host_name[BUFF_SIZE];
static char user_name[BUFF_SIZE];
static char password[BUFF_SIZE];
static char db_name[BUFF_SIZE];
static char oj_home[BUFF_SIZE];
static int port_number;

#define OJ_WT0 0 //Pending:等待中
#define OJ_WT1 1 //Waiting:排队中
#define OJ_CI 2  //compiling:编译中
#define OJ_JI 3  //Judging:运行中
#define OJ_AC 4  //Accepted:答案正确，请再接再厉。
#define OJ_PE 5  //Presentation Error:答案基本正确，但是格式不对。
#define OJ_WA 6  //Wrong Answer:答案不对，仅仅通过样例数据的测试并不一定是正确答案，一定还有你没想到的地方，点击查看系统可能提供的对比信息。
#define OJ_TL 7  //Time Limit Exceeded:运行超出时间限制，检查下是否有死循环，或者应该有更快的计算方法
#define OJ_ML 8  //Merrory Limit Exceeded:超出内存限制，数据可能需要压缩，检查内存是否有泄露
#define OJ_OL 9  //Output Limit Exceeded:输出超过限制，你的输出超出了正确答案，一定是哪里弄错了
#define OJ_RE 10 //Runtime Error:运行时错误，非法的内存访问，数组越界，指针漂移，调用禁用的系统函数。请点击后获得详细输出
#define OJ_CE 11 //Compile Error:编译错误，请点击后获得编译器的详细输出
#define OJ_CO 12 //Competition Over:竞赛结束???? 废弃
#define OJ_PA 13 //Partial Accepted:部分正确
#define OJ_SE 14 //System Error

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

    sprintf(sql, "select problem_id, user_id, language from solution where solution_id='%s'", solution_id);
    printf("sql:%s\n", sql);

    mysql_real_query(conn, sql, strlen(sql));
    res = mysql_store_result(conn);
    row = mysql_fetch_row(res);
    if (mysql_num_rows(res) == 0)
    {
        puts("solution不存在!");
        exit(1);
    }
    problem_id = atoi(row[0]);
    user_id = atoi(row[1]);
    lang = atoi(row[2]);
    mysql_free_result(res);
}
//读取源码生成main.c/main.cpp
void get_code_mysql(char *solution_id, int lang)
{
    MYSQL_RES *res;
    MYSQL_ROW row;

    char sql[BUFF_SIZE], code_path[BUFF_SIZE];

    sprintf(sql, "select source from source_code where solution_id='%s'", solution_id);
    mysql_real_query(conn, sql, strlen(sql));
    res = mysql_store_result(conn);
    row = mysql_fetch_row(res);
    if (mysql_num_rows(res) == 0)
    {
        puts("solution不存在!");
        exit(1);
    }

    sprintf(code_path, "./judge/main.%s", lang_txt[lang]);

    FILE *fp_code = fopen(code_path, "w");
    fprintf(fp_code, "%s", row[0]);

    mysql_free_result(res);

    fclose(fp_code);
}
//初始化参数
void init_parameters(int argc, char **argv, char *&solution_id)
{
    if (argc < 2)
    {
        //error
        puts("init_parameters: 参数错误，程序退出！");
        exit(1);
    }
    solution_id = argv[1];
}

// 获取制定文件的大小
long get_file_size(const char *filename)
{
    struct stat f_stat;

    if (stat(filename, &f_stat) == -1)
    {
        return 0;
    }
    return (long)f_stat.st_size;
}
//编译
int compile(int lang)
{
    int pid;
    // freopen("log/error.out", "w", stdout);
    char *compile_cpp[] = {"g++", "judge/main.cpp", "-o", "main", "-w", NULL};
    // char *arg[] = {"./alarm", NULL};
    pid = fork();
    if (pid == 0) //子进程
    {
        struct rlimit LIM;
        LIM.rlim_max = 60;
        LIM.rlim_cur = 60;
        setrlimit(RLIMIT_CPU, &LIM);
        alarm(60);
        LIM.rlim_max = 100 * STD_MB;
        LIM.rlim_cur = 100 * STD_MB;
        setrlimit(RLIMIT_FSIZE, &LIM);
        LIM.rlim_max = STD_MB << 10;
        LIM.rlim_cur = STD_MB << 10;
        setrlimit(RLIMIT_AS, &LIM);

        freopen("log/ce.txt", "w", stderr);
        if (execvp("g++", compile_cpp) == -1)
        {
            printf("编译错误");
            exit(1);
        }
    }
    else //父进程
    {
        int status = 0;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status) != 0)
        {
            status = get_file_size("log/ce.txt");
            return status;
        }
        else
        {
            return -1;
        }
    }
}
//执行cmd命令
int execute_cmd(const char *fmt, ...)
{
    char cmd[BUFF_SIZE];
    int ret = 0;
    va_list ap;

    va_start(ap, fmt);
    vsprintf(cmd, fmt, ap);

    ret = system(cmd);

    va_end(ap);
    return ret;
}
//创建工作空间
void mk_work_dir(char *work_dir)
{
    char shm_path[BUFF_SIZE];
    sprintf(shm_path, "%s", work_dir);
    execute_cmd("/bin/mkdir -p %s", shm_path);
    // execute_cmd("/bin/rm -rf %s", work_dir);
    // execute_cmd("/bin/ln -s %s %s/", shm_path, oj_home);
    // execute_cmd("/bin/chown judge %s ", shm_path);
    // execute_cmd("chmod 755 %s ", shm_path);
    //sim need a soft link in shm_dir to work correctly
    // sprintf(shm_path, "/dev/shm/hustoj/%s/", oj_home);
    // execute_cmd("/bin/ln -s %s/data %s", oj_home, shm_path);
}

//上传错误信息
void add_ce_info(char *solution_id)
{
    char sql[BUFF_SIZE], buf[BUFF_SIZE], *error = NULL;
    char result[BUFF_SIZE * 10];

    snprintf(sql, sizeof(sql), "delete from compile_info where solution_id='%s')", solution_id);
    if (mysql_real_query(conn, sql, strlen(sql)))
    {
        printf("%s\n", mysql_error(conn));
    }

    FILE *fp = fopen("log/ce.txt", "r");
    while (fgets(buf, BUFF_SIZE, fp) != NULL)
    {
        strcat(result, buf);
    }
    snprintf(sql, sizeof(sql),
             "insert into compile_info (solution_id,error) values ('%s','%s')", solution_id, result);
    if (mysql_real_query(conn, sql, strlen(sql)))
    {
        printf("%s\n", mysql_error(conn));
    }
    fclose(fp);
}
void update_solution_info(char *solution_id, int result, int time, int memory)
{
    char sql[BUFF_SIZE];
    sprintf(sql,
            "update solution set result=%d,time=%d,memory=%d where solution_id='%s'", result, time, memory, solution_id);
    if (mysql_real_query(conn, sql, strlen(sql)))
    {
        printf("..update failed! %s\n", mysql_error(conn));
    }
}
void update_user_submition(int user_id)
{
    char sql[BUFF_SIZE];
    sprintf(sql, "update users set submit=submit+1 where user_id = %d", user_id);
    if (mysql_real_query(conn, sql, strlen(sql)))
    {
        printf("..update failed! %s\n", mysql_error(conn));
    }
}
void update_problem_submition(int problem_id)
{
    char sql[BUFF_SIZE];
    sprintf(sql, "update problems set submit=submit+1 where problem_id = %d", problem_id);
    if (mysql_real_query(conn, sql, strlen(sql)))
    {
        printf("..update failed! %s\n", mysql_error(conn));
    }
}
int main(int argc, char **argv)
{
    int problem_id = 0, user_id = 0, lang = 0;

    char *solution_id;

    char work_dir[BUFF_SIZE];

    init_parameters(argc, argv, solution_id);

    init_mysql_conf();

    if (!init_mysql_conn())
    {
        exit(0);
    }

    mk_work_dir(work_dir);

    chdir(work_dir);

    get_solution_info_mysql(solution_id, problem_id, user_id, lang);

    puts("获取solution信息成功！");

    get_code_mysql(solution_id, lang);

    puts("获取源代码成功！");

    int comile_flag = compile(lang);

    printf("编译");
    puts(comile_flag == 0 ? "成功!" : "失败!");

    if (comile_flag != 0)
    {
        add_ce_info(solution_id);
        update_solution_info(solution_id, OJ_CE, 0, 0);
        update_user_submition(user_id);
        update_problem_submition(problem_id);
    }
    else
    {
        update_solution_info(solution_id, OJ_JI, 0, 0);
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