#include <os/list.h>
#include <os/fs.h>
#include <os/fs_tool.h>
#include <sbi.h>
#include <pgtable.h>
#include <os/stdio.h>
#include <os/mm.h>
#include <os/time.h>
#include <screen.h>
#include <os/string.h>

/* mkfs */
bool do_mkfs(){
    prints("[FS] Start superblock...\n");
    kmemset(tmp_sb, 0 , SIZE_SR);
    superblock_t * sb_now = (superblock_t *) tmp_sb;    
    sb_now->magic               = SB_MAGIC;
    /* begin */
    sb_now->fs_size             = SR_MAP_NUM;
    sb_now->fs_start            = SB_START;
    /* block map */
    sb_now->block_map_offset    = SR_MAP_OFFSET;
    sb_now->block_map_num       = SR_MAP_NUM;
    /* inode map */
    sb_now->inode_map_offset    = ID_MAP_OFFSET;
    sb_now->inode_map_num       = ID_MAP_NUM;
    /* inode */
    sb_now->inode_offset        = ID_BLOCK_OFFSET;
    sb_now->inode_num           = 0;
    /* data block */
    sb_now->datablock_offset    = DT_BLOCK_OFFSET;
    sb_now->datablock_num       = 0;
    // Write to SD card
    sbi_sd_write(kva2pa((uintptr_t)tmp_sb), 1, SB_START);
    kmemset((uintptr_t)tmp_sb, 0 ,SIZE_SR);
    sbi_sd_read(kva2pa((uintptr_t)tmp_sb), 1, SB_START);
    sb_now = (superblock_t *) tmp_sb;   
    // Check and print info
    prints("     magic: 0x%x \n", sb_now->magic);
    prints("     num sector: %d, start sector: %d\n",
            sb_now->fs_size, sb_now->fs_start);
    prints("     sector map offset: %d (%d)\n",
            sb_now->block_map_offset, SIZE_SR_MAP);
    prints("     inode map offset: %d (%d)\n",
            sb_now->inode_map_offset, SIZE_ID_MAP);
    prints("     inode offset: %d (%d)\n",
            sb_now->inode_offset, SIZE_ID_BLOCK);
    prints("     data offset: %d (%d)\n",
            sb_now->datablock_offset, SIZE_DT_BLOCK);
    prints("     inode entry size : %dB, dir entry size : %dB\n",
			ID_SZ, DENTRY_SZ);
    // Setting inode-map && block-map
    //2MB
    prints("[FS] Setting inode-map...\n");
    prints("[FS] Setting sector-map...\n");  
    /* memset */
    kmemset(tmp_id_map, 0, ID_MAP_NUM);
    kmemset(tmp_block_map, 0, SR_MAP_NUM);
    /* set sector map */
    sbi_sd_write(kva2pa((uintptr_t)tmp_block_map)
            , SIZE_SR_MAP, SB_START + SR_MAP_OFFSET);
    // setting first inode
    kmemset(tmp_inode, 0, SIZE_SR);
    inode_t *inode = (inode_t *)tmp_inode;
    inode->ino = 0;
    inode->mode = IS_DIR;
    inode->access = O_RDWR;
    inode->block_num = 1;
	inode->child_num = 1;
    inode->create_time = get_timer();
    inode->modify_time = get_timer();
	kmemcpy((char *)(&current_ino), tmp_inode, ID_SZ);
    sbi_sd_write(kva2pa((uintptr_t)tmp_inode), 
            1, SB_START + ID_BLOCK_OFFSET);
    //  setting inode-map
    tmp_id_map[0] = 1;
    /* write the inode map */
	sbi_sd_write(kva2pa((uintptr_t)tmp_id_map)
            , SIZE_ID_MAP, SB_START + ID_MAP_OFFSET);
    //  setting dentry
    /* "." */
    kmemset(tmp_dentry, 0, SIZE_SR);
    dentry_t *de = (dentry_t *)tmp_dentry;
    kstrcpy(de->name, ".");
    de->access = O_RDWR;
    de->ino = 0;
	sbi_sd_write(kva2pa((uintptr_t)tmp_dentry)
            , 1 , SB_START + DT_BLOCK_OFFSET);
    tmp_block_map[0] = 1;
    sbi_sd_write(kva2pa((uintptr_t)tmp_block_map)
            , SIZE_SR_MAP, SB_START + SR_MAP_OFFSET);
	sb_now->datablock_num ++;
	sb_now->inode_num ++;
	sbi_sd_write(kva2pa((uintptr_t)sb_now), 1, SB_START);
    prints("[FS] Initialize filesystem finished!\n");
    screen_reflush();    
    return true;
}

/* statfs */
void do_statfs(){
	kmemset(tmp_sb, 0 , SIZE_SR);
    sbi_sd_read(kva2pa((uintptr_t)tmp_sb), 1, SB_START);
    superblock_t * sb_now = (superblock_t *)tmp_sb;
    /* display message */
    prints("[FS] magic: %x (KFS)\n", sb_now->magic);
    prints("     used sector : %d(%d), start sector : %d\n", 
		sb_now->datablock_num, sb_now->block_map_num, sb_now->datablock_offset);
	prints("     sector map offset : %d, occupied sector : %d, used : %d/%d\n",
		sb_now->block_map_offset, SIZE_SR_MAP, sb_now->datablock_num, sb_now->block_map_num);
	prints("     inode map offset : %d, occupied sector : %d, used : %d/%d\n",
		sb_now->inode_map_offset, SIZE_ID_MAP, sb_now->inode_num, sb_now->inode_map_num);
	prints("     inode offset : %d, occupies sector : %d\n",
		sb_now->inode_offset, SIZE_ID_BLOCK);
	prints("     data offset : %d, occupied sector : %d\n", 
		sb_now->datablock_offset, SIZE_DT_BLOCK);
    prints("     inode entry size : %dB, dir entry size : %dB\n",
			ID_SZ, DENTRY_SZ);	
	screen_reflush();   
}

/* ls */
void do_ls(int mode, char * path){
    inode_t * pre_inode; 
    if(path[0] != NULL){
        pre_inode = get_inode_from_dir(path);
        if(pre_inode == NULL){
            return;
        }     
    }else 
        pre_inode = &current_ino;
    if(mode){
        prints("--------------------[File table]--------------------\n");
        prints("Acess  Inode   B-num      Link     Type           Size          Name   \n");
    }
    
    if(pre_inode->mode == IS_FILE){
        prints("[Error] %s is a file\n", path);
        return;
    }else{
        dentry_t * pre_de;
        int offset = 0;
        int num = 0;
        int name_i = 0;
        while (1)
        {
            while(get_sector_id(pre_inode, offset) == 0){
                if(pre_inode->ino == 0 && offset == 0)
                    break;
                else
                    offset ++;
            }            
            sbi_sd_read(kva2pa((uintptr_t)tmp_dentry), 1, SB_START + DT_BLOCK_OFFSET + get_sector_id(pre_inode, offset));
            offset ++;
            pre_de = (dentry_t *)tmp_dentry;
            if(mode){
                sbi_sd_read(kva2pa((uintptr_t)tmp_inode),
                        1, SB_START + ID_BLOCK_OFFSET + pre_de->ino);
                inode_t * read_inode = (inode_t *)tmp_inode;
                if(pre_de->access == O_RDONLY)
                    prints("R-     ");
                else if(pre_de->access == O_WRONLY)
                    prints("-W     ");
                else if(pre_de->access == O_RDWR)
                    prints("RW     ");
                prints("%d         ", pre_de->ino);
                prints("%d         ", read_inode->block_num);
                prints("%d         ", read_inode->link_num);
                if(read_inode->mode == IS_DIR)
                    prints("DIR           %d       \t", SIZE_SR);
                else
                    prints("FILE          %d       \t", read_inode->size);
            }
            prints("[%d]%s  ", name_i++,pre_de->name);        
            if(mode)
                prints("\n");

            if(++num == pre_inode->child_num)
                break;
        }
        if(!mode)         
            prints("\n");
    }
    screen_reflush();
}

/* mkdir */
bool do_mkdir(const char * path){
    inode_t *pre_ino = (inode_t *)kmalloc(ID_SZ);
    int mkdir_flag = 0;
    char path_two_dim[DEPTH][MAX_PATH_LENGTH] = {0};
    /* analysis path */
    int judge = analysis_path(path, path_two_dim);
    int j;
    if (path_two_dim[0][0] == 0){
        /* mkdir / */
        if(judge == 0){
            prints("[Error] illegal input: %s\n", path);
            return false;
        }
        /* root path */
        sbi_sd_read(kva2pa((uintptr_t)tmp_inode), 1, SB_START + ID_BLOCK_OFFSET);
        kmemcpy(pre_ino, tmp_inode, ID_SZ);
        j = 1;        
    }
    /* for the oslab/ */
    else {
        kmemcpy(pre_ino, &current_ino, ID_SZ);
        j = 0;        
    }
    for (; j <= judge; j++){
        /* not a dir */
        if(pre_ino->mode == IS_FILE){
            prints("[Error] the %s is a file\n", path_two_dim[j]);
            return false;
        }
        if(path_two_dim[j][0] == NULL){
            prints("[Error] illegal input: %s\n", path);
            return false;
        }
        pre_ino->modify_time = get_timer();
        if(!kstrcmp(path_two_dim[j], "."));
        else{
            int find_ino = find_inode_from_name(path_two_dim[j], pre_ino);
            if(find_ino == -1){
                int creat_ino = creat_entry(pre_ino, path_two_dim[j], IS_DIR);
                /* multi */
                kmemset(tmp_inode, 0 , SIZE_SR);
                sbi_sd_read(kva2pa((uintptr_t)tmp_inode), 1, SB_START + ID_BLOCK_OFFSET + creat_ino);
                kmemcpy(pre_ino, tmp_inode, ID_SZ);                
                mkdir_flag = 1;
            }else{
                /* get inode from the ino number */
                sbi_sd_read(kva2pa((uintptr_t)tmp_inode), 1, SB_START + ID_BLOCK_OFFSET + find_ino);
                kmemcpy(pre_ino, tmp_inode, ID_SZ);
            } 
        }
    }    
    if(mkdir_flag == 0){
        prints("[Error] the %s has exited\n", path);
        return false;
    }
}

/* cd */
bool do_cd(const char * path){
    inode_t *pre_ino = get_inode_from_dir(path);
    if(path[0] == NULL){
        prints("[Error] no input!\n");
        return false;
    }
    if(pre_ino != NULL){
        if(pre_ino->mode == IS_FILE){
            prints("[Error] the %s is a file\n", path);
            return false;
        }
        kmemcpy(&current_ino, pre_ino, ID_SZ);
        return true;
    }
    return false;
}

/* rmdir */
bool do_rmdir(const char * path){
    inode_t *pre_ino = get_inode_from_dir(path);
    char path_two_dim[DEPTH][MAX_PATH_LENGTH] = {0}; 
    /* analysis path */
    int judge = analysis_path(path, path_two_dim);
    if(path[0] == NULL){
        prints("[Error] no input!\n");
        return false;
    }
    if(pre_ino == NULL)
        return false;   
    if(pre_ino->mode == IS_FILE){
        prints("[Error] the %s is a file\n", path);
        return false;
    }  
    if(pre_ino->ino == 0){
        prints("[Error] Don't try to rmdir the root\n");
        return false;        
    }  
    /* find the father from the direct[1] */
    int father_inoid = get_father(pre_ino);
    inode_t * father_ino = get_inode_from_inoid(father_inoid);
    // screen_reflush();
    father_ino->modify_time = get_timer();
    delete_entry(father_ino, path_two_dim[judge]);
    return false;    
}
/* rm */
bool do_rm(char * path){
    int length = kstrlen(path);
    char name[32] = {0};
    int i,j;
    int has = 0;
    j = 0;
    if(path[0] == NULL){
        prints("[Error] no input\n");
        return false;
    }
    for(i = 0; i < length; i++){
        if(path[i] == '/'){
            has = 1;
            j = i;
        }   
    }
    if(has == 0){
        kstrcpy(name, path);
        path[0] = 0;
    }else{
        if(j == 0){
            kmemcpy(name, &path[1], length -1);
            path[1] = 0;
        }else{
            kmemcpy(name, &path[j+1], length -j -1);
            path[j] = 0;
        }
    }
    inode_t *pre_ino;
    if(path[0] == 0){
        pre_ino = (inode_t *) kmalloc(ID_SZ);
        kmemcpy(pre_ino, &current_ino, ID_SZ);
    }else
        pre_ino = get_inode_from_dir(path);
    if(pre_ino == NULL)
        return false;       
    if(pre_ino->mode == IS_FILE){
        prints("[Error] %s is not a dir\n", path);
        return false;
    }
    if(find_inode_from_name(name, pre_ino)==-1){
        prints("[Error] failed to find %s\n", name);
        return false;
    }
    pre_ino->modify_time = get_timer();    
    delete_entry(pre_ino, name);
    return true;
}

/* touch a file */
bool do_touch(const char * path){
    char my_path[128];
    kstrcpy(my_path, path);
    int length = kstrlen(my_path);
    char name[32] = {0};
    int i,j;
    int has = 0;
    j = 0;
    if(my_path[0] == NULL){
        prints("[Error] no input\n");
        return false;
    }
    for(i = 0; i < length; i++){
        if(my_path[i] == '/'){
            has = 1;
            j = i;
        }   
    }
    if(has == 0){
        kstrcpy(name, my_path);
        my_path[0] = 0;
    }else{
        if(j == 0){
            kmemcpy(name, &my_path[1], length -1);
            my_path[1] = 0;
        }else{
            kmemcpy(name, &my_path[j+1], length -j -1);
            my_path[j] = 0;
        }
    }
    inode_t *pre_ino;
    if(my_path[0] == 0){
        pre_ino = (inode_t *) kmalloc(ID_SZ);
        kmemcpy(pre_ino, &current_ino, ID_SZ);
    }else
        pre_ino = get_inode_from_dir(my_path);
    if(pre_ino == NULL)
        return false;       
    if(pre_ino->mode == IS_FILE){
        prints("[Error] %s is not a dir\n", my_path);
        return false;
    }
    if(find_inode_from_name(name, pre_ino)!=-1){
        prints("[Error] %s has existed\n", name);
        return false;
    }
    pre_ino->modify_time = get_timer();
    creat_entry(pre_ino, name, IS_FILE);
    return true;
}

/* cat */
bool do_cat(char * path){
    int length = kstrlen(path);
    char name[32] = {0};
    int i,j;
    int has = 0;
    j = 0;
    if(path[0] == NULL){
        prints("[Error] no input\n");
        return false;
    }
    for(i = 0; i < length; i++){
        if(path[i] == '/'){
            has = 1;
            j = i;
        }   
    }
    if(has == 0){
        kstrcpy(name, path);
        path[0] = 0;
    }else{
        if(j == 0){
            kmemcpy(name, &path[1], length -1);
            path[1] = 0;
        }else{
            kmemcpy(name, &path[j+1], length -j -1);
            path[j] = 0;
        }
    }
    inode_t *pre_ino;
    if(path[0] == 0){
        pre_ino = (inode_t *) kmalloc(ID_SZ);
        kmemcpy(pre_ino, &current_ino, ID_SZ);
    }else
        pre_ino = get_inode_from_dir(path);
    if(pre_ino == NULL)
        return false;       
    if(pre_ino->mode == IS_FILE){
        prints("[Error] %s is not a dir\n", path);
        return false;
    }
    /* no more than 512B */
    int file_id = find_inode_from_name(name, pre_ino);
    if(file_id == -1){
        prints("[Error] no this file %s\n", name);
        return false;
    }
    inode_t * file_inode = get_inode_from_inoid(file_id);
    int buffer_num = file_inode->size / SIZE_SR + 
                     ((file_inode->size % SIZE_SR) != 0);    
    int begin_sector =  0;
    int op_num = 0;
    int r_pos = 0;
    while(buffer_num--){
        sbi_sd_read(kva2pa((uintptr_t)R_W_buffer),  
                1, SB_START + DT_BLOCK_OFFSET + get_sector_id(file_inode, begin_sector));    
        int sector_offset = 0;
        for(;r_pos < file_inode->size && sector_offset < SIZE_SR; 
                                        r_pos++, sector_offset++){
            prints("%c", R_W_buffer[sector_offset]);
        }
        op_num ++;
        begin_sector ++;
    }

}

/* ln */
void do_ln( char * src_file, char * des_file){
    /* for the des inode which is a file */
    int length = kstrlen(des_file);
    char name[32] = {0};
    int i,j;
    int has = 0;
    j = 0;
    if(des_file[0] == NULL){
        prints("[Error] no input\n");
        return;
    }
    for(i = 0; i < length; i++){
        if(des_file[i] == '/'){
            has = 1;
            j = i;
        }   
    }
    if(has == 0){
        kstrcpy(name, des_file);
        des_file[0] = 0;
    }else{
        if(j == 0){
            kmemcpy(name, &des_file[1], length -1);
            des_file[1] = 0;
        }else{
            kmemcpy(name, &des_file[j+1], length -j -1);
            des_file[j] = 0;
        }
    }
    inode_t *pre_ino;
    if(des_file[0] == 0){
        pre_ino = (inode_t *) kmalloc(ID_SZ);
        kmemcpy(pre_ino, &current_ino, ID_SZ);
    }else
        pre_ino = get_inode_from_dir(des_file);
    if(pre_ino == NULL)
        return false;       
    if(pre_ino->mode == IS_FILE){
        prints("[Error] %s is not a dir\n", des_file);
        return false;
    }
    /* no more than 512B */
    int file_id = find_inode_from_name(name, pre_ino);
    if(file_id == -1){
        prints("[Error] no this file %s\n", name);
        return;
    }
    inode_t * des_inode = get_inode_from_inoid(file_id);
    /* for the src inode which is a dir */
    inode_t *src_inode = get_inode_from_dir(src_file);
    if(src_file[0] == NULL){
        prints("[Error] no input!\n");
        return;
    }
    if(src_inode == NULL){
        return;
    }
    if(src_inode->mode == IS_FILE){
        prints("[Error] the %s is a file\n", src_file);
        return;
    }
    des_inode->link_num ++;
    int new_sector = alloc_sector();
    kmemset(tmp_inode, 0 , SIZE_SR);
    kmemcpy(tmp_inode, des_inode, ID_SZ);
    sbi_sd_write(kva2pa((uintptr_t)tmp_inode),
                1, SB_START + ID_BLOCK_OFFSET + des_inode->ino);
    /* write to src_inode dentry*/
    kmemset(tmp_dentry, 0, SIZE_SR);
    dentry_t * de = (dentry_t *)tmp_dentry;
    de->ino = des_inode->ino;
    de->access = des_inode->access;
    kstrcpy(de->name, name);    
    sbi_sd_write(kva2pa((uintptr_t)tmp_dentry),
                1, SB_START + DT_BLOCK_OFFSET + new_sector);
    
    src_inode->child_num++;
    src_inode->modify_time = get_timer();
    set_sector_id(src_inode, get_free_entry(src_inode), new_sector);
    /* write back the src_inode */
    kmemset(tmp_inode, 0 , SIZE_SR);
    kmemcpy(tmp_inode, src_inode, ID_SZ);
    sbi_sd_write(kva2pa((uintptr_t)tmp_inode),
                1,  SB_START + ID_BLOCK_OFFSET + src_inode->ino);
    return;        
    
}

/* file system operation */
/* fopen */
int do_fopen(const char * name, int access){
    char my_name[32];
    kstrcpy(my_name, name);
    inode_t * pre_ino = (inode_t *)kmalloc(ID_SZ);
    kmemcpy(pre_ino, &current_ino, ID_SZ);
    int inoed_id;
    if((inoed_id = find_inode_from_name(my_name, pre_ino)) == -1){
        prints("[Error] falied to inoed_id find %s\n", my_name);
        return -1;
    }  
    inode_t * file_inode = get_inode_from_inoid(inoed_id);
    if(file_inode->mode == IS_DIR){
        prints("[Error] This is a DIR\n");
        return -1;
    }
    // prints("open inode %d\n", inoed_id);
    int fd = alloc_fd();
    if(fd == -1){
        prints("[Error] failed to alloc one fd\n");
        return -1;
    }
    
    file_t * a_fd = &openfile[fd];
    a_fd->access = access;
    a_fd->inode  = inoed_id;
    a_fd->addr   = allocPage();
    a_fd->r_pos  = 0;
    a_fd->w_pos  = 0;
    return fd;
}

/* fwrite */
int do_fwrite(int fd, char * buff,const int size){
    // prints("write %d\n", size);
    // screen_reflush();
    file_t * a_fd = &openfile[fd];
    inode_t * file_inode = get_inode_from_inoid(a_fd->inode);
    file_inode->modify_time = get_timer();
    if(file_inode->mode == IS_DIR){
        prints("[Error] This is a DIR\n");
        return 0;
    }
    if(a_fd->access == O_RDONLY || file_inode->access == O_RDONLY){
        prints("[Error] permisson delay\n");
        return 0;
    }
    /* no_alloc_place */
    int buffer_num = (a_fd->w_pos % SIZE_SR + size) / SIZE_SR + 
                     (((a_fd->w_pos % SIZE_SR + size) % SIZE_SR) != 0);
    int begin_sector = a_fd->w_pos / SIZE_SR;
    int cp_begin = a_fd->w_pos % SIZE_SR;
    int cp_now = size;
    int op_num = 0;
    while(buffer_num--){
        if(get_sector_id(file_inode, begin_sector) == 0){
            kmemset(R_W_buffer, 0, SIZE_SR);
            set_sector_id(file_inode, begin_sector, alloc_sector());
            file_inode->block_num++;
        } else {
            sbi_sd_read(kva2pa((uintptr_t)R_W_buffer),  
                    1, SB_START + DT_BLOCK_OFFSET + get_sector_id(file_inode, begin_sector));    
        }     
        uint32_t copy_length;
        if(buffer_num == 0)
            copy_length = cp_now;
        else{
            if(a_fd->w_pos % SIZE_SR != 0 && op_num == 0){
                copy_length = SIZE_SR - cp_begin;
            } else
                copy_length = SIZE_SR;
        }
        kmemcpy(&R_W_buffer[cp_begin], (uintptr_t)buff, copy_length);
        buff = (char *)(copy_length + (uintptr_t)buff);
        sbi_sd_write(kva2pa((uintptr_t)R_W_buffer),
                1, SB_START + DT_BLOCK_OFFSET + get_sector_id(file_inode, begin_sector));
        cp_begin = 0;
        cp_now -= copy_length;
        begin_sector ++;
        op_num ++;
    }
    a_fd->w_pos += size;
    if(a_fd->w_pos > file_inode->size)
        file_inode->size = a_fd->w_pos;
    sbi_sd_write(kva2pa((uintptr_t)file_inode),
            1, SB_START + ID_BLOCK_OFFSET + file_inode->ino);    
}

/* fread */
int do_fread(int fd, char * buff,const int size){
    // prints("read %d    \n", size);
    // screen_reflush();
    file_t * a_fd = &openfile[fd];
    inode_t * file_inode = get_inode_from_inoid(a_fd->inode);
    if(file_inode->mode == IS_DIR){
        prints("[Error] This is a DIR\n");
        return 0;
    }
    if(a_fd->access == O_WRONLY || file_inode->access == O_WRONLY){
        prints("[Error] permisson delay\n");
        return 0;
    }
    int buffer_num = (a_fd->r_pos % SIZE_SR + size) / SIZE_SR + 
                     (((a_fd->r_pos % SIZE_SR + size) % SIZE_SR) != 0);    
    int begin_sector =  a_fd->r_pos / SIZE_SR;
    int sector_offset = a_fd->r_pos % SIZE_SR;
    int cp_now = size;
    int op_num = 0;
    if((a_fd->r_pos + size) > file_inode->size){
        prints("[Error] cross the border\n");
        return -1;
    }
    while(buffer_num--){
        sbi_sd_read(kva2pa((uintptr_t)R_W_buffer),  
                1, SB_START + DT_BLOCK_OFFSET + get_sector_id(file_inode, begin_sector));    
        uint32_t copy_length;
        if(buffer_num == 0)
            copy_length = cp_now;
        else{
            if(a_fd->r_pos % SIZE_SR != 0 && op_num == 0)
                copy_length = SIZE_SR - sector_offset;
            else
                copy_length = SIZE_SR;
        }
        kmemcpy(buff, &R_W_buffer[sector_offset], copy_length);
        buff = (char *)(copy_length + (uintptr_t)buff);
        sector_offset = 0;
        cp_now -= copy_length;
        begin_sector ++;
        op_num ++;
    }
    a_fd->r_pos += size;
}

/* fclose */
void do_close(int fd){
    openfile[fd].used = 0;
}

/* do_lseek for huge file */
int do_lseek(int fd, int offset, int whence){
    file_t * o_fd = &openfile[fd];
    inode_t * file_inode = get_inode_from_inoid(o_fd->inode);
    int target;
    switch (whence){
        case SEEK_SET:
            target = offset;
            break;
        case SEEK_CUR:
            target = o_fd->r_pos + offset;
            break;
        case SEEK_END:
            target = file_inode->size + offset;
            break;
    }
    o_fd->r_pos = target;
    o_fd->w_pos = target;
    int begin_sector = file_inode -> size / SIZE_SR + ((file_inode -> size % SIZE_SR) != 0);
    int target_sector = target / SIZE_SR + ((target % SIZE_SR) != 0);
    /* block_map */    
    int sro = 0;
    sbi_sd_read(kva2pa((uintptr_t)tmp_block_map)
            , SIZE_SR_MAP, SB_START + SR_MAP_OFFSET); 
    if(target_sector > begin_sector){
        for (int i = begin_sector; i < target_sector; ){
            if(i < MAX_DIR_MAP){
                /* direct point */
                sro = alloc_sector_continue(tmp_block_map, sro);
                file_inode->direct[i++] = sro;
            }else if(i < MAX_FST_MAP){
                /* first point */
                int find_level_1 = (i - MAX_DIR_MAP)/128;
                int find_final = i - MAX_DIR_MAP - 128 * find_level_1;
                if(file_inode->level_1[find_level_1] == 0){
                    sro = alloc_sector_continue(tmp_block_map, sro);                               
                    file_inode->level_1[find_level_1] = sro;
                    kmemset(first_map, 0 ,SIZE_SR);
                }else{
                    sbi_sd_read(kva2pa((uintptr_t)first_map), 
                            1, SB_START + DT_BLOCK_OFFSET + file_inode->level_1[find_level_1]);    
                }
                for(;find_final < 128 && i< target_sector; find_final++, i++){
                    sro = alloc_sector_continue(tmp_block_map, sro);
                    first_map[find_final] = sro;             
                }
                sbi_sd_write(kva2pa((uintptr_t)first_map), 
                        1, SB_START + DT_BLOCK_OFFSET + file_inode->level_1[find_level_1]);
                
            }else if(i < MAX_SEC_MAP){
                /* second point */
                int find_level_1 = (i - MAX_FST_MAP) / (128 * 128);
                int find_level_2 = (i - MAX_FST_MAP - (128 * 128) * find_level_1) / 128;
                int find_final =  i - MAX_FST_MAP - (128 * 128) * find_level_1 - 128 * find_level_2;
                if(file_inode->level_2[find_level_1] == 0){
                    sro = alloc_sector_continue(tmp_block_map, sro);
                    file_inode->level_2[find_level_1] = sro;
                    kmemset(first_map, 0, SIZE_SR);
                } else {
                    sbi_sd_read(kva2pa((uintptr_t)first_map), 
                            1, SB_START + DT_BLOCK_OFFSET + file_inode->level_2[find_level_1]); 
                }
                if(first_map[find_level_2] == 0){
                    sro = alloc_sector_continue(tmp_block_map, sro);
                    first_map[find_level_2] = sro;
                    sbi_sd_write(kva2pa((uintptr_t)first_map),
                            1, SB_START + DT_BLOCK_OFFSET + file_inode->level_2[find_level_1]);
                    kmemset(second_map, 0, SIZE_SR);
                } else {
                    sbi_sd_read(kva2pa((uintptr_t)second_map),
                            1, SB_START + DT_BLOCK_OFFSET + first_map[find_level_2]);                 
                }
                for(;find_final < 128 && i< target_sector; find_final++, i++){
                    sro = alloc_sector_continue(tmp_block_map, sro);
                    second_map[find_final] = sro;        
                }
                sbi_sd_write(kva2pa((uintptr_t)second_map),
                        1, SB_START + DT_BLOCK_OFFSET + first_map[find_level_2]);     
            }else if(i < MAX_THR_MAP){
                /* third point */
                int find_level_1 = (i - MAX_SEC_MAP) / (128 * 128 * 128);
                int find_level_2 = (i - MAX_SEC_MAP - (128 * 128 * 128) * find_level_1) / (128 * 128);
                int find_level_3 = (i - MAX_SEC_MAP - (128 * 128 * 128) * find_level_1 - (128 * 128) * find_level_2) / 128;
                int find_final =  i - MAX_SEC_MAP - (128 * 128 * 128) * find_level_1 - (128 * 128) * find_level_2 - 128 * find_level_3; 
                if(file_inode->level_3[find_level_1] == 0){
                    sro = alloc_sector_continue(tmp_block_map, sro);
                    file_inode->level_3[find_level_1] = sro;
                    kmemset(first_map, 0, SIZE_SR);
                } else {
                    sbi_sd_read(kva2pa((uintptr_t)first_map), 
                            1, SB_START + DT_BLOCK_OFFSET + file_inode->level_3[find_level_1]); 
                }       
                if(first_map[find_level_2] == 0){
                    sro = alloc_sector_continue(tmp_block_map, sro);
                    first_map[find_level_2] = sro;
                    sbi_sd_write(kva2pa((uintptr_t)first_map),
                            1, SB_START + DT_BLOCK_OFFSET + file_inode->level_3[find_level_1]);
                    kmemset(second_map, 0, SIZE_SR);                
                } else {
                    sbi_sd_read(kva2pa((uintptr_t)second_map),
                            1, SB_START + DT_BLOCK_OFFSET + first_map[find_level_2]);
                }
                if(second_map[find_level_3] == 0){
                    sro = alloc_sector_continue(tmp_block_map, sro);
                    second_map[find_level_3] = sro;
                    sbi_sd_write(kva2pa((uintptr_t)second_map),
                            1, SB_START + DT_BLOCK_OFFSET + first_map[find_level_2]);
                    kmemset(third_map, 0, SIZE_SR);
                }  else {
                    sbi_sd_read(kva2pa((uintptr_t)third_map),
                            1, SB_START + DT_BLOCK_OFFSET + second_map[find_level_3]);
                }
                for(;find_final < 128 && i< target_sector; find_final++, i++){
                    sro = alloc_sector_continue(tmp_block_map, sro);
                    third_map[find_final] = sro;
                }
                sbi_sd_write(kva2pa((uintptr_t)third_map),
                        1, SB_START + DT_BLOCK_OFFSET + second_map[find_level_2]);
            }
        }   
    }
    /* write back block map */
    sbi_sd_write(kva2pa((uintptr_t)tmp_block_map)
            , SIZE_SR_MAP, SB_START + SR_MAP_OFFSET); 

    /* refresh the super block */
    kmemset(tmp_sb, 0, SIZE_SR);
    sbi_sd_read(kva2pa((uintptr_t)tmp_sb), 1, SB_START);
    superblock_t * sb_now = (superblock_t *)tmp_sb;
    /* refresh size */  
    if(target > file_inode->size){
        file_inode->size = target;
        file_inode->block_num += (target_sector - begin_sector);
        sb_now->datablock_num += (target_sector - begin_sector);
    }      
    sbi_sd_write(kva2pa((uintptr_t)tmp_sb), 1, SB_START); 

    /* write inode back SD */
    kmemset(tmp_inode, 0, SIZE_SR);
    kmemcpy(tmp_inode, file_inode, ID_SZ);
    sbi_sd_write(kva2pa((uintptr_t)tmp_inode), 
                1, SB_START + ID_BLOCK_OFFSET + file_inode->ino);
    return true;
}