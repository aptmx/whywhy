#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

// 用标准IO打开文件的模式（共6种）：
// fopen("a.txt", "r"); // 只读，不存在则报错
// fopen("a.txt", "w"); // 只写，不存在则创建，存在则清空
// fopen("a.txt", "a"); // 只写，不存在则创建，存在则追加，写时文件位置会自动调整到末尾
// fopen("a.txt", "r+");// 读写，不存在则报错
// fopen("a.txt", "w+");// 读写，不存在则创建
// fopen("a.txt", "a+");// 读写，不存在则创建，存在则追加，写时文件位置会自动调整到末尾

// #define MB 1024*1024

typedef struct List
{
    int ID; //商品编号
    int cat; //商品种类
    char Name[32]; //商品名称
    float Price; //商品单价
    char PicPath[128]; //图片路径
    int Remain; //余量

    struct List *next;
    struct List *prev;
}LL;

LL *List_Init(void)
{
    LL *Head = malloc(sizeof(LL));
    if(Head == NULL)
    {
        perror("malloc");
        return NULL;
    }

    Head->next = Head;
    Head->prev = Head;

    return Head; //返回头指针
}

void list_TailInsert(LL *Head, LL Data)
{
    LL *NewNode = malloc(sizeof(LL));

    //数据域
    strcpy(NewNode->Name, Data.Name);
    strcpy(NewNode->PicPath, Data.PicPath);
    NewNode->ID = Data.ID;
    NewNode->cat = Data.cat;
    NewNode->Price = Data.Price;
    NewNode->Remain = Data.Remain;

    //指针域
    LL *p = Head->prev;
    Head->prev = NewNode;
    NewNode->next = Head;
    NewNode->prev = p;
    p->next = NewNode;

    printf("insert:%s\n", Data.Name);
}

int main(int argc, char const *argv[]) // ./copy file1 file2
{
    const char* filename = "/home/gec/code/VendingMachine/product.txt";
    FILE *file = fopen(filename, "r+");
    if(file == NULL)
    {
        perror("open() failed");
        exit(0);
    }

    char line[1024];

    LL *Head = List_Init();
    while(fgets(line, sizeof(line), file))
    {   
        LL Data;
        int k;
        printf("%s", line);
        sscanf(line, "ID:%d, Cat:%d, Name:%[^,], Price:%f, Path:%[^,], Remain:%d;", &Data.ID, &(Data.cat), Data.Name, &(Data.Price), Data.PicPath, &(Data.Remain));
        
        printf("ID:%d Name:%s Price:%.2f, Path:%s\n", Data.ID, Data.Name,Data.Price,Data.PicPath);
    
        Data.ID =222;
        // if(!strcmp(Data.Name,"apple"))
        // {
        //     Data.ID = 222;
        //     printf("apple\n");
        // }

        list_TailInsert(Head,Data);
    }


    FILE *fp_prod = fopen(filename, "w+");
    fseek(file,0,SEEK_SET);
    LL* p = Head->next;
    for(;p!=Head; p=p->next)
    {
        printf("LL Data%d = {%d, %d, %s, %.2f, %s, %d}\n",p->ID, p->ID,p->cat, p->Name,p->Price,p->PicPath,p->Remain);
        fprintf(file, "ID:%d, Cat:%d, Name:%s, Price:%.2f, Path:%s, Remain:%d;\n",p->ID,p->cat, p->Name,p->Price,p->PicPath,p->Remain);
        fflush(file);
    }
    fclose(file);
    













// {
//     if(argc != 3)
//     {
//         printf("参数非法！再见！\n");
//         exit(0);
//     }

//     // 1. 以只读模式打开待拷贝文件
//     FILE *fp1 = fopen(argv[1], "r");
//     if(fp1 == NULL)
//     {
//         perror("打开文件失败");
//         exit(0);
//     }

//     // 2. 以只写方式打开目标文件，若不存在则创建，若存在则清空
//     FILE *fp2 = fopen(argv[2], "w");
//     if(fp2 == NULL)
//     {
//         printf("打开文件[%s]失败:%s\n", argv[2], strerror(errno));
//         exit(0);
//     }

//     // 3. 循环地读取fp1，将数据写入fp2
//     // 标准IO读写文本文件的操作函数：
//     // fgetc()/fputc()    // 读写取字符，每次读写一个字符
//     // fgets()/fputs()    // 读写取字符，每次读写一行字符
//     // fscanf()/fprintf() // 按指定格式读写字符

//     // 标准IO读写任意文件的操作函数：
//     // fread()/fwrite() // 读写字符或二进制数据（程序、图片、视频、压缩包……），每次读写指定的一块

//     // 取得待拷贝文件的大小
//     fseek(fp1, 0L, SEEK_END); // 将文件fp1的位置调整到距离文件末尾0字节处
//     long size = ftell(fp1);
//     fseek(fp1, 0L, SEEK_SET); // 将文件fp1的位置调整到距离文件开头0字节处

//     // fread()/fwrite()
//     char *buf;
//     int   bufsize=0;
//     if(size <= 1*MB)
//     {
//         buf = calloc(1, size);
//         bufsize = size;
//     }
//     else if(size <= 100*MB)
//     {
//         buf = calloc(1, 10*MB);
//         bufsize = 10*MB;
//     }
//     else
//     {
//         buf = calloc(1, 100*MB);
//         bufsize = 100*MB;
//     }

//     while(1)
//     {
//         // 从源文件读取若干字节:从fp1中读取1块数据，每块bufsize个字节，放入buf
//         // 返回值n代表成功读取的块数，向下取整。比如n等于0时，可能读到0-(bufsize-1)个字节
//         // 读之前，记录当前的文件位置
//         long begin = ftell(fp1);
//         int n = fread(buf, bufsize, 1, fp1);

//         // 正常读取
//         if(n == 1)
//         {
//             // 将数据写入目标文件
//             fwrite(buf, bufsize, 1, fp2);
//         }

//         // 遇到错误，或读完
//         else
//         {
//             // 遇到错误
//             if(ferror(fp1))
//             {
//                 perror("读取失败");
//                 break;
//             }

//             // 读完
//             if(feof(fp1))
//             {
//                 // 将剩余的若干字节当成一块数据写入目标文件
//                 long end = ftell(fp1);
//                 fwrite(buf, end-begin, 1, fp2);

//                 printf("拷贝完毕！\n");
//                 break;
//             }
//         }
//     }

//     free(buf);
//     fclose(fp1);
//     fclose(fp2);

    return 0;
}