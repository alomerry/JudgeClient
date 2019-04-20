#define BUFF_SIZE 512
#define DEBUG 1
#include <iostream>
#include <unistd.h>
#include <mysql/mysql.h>
#include <string.h>
using namespace std;

MYSQL *conn;
static int Mode = 0;
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

    if (Mode == DEBUG) //Debug Mode
    {
        printf("初始化数据库中......\n\thost_name(%s)\n\tuser_name(%s)\n\tdb_name(%s)\n\t初始化完毕。\n", host_name, user_name, db_name);
    }
}

int main(int argc, char **argv)
{
    strcpy(oj_home, "/home/judge");
    chdir(oj_home); // change the dir

    if (!DEBUG)
        daemon_init(); //创建一个daemon守护进程
    if (strcmp(oj_home, "/home/judge") == 0 && already_running())
    {
        syslog(LOG_ERR | LOG_DAEMON,
               "This daemon program is already running!\n");
        return 1;
    }
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
        //这个while的好处在于，只要一有任务就抓紧占用系统优先把所以任务处理完成，哪怕会空循环几次的可能存在
        //但是没有任务后，就会进入到“懒散”的 休息sleep(time)后再轮询是不是有任务，释放系统的资源，避免Damon一直
        //死循环占用系统
        while (j && (http_judge || !init_mysql()))
        {

            j = work(); //如果读取失败或者没有要评测的数据，那么返回0，利用那么有限的几个进程来评测无限的任务量
        }
        sleep(sleep_time);
        j = 1;
    }
    return 0;
}