#ifndef MYSHELL_H
#define MYSHELL_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#define COMMAND_MAX     2048                        // 外部命令(PATH所有目录中的可执行程序)最大条数
#define VAR_MAX         1024                        // 本地变量最大个数
#define BUF_LENGTH      1024                        // 缓存最大长度
#define INPUT_MAX       256                         // 一次命令行输入最大长度
#define PATH_LENGTH     256                         // 路径最大长度
#define HISTORY_MAX     128                         // 最大保存历史记录条数
#define BUILTIN_MAX     128                         // 内建命令最大条数
#define PARAM_MAX       128                         // 参数最多个数
#define PROCESS_MAX     128                         // 进程最大个数
#define NAME_LENGTH     64                          // 主机名， 用户名，文件名等的最大长度
#define PROMPT_LENGTH   64                          // 命令提示符最大长度
#define VAR_LENGTH      PATH_LENGTH                 // 变量的值的最大长度

#define QueueElem Process
#define ListElem  WORD

/*              环境变量                            */
char HOME[PATH_LENGTH];
char SHELL[PATH_LENGTH];                            //注意SHELL不是系统变量，只是某个SHELL下的本地变量
char PATH[PATH_LENGTH];
char PWD[PATH_LENGTH];                              //注意PWD和HOME都应该是完整(无～)的路径名
char OLD_PWD[PATH_LENGTH];
char PS1[PROMPT_LENGTH];                            //命令提示符
char PS2[PROMPT_LENGTH];
char USER[NAME_LENGTH];
char HOST[NAME_LENGTH];

typedef struct Var_Stru Varialble;
struct Var_Stru{
    char name[NAME_LENGTH];                         //变量名
    char value[VAR_LENGTH];                         //变量值
};
Varialble * Var[VAR_MAX];                           //变量表(变量数组)
int Var_Num ;
Varialble * Arg_Var[VAR_MAX];                            //全局参数表，Arg_Var[0]对应$?,[1]~[9]依次对应$1~$9, shift可往前移
int Arg_Var_Num ;

enum PROCESS_STATE{
    DONE = 1,
    RUNNING = 2,
    STOPPED = 3,
};

typedef struct Process_Stru Process;
struct Process_Stru{
    int number;
    pid_t pid;
    enum PROCESS_STATE state;
    char name[INPUT_MAX];
};

char History[HISTORY_MAX][INPUT_MAX];           //输入历史
int  History_Size, History_Ptr;                 //已存储的历史记录条数和浏览历史时的指针


/*                       数据结构                    */

#define CONFLICT 5                                  //允许存放在同一个结点(同名)的单词最多个数
#define SIZE 40                                     //一个结点最大字节点数目
typedef char ListElem[NAME_LENGTH];                         //文件名

typedef struct TrieNode_Stru TrieNode;
struct TrieNode_Stru{                   // 字典树(多叉树)结点数据结构
    ListElem Word[CONFLICT];                //存放文件名，只有当前TrieNode的isWord==1才能访问
    char filepath[NAME_LENGTH];         //存放文件所在路径
    char Letter;                        //当前节点存放的字母
    int isWord;                        //从根节点到该节点处是否形成一个单词
    int number;                         //这个结点存放的单词个数(为了使字母构成相同的单词都能输出)
    TrieNode* Next[SIZE];               //所有子节点(开始时都是空的,要用到才自行申请内存)
};

TrieNode* Tree;

typedef struct ListNode_Stru ListNode;
struct ListNode_Stru{
    ListElem entry;
    char filepath[NAME_LENGTH];
    ListNode * next;
};

typedef struct LinkList_Stru LinkList;
struct LinkList_Stru{
    ListNode * head;
    ListNode * tail;
    int number;
};

// 顺序结构存储的队列
typedef struct Queue_Stru{
	QueueElem Data[PROCESS_MAX];
	int front;
	int rear;
	int size;
} Queue ;




#endif // MYSHELL_H
