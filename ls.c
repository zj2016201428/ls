/*************************************************************************
	> File Name: ls.c
	> Author: 
	> Mail: 
	> Created Time: 四  3/19 11:34:05 2020
 ************************************************************************/

#include <stdio.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <getopt.h>
#include <grp.h>
#include <stdlib.h>
#include <pwd.h>
#include <time.h>
#include <string.h>
#include <sys/ioctl.h>

void do_ls(char[]);
void output(char *);
void judge_type(mode_t, char *);
char *getUserName(uid_t);
char *getGroupName(gid_t);
void sort(char *file_name[], int cnt);
void swap(char *, char *);
void get_windowsize(char *file_name[], int cnt, int *col, int *max);
int flag = 0;

int main(int argc, char **argv) {
    //printf("%d\n", argc);
    int select;
    while((select = getopt(argc, argv, "al")) != -1) {
        switch (select) {
            case 'a':
                flag++;
                break;
            case 'l':
                flag += 2;
                break;
            default :
                break;
        }
    }
    argc -= (optind - 1);//optind指示下一个要解析的参数位置，初始时为1.
    argv -= (optind - 1);
    //printf("%d\n", argc);
    if (argc == 1) {
        do_ls(".");
    } else {
        while (--argc) {
            printf("%s:\n", *++argv);
            do_ls(*argv);
        }
    }

    return 0;
}


void do_ls(char dirname[]) {
    DIR *dirp;
    int num = 0, cnt;
    int col, max = 0;//
    int temp;
    struct dirent *direntp;

    if ((dirp = opendir(dirname)) == NULL) {
        perror("opendir");
        return;
    }
    char *file_name[1024];//数组指针，一连串字符串的存储方法！
    while ((direntp = readdir(dirp)) != NULL) {
    // printf("%s\n", direntp->d_name);
        file_name[num] = (char *)malloc(sizeof(char) * 100);
        strcpy(file_name[num++], direntp->d_name);//指向字符串的指针不能直接赋值！
    }
    sort(file_name, num);
    if (flag < 2) {
        get_windowsize(file_name, num, &col, &max);//只需要获得一行能放多少文件的数量即可
        temp = col;//用于90行
        int i = 0;
        cnt = num;//因为最后要输出文件数量。要保留num
        while (cnt--) {
            temp--;
            printf("%-*s", max + 2, file_name[i++]);//小技巧，动态的规定输出的位数大小，规定格式。
            if (!temp || cnt == 0) {
                printf("\n");
                temp = col;
            }
        }
    } else {
        for (int i = 0 ; i < num; i++) {
            output(file_name[i]);
        }

    }

    printf("\n文件的数量为：%d", num);
    closedir(dirp);
    printf("\n");
}


void output(char *name) {
    struct stat s;
    if (stat(name, &s) < 0) {
        perror("output");
        return ;
    }
    char mode[] = "----------";
    char *ctime();
    judge_type(s.st_mode, mode);//千万注意参数的使用，别使用了参数类型还不知道！！这里写s，不是写stat！！
    //注意s结构体中成员的类型，可不是什么整形，字符串。而是人为创造定义的类型，要输出必须转换类型！！！！
    printf("%s %2d %6s %6s %6lld %.20s %s", mode, (int)s.st_nlink, getUserName(s.st_uid), getGroupName(s.st_gid), s.st_size, ctime(&s.st_mtime), name);//类型前可规定输出位数，统一格式
    printf("\n");
    return ;
}

void judge_type(mode_t t, char *mode) {
    //文件类型
    if(S_ISREG(t)) mode[0] = '-';
    if(S_ISLNK(t)) mode[0] = 'l';
    if(S_ISDIR(t)) mode[0] = 'd';
    if(S_ISCHR(t)) mode[0] = 'c';
    if(S_ISBLK(t)) mode[0] = 'b';
    if(S_ISSOCK(t)) mode[0] = 's';
    if(S_ISFIFO(t)) mode[0] = 'p';


    //用户权限
    if(t & S_IRUSR) mode[1] = 'r';
    if(t & S_IWUSR) mode[2] = 'w';
    if(t & S_IXUSR) mode[3] = 'x';

    //小组权限
    if(t & S_IRGRP) mode[4] = 'r';
    if(t & S_IWGRP) mode[5] = 'w';
    if(t & S_IXGRP) mode[6] = 'x';

    //其他用户权限
    if(t & S_IROTH) mode[7] = 'r';
    if(t & S_IWOTH) mode[8] = 'w';
    if(t & S_IXOTH) mode[9] = 'x';
}
//通过uid求用户名字
char *getUserName(uid_t uid) {
    struct passwd *psd;
    if ((psd = getpwuid(uid)) == NULL) {
        return NULL;
    } else {
        return psd->pw_name;
    }
}
//通过gid求小组名字
char *getGroupName(gid_t gid) {
    struct group *grp;
    if ((grp = getgrgid(gid)) == NULL) {
        return NULL;
    } else {
        return grp->gr_name;
    }
}

//注意参数的传入形式
void sort(char *file_name[], int cnt) {
    //插入排序
    for (int i = 1; i < cnt; i++) {
        for (int j = i; j > 0 && file_name[j][0] < file_name[j - 1][0]; j--) {
            swap(file_name[j], file_name[j - 1]);
        }
    }
}

void swap(char *a, char *b) {
    char *temp = (char *)malloc(sizeof(char) * 100);
    strcpy(temp, a);
    strcpy(a, b);
    strcpy(b, temp);
    return ;
}

void get_windowsize(char *file_name[], int cnt, int *col, int *max) {
    struct winsize size;//结构体具体说明见笔记——ls的实现
    int len;
    int length;
    if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &size) < 0) {
        perror("ioctl");
        return;
    }
    //printf("size_row = %d, size_col =%d\n", size.ws_row, size.ws_col);

    for (int i = 0; i < cnt; i++) {
        length = strlen(file_name[i]);
        if (length > *max) {
            *max = length;
            len = i;
        }

    }
    //printf("%s\n", file_name[len]);
    //printf("%d\n", *max);
    *col = (size.ws_col / (*max + 2 ));
    //printf("%d\n", *col);
    return ;
}
