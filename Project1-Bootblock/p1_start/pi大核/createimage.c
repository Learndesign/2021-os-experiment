#include <assert.h>
#include <elf.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define IMAGE_FILE "./image"
#define ARGS "[--extended] [--vm] <bootblock> <executable-file> ..."
#define OS_SIZE_LOC 0x1fc

/* structure to store command line options */
static struct {
    int vm;
    int extended;
} options;

/* prototypes of local functions */
static void create_image(int nfiles, char *files[]);
static void error(char *fmt, ...);
static void read_ehdr(Elf64_Ehdr * ehdr, FILE * fp);
static void read_phdr(Elf64_Phdr * phdr, FILE * fp, int ph,
                      Elf64_Ehdr ehdr);
static void write_segment(Elf64_Ehdr ehdr, Elf64_Phdr phdr, FILE * fp,
                          FILE * img, int *nbytes, int *first);
static void write_os_size(int nbytes, FILE * img);

int main(int argc, char **argv)
{
    //The first line is the name of this program
    char *progname = argv[0];

    /* process command line options */
    options.vm = 0;
    options.extended = 0;
    //analysis of the command option，the form is --xx
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
    if (argc < 4) {
        /* at least 4 args (createimage bootblock kernel0 kernel1) */
        error("usage: %s %s\n", progname, ARGS);
    }
    //skip the name of program
    create_image(argc - 1, argv + 1);
    return 0;
}

static void create_image(int nfiles, char *files[])
{
    int ph, nbytes = 0, first = 1;
    FILE *fp, *img;
    Elf64_Ehdr ehdr;
    Elf64_Phdr phdr;

    /* open the image file */
    if ((img = fopen(IMAGE_FILE, "wb+")) == NULL){
        printf("ERROR: IMAGE_FILE cann't be opened\n");
    }
        

    /* for each input file */
    while (nfiles-- > 0) {

        /* open input file */
        int i = 0;
        i++;
        if((fp = fopen(*files,"rb+")) == NULL){
            printf("ERROR: The %s file cann't be opened", *files);
            return;
        }

        /* read ELF header */
        read_ehdr(&ehdr, fp);
        printf("0x%lx: %s\n", ehdr.e_entry, *files);

        /* for each program header */
        for (ph = 0; ph < ehdr.e_phnum; ph++) {
            if(options.extended){
                printf("\tsegment %d\n",ph);
            }
            /* read program header */
            read_phdr(&phdr, fp, ph, ehdr);
        if(options.extended == 1){
            printf("\t\toffset 0x%lx",phdr.p_offset);
            printf("\tvaddr 0x%lx\n",phdr.p_vaddr);
            printf("\t\tfilesz 0x%lx",phdr.p_filesz);
            printf("\tmemsz 0x%lx\n",phdr.p_memsz);
            printf("\t\twriting 0x%lx bytes\n",phdr.p_memsz);
    }
            /* write segment to the image */
            write_segment(ehdr, phdr, fp, img, &nbytes, &first);
        }
        fclose(fp);
        files++;
    }
    write_os_size(nbytes, img);
    fclose(img);
}

static void read_ehdr(Elf64_Ehdr * ehdr, FILE * fp)
{
    fread(ehdr, sizeof(Elf64_Ehdr), 1, fp);
}

static void read_phdr(Elf64_Phdr * phdr, FILE * fp, int ph,
                      Elf64_Ehdr ehdr)
{
    long offset = ehdr.e_phoff + ph*ehdr.e_phentsize;
    fseek(fp, offset, SEEK_SET);
    fread(phdr, ehdr.e_phentsize, 1, fp);
}


static void write_segment(Elf64_Ehdr ehdr, Elf64_Phdr phdr, FILE * fp,
                          FILE * img, int *nbytes, int *first)
{   
    int origin_byte_size = *nbytes;
    
    //get this sefment really occupy sector size by round number
    int size = (phdr.p_memsz + 511) / 512 * 512;

    //read segment
    char *segment = (char*)malloc(size);
    fseek(fp, phdr.p_offset, SEEK_SET);
    fread(segment, phdr.p_filesz, 1, fp);

    //first == 1 means write bootblock
    if(*first){
        *first = 0;
        fwrite(segment, size, 1, img);
	}else{
        fseek(img, *nbytes, SEEK_SET);
        fwrite(segment, size, 1, img);
	}
    if(options.extended)
	    printf("\t\tpadding up to 0x%lx\n",(size - phdr.p_memsz));
    *nbytes += size;
    int real_size = (*nbytes - origin_byte_size)/512;
    printf("this_section_size: %d sectors\n" , real_size);
}

static void write_os_size(int nbytes, FILE * img)
{
    int kernel_size = nbytes / 512 - 1;
    fseek(img, OS_SIZE_LOC, SEEK_SET);
    fwrite(&kernel_size, 2, 1, img);
    printf("os_total_size: %d sectors\n", kernel_size);
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
