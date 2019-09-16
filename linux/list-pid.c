#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h> 
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <pwd.h>
/*
函数名: print_cmdline 
函数功能 :根据pid来打开一个对应的文件，并把文件的内容打印出来，
          至于为什么根据pid可以找到对于唯一的文件就不是我可以理解的了的了！
函数参数 ：pid 
函数返回值：ret : 返回读取到的字节数
*/
int print_cmdline(pid_t pid){
    char buf[1024];

    ssize_t n, i;
    int fd, ret = 0;

    sprintf(buf, "/proc/%d/cmdline", pid);
    if((fd = open(buf, O_RDONLY)) == -1)
        goto end; //新手谨慎使用，虽然这样子确实挺方便的！
    //读取文件内容并且打印出来！
    while((n = read(fd, buf, sizeof(buf))) > 0){
        for(i=0; i<n; i++){
            char c = buf[i];
            putchar(c ? c : ' ');
        }
        ret += n;
    }

    close(fd);

    end:
    return ret;
}
/*
函数名: print_status 
函数功能 :根据pid来打开一个对应的文件，并把文件的内容打印出来，
          至于为什么根据pid可以找到对于唯一的文件就不是我可以理解的了的了！
函数参数 ：pid 
函数返回值：ret : 返回读取到的字节数
*/
void print_status(pid_t pid){
    char buf[1024];

    ssize_t n, i;
    int fd;

    sprintf(buf, "/proc/%d/status", pid);            // 拼接文件路径
    if((fd = open(buf, O_RDONLY)) == -1)             //打开失败
        return;

    lseek(fd, 6, SEEK_SET);                          //偏移六个字节，估计和文件内部的存放位置有关系，感兴趣的一定要百度去看一下

    putchar('[');

    while((n = read(fd, buf, sizeof(buf))) > 0){    //读取并打印出来，逐字节打印！
        for(i=0; i<n; i++){
            if(buf[i] == '\n')                      // 遇到换行符就退出了
                goto end;

            putchar(buf[i]);
        }
    }

    end:
    putchar(']');

    close(fd);

}
/*
函数名: getpid
函数功能 :打开/proc文件夹并且把所有的0--9开头的文件名称存放到堆内存中
函数参数 ：无 
函数返回值：ret : 返回了一个指针，这个指针指向存放文件名称的堆内存
*/
pid_t *getpids(void){
    pid_t *ret = NULL;
    size_t len = 0;

    struct dirent *dirp;
    DIR *dir;

    if((dir = opendir("/proc")) == NULL)
        goto end;

    while((dirp = readdir(dir))){
        if(dirp->d_type != DT_DIR ||
          (dirp->d_name[0] < '0' || dirp->d_name[0] > '9'))      //如果不是0---9以内的文件或者是文件夹，则跳过忽略！
            continue;
        //找到一个就扩展一块内存，把之前的内存增大
        ret = realloc(ret, (len+1)*sizeof(pid_t));          //realloc可以用于扩展以前动态分配的内存，属于保留以前，再进行修改的函数
        if(ret == NULL)
            break;

        ret[len++] = atoi(dirp->d_name);                //字符串转整形函数，就是把“123”转换成整形的123！
    }

    closedir(dir);

    end:
    return ret;
}

int main(void){
    //变量的定义
    struct passwd *pw;
    struct stat st;

    char filename[32];
    pid_t *pids, pid;
    //获取当前所有进程号名称？
    if((pids = getpids()) == NULL){
        perror("getpids()");
        return 1;
    }

    printf("USER\tPID\tCOMMAND\n");
    while((pid = *pids++)){                     //先赋值，后自加
        sprintf(filename, "/proc/%d", pid);     //拼接文件名
            
        /*
        stat 函数介绍：
            表头文件:    #include <sys/stat.h>
                         #include <unistd.h>
            定义函数:    int stat(const char *file_name, struct stat *buf);
            函数说明:    通过文件名filename获取文件信息，并保存在buf所指的结构体stat中
            返回值:      执行成功则返回0，失败返回-1，错误代码存于errno

            错误代码:
                ENOENT         参数file_name指定的文件不存在
                ENOTDIR        路径中的目录存在但却非真正的目录
                ELOOP          欲打开的文件有过多符号连接问题，上限为16符号连接
                EFAULT         参数buf为无效指针，指向无法存在的内存空间
                EACCESS        存取文件时被拒绝
                ENOMEM         核心内存不足
                ENAMETOOLONG   参数file_name的路径名称太长
        */
      /*  
        stat 结构体具体成员：
        struct stat {
            dev_t         st_dev;       //文件的设备编号
            ino_t         st_ino;       //节点
            mode_t        st_mode;      //文件的类型和存取的权限
            nlink_t       st_nlink;     //连到该文件的硬连接数目，刚建立的文件值为1
            uid_t         st_uid;       //用户ID
            gid_t         st_gid;       //组ID
            dev_t         st_rdev;      //(设备类型)若此文件为设备文件，则为其设备编号
            off_t         st_size;      //文件字节数(文件大小)
            unsigned long st_blksize;   //块大小(文件系统的I/O 缓冲区大小)
            unsigned long st_blocks;    //块数
            time_t        st_atime;     //最后一次访问时间
            time_t        st_mtime;     //最后一次修改时间
            time_t        st_ctime;     //最后一次改变时间(指属性)
        };
        */
        if(stat(filename, &st) == -1){
            continue;
        }

        if((pw = getpwuid(st.st_uid)) == NULL){
            continue;
        }

        printf("%s\t", pw->pw_name);
        printf("%d\t", pid);

        if(!print_cmdline(pid)){
            print_status(pid);
        }

        putchar('\n');
    }

    return 0;
}
