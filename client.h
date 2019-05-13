#ifndef _CLIENT_H_
#define _CLIENT_H_

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

#endif