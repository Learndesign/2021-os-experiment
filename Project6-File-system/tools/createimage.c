#include <assert.h>
#include <elf.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define IMAGE_FILE "./image"
#define ARGS "[--extended] [--vm] <bootblock> <executable-file> ..."
#define OS_SIZE_LOC_1 0x1fc
#define OS_SIZE_LOC_2 0x1fe
#define DISTANCE_OF_OS 0x1000
/* structure to store command line options */
static struct {
    int vm;
    int extended;
} options;

/* prototypes of local functions */
static void create_image(int nfiles, char *files[]);//制作内核镜像，nfiles==>需要制作内核的ELF文件数， files,文件名数组
static void error(char *fmt, ...);
static void read_ehdr(Elf64_Ehdr * ehdr, FILE * fp);//读取所指向文件的ELF头
static void read_phdr(Elf64_Phdr * phdr, FILE * fp, int ph,
                      Elf64_Ehdr ehdr);//根据ELF头的信息，读取该文件的一个程序头，ph为程序头编号
static void write_segment(Elf64_Ehdr ehdr, Elf64_Phdr phdr, FILE * fp,
                          FILE * img, int *nbytes, int *first);//
static void write_os_size(int nbytes, FILE * img, char *files);//将kernel段的大小(双字节整数，按扇区计)写入镜像中OS_SIZE_LOC
//处，供引导块执行时取用。

int main(int argc, char **argv)
{
    char *progname = argv[0];

    /* process command line options */
    options.vm = 0;
    options.extended = 0;
    while ((argc > 1) && (argv[1][0] == '-') && (argv[1][1] == '-')) {
        char *option = &argv[1][2];

        if (strcmp(option, "vm") == 0) {
            options.vm = 1;
        } else if (strcmp(option, "extended") == 0) {
            options.extended = 1;
        } else {
            error("%s: invalid option\nusage: %s %s\n", progname,
                  progname, ARGS);
        }
        argc--;
        argv++;
    }
    if (options.vm == 1) {
        error("%s: option --vm not implemented\n", progname);
    }
    if (argc < 3) {
        /* at least 3 args (createimage bootblock kernel) */
        error("usage: %s %s\n", progname, ARGS);
    }
    create_image(argc - 1, argv + 1);
    return 0;
}

static void create_image(int nfiles, char *files[])
{
    //FILE*fp  定义一个名为fp的指针，属于FILE（文件）类型。FILE是一类特殊的指针，用来操作 文件。
    int ph, nbytes = 0, first = 1;
    FILE *fp, *img;
    Elf64_Ehdr ehdr;//ELF文件头
    Elf64_Phdr phdr;//ELF程序头
    /* open the image file */
    if((img=fopen(IMAGE_FILE,"wb+"))==NULL){
        printf("falied to open the imag!\n");
    }
    /* for each input file */
    while (nfiles-- > 0) {

        /* open input file */
        if((fp=fopen(*files,"r"))==NULL){
            printf("falied to open the fp!\n");
        }

        /* read ELF header */
        read_ehdr(&ehdr, fp);
	
        printf("0x%lx: %s\n", ehdr.e_entry, *files);
        //ehdr.e_entry虚拟的入口地址
        //if(strcmp(*files,"kernel2")==0)
        //    fseek(img, DISTANCE_OF_OS,SEEK_SET);//跳转到kernel2入口处	
        /* for each program header */
        /* Program header table entry count */
        for (ph = 0; ph < ehdr.e_phnum; ph++) {
            /* read program header */
            read_phdr(&phdr, fp, ph, ehdr);
            write_segment(ehdr, phdr, fp, img, &nbytes, &first);
            if(options.extended){
                printf("\tsegment %d\n",ph);      
                printf("\t\toffset 0x%04lx\t\t",phdr.p_offset);
                printf("vaddr 0x%04lx\n",phdr.p_vaddr);
                printf("\t\tfilesz 0x%04lx\t\t",phdr.p_filesz);
                printf("memsz 0x%04lx\n",phdr.p_memsz);
                if(phdr.p_memsz){
                    printf("\t\twriting 0x%04lx bytes\n",phdr.p_memsz);
                    printf("\t\tpadding up to 0x%04x\n",nbytes);
                }
            }
            /* write segment to the image */
        }
        fclose(fp);   
        if(strcmp(*files,"bootblock")!=0)
            write_os_size(nbytes, img, *files);	    
        files++;        
    }
    fclose(img);
}

static void read_ehdr(Elf64_Ehdr * ehdr, FILE * fp)
{
    /*
     *ELF头位于文件开头64byte的位置，因此可以直接读取
     *函数原型：fread(buffer, size, count, fp);
     *buffer：对于fread来说，指的是读入数据的存放地址；对于fwrite来说，是要输出数据的地址。
     *size：读写数据时，每笔数据的大小
     *count：读写数据的笔数
     *fp：文件指针
     */
    fread(ehdr, sizeof(Elf64_Ehdr), 1, fp);
}

static void read_phdr(Elf64_Phdr * phdr, FILE * fp, int ph,
                      Elf64_Ehdr ehdr)
{
    /*
     *读取改文件的一个程序头，ph文件头的编号
     *ELF头文件下部即为程序头表,p_phoff为程序头入口。
     *int fseek(FILE * stream, long offset, int whence);
     *光标重定位函数:
     *SEEK_SET: 文件开始处
     *SEEK_CUR: 文件当前位置 
     *SEEK_END：文件结束处
     *fseek(fp, 10L, SEEK_SET);      //定位至文件中的第10个字节
    */
    if (ph >= ehdr.e_phnum) {
        error("ERROR: Segment %d not found, only have %d segments.\n", ph, ehdr.e_phnum);
    }
    fseek(fp, (long)(ehdr.e_phoff + ph*sizeof(Elf64_Phdr)),SEEK_SET);//光标移动到对应的程序表头前。
    fread(phdr,sizeof(Elf64_Ehdr),1 ,fp);//读取ELF中一个表的数据
}

static void write_segment(Elf64_Ehdr ehdr, Elf64_Phdr phdr, FILE * fp,
                          FILE * img, int *nbytes, int *first)
{
    /*
     *程序头指定的段从fp中读出，并写在内核中正确的位置。
     *nbytes为img中已经写入的字节数目
     *first记录变量是否首次写入，对应bootblock,计算段在镜像中的偏移地址时用到
     *p_filesz域该段在ELF文件中的大小
     *p_memsz的程序被加载后在内存中所占的大小，应当写入p_memsz
     *其中p_filesz个文件来自ELF文件
     *写入一个段后，将该扇区填零补齐
    */    
    /*读取出一定的大小*/
    char *read_files;
    read_files=(char *)malloc(phdr.p_memsz);
    memset(read_files, 0, phdr.p_memsz);//初始化为零
    //fseek(img, *nbytes, SEEK_SET);//修改光标的位置,这里是要写文件的地方
    fseek(fp,phdr.p_offset,SEEK_SET);//设置读取数据的光标的位置
    fread(read_files, phdr.p_filesz,1,fp);//读取第一个程序头的数据到这里
    int judge=512;//512字节对应着一个扇区的大小
    char padding[512]="";
    if(phdr.p_memsz){
        fwrite(read_files,phdr.p_memsz,1,img);//写下数据
        if(phdr.p_memsz<=judge){    
            int free = judge-phdr.p_memsz;
            if(free){
                fwrite(padding,free,1,img);
            } 
        }else{//如果大于一个扇区
            int free = judge - (phdr.p_memsz%judge);
            fwrite(padding,free,1,img);
        }
        *nbytes += judge * (phdr.p_memsz / judge + (int)(phdr.p_memsz % judge!=0));//直接一次写一个扇区
    }
}
short ago_num_of_blocks=0;
static void write_os_size(int nbytes, FILE * img, char *files)
{
    short num_of_blocks=num_of_blocks = nbytes/512 + (int)(nbytes % 512 != 0)-1;//减去bootblock所占的一个扇区
    short true_num_blocks = num_of_blocks-ago_num_of_blocks;
    fseek(img, OS_SIZE_LOC_1, SEEK_SET);//跳转到位置写占用的扇区数目
    if(strcmp(files,"kernel2")==0){
        fseek(img, OS_SIZE_LOC_2, SEEK_SET);//如果是第二个内核，则移动到这里
    }
    fwrite(&true_num_blocks, 2, 1, img);
    if(options.extended == 1){
        printf("os_size: %hd sectors\n",true_num_blocks);
    }
    ago_num_of_blocks=num_of_blocks;
    fseek(img,(ago_num_of_blocks + 1)*0x200,SEEK_SET);
    return;       
}

/* print an error message and exit */
static void error(char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    if (errno != 0) {
        perror(NULL);
    }
    exit(EXIT_FAILURE);
}
