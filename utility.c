#include "myshell.h"
#include <fcntl.h>
/*===========================          数组操作         ====================================*/


void Array_Insert(int len, char array[], int index, char ch){
    // 传进来的index指的是数组下标
    if( index>len ) return ;
    int i;
    // 先把index后的元素都往后移动
    for(i=len; i>index; i--){
        array[i] = array[i-1];
    }
    array[i] = ch;
    return ;
}

void Array_Fill(int index, char array[], char ch){
    array[index] = ch;
    return ;
}

void Array_Delete(int len, char array[], int index){
    if( index>len ) return ;
    int i;
    for( i=index; i<len-1; i++){
        array[i] = array[i+1];
    }
    array[i] = '\0';
    return;
}

/*===========================          变量操作         ====================================*/


int Find_Var(char *var_name){                   //找出变量在表中的编号返回
    int i;
    for( i=1; Var[i]!=NULL; i++){
        if( strcmp(Var[i]->name,var_name)==0 ){
            return i;
        }
    }
    return 0;                                  //找不到则返回0
}

char * Get_Var(char * var_name){                // 返回var_name对应的变量的值, 注意不能让其返回NULL
    int i;
    if( var_name==NULL ) return "";
    else if( *var_name >= '0' && *var_name<='9' ){
        if( Arg_Var[*var_name-'0'] && Arg_Var[*var_name-'0']->value )
            return (Arg_Var[*var_name-'0']->value);
    }else if( *var_name=='?' ){
        if( Arg_Var[0] && Arg_Var[0]->value )
            return Arg_Var[0]->value;
    }else if( *var_name=='#' ){
        char * str = malloc(4);
        sprintf(str,"%d",Arg_Var_Num);
        if( str )
            return str;
    }else if( *var_name=='*' || *var_name=='@' ){       // 都是列出所有全局参数(其实这两个得到的字符串有所不同）
        char * str = malloc(NAME_LENGTH);
        int i;
        memset(str,0,sizeof(str));
        for( i=1; i<=Arg_Var_Num; i++){
            strcat(str,Arg_Var[i]->value);
            strcat(str," ");
        }
        if( str )
            return str;
    }else{
        if( Var[Find_Var(var_name)] && Var[Find_Var(var_name)]->value )
            return (Var[Find_Var(var_name)]->value);
    }
    return "";
}

void Add_Var(char const* var_name, char const* var_value){
    // 如果变量名对应变量已经存在则把该变量的值改为新的值， 否则添加新的变量
    int i = Find_Var(var_name);
    if( i != 0 ){                                           // 返回值不为0就表示在表中搜索到了
        strcpy(Var[i]->value,var_value);
    }else{
        Var[Var_Num] = (Varialble*)malloc(sizeof(Varialble));
        strcpy(Var[Var_Num]->name,var_name);
        strcpy(Var[Var_Num]->value,var_value);
        Var_Num++;
    }
    return ;
}

void Arg_Var_Initial(){
    int i;
    for( i=0; i<VAR_MAX; i++){
        Arg_Var[i] = (Varialble*)malloc(sizeof(Varialble));
        strcpy(Arg_Var[i]->name,"");
        strcpy(Arg_Var[i]->value,"");
    }
    strcpy(Arg_Var[0]->value,"0");                  //Arg_Var[0] == $?
    Arg_Var_Num = 1;
    return ;
}

/*===========================        执行输入输出         ====================================*/


void Get_Command_Output(char buf[BUF_LENGTH],int argc, char *const argv[]){               // 命令执行的结果存储在Arg_Var[0]
    pid_t pid;
    int mypipefd[2];              // 0->read 1->write
    int *read_fd = &mypipefd[0];
    int *write_fd = &mypipefd[1];
    if( pipe(mypipefd)==-1 ){                           //创建管道
        perror("pipe");
        exit(1);
    }
    Arg_Var_Initial();
    memset(buf,0,BUF_LENGTH);
    pid = fork();
    if( pid==0 ){
        // Child Process
        close(*read_fd);
        // 把标准输出重定向到新建立的管道(写入端为1)
        dup2(*write_fd,STDOUT_FILENO);                  // STDOUT -> fd[1]
        // 把fd[1]关掉(STDOUT仍能写入管道)
        close(*write_fd);                                   //命令执行失败返回1
        Execute(argc,argv);
        exit(1);
    }else{
        // Parent Process
        close(*write_fd);
        wait(NULL);
        read(*read_fd,buf,BUF_LENGTH);
        close(*read_fd);
    }
}

void Put_Command_Input(char buf[BUF_LENGTH],int argc, char *const argv[]){
    int pid = 0;
    if (pid = fork()) {
        int status;
        waitpid(pid, &status, 0);
    }else {
        char tempinput[] = "tempinput-XXXXXX";
        char tempoutput[] = "tempoutput-XXXXXX";
        int fd_in = open(tempinput, O_RDONLY);
        int fd_out = open(tempoutput, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        write(fd_in,buf,sizeof(buf));
        if (fd_in > 0 && fd_out > 0) {
            dup2(fd_in, 0);
            dup2(fd_out, 1);
            Execute(argc,argv);
            close(fd_in);
            close(fd_out);
        }
        remove(tempinput);
        remove(tempoutput);
    }
    return ;
}

/*===========================          内建命令         ====================================*/


int cd(char * path){
    if( path==NULL ){                                   //cd后面不加路径则转到主目录
        path="~";
    }
    if( *path=='~' ){                                   //若有～，转换成实际地址
        char real_path[PATH_LENGTH];
        strcpy(real_path,HOME);
        strcat(real_path,&(path[1]));
        path=real_path;
    }
    if( chdir(path) == -1 ){
        printf("%s :",path);
        Print_Error(errno);
    }
    UpdateStatus();
    return 0;
}

int clr(){
    printf("\033c");
    return 0;
}

int dir(char * path){
    if( path!=NULL && *path=='~' ){                                   //若有～，转换成实际地址
        char real_path[PATH_LENGTH];
        strcpy(real_path,HOME);
        strcat(real_path,&(path[1]));
        path=real_path;
    }
    char * Param[PARAM_MAX] = {"ls",path};                            //不能以char * Param[]={"ls",path}的方式定义参数数组
    if( fork()==0 ){
        execvp("ls",Param);
        Print_Error(errno);
    }else{
        wait();
    }
    return 0;
}

int environ(){
    /* 列出环境变量：cwd, home, path, PS1, PS2, shell, term, user */
    int i;
    for( i=0; i<Var_Num; i++){
        printf("%s=%s\n",Var[i]->name,Var[i]->value);
    }
//    printf("TERM=%s\n",TERM);
    return 0;
}

int echo(int argc, char const * argv[]){
    int i;
    for(i=1; i<argc; i++){                      // 以空格为分隔， 一个一个参数输出
        printf("%s ",argv[i]);
    }
    printf("\n");
}

int jobs(){
    if( fork()==0 ){
        execlp("ps","ps");
        exit(0);
    }
    return 0;
}

int help(){
    char helpInfo[] = "Myshell beta v0.9                                        \n\
These shell commands are defined internally.  Type `help' to see this list.     \n\
Type `man commmand' to find out more about the commmand.                        \n\
Use `info program' to find out more about a program in general.                 \n\
Get more information about this shell at the text file Readme.                  \n\
                                                                                \n\
    Builtin commands                      introduction                          \n\
cd [dir]                          change current working directory              \n\
clr                               clear screen                                  \n\
dir [dir]                         list all the names of files in dir            \n\
environ                           list all environment variables                \n\
echo [comment] [$(varialble)]     print out messages or value                   \n\
help                              show this list                                \n\
quit                              normal process terminate                      \n\
myshell (filename)                interpret and execute a shell script          \n\
set [command]                     list variables or set argument variables      \n\
export [(var)=(value)]            create new environment variables              \n\
history                           list your input history                       \n\
                                                                                \n";
    int tempfd;
    char tempfile[] = "tempfile-XXXXXX";
    tempfd = mkstemp(tempfile);                     //建立临时文件
    write(tempfd,helpInfo,sizeof(helpInfo));
    if( fork()==0 ){
        execlp("more","more","-5",tempfile);
        exit(1);
    }else{
        wait();
    }
    remove(tempfile);
}

int quit(){
    exit(0);
}

int myshell(int argc, char const * argv[]){
    /* 命令格式: "myshell shellScript/command argv1 argv2" */
    // 用myshell来解释一个shell脚本文件
    char filepath[PATH_LENGTH];
    char linebuf[BUF_LENGTH],*tmp;
    char sh[PATH_LENGTH];

    if( argc==1 ){
        fgets(filepath,PATH_LENGTH,stdin);
    }else{
        strcpy(filepath,argv[1]);
    }
    FILE * fp;
    if( (fp=fopen(filepath,"r"))==NULL ){
        Print_Error(errno);
        return 1;
    }
    // 先读第一行判断是否为#!开头
    fgets(linebuf,BUF_LENGTH,fp);
    if( (tmp=strstr("#!",linebuf))!=NULL ){
        strcpy(sh,linebuf+2);
        fgets(linebuf,BUF_LENGTH,fp);
    }else{
        strcpy(sh,SHELL);
    }
    do{
        linebuf[strlen(linebuf)-1] = '\0';                  //去掉换行符
        if( strlen(linebuf)>0 ){
            if( (tmp=strchr(linebuf,'#'))!=NULL ){          //有注释
                *tmp = '\0';
            }
            Input_Parse(linebuf);
        }
        if( feof(fp) ){
            break;
        }
        fgets(linebuf,BUF_LENGTH,fp);
    }while( 1 );
    return 0;
}

int set(int argc, char const * argv[]){
    // 枚举set命令可能的几种合法形式
    if( argc==1 ){                                          //无附加参数，则列出所有变量
        environ();
    }else{                                                  //有附加参数，
        char buf[BUF_LENGTH];
        Get_Command_Output(buf,argc-1,&(argv[1]));
        if( strcmp(Arg_Var[0]->value,"0")==0 ){                                   // 返回值为0则表示执行成功
            int i,length;
            length = strlen(buf);
            char * ptr = buf;
            for( i=0; i<length; i++){                          // 命令执行成功，把命令执行的结果放到全局参数表
                if( buf[i]==' ' ){
                    buf[i] = '\0';
                    strcpy(Arg_Var[Arg_Var_Num++]->value,ptr);                  // 全局参数只有变量值没有变量名,只能用$?,$1~9来访问
                    ptr = (buf+i+1);
                }
            }
            strcpy(Arg_Var[Arg_Var_Num++]->value,ptr);
        }
    }
    return 0;
}

int unset(int argc, char const * argv[]){
    int i;
    for( i=1; i<argc; i++){
        int number = Find_Var(argv[i]);
        strcpy(Var[number]->value,"");
    }
}


void export(int argc, char const * argv[]){
    int i;
    if( argc==1 ){
        for( i=0; i<Var_Num; i++){
            printf("declare -x %s = %s\n",Var[i]->name,Var[i]->value);
        }
    }else{
        for( i=1; i<argc; i++){
            char tmpstr[NAME_LENGTH];
            sprintf(tmpstr,"%s = %s",argv[i],Get_Var(argv[i]));
            putenv(tmpstr);
        }
    }
}

void shift(int n){
    int i;
    for( i=1; i<Arg_Var_Num; i++){
        Arg_Var[i] = Arg_Var[(i+n)%Arg_Var_Num];
    }
    for( ; i<VAR_MAX; i++){
        strcpy(Arg_Var[i]->value,"");
    }
    return ;
}

void history(){
    int i;
    for(i=0; i<History_Size; i++){
        printf("[%d] %s\n",i+1,History[i]);
    }
    return;
}

void Assign(char *left, char * right){
    //赋值命令
    while( *left==' ' ){
        left++;
    }
    while( *right==' ' ){
        right++;
    }
    char *temp[PARAM_MAX];
    memset(temp,0,sizeof(char*)*PARAM_MAX);
    temp[0] = right;
    Var_Substitute(1,temp);                          // Assign类型的要再单独处理(因为在Input_Parse中没有分割变量就直接进来了)
    right = temp[0];
    Add_Var(left,right);
    return ;
}

void Print_Error(int errorNum){                          //  错误提示
    switch(errorNum){
        case 1 : printf("Operation not permitted ");
                   break;
        case 2 : printf("No such file or directory ");
                   break;
        case 3 : printf("No such process ");
                   break;
        case 4 : printf("Interrupted system call ");
                   break;
        case 5 : printf("I/O error ");
                   break;
        case 6 : printf("No such device or address ");
                   break;
        case 7 : printf("Argument list too long ");
                   break;
        case 8 : printf("Exec format error ");
                   break;
        case 9 : printf("Bad file number ");
                   break;
        case 10 : printf("No child processes ");
                   break;
        case 11 : printf("Try again ");
                   break;
        case 12 : printf("Out of memory ");
                   break;
        case 13 : printf("Permission denied ");
                   break;
        case 14 : printf("Bad address ");
                   break;
        case 15 : printf("Block device required ");
                   break;
        case 16 : printf("Device or resource busy ");
                   break;
        case 17 : printf("File exists ");
                   break;
        case 18 : printf("Cross-device link ");
                   break;
        case 19 : printf("No such device ");
                   break;
        case 20 : printf("Not a directory ");
                   break;
        case 21 : printf("Is a directory ");
                   break;
        case 22 : printf("Invalid argument ");
                   break;
        case 23 : printf("File table overflow ");
                   break;
        case 24 : printf("Too many open files ");
                   break;
        case 25 : printf("Not a typewriter ");
                   break;
        case 26 : printf("Text file busy ");
                   break;
        case 27 : printf("File too large ");
                   break;
        case 28 : printf("No space left on device ");
                   break;
        case 29 : printf("Illegal seek ");
                   break;
        case 30 : printf("Read-only file system ");
                   break;
        case 31 : printf("Too many links ");
                   break;
        case 32 : printf("Broken pipe ");
                   break;
        case 33 : printf("Math argument out of domain of func ");
                   break;
        case 34 : printf("Math result not representable ");
                   break;
        case 35 : printf("Resource deadlock would occur ");
                   break;
        case 36 : printf("File name too long ");
                   break;
        case 37 : printf("No record locks available ");
                   break;
        case 38 : printf("Function not implemented ");
                   break;
        case 39 : printf("Directory not empty ");
                   break;
        case 40 : printf("Too many symbolic links encountered ");
                   break;
        case 41 : printf("Operation would block ");
                   break;
        case 42 : printf("No message of desired type ");
                   break;
        case 43 : printf("Identifier removed ");
                   break;
        case 44 : printf("Channel number out of range ");
                   break;
        case 45 : printf("Level 2 not synchronized ");
                   break;
        case 46 : printf("Level 3 halted ");
                   break;
        case 47 : printf("Level 3 reset ");
                   break;
        case 48 : printf("Link number out of range ");
                   break;
        case 49 : printf("Protocol driver not attached ");
                   break;
        case 50 : printf("No CSI structure available ");
                   break;
        case 51 : printf("Level 2 halted ");
                   break;
        case 52 : printf("Invalid exchange ");
                   break;
        case 53 : printf("Invalid request descriptor ");
                   break;
        case 54 : printf("Exchange full ");
                   break;
        case 55 : printf("No anode ");
                   break;
        case 56 : printf("Invalid request code ");
                   break;
        case 57 : printf("Invalid slot ");
                   break;
        case 58 : printf("Unknown error ");
                   break;
        case 59 : printf("Bad font file format ");
                   break;
        case 60 : printf("Device not a stream ");
                   break;
        case 61 : printf("No data available ");
                   break;
        case 62 : printf("Timer expired ");
                   break;
        case 63 : printf("Out of streams resources ");
                   break;
        case 64 : printf("Machine is not on the network ");
                   break;
        case 65 : printf("Package not installed ");
                   break;
        case 66 : printf("Object is remote ");
                   break;
        case 67 : printf("Link has been severed ");
                   break;
        case 68 : printf("Advertise error ");
                   break;
        case 69 : printf("Srmount error ");
                   break;
        case 70 : printf("Communication error on send ");
                   break;
        case 71 : printf("Protocol error ");
                   break;
        case 72 : printf("Multihop attempted ");
                   break;
        case 73 : printf("RFS specific error ");
                   break;
        case 74 : printf("Not a data message ");
                   break;
        case 75 : printf("Value too large for defined data type ");
                   break;
        case 76 : printf("Name not unique on network ");
                   break;
        case 77 : printf("File descriptor in bad state ");
                   break;
        case 78 : printf("Remote address changed ");
                   break;
        case 79 : printf("Can not access a needed shared library ");
                   break;
        case 80 : printf("Accessing a corrupted shared library ");
                   break;
        case 81 : printf(".lib section in a.out corrupted ");
                   break;
        case 82 : printf("Attempting to link in too many shared libraries ");
                   break;
        case 83 : printf("Cannot exec a shared library directly ");
                   break;
        case 84 : printf("Illegal byte sequence ");
                   break;
        case 85 : printf("Interrupted system call should be restarted ");
                   break;
        case 86 : printf("Streams pipe error ");
                   break;
        case 87 : printf("Too many users ");
                   break;
        case 88 : printf("Socket operation on non-socket ");
                   break;
        case 89 : printf("Destination address required ");
                   break;
        case 90 : printf("Message too long ");
                   break;
        case 91 : printf("Protocol wrong type for socket ");
                   break;
        case 92 : printf("Protocol not available ");
                   break;
        case 93 : printf("Protocol not supported ");
                   break;
        case 94 : printf("Socket type not supported ");
                   break;
        case 95 : printf("Operation not supported on transport endpoint ");
                   break;
        case 96 : printf("Protocol family not supported ");
                   break;
        case 97 : printf("Address family not supported by protocol ");
                   break;
        case 98 : printf("Address already in use ");
                   break;
        case 99 : printf("Cannot assign requested address ");
                   break;
        case 100 : printf("Network is down ");
                   break;
        case 101 : printf("Network is unreachable ");
                   break;
        case 102 : printf("Network dropped connection because of reset ");
                   break;
        case 103 : printf("Software caused connection abort ");
                   break;
        case 104 : printf("Connection reset by peer ");
                   break;
        case 105 : printf("No buffer space available ");
                   break;
        case 106 : printf("Transport endpoint is already connected ");
                   break;
        case 107 : printf("Transport endpoint is not connected ");
                   break;
        case 108 : printf("Cannot send after transport endpoint shutdown ");
                   break;
        case 109 : printf("Too many references: cannot splice ");
                   break;
        case 110 : printf("Connection timed out ");
                   break;
        case 111 : printf("Connection refused ");
                   break;
        case 112 : printf("Host is down ");
                   break;
        case 113 : printf("No route to host ");
                   break;
        case 114 : printf("Operation already in progress ");
                   break;
        case 115 : printf("Operation now in progress ");
                   break;
        case 116 : printf("Stale NFS file handle ");
                   break;
        case 117 : printf("Structure needs cleaning ");
                   break;
        case 118 : printf("Not a XENIX named type file ");
                   break;
        case 119 : printf("No XENIX semaphores available ");
                   break;
        case 120 : printf("Is a named type file ");
                   break;
        case 126 : printf("Required key not available ");
                   break;
        case 127 : printf("Key has expired ");
                   break;
        case 128 : printf("Key has been revoked ");
                   break;
        case 129 : printf("Key was rejected by service ");
                   break;
        case 130 : printf("Owner died ");
                   break;
        case 131 : printf("State not recoverable ");
                   break;
        case 132 : printf("Operation not possible due to RF-kill ");
                   break;
        case 133 : printf("Memory page has hardware error ");
                   break;
        default  : printf("Unknown error ");
    }
    printf("\n");
    return ;
}
