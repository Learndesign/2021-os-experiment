#ifndef _FS_TOOL_H
#define _FS_TOOL_H

#include <os/list.h>
#include <os/fs.h>
#include <sbi.h>
#include <pgtable.h>
#include <os/stdio.h>
#include <os/mm.h>
#include <os/time.h>
#include <screen.h>
#include <os/string.h>
/* analysis path, return line */
int analysis_path(char * path, char two_dim[DEPTH][MAX_PATH_LENGTH]);
/* get the path inode */
inode_t * get_inode_from_dir(char * path);
/* get the inode num form the inode by name */
int find_inode_from_name(char * name, inode_t * fd_inode);
/* get the inode from the ino_num */
inode_t * get_inode_from_inoid(int inoid);
/* get the inode_id from the inode */
int get_entry_from_inode(inode_t * fd_inode, char * name);
/* get sector id from offset */
int get_sector_id(inode_t * fd_inode, int offset);
/* set sector id from offset */
int set_sector_id(inode_t * fd_inode, int offset, int sector_id);
/* delete the entry from inode */
int delete_entry_from_inode(inode_t * fd_inode, char * name);
/* alloc sector from inode */
int free_entry_from_inode(inode_t * fr_inode);
/* get one free entry from the inode */
int get_free_entry(inode_t * pre_ino);
/* check the existed of file system */
bool check_fs();
/* alloc inode */
int alloc_inode();
/* alloc sector */
int alloc_sector();
/* alloc continue */
int alloc_sector_continue(int * sr_map, int sro);
/* free inode */
void free_inode(int id);
/* free sector */
void free_sector(int id);
/* set the inode first for the dir or file */
int set_inode_first(int father_ino, int new_inode, int mode);
/* creat one dentry , return the ino of the new*/
int creat_entry(inode_t * father_ino, char * name, int mode);
/* delete one dentry */
void delete_entry(inode_t * father_ino, char * name);
/* get the father inode num */
int get_father(inode_t * pre_inode);
/* file operation */
void init_fd();
/* alloc_fd */
int alloc_fd();

/* load elf */
uintptr_t load_elf_sd(                      const int fd,
                                    unsigned length, uintptr_t pgdir,
    uintptr_t (*prepare_page_for_va)(uintptr_t va, uintptr_t pgdir));
/* re load elf for fork */
void re_load_elf_sd(
    const int fd, unsigned length, uintptr_t pgdir_des, uintptr_t pgdir_src,
    PTE * (*prepare_page_for_va)(uintptr_t va, uintptr_t pgdir, uint64_t kva, uint32_t mode)
    );


#endif