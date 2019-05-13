#define BUFF_SIZE 512
#define STD_MB 1048576

#define DEBUG 1
#define BUILD 0

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/ptrace.h>
#include <mysql/mysql.h>
#include <dirent.h>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <string>
#include <string.h>
#include <wait.h>

#include "configer.h"
#include "client.h"
#include "logger.h"
using namespace std;
// #include <errno.h>
// #include <algorithm>

// extern int errno;
static const char lang_txt[4][8] = {"java", "c", "cpp", "py"};
static char compire_txt[4][8] = {"javac", "gcc", "g++", "py"};

MYSQL *conn;
static int Mode = 0;
static char host_name[BUFF_SIZE];
static char user_name[BUFF_SIZE];
static char password[BUFF_SIZE];
static char db_name[BUFF_SIZE];
static char oj_home[BUFF_SIZE];
static int port_number;
int Judge_Result = OJ_AC;

static char work_dir[BUFF_SIZE];

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
    sprintf(buf, "%s/etc/judge.conf", oj_home);
    fp = fopen(buf, "r");
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

    if (Mode == DEBUG) //Debug Mode
    {
        fprintf(stderr, "初始化数据库中......\n\thost_name(%s)\n\tuser_name(%s)\n\tdb_name(%s)\n\t初始化完毕。\n", host_name, user_name, db_name);
        printf("初始化数据库中......\n\thost_name(%s)\n\tuser_name(%s)\n\tdb_name(%s)\n\t初始化完毕。\n", host_name, user_name, db_name);
    }
}
//初始化mysql连接
int init_mysql_conn()
{
    conn = mysql_init(NULL);
    const char timeout = 30;
    //配置连接时间
    mysql_options(conn, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);

    if (Mode == DEBUG) //Debug Mode
    {
        printf("初始化数据库连接中......\n");
    }

    if (!mysql_real_connect(conn, host_name, user_name, password, db_name, port_number, NULL, 0))
    {
        if (Mode == DEBUG) //Debug Mode
        {
            printf("\t初始化失败。\n");
        }
        return 0;
    }
    if (Mode == DEBUG) //Debug Mode
    {
        printf("\t初始化成功。\n");
    }
    return 1;
}
//获取solution的信息
void get_solution_info_mysql(char *solution_id, int &problem_id, int &user_id, int &lang)
{
    MYSQL_RES *res;
    MYSQL_ROW row;

    char sql[BUFF_SIZE];

    sprintf(sql, "select problem_id, user_id, language from solution where solution_id='%s'", solution_id);

    if (Mode == DEBUG) //Debug Mode
    {
        fprintf(stderr, "正在查询数据库，获取[solution]表信息\n\t%s\n", sql);
        printf("正在查询数据库，获取[solution]表信息\n\t%s\n", sql);
    }

    mysql_real_query(conn, sql, strlen(sql));
    res = mysql_store_result(conn);
    row = mysql_fetch_row(res);
    if (mysql_num_rows(res) == 0)
    {
        if (Mode == DEBUG) //Debug Mode
        {
            fprintf(stderr, "\tsolution不存在!\n");
            printf("\tsolution不存在!\n");
        }
        exit(1);
    }
    problem_id = atoi(row[0]);
    user_id = atoi(row[1]);
    lang = atoi(row[2]);

    if (Mode == DEBUG) //Debug Mode
    {
        printf("\t查询成功。\n\tproblem_id(%d)\n\tuser_id(%d)\n\tlang(%d)\n", problem_id, user_id, lang);
    }
    fprintf(stderr, "查询结束!\n");
    mysql_free_result(res);
}
//读取源码生成main.c/main.cpp
void get_code_mysql(char *solution_id, int lang)
{
    MYSQL_RES *res;
    MYSQL_ROW row;

    char sql[BUFF_SIZE], code_path[BUFF_SIZE];

    sprintf(sql, "select source from source_code where solution_id='%s'", solution_id);

    if (Mode == DEBUG) //Debug Mode
    {
        printf("正在查询数据库，获取[source[表信息\n\t%s\n", sql);
    }

    mysql_real_query(conn, sql, strlen(sql));
    res = mysql_store_result(conn);
    row = mysql_fetch_row(res);
    if (mysql_num_rows(res) == 0)
    {
        if (Mode == DEBUG) //Debug Mode
        {
            printf("\tsource不存在!\n");
        }
        exit(1);
    }

    sprintf(code_path, "%s/code/main.%s", work_dir, lang_txt[lang]);

    FILE *fp_code = fopen(code_path, "w");
    fprintf(fp_code, "%s", row[0]);

    mysql_free_result(res);

    if (Mode == DEBUG) //Debug Mode
    {
        printf("\t查询成功。\n");
    }

    fclose(fp_code);
}
/**
 * 初始化参数
 * argv[0]: judge_client
 * argv[1]: solution_id
 * argv[2]: Mode(DEBUG/BUILD)
 */
void init_parameters(int argc, char **argv, char *&solution_id)
{
    for (int i = 0; i < argc; i++)
    {
        write_log("arg[%d]:{%s}", i + 1, argv[i]);
        printf(argv[i]);
    }

    if (argc < 3)
    {
        if (Mode == DEBUG) //Debug Mode
        {
            printf("初始化参数中......\n\t参数错误，程序退出！\n");
        }
        exit(1);
    }
    solution_id = argv[1];
    Mode = atoi(argv[2]);

    if (Mode == DEBUG) //Debug Mode
    {
        printf("初始化参数中......\n\tsolution id(%s)\n\t初始化完毕。\n", solution_id);
    }
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
    char buffer[BUFF_SIZE], out[BUFF_SIZE];
    sprintf(buffer, "%s/code/main.%s", work_dir, lang_txt[lang]);
    sprintf(out, "%s/code/main", work_dir);
    char *compire_type = compire_txt[lang];
    char *compile_cpp[] = {compire_type, buffer, "-o", out, "-w", NULL};
    // char *arg[] = {"./alarm", NULL};

    // sprintf(buffer, "%s/log/ce.txt", work_dir);
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

        if (Mode == DEBUG) //Debug Mode
        {
            printf("编译中.....\n");
        }
        char err[BUFF_SIZE];
        sprintf(err, "%s/log/ce.txt", work_dir);
        freopen(err, "w", stderr);
        if (execvp(compire_type, compile_cpp) == -1)
        {
            if (Mode == DEBUG) //Debug Mode
            {
                printf("\t调用%s编译器错误\n", compire_type);
            }
            exit(1);
        }
    }
    else //父进程
    {
        int status = 0;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status) != 0)
        {
            // status = get_file_size("log/ce.txt");
            char err[BUFF_SIZE];
            sprintf(err, "%s/log/ce.txt", work_dir);
            status = get_file_size(err);
            if (Mode == DEBUG) //Debug Mode
            {
                if (status == 0)
                {
                    printf("\t编译成功。\n");
                }
                else
                {
                    printf("\t编译错误。结束运行\n");
                    exit(0);
                }
            }
            return status;
        }
        else
        {
            if (Mode == DEBUG) //Debug Mode
            {
                printf("\t编译错误。\n");
            }
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
void mk_work_dir()
{
    char shm_path[BUFF_SIZE];
    sprintf(shm_path, "%s", work_dir);
    execute_cmd("/bin/mkdir -p %s", shm_path);
    execute_cmd("/bin/mkdir -p %s/code", shm_path);
    execute_cmd("/bin/mkdir -p %s/log", shm_path);
    execute_cmd("/bin/mkdir -p %s/data", shm_path);
    execute_cmd("/usr/bin/touch  %s/data/user.out", shm_path);
    execute_cmd("/usr/bin/touch  %s/log/log.txt", shm_path);
    // execute_cmd("/bin/ln -s %s %s/", shm_path, oj_home);
    // execute_cmd("/bin/chown judge %s ", shm_path);
    // execute_cmd("chmod 755 %s ", shm_path);
    //sim need a soft link in shm_dir to work correctly
    // sprintf(shm_path, "/dev/shm/hustoj/%s/", oj_home);
    // execute_cmd("/bin/ln -s %s/data %s", oj_home, shm_path);

    fprintf(stderr, "创建工作空间OK\n");
}

//上传错误信息
void add_ce_info(char *solution_id)
{
    char sql[BUFF_SIZE], buf[BUFF_SIZE], buffer[BUFF_SIZE], *error = NULL;
    char result[BUFF_SIZE * 10];

    snprintf(sql, sizeof(sql), "delete from compile_info where solution_id='%s')", solution_id);
    sprintf(buffer, "%/log/ce.txt", work_dir);
    if (Mode == DEBUG) //Debug Mode
    {
        printf("正在操作数据库，删除[compile_info]表信息\n\t%s\n", sql);
    }

    if (mysql_real_query(conn, sql, strlen(sql)))
    {
        printf("%s\n", mysql_error(conn));
    }

    FILE *fp = fopen(buffer, "r");
    while (fgets(buf, BUFF_SIZE, fp) != NULL)
    {
        strcat(result, buf);
    }
    snprintf(sql, sizeof(sql), "insert into compile_info (solution_id,error) values ('%s','%s')", solution_id, result);
    if (Mode == DEBUG) //Debug Mode
    {
        printf("正在操作数据库，插入[compile_info]表信息\n\t%s\n", sql);
    }

    if (mysql_real_query(conn, sql, strlen(sql)))
    {
        printf("%s\n", mysql_error(conn));
    }

    fclose(fp);
}
//更新solution信息
void update_solution_info(char *solution_id, int result, int time, int memory)
{
    char sql[BUFF_SIZE];
    sprintf(sql, "update solution set result=%d,time=%d,memory=%d where solution_id='%s'", result, time, memory, solution_id);
    if (Mode == DEBUG) //Debug Mode
    {
        printf("正在更新数据库，获取solution表信息\n\t%s\n", sql);
    }

    if (mysql_real_query(conn, sql, strlen(sql)))
    {
        printf("..update failed! %s\n", mysql_error(conn));
    }
}
//更新用户提交数
void update_user_submition(int user_id)
{
    char sql[BUFF_SIZE];
    sprintf(sql, "update users set submit=submit+1 where user_id = %d", user_id);
    if (Mode == DEBUG) //Debug Mode
    {
        printf("正在更新数据库，获取users表信息\n%s\n", sql);
    }

    if (mysql_real_query(conn, sql, strlen(sql)))
    {
        printf("..update failed! %s\n", mysql_error(conn));
    }
}
//更新题目提交数
void update_problem_submition(int problem_id)
{
    char sql[BUFF_SIZE];
    sprintf(sql, "update problems set submit=submit+1 where problem_id = %d", problem_id);
    if (Mode == DEBUG) //Debug Mode
    {
        printf("正在更新数据库，获取problems表信息\n%s\n", sql);
    }

    if (mysql_real_query(conn, sql, strlen(sql)))
    {
        printf("..update failed! %s\n", mysql_error(conn));
    }
}
//执行编译结果
void run_solution(int lang, int time_limit, int memery_limit)
{
    char buffer[BUFF_SIZE];
    sprintf(buffer, "%s/log/error.txt", work_dir);
    freopen(buffer, "a+", stderr);

    sprintf(buffer, "%s/data/data.in", work_dir);
    cout << "样例输入路径[" << buffer << "]" << endl;
    freopen(buffer, "r", stdin);

    sprintf(buffer, "%s/data/user.out", work_dir);
    cout << "用户打印路径[" << buffer << "]" << endl;
    freopen(buffer, "w", stdout);

    //请求父进程追踪
    ptrace(PTRACE_TRACEME, 0, NULL, NULL);

    //设置进程时钟片使用时间
    struct rlimit LIM;
    LIM.rlim_max = time_limit + 1;
    LIM.rlim_cur = LIM.rlim_max;
    setrlimit(RLIMIT_CPU, &LIM);

    //设置进程运行时间
    alarm(0); //清除定时
    alarm(time_limit * 10);

    LIM.rlim_max = STD_MB; //设置文件大小限制
    LIM.rlim_cur = STD_MB;
    setrlimit(RLIMIT_FSIZE, &LIM);

    LIM.rlim_max = STD_MB << 10;
    LIM.rlim_cur = STD_MB << 10;
    setrlimit(RLIMIT_AS, &LIM);

    sprintf(buffer, "%s/code/main", work_dir);
    if (execl(buffer, buffer, (char *)NULL) == -1)
    {
        exit(1);
    }
    exit(0);
}

int get_proc_status(pid_t pid, const char *mark)
{
    FILE *file;
    char fileName[BUFF_SIZE], buf[BUFF_SIZE];

    sprintf(fileName, "/proc/%d/status", pid);
    file = fopen(fileName, "r");
    int len = strlen(mark), res = 0;
    while (file && fgets(buf, BUFF_SIZE - 1, file))
    {
        if (strncmp(mark, buf, len) == 0)
        {
            // printf("%s", buf);
            sscanf(buf + len, "%d", &res);
            // cout << (res << 10) << endl;
        }
    }
    if (file)
        fclose(file);
    return res;
}

void print_runtimeerror(char *err)
{
    FILE *ferr = fopen("log/error.out", "a+");
    fprintf(ferr, "Runtime Error:%s\n", err);
    fclose(ferr);
}
/**
 * 获取文件后缀名
 * filename 文件名
 * extension 后缀名
 */
bool check_file_type(char *file_name, char *extension)
{
    // printf("#-判断文件后缀名-%s(%s) = %d \n", strrchr(file_name, '.'), extension, strcasecmp(extension, strrchr(file_name, '.')) == 0);
    return strcasecmp(extension, strrchr(file_name, '.')) == 0;
}
//查看运行结果
void watch_solution(pid_t pidApp, int &Judge_Result, int &usedtime)
{
    int status, memory_usage = 0;
    struct rusage ruse;
    while (true)
    {
        wait4(pidApp, &status, 0, &ruse);
        memory_usage = get_proc_status(pidApp, "VmPeak:");
        if (memory_usage > 100000)
        {
            Judge_Result = OJ_ML;
            printf("内存超限(%d)\n", memory_usage);
            ptrace(PTRACE_KILL, pidApp, NULL, NULL);
            break;
        }
        if (WIFEXITED(status))
        {
            cout << "\t子进程正常运行结束" << endl;
            break;
        }
        char buffer[BUFF_SIZE];
        sprintf(buffer, "%s/log/error.txt", work_dir);
        if (get_file_size(buffer))
        {
            Judge_Result = OJ_RE;
            //addreinfo(solution_id);
            ptrace(PTRACE_KILL, pidApp, NULL, NULL);
            break;
        }
        int exitcode = WEXITSTATUS(status);
        // printf("exit code : %d\n", exitcode);
        if (!(exitcode == 0 || exitcode == 0x05 || exitcode == 17))
        {
            if (Judge_Result == OJ_AC)
            {
                switch (exitcode)
                {
                case SIGCHLD:
                case SIGALRM:
                    alarm(0);
                case SIGKILL:
                case SIGXCPU:
                    Judge_Result = OJ_TL;
                    printf("%s\n", "时间超限");
                    break;
                case SIGXFSZ:
                    Judge_Result = OJ_OL;
                    break;
                default:
                    Judge_Result = OJ_RE;
                }
                print_runtimeerror(strsignal(exitcode));
            }
            ptrace(PTRACE_KILL, pidApp, NULL, NULL);
            break;
        }
        if (WIFSIGNALED(status))
        {
            int sig = WTERMSIG(status);
            if (Judge_Result == OJ_AC)
            {
                switch (sig)
                {
                case SIGCHLD:
                case SIGALRM:
                    alarm(0);
                case SIGKILL:
                case SIGXCPU:
                    printf("%s\n", "时间超限");
                    Judge_Result = OJ_TL;
                    break;
                case SIGXFSZ:
                    Judge_Result = OJ_OL;
                    break;

                default:
                    Judge_Result = OJ_RE;
                }
                printf("print_runtimeerror....\n");
                print_runtimeerror(strsignal(sig));
            }
            break;
        }
        ptrace(PTRACE_SYSCALL, pidApp, NULL, NULL);
    }
    usedtime += (ruse.ru_utime.tv_sec * 1000 + ruse.ru_utime.tv_usec / 1000);
    usedtime += (ruse.ru_stime.tv_sec * 1000 + ruse.ru_stime.tv_usec / 1000);
}
/**
 * 准备测试用例
 * work_dir 工作空间
 * input_file 测试用例
 * input_result 测试用例结果
 * p_id
 */
void prepare_file_to_run(int problem_id, char *input_file, char *input_result)
{
    printf("正在准备文件......\n\t输入文件(%s)  输出文件(%s)\n", input_file, input_result);
    char fullname[BUFF_SIZE];
    sprintf(fullname, "%s/problem_cases/%d/%s", oj_home, problem_id, input_file); //ojhome中的文件
    execute_cmd("/bin/cp -f %s %s/data/data.in", fullname, work_dir);             //workdir

    sprintf(fullname, "%s/problem_cases/%d/%s", oj_home, problem_id, input_result); //ojhome中的文件
    execute_cmd("/bin/cp  -f %s %s/data/data.out", fullname, work_dir);             //workdir
}
void compare(const char *correct_result, const char *user_file)
{
    FILE *res = fopen(correct_result, "r"), *user = fopen(user_file, "r");
    char *res_buf = new char[BUFF_SIZE * 10], *user_buf = new char[BUFF_SIZE * 10];

    while (Judge_Result == OJ_AC && fgets(res_buf, BUFF_SIZE, res) != NULL)
    {
        printf("[%s", res_buf);
        if (fgets(user_buf, BUFF_SIZE, user) != NULL)
        {
            printf("{%s", user_buf);
            //判断两个缓冲区是否相同
            printf("\t对比答案中.......\n");
            for (int i = 0; user_buf[i] != '\0' && i <= BUFF_SIZE - 1; i++)
            {
                printf("\t\t user[%d] res[%d]\n", user_buf[i], res_buf[i]);
                if (user_buf[i] != res_buf[i])
                {
                    if ((res_buf[i] == 10 || res_buf[i] == 13) && user_buf[i] == 10 || user_buf[i] == 13)
                    {
                        continue;
                    }
                    Judge_Result = OJ_WA;
                    if (Mode == DEBUG) //Debug Mode
                    {
                        printf("\n\t用户答案与标准答案不一致，答案错误\n");
                    }
                    break;
                }
            }
        }
        else
        {
            //答案错误
            if (Mode == DEBUG) //Debug Mode
            {
                printf("\n\t用户答案与标准答案不一致，答案错误\n");
            }
            Judge_Result = OJ_WA;
            break;
        }
    }

    printf("\t\t Judge_Result[%d]\n", Judge_Result);

    fclose(user);
    fclose(res);
    delete[] res_buf;
    delete[] user_buf;
}
void judge_solution()
{
    char buf1[BUFF_SIZE], buf2[BUFF_SIZE];
    sprintf(buf1, "%s/data/data.out", work_dir);
    sprintf(buf2, "%s/data/user.out", work_dir);
    compare(buf1, buf2);
    if (Judge_Result != OJ_AC)
    {
        cout << "\t答案错误" << endl;
    }
    else
    {
        cout << "\t答案正确" << endl;
    }
}

int main(int argc, char **argv)
{

    int problem_id = 0, user_id = 0, lang = 0, usedtime = 0;

    char *solution_id;

    char buffer[BUFF_SIZE];

    strcpy(oj_home, "/oj-home");
    chdir(oj_home);

    // open DIRs
    DIR *dp;
    dirent *dirp;

    execute_cmd("/bin/mkdir -p /oj-home/log/");
    execute_cmd("/usr/bin/touch  /oj-home/log/log.txt");

    init_parameters(argc, argv, solution_id);

    init_mysql_conf();

    if (!init_mysql_conn())
    {
        fprintf(stderr, "数据库链接失败！");
        exit(0);
    }
    fprintf(stderr, "数据库链接成功！");
    sprintf(work_dir, "/%s/judge/%s", oj_home, solution_id);

    mk_work_dir();

    chdir(work_dir);

    get_solution_info_mysql(solution_id, problem_id, user_id, lang);

    get_code_mysql(solution_id, lang);

    int compile_flag = compile(lang);
    fprintf(stderr, "compile_flag = %d!\n", compile_flag);
    if (compile_flag != 0)
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
    //是否可运行
    pid_t pidApp = fork();
    if (pidApp == 0) //子进程
    {
        fprintf(stderr, "run_solution");
        run_solution(lang, 100, 100);
    }
    else
    {
        if (Mode == DEBUG) //Debug Mode
        {
            fprintf(stderr, "父进程:\n\t开始检查子进程运行之后的结果(子进程Id:%d)\n", (int)pidApp);
            printf("父进程:\n\t开始检查子进程运行之后的结果(子进程Id:%d)\n", (int)pidApp);
        }
        watch_solution(pidApp, Judge_Result, usedtime);
    }
    sprintf(buffer, "%s/problem_cases/%d", oj_home, problem_id);
    //读取目录文件失败则判题子程序退出，-1
    if ((dp = opendir(buffer)) == NULL)
    {
        if (Mode == DEBUG) //Debug Mode
        {
            printf("不存在测试文件目录\n");
        }
        mysql_close(conn);
        exit(-1);
    }
    printf("开始轮训样例......\n\t当前 Judge_Result [%s]\n", Judge_Result == OJ_AC ? "AC" : "WA");
    for (; (Judge_Result == OJ_AC) && (dirp = readdir(dp)) != NULL;)
    {
        if (!check_file_type(dirp->d_name, ".in") && !check_file_type(dirp->d_name, ".out"))
        {
            //资源有误
            // printf("\t读取文件:%s,非测试用例\n", dirp->d_name);
            continue;
        }
        else
        {
            if (Mode == DEBUG)
            {
                // printf("\t读取测试用例:%s\n", dirp->d_name);
            }
            /*
            1.拷贝测试用例
            2.生成输出文件
            */
            /*
            3.子进程运行题目
            4.父进程监视是否可完整运行
            5.
            */
            if (check_file_type(dirp->d_name, ".in") == 1)
            {
                char tmp[BUFF_SIZE] = {'\0'};
                strcpy(tmp, dirp->d_name);
                strtok(tmp, ".");
                prepare_file_to_run(problem_id, dirp->d_name, strcat(tmp, ".out"));

                pid_t pidApp = fork();
                if (pidApp == 0)
                {
                    run_solution(lang, 100, 100);
                }
                else
                {
                    watch_solution(pidApp, Judge_Result, usedtime);
                    judge_solution();
                }
            }
        }
    }
    printf("整个程序结束，最终结果 [%s]\n", Judge_Result == OJ_AC ? "正确" : "失败");
    mysql_close(conn);

    printf("%s\n", "/************************** client结束 ***********************/");

    return 0;
}
