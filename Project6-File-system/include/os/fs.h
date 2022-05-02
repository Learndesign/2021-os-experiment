#ifndef _FS_H
#define _FS_H
#include <os/sched.h>
#include <type.h>
// On-disk file system format.

/**************************************************************************************************
Disk layout:
[ boot block | kernel image ] (1GB)
[ Superblock(512B) | sector map(2MB) | inode map(1KB) | inode blocks(512KB) | data blocks (1GB)] 
****************************************************************************************************/
//SB
#define SB_MAGIC 0x66666666
#define SB_START (1lu << 20)                                    //512MB
#define SR_MAP_OFFSET 1lu                                       //512B
/* sector map */
#define SIZE_SR_MAP (1lu << 12)                                 //2MB 
#define ID_MAP_OFFSET (SR_MAP_OFFSET + SIZE_SR_MAP)             //2MB
/* inode map */
#define SIZE_ID_MAP (1lu << 1)                                  //1KB
#define ID_BLOCK_OFFSET (ID_MAP_OFFSET + SIZE_ID_MAP )          //1KB
/* inode block */
#define SIZE_ID_BLOCK (1lu << 10)                               //512KB
#define DT_BLOCK_OFFSET (ID_BLOCK_OFFSET + SIZE_ID_BLOCK)       //512KB
#define SIZE_DT_BLOCK (1lu << 21)

#define SB_SZ (sizeof(superblock_t))                            //size of supersize
#define ID_SZ (sizeof(inode_t))                                 //size of inode
#define DENTRY_SZ (sizeof(dentry_t))

#define SR_MAP_NUM (1lu << 21)                                  //2M
#define ID_MAP_NUM (1lu << 10)                                  //1KB
#define SIZE_SR (1lu << 9)                                      //512B one sector

//inode
#define MAX_DIR_BLK 7
#define MAX_FST_BLK 3//first level
#define MAX_SEC_BLK 2//second level
#define MAX_THR_BLK 1//third leve;
#define MAX_NAME_LEN 32
#define MAX_FILE_NUM 16

//offset
#define MAX_DIR_MAP (MAX_DIR_BLK + 5)
#define MAX_FST_MAP (MAX_DIR_MAP + MAX_FST_BLK * 128)
#define MAX_SEC_MAP (MAX_FST_MAP + MAX_SEC_BLK * 128 * 128)
#define MAX_THR_MAP (MAX_SEC_MAP + MAX_THR_BLK * 128 * 128 * 128)


//file 
#define O_RDONLY 1 /* read only open */
#define O_WRONLY 2 /* write only open */
#define O_RDWR 3 /* read/write open */

//type
#define IS_FILE 1
#define IS_DIR 2


typedef struct superblock
{
  uint32_t magic;     // Magic number
  uint32_t fs_size;   // file system size
  uint32_t fs_start;  // file system start sector
  /* map of data block */  
  uint32_t block_map_offset;
  uint32_t block_map_num;
  /* map of inode */
  uint32_t inode_map_offset;
  uint32_t inode_map_num;
  /* offset of inode */
  uint32_t inode_offset;
  uint32_t inode_num;
  /* offset of data block */
  uint32_t datablock_offset;
  uint32_t datablock_num;
}superblock_t;

// On-disk inode structure
typedef struct inode
{
    uint8_t ino;
    /* file or dir */ 
    uint8_t mode; 
    /* link */
    uint32_t link_num;   
    /* wite/rd */           
    uint8_t access;    
    uint64_t size;             
    uint64_t block_num;
    uint64_t child_num;
    uint64_t create_time;
    uint64_t modify_time;
    /* 1 to 8 */
    /* first direct block */
    uint32_t direct[MAX_DIR_BLK + 5];
    /* first leve Indirect block */
    uint32_t level_1[MAX_FST_BLK];
    /* second leve Indirect block */
    uint32_t level_2[MAX_SEC_BLK];
    /* third leve Indirect block */
    uint64_t level_3[MAX_THR_BLK];
}inode_t;

typedef struct dentry
{
    /* name */
    char name[MAX_NAME_LEN];
    /* dir or file */
    uint8_t access;
    uint8_t ino;
}dentry_t;

typedef struct rec_dir
{
    char name[MAX_NAME_LEN];
    /* the dir inode */
    uint8_t ino;
}rec_dir_t;

typedef struct file
{
    uint32_t used;
    uint32_t inode;
    uint32_t access;
    uint32_t addr;
    /* read pointer */
    uint32_t r_pos;
    /* write pointer */
    uint32_t w_pos;
}file_t;
/* whole situation */
/* bridge */
file_t openfile[MAX_FILE_NUM];
char tmp_sb[SIZE_SR];
char tmp_inode[SIZE_SR];
char tmp_dentry[SIZE_SR];

char tmp_block_map[SR_MAP_NUM];
char tmp_id_map[ID_MAP_NUM];

/* data buffer for sector map */
uint32_t first_map[SIZE_SR / 4];
uint32_t second_map[SIZE_SR / 4];
uint32_t third_map[SIZE_SR / 4];
/* R/W buffer */
char R_W_buffer[SIZE_SR];
/* load elf buff */
char load_buff[NORMAL_PAGE_SIZE];//4KB
inode_t current_ino;
// /* tools */
#define DEPTH 7
#define MAX_PATH_LENGTH 12

/* 
* （1）SEEK_SET，参数 offset 即为新的读写指针所在的位置
* （2）SEEK_CUR，以目前的读写指针所在的位置往后增加 offset 个偏移量
* （3）SEEK_END，文件尾后再增加offset 个偏移量作为新的读写指针所在的位置 
*/
// huge file
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

/* shell command */
bool do_mkfs();
void do_statfs();
bool do_mkdir(const char * path);
bool do_rmdir(const char * path);
bool do_rm(char * path);
void do_ls(const int mode, char * path);
bool do_cd(const char * path);
bool do_touch(const char * path);
bool do_cat(char * path);
void do_ln( char * des_file, char * src_file);
/* file op */
int do_fopen(const char * name, int access);
int do_fwrite(int fd, char * buff, const int size);
int do_fread(int fd, char * buff, const int size);
void do_close(int fd);

/* huge file */
int do_lseek(int fd, int offset, int whence);
int do_load(const char *file_name, int argc, char* argv[], spawn_mode_t mode);



#endif