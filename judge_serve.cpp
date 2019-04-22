#define BUFF_SIZE 512
#define DEBUG 1
#include <mysql/mysql.h>
#include <sys/stat.h>
#include <iostream>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
using namespace std;

MYSQL *conn;
static int Mode = 0;
static bool STOP = false;
static char host_name[BUFF_SIZE];
static char user_name[BUFF_SIZE];
static char password[BUFF_SIZE];
static char db_name[BUFF_SIZE];
static char oj_home[BUFF_SIZE];
static int port_number;
static int sleep_time;

void call_for_exit(int s)
{
    STOP = true;
    printf("Stopping judged...\n");
}
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
            read_int(buf, "OJ_SLEEP_TIME", &sleep_time);
        }
        fclose(fp);
    }

    if (Mode == DEBUG) //Debug Mode
    {
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
int daemon_init(void)
{
    pid_t pid;
    if ((pid = fork()) < 0)
        return (-1);
    else if (pid != 0)
        exit(0); /* parent exit */

    /* child continues */
    setsid();       /* become session leader */
    chdir(oj_home); /* change working directory */
    umask(0);       /* clear file mode creation mask */
    close(0);       /* close stdin */
    close(1);       /* close stdout */
    close(2);       /* close stderr */
    return (0);
}

int lockfile(int fd)
{
    struct flock fl;
    fl.l_type = F_WRLCK;
    fl.l_start = 0;
    fl.l_whence = SEEK_SET;
    fl.l_len = 0;
    return (fcntl(fd, F_SETLK, &fl));
}
int already_running()
{
    int fd;
    char buf[16];
    fd = open("", O_RDWR | O_CREAT, (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH));
    if (fd < 0)
    {
        strerror(errno);
        exit(1);
    }
    if (lockfile(fd) < 0)
    {
        if (errno == EACCES || errno == EAGAIN)
        {
            close(fd);
            return 1;
        }
        exit(1);
    }
    ftruncate(fd, 0);
    sprintf(buf, "%d", getpid());
    write(fd, buf, strlen(buf) + 1);
    return (0);
}
int main(int argc, char **argv)
{
    strcpy(oj_home, "/home/judge");
    chdir(oj_home); // change the dir

    // if (!DEBUG)
    //     daemon_init(); //创建一个daemon守护进程
    // if (strcmp(oj_home, "/home/judge") == 0 && already_running())
    // {
    //     printf("%s\n", "This daemon program is already running!\n");
    //     return 1;
    // }
    //	struct timespec final_sleep;
    //	final_sleep.tv_sec=0;
    //	final_sleep.tv_nsec=500000000;
    init_mysql_conf(); // set the database info
    signal(SIGQUIT, call_for_exit);
    signal(SIGKILL, call_for_exit);
    signal(SIGTERM, call_for_exit);
    int j = 1;
    while (1)
    { // start to run
        while (j && !init_mysql_conn())
        {

            j = work(); //如果读取失败或者没有要评测的数据，那么返回0，利用那么有限的几个进程来评测无限的任务量
        }
        sleep(sleep_time);
        j = 1;
    }
    return 0;
}