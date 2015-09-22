/*
    This is a simple shell program

    by Pang Bo - ZheJiang University

注意：1.若直接用execvp函数执行无效的命令时， 会提示No such file or directory（与执行不存在的文件提示相同）
        因此要让程序提示 command not found 还需要额外检测是否是外部命令
     2.注意字符串数组(如*argv[])用形参传递, 一定要类型宽度完全一致, 比如Param[1][2]不能传给形参*argv[1]
     3.在C中strcpy中指向用于赋值的字符串的指针不能是NULL, 即strcpy(str,NULL)是不合法的(但在C++中允许)
     4.[重点]传一个常量字符串进函数,如func("path"),则函数func(char* str)执行过程中
        无论如何都不能对传入的指针str指向的字符串进行修改(即str是只读的)
     5.重定向>的实现：把一条命令执行的标准输出先用buf存起来，写入指定文件，
       < : 把文件的内容读取出来作为<前面命令的输入流(注意这里输入流不是参数,不能作为参数处理)
     6.脚本解释
     7.后台执行
     8.左右键
     9.管道｜的实现： 把一条命令执行的标准输出先用buf存起来，再写入一个文件， 再把这个［文件路径］当作参数传给｜后面的命令
     10.全部实现后拆分整理功能模块
     11.在子进程(fork()==0)改变变量不会影响主进程的变量，因此要执行改变变量的命令必须用主进程执行
     12.关键：如何把输入重定向后的命令执行结果重定向到buf内而不是输出到屏幕上
*/

/*  2015-07-20 create
 *	2015-08-10 updated
 */
#include <pwd.h>
#include <ctype.h>
#include <dirent.h>
#include <termios.h>
#include "myshell.h"
#define MAXLINE 80 /* The maximum length command */
#define PRINT(x) (printf("%s\n",x))
//#define _DEBUG

char *BUILTIN[]={"cd", "clr", "dir", "environ", "echo", "help", "quit", "myshell","set", "export", "shift", "jobs","unset","history"};     // 内建命令
int should_run = 1;                             /* flag to determine when to exit program */
Queue * ProcQueue;              //进程控制队列


void  Print_Proc(){
    int i,j;
    Process proc;
    for(j=0, i=ProcQueue->front; i<=ProcQueue->rear; j++,i++){
        printf("[%d]%d\t\t\t%s\n",j,ProcQueue->Data[i].pid,ProcQueue->Data[i].name);
    }
    if( ProcQueue->Data[ProcQueue->front].state==DONE ){
        DeleteQ(ProcQueue);
    }
    return ;
}

void Real_Path(char * path){                                //把相对路径转换成绝对路径(结果仍用原来的指针指向)
    char newdir[PATH_LENGTH];
    if( *path=='.' || *path=='~' ){
        if( *path=='~' ){                            // 以～开头的路径还需要转换
            strcpy(newdir,HOME);
            strcat(newdir,(path+1));
        }else if( *path=='.' ){                      // 执行当前目录下的文件
            strcpy(newdir,PWD);
            strcat(newdir,(path+1));
        }
        strcpy(path,newdir);
    }
}

void UpdateStatus(){                            //更新命令提示符和当前工作目录的状态以及某些环境变量(PWD,PS1)
    char cwd[PATH_LENGTH];
    getcwd(cwd,PATH_LENGTH);
    strcpy(OLD_PWD,PWD);
    Add_Var("OLD_PWD",OLD_PWD);
    strcpy(PWD,cwd);
    Add_Var("PWD",PWD);
    if( strstr(cwd,HOME) ){
        strcpy(cwd,"~");
        strcat(cwd,(&(cwd[strlen(HOME)])));
    }
    sprintf(PS1,"[myShell]%s@%s:%s$",USER,HOST,cwd);
    Add_Var("PS1",PS1);
}

void Initial(char * ShellPath){

    //所有环境变量的初始化并添加进变量表
    Add_Var("NULL","");                                 // 变量表中0号变量为NULL，空字符串，当找不到目标变量时作为返回对象
    //主要环境变量包括{ "HOST", "USER", "PATH", "PWD", "HOME", "PS1", "PS2", "SHELL" }
    char path[PATH_LENGTH];
    gethostname(HOST,NAME_LENGTH);
    Add_Var("HOST",HOST);
    strcpy(USER,getenv("USER"));
    Add_Var("USER",USER);
    strcpy(PATH,getenv("PATH"));
    Add_Var("PATH",PATH);
    strcpy(PWD,getenv("PWD"));
    Add_Var("PWD",PWD);
    strcpy(HOME,getenv("HOME"));
    Add_Var("HOME",HOME);
    strcpy(path,ShellPath);
    Real_Path(path);
    strcpy(SHELL,path);                                          //环境变量SHELL等于此可执行文件myshell所在目录
    Add_Var("SHELL",SHELL);
    Add_Var("OLD_PWD",PWD);
    Add_Var("PS1","");
    Add_Var("PS2",">");

    UpdateStatus();
    /* 初始化外部命令名称数组Exec_Command */

    int i,k;
    char *path_ptr;
    struct dirent * dir_ptr;
    DIR * dir_stream;

    strcpy(path,PATH);
    path_ptr = path;
    Tree = (ListNode*)Create_Node();
    for(i=0,k=0 ; i<strlen(PATH); i++){                                                         // 把环境变量PATH中所有目录下的可执行程序都加进字典树
        if( path[i]==':' ){
            path[i] = '\0';
            dir_stream = opendir(path_ptr);
            while( dir_ptr=readdir(dir_stream) ){
                if( isalpha(*(dir_ptr->d_name)) && dir_ptr->d_type==DT_REG ){                    //如果开头是字母且文件类型为普通文件的则存入命令集
//                    strcpy(Exec_Command[k++],dir_ptr->d_name);
                    WORD word;
                    strcpy(word,dir_ptr->d_name);
                    Trie_Insert(word,path_ptr);
//                    printf("%s\n", Exec_Command[k-1]);
                }
            }
            closedir(dir_stream);
            path_ptr = (path+i+1);
        }
    }
    for( i=0; i<sizeof(BUILTIN)/sizeof(char*); i++){                                  // 把所有内建命令加入字典树
        WORD word;
        strcpy(word,BUILTIN[i]);
        Trie_Insert(word);
    }
//    printf("test : %d",isWord("bash"));
    // 初始化参数表
    Arg_Var_Initial();
    ProcQueue = CreateQueue();
    return ;
}

void Input_Completion(char *Input){
    //1.如果当前目录下存在以Input字符串开头的文件则补全其中一个文件名
    struct dirent * dir_ptr;
    DIR * dir_stream = opendir(PWD);
    char tmpstr[NAME_LENGTH];
    int length,i;
    length = strlen(Input);
    // 先把要补全的对象确定下来(以免有空格影响)
    for( i=0 ;i<length; i++){
        if( Input[i]==' ' && i+1<length )
        Input = Input+i+1;
    }
    length = strlen(Input);
    while( dir_ptr=readdir(dir_stream) ){
        strcpy(tmpstr,dir_ptr->d_name);
        if( strlen(tmpstr)>=length ){
            tmpstr[length] = '\0';
            if( strcmp(tmpstr,Input)==0 ){                      //找到了以Input开头的一个文件名,开始补全
                strcpy(Input,dir_ptr->d_name);
                for( i=length; i>0; i--){                       //清空输入
                    printf("\b \b");
                }
                printf("%s",Input);
                return ;
            }
        }
    }

    //2.如果1不成立(目录下没有以Input开头的文件名)， 则列出以Input字符串开头的所有命令(每行五个,命令提示符在下面更新)
    LinkList* pList = (LinkList*)Create_List();
    strcpy(tmpstr,Input);
    strcat(tmpstr,"*");                                         //*则表示搜索对象后面可以有不定长的字符串
    Trie_Search(Tree,pList,tmpstr);
    ListNode * pNode = pList->head;
    if( pNode ){                                                //存在以input开头的命令，另起一行来输出
        printf("\n");
        int i = 0;
        while( pNode ){
            printf("%s\t\t",pNode->entry);
            i += 1;
            if( i==5 ){
                printf("\n");
                i = 0;
            }
            pNode = pNode->next;
        }
        printf("\n%s ",PS1);
        printf("%s",Input);
    }
    return ;
}

void Var_Substitute(int argc, char* argv[PARAM_MAX]){          // 把变量的引用用变量的值替换掉， 去掉引号
    // 一个字符串可以归为3类：带双引号(1)， 带单引号(2)， 不带引号(3),带反引号(4?) 默认输出为且仅为其中一类(且默认一个字符串(已经用空格分割出来的)只出现一对引号)
    int i,j,k,flag,length;
    for(i=0; i<argc; i++){                      // 以空格为分隔， 一个一个参数输出
        char * new_argv = (char*)malloc(NAME_LENGTH);
        char * str      = (char*)malloc(NAME_LENGTH);      // 用str来指向当前处理的参数
        strcpy(str,argv[i]);
        flag = 3;                                         // 默认是第三种情况
        if( str==NULL ){
            continue;
        }
        length = strlen(str);
        for( j=0; j<length; j++){               // (如果有的话)j为第一个引号的下标
            // 1.先处理引号
            if( str[j]=='\"' || str[j]=='\'' ){
                if( j>0 && str[j-1]=='\\' ){    // 引号被转义了， 不算
                    continue;
                }
                switch(str[j]){
                    case '\"' : flag = 1;
                                break;
                    case '\'' : flag = 2;
                                break;
                }
                str = str+1;                    // 跳过第一个引号
                break;                          // 找到第一个有效的引号可以跳出循环
            }
        }
        // 2.再处理变量
        if( str!=NULL && *str=='$' ){                    // 带有$, 输出变量的值
            if( flag==1 || flag==2 ){
                for( j=j+1 ;j<strlen(str); j++){            //定位第二个出现的引号
                    if( str[j]=='\"' || str[j]=='\''){
                        str[j] = '\0';
                    }
                }
                char * var_name;
                if( flag==1 ){                                   // 带双引号，是取变量
                    var_name = str+1;
                    strcpy(new_argv,Get_Var(var_name));
                }else{                                          // 单引号， 不处理
                    var_name = str;
                    strcpy(new_argv,var_name);
                }
                strcat(new_argv,str+j+1);                       // 引号之外的字符串原样输出
            }else{
                char *var_name = str+1;
                strcpy(new_argv,Get_Var(var_name));
            }
        }else{                                          //不带$，输出语句
            if( flag==1 || flag==2 ){
                for( j=0 ;j<strlen(str); j++){
                    if( str[j]=='\"' || str[j]=='\''){
                        str[j] = '\0';
                    }
                }
                strcpy(new_argv,str);
                strcat(new_argv,str+j+1);
            }else{
                strcpy(new_argv,str);
            }
        }
        argv[i] = new_argv;
    }
    return ;
}

void Execute(int argc, char *argv[PARAM_MAX]){
    char * command = argv[0];
    //1.检查是否为内建命令
    int i;
    for( i=0; i<sizeof(BUILTIN)/sizeof(char*); i++){
        if( strcmp(command,BUILTIN[i])==0 ){                    //是第i个内建命令
/*BUILTIN[]={"cd", "clr", "dir", "environ", "echo", "help", "quit", "myshell","set", "export", "shift", "jobs","unset"};     // 内建命令 */
            switch(i){
                case 0: cd(argv[1]);
                        break;
                case 1: clr();
                        break;
                case 2: dir(argv[1]);
                        break;
                case 3: environ();
                        break;
                case 4: echo(argc,argv);
                        break;
                case 5: help();
                        break;
                case 6: quit();
                        break;
                case 7: myshell(argc,argv);
                        break;
                case 8: set(argc,argv);
                        break;
                case 9: export(argc,argv);
                        break;
                case 10:shift(argc,argv);
                        break;
                case 11:jobs();
                        break;
                case 12:unset(argc,argv);
                        break;
                case 13:history(argc,argv);
                        break;
            }
            return ;
        }
    }
    //2.检查是否是可执行文件
    if( *command=='.' || *command=='~' || *command=='/' ){
        Real_Path(command);
        argv[0] = command;
        execv(command,argv);
        //执行失败， 不存在的文件或权限不足等
        printf("%s: ",command);
        Print_Error(errno);
        return ;
    }
    //3.是外部命令
    if( fork()==0 ){
        execvp(command,argv);
        strcpy(Arg_Var[0]->value,"1");
        // 外部命令执行失败， 利用字典树检查是否为外部命令，若非外部命令则在下面列出相似的命令(仅相差一个字母的)
        LinkList *pList = (LinkList*)Create_List();
        char tmpstr[NAME_LENGTH];                  // 存储要搜索的命令
        strcpy(tmpstr,"?");
        strcat(tmpstr,command);
        for( i=0; i<=strlen(command) && i<NAME_LENGTH && strlen(command)>1; i++){
            strcpy(tmpstr,command);
            tmpstr[i] = '?';
            Trie_Search(Tree, pList, tmpstr);       // 搜索类似命令并自动存入pList指向的链表中
        }
        ListNode * pNode = pList->head;
        printf("No command '%s' found",command);
        if( pNode ){
            printf(", did you mean:\n");
            while( pNode ){
                printf("Command %s from %s\n",pNode->entry,pNode->filepath);
                pNode = pNode->next;
            }
        }else{                                      // 没有类似的命令
            printf("\n");
        }
        exit(0);
    }else{
        wait();
    }
    return ;
}

void Input_Parse(char *Input){                              //默认每个命令，符号两边都有空格
    /* 拆分参数和命令 */
    char *Param[PARAM_MAX],*command;                        //［重点］：传进exec函数的参数数组必须是定长的
    while( *Input==' ' ){                                   //清除前面的空格（如果有）
        Input++;
    }
    int length = strlen(Input);
    if( length==0 ){
        return;
    }
    int i,Param_num;
    memset(Param,0,sizeof(Param));
    command=Param[0]=Input;
    Param_num = 1;
    for( i=1; i<length; i++){
        if( Input[i]=='=' ){                            // 一旦有等号存在， 一律归为赋值命令
            Input[i] = '\0';
            Assign(Input,(Input+i+1));
            return ;
        }
        if( Input[i]==' ' ){
            do{
                Input[i]='\0';
                i++;
            }while( Input[i]==' ' );
            i -= 1;
            Param[Param_num++] = &(Input[i+1]);
        }
    }
    Var_Substitute(Param_num,Param);                            // 把一个个整理好的字符串替换其中的变量，处理引号
//    char buf[BUF_LENGTH];
//    memset(buf,0,sizeof(buf));
//    Get_Command_Output(buf,Param_num,Param);
//    printf("%s",buf);
//    Execute(Param_num,Param);
    Input_Analysis(Param_num,Param);
    return ;
}

void Input_Analysis(int argc, char *argv[PARAM_MAX]){
    /* 命令分析 :分析被拆分的参数中的重定向和管道执行命令*/
    int i,real_argc;
    char * real_argv[PARAM_MAX];
    memset(real_argv,0,sizeof(char*)*PARAM_MAX);
    char buf[BUF_LENGTH];
    FILE * fp;
    char tempfile[] = "tempfile-XXXXXX";
    int tempfd = mkstemp(tempfile);
//    Get_Command_Output(buf,Param_num,Param);
    for( i=0, real_argc=0; i<argc && argv[i]!=NULL; i++){   // 从左到右一一处理
        if( *(argv[i])=='>' ){                      // 输出写入文件(文件名必须为>后面那个参数)      // 1>&2额外处理
            memset(buf,0,sizeof(buf));
            char *filename = argv[++i];
            if( argv[i][1]=='>' ){
                fp = fopen(filename,"a");
            }else{
                fp = fopen(filename,"w");
            }
            Get_Command_Output(buf,real_argc,real_argv);
            write(tempfd,buf,sizeof(buf));
            memset(real_argv,0,sizeof(char*)*PARAM_MAX);
            real_argc = 0;
            if( fp==NULL || fwrite(buf,strlen(buf),1,fp)==0 || fclose(fp)==-1 ){
                Print_Error(errno);
                return ;
            }
        }else if( *(argv[i])=='<' ){                // 从文件读取输入(文件名必须为>后面那个参数)
            memset(buf,0,sizeof(buf));
            fp = fopen(argv[++i],"r");
            if( fp==NULL || fread(buf,1,sizeof(buf),fp)==0 || fclose(fp)==-1 ){
                Print_Error(errno);
                return ;
            }
            Put_Command_Input(buf,real_argc,real_argv);
            write(tempfd,buf,sizeof(buf));
            memset(real_argv,0,sizeof(char*)*PARAM_MAX);
            real_argc = 0;
        }else if( *(argv[i])=='&' ){                             //后台执行命令
            pid_t pid = vfork();                                 //用vfork保证子进程先运行
            int num = ProcQueue->rear+1;                         //在数组DATA中的序号
            if( pid==0 ){
                Process newProc;
                int k;
                for(k=0; k<real_argc; k++){
                    strcat(newProc.name,real_argv[k]);
                }
                newProc.number = num+1;
                newProc.pid = pid;
                newProc.state = RUNNING;
                AddQ(ProcQueue,newProc);
                printf("[%d] %d\n",newProc.number,newProc.pid);
                exit(0);
            }else{
                if( fork()==0 ){
                    execvp(real_argv[0],real_argv);
                    Print_Error(errno);
                    exit(0);
                }
                ProcQueue->Data[num].state = DONE;
            }
            memset(real_argv,0,sizeof(char*)*PARAM_MAX);
            real_argc = 0;
        }else {
            real_argv[real_argc++] = argv[i];
        }
    }
    remove(tempfile);
    if( real_argc==i ){                 //若检测完毕都没有一个argv[i]有重定向或管道时则直接执行execvp+命令
        Execute(argc,argv);
    }
    return ;
}

char getch(){
    char buf=0;
    struct termios old={0};
    fflush(stdout);
    if(tcgetattr(0, &old)<0)
        Print_Error(errno);
    old.c_lflag&=~ICANON;
    old.c_lflag&=~ECHO;
    old.c_cc[VMIN]=1;
    old.c_cc[VTIME]=0;
    if(tcsetattr(0, TCSANOW, &old)<0)
        Print_Error(errno);
    if(read(0,&buf,1)<0)
        Print_Error(errno);
    old.c_lflag|=ICANON;
    old.c_lflag|=ECHO;
    if(tcsetattr(0, TCSADRAIN, &old)<0)
        Print_Error(errno);
//    printf("%c\n",buf);
    return buf;
}



int Clear_Line(int n){
    printf("\r");
    printf("%s ",PS1);
    int i;
    for( i=n; i>0; i-- ) printf(" ");
    for( i=n; i>0; i-- ) printf("\b");
    return n;
}



void Test(){

#ifdef _DEBUG

    char *param[10] = {"echo", "$SHELL"};
    if( fork()==0 ){
        execvp("echo",param);
    }else{
        wait();
    }

#endif // _DEBUG
//    FILE * fp = fopen("fdata","a");
//    char buf[] = "asd";
//    fwrite(buf,sizeof(buf),1,fp);
//    fclose(fp);
//    exit(0);
    wait();
    return ;
}

int main(int argc, char *argv[]){
    char *args[MAXLINE/2 + 1]; /* command line arguments */
    char Input[INPUT_MAX];
    History_Size = History_Ptr = 0;

    Initial(argv[0]);

    memset(Input,0,sizeof(Input));
    memset(History,0,sizeof(History));
    Test();
    int nread;
    while (should_run) {
        printf("%s ",PS1);
        fflush(stdout);
        unsigned char ch;
        int index = 0, length=0;
        memset(Input,0,sizeof(Input));
        do{
            //Show Input String
            Clear_Line(length+1);
            length = printf("%s",Input);
            int k;
            for( k=index; k<length; k++){
                printf("\b");
            }
            //Get Input
            ch=getch();
            if( isprint(ch) ){
                if( index>=1 && Input[index-1]=='['){         //满足则一定是方向键
                    switch(ch){
                        case 'A' :  Array_Delete(length--,Input,index-1);   //删除'['
                                    index--;
                                    if( History_Ptr>0 ){                            //方向键上
                                        History_Ptr -= 1;
                                        strcpy(Input,History[History_Ptr]);
                                        Clear_Line(length+1);
                                        index = length = strlen(Input);
                                    }
                                    break;
                        case 'B' :  Array_Delete(length--,Input,index-1);   //删除'['
                                    index--;
                                    Clear_Line(length+1);
                                    if( History_Ptr<History_Size ){                 //方向键下，判断是否可以调用历史记录
                                        History_Ptr += 1;
                                        strcpy(Input,History[History_Ptr]);
                                        index = length = strlen(Input);
                                    }else{
                                        strcpy(Input,"");
                                        index = length = 0;
                                    }
                                    break;
                        case 'C' :  Array_Delete(length--,Input,index-1);   //删除'['
                                    index--;
                                    if( index<length ){                             //方向键右
                                        index++;
                                    }
                                    break;
                        case 'D' :  Array_Delete(length--,Input,index-1);   //删除'['
                                    index--;
                                    if( index>0 ){                                  //方向键左
                                        index--;
                                    }
                                    break;
                    }
                    ch = '\0';
                    continue;
                }else{
                    Array_Insert(length,Input,index,ch);     // Insert(数组长度，数组指针，插入位置下标，插入字符)
                    index++;
                    length++;
                }
            }else if( ch==9 && index==length ){                //TAB键(光标必须在末尾)
                Input_Completion(Input);
                index = strlen(Input);
            }else if( ch == 127 && index>0 ){                           //退格键
                Array_Delete(length,Input,index-1);
                index--;
                length--;
                continue;
            }
        }while( ch!='\n');
        Array_Fill(index,Input,'\0');
        printf("\n");
        if( strlen(Input)==0 ){                             // 输入为空时则不放入历史记录也不进行分析
            continue;
        }
        strcpy(History[History_Size++],Input);
        History_Ptr = History_Size;
        Input_Parse(Input);
        /**
        * After reading user input, the steps are:
        *内部命令：
        *…..
        *外部命令：
        * (1) fork a child process using fork()
        * (2) the child process will invoke execvp()
        * (3) if command included &, parent will invoke wait()
        * ..
        */
    }
    quit();
}

/*
编程开发一个 shell 程序
shell 或者命令行解释器是操作系统中最基本的用户接口。写一个简单的
shell 程序——myshell，它具有以下属性：
(一) 这个 shell 程序必须支持以下内部命令：
1) cd <directory> ——把当前默认目录改变为<directory>。如果没有
<directory>参数，则显示当前目录。如该目录不存在，会出现合适的错
误信息。这个命令也可以改变 PWD 环境变量。
2) clr ——清屏。
3) dir <directory> ——列出目录<directory>的内容。
4) environ ——列出所有的环境变量。
5) echo <comment> ——在屏幕上显示<comment>并换行（多个空格和制
表符可能被缩减为一个空格）。
6) help ——显示用户手册，并且使用 more 命令过滤。
7) quit ——退出 shell。
8) shell 的环境变量应该包含 shell=<pathname>/myshell，其中
<pathname>/myshell 是可执行程序 shell 的完整路径（不是你的目录下的
硬连线路径，而是它执行的路径）。
(二) 其他的命令行输入被解释为程序调用，shell 创建并执行这个程序，并作为
自己的子进程。程序的执行的环境变量包含一下条目：
parent=<pathname>/myshell。
(三) shell 必须能够从文件中提取命令行输入，例如 shell 使用以下命令行被调
用：
myshell batchfile
这个批处理文件应该包含一组命令集，当到达文件结尾时 shell 退出。很明
显，如果 shell 被调用时没有使用参数，它会在屏幕上显示提示符请求用户输入。
(四) shell 必须支持 I/O 重定向，stdin 和 stdout，或者其中之一，例如命令行为：
programname arg1 arg2 < inputfile > outputfile
使用 arg1 和 arg2 执行程序 programname，输入文件流被替换为 inputfile，
输出文件流被替换为 outputfile。
stdout 重定向应该支持以下内部命令：dir、environ、echo、help。
使用输出重定向时，如果重定向字符是>，则创建输出文件，如果存在则覆
盖之；如果重定向字符为>>，也会创建输出文件，如果存在则添加到文件尾。
(五) shell 必须支持后台程序执行。如果在命令行后添加&字符，在加载完程序
后需要立刻返回命令行提示符。
(六) 命令行提示符必须包含当前路径。
提示：
1) 你可以假定所有命令行参数（包括重定向字符<、>、>>和后台执行字符&）
和其他命令行参数用空白空间分开，空白空间可以为一个或多个空格或制表
符（见上面（四） 中的命令行）。
2) 程序的框架：

项目要求 ：
1) 设计一个简单的全新命令行 shell，满足上面的要求并且在指定的 Linux 平
台上执行。用 不能使用 system  函数调用原 shell  程序运行外部命令。拒绝使用
的 已有的 shell  程序的任何 环境及功能。
2) 写一个关于如何使用 shell 的简单的用户手册，用户手册应该包含足够的细
节以方便 Linux 初学者使用。例如：你应该解释 I/O 重定向、程序环境和后
台程序执行。用户手册必须命名为 readme，必须是一个标准文本编辑器可以
打开的简单文本文档。
3) 源码必须有很详细的注释，并且有很好的组织结构以方便别人阅读和维护。
结构和注释好的程序更加易于理解，并且可以保证批改你作业的人不用很
费劲地去读你的代码。
4) 在截止日期之前，要提供很详细的提交文档。
5) 提交内容为源码文件，包括文件、makefile和readme文件。批改作业者会重
新编译源码，如果编译不通过将没办法打分。
6) makefile 文件必须能用 make 工具产生二进制文件 myshell，即命令提示符下
键入 make 就会产生 myshell 程序。
7) 根据上面提供的实例，提交的目录至少应该包含以下文件：
makefile
myshell.c
utility.c
myshell.h
readme
提交 ：
需要 makefile 文件，所有提交的文件将会被复制到一个目录下，所以不要
在 makefile 中包含路径。makefile 中应该包含编译程序所需的依赖关系，如果
包含了库文件，makefile 也会编译这个库文件的。
为了清楚起见，再重复一次：不要提交二进制或者目标代码文件。所需的只
是源码、makefile 和readme 文件。提交之前测试一下，把源码复制到一个空目
录下，键入 make 命令编译。
所需的文档 要求：
首先，源码和用户手册都将被评估和打分，源码需要注释，用户手册可以是
你自己选择的形式（但要能被简单文本编辑器打开）。其次，手册应该包含足够
的细节以方便 Linux 初学者使用，例如，你应该解释 I/O 重定向、程序环境和
后台程序执行。用户手册必须命名为 readme。

*/
