#ifndef _CONFIGER_H_
#define _CONFIGER_H_

#define configer_buffer_size 1024

/**
 * 读取string
 */
bool read_buf(char *buf, const char *key, char *value);
/**
 * 清除字符串前后的空白
 * str 待操作字符串
 */
void trim(char *str);
/**
 * 定位到 ‘=’ 后面
 * str 待定位字符串 
 */
int after_equal(char *str);
/**
 * 读取int
 */
void read_int(char *buf, const char *key, int *value);

#endif