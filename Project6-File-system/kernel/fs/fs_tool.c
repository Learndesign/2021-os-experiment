#include <os/list.h>
#include <os/fs_tool.h>
#include <sbi.h>
#include <pgtable.h>
#include <os/stdio.h>
#include <os/mm.h>
#include <os/time.h>
#include <screen.h>
#include <os/string.h>
#include <os/sched.h>
/* analysis path */
int analysis_path(char * path, char two_dim[DEPTH][MAX_PATH_LENGTH]){
    int length = kstrlen(path);
    int i;
    int index = 0;
    int judge = 0;
    for (i = 0; i < length; i++){
        char ch = path[i];
        if(ch == '/'){
            two_dim[judge++][index++] = 0;
            index = 0;
        }            
        else      
            two_dim[judge][index++] = ch;    
    }
    two_dim[judge][index] = 0;   
    if(two_dim[judge][0] == 0)
        judge -- ;
    return judge;
}

/* check the file system existed */
bool check_fs(){
    sbi_sd_read(kva2pa((uintptr_t)tmp_sb), 1, SB_START);
    superblock_t * sb_now = (superblock_t *) tmp_sb;
    if(sb_now->magic == SB_MAGIC)
        return true;
    else
        return false;
}

/* find name from the inode */
int find_inode_from_name(char * name, inode_t * fd_inode){ 
    /* read the sb */
    dentry_t * pre_de = NULL;
    int num = 0;
    int offset = 0;
    while(1){    
        while(get_sector_id(fd_inode, offset) == 0){
            if(fd_inode->ino == 0 && offset ==0)
                break;
            else
                offset ++;
        }
        sbi_sd_read(kva2pa((uintptr_t)tmp_dentry), 1, SB_START + DT_BLOCK_OFFSET + get_sector_id(fd_inode, offset));
        offset++ ;
        pre_de = (dentry_t *)tmp_dentry;
        if(!kstrcmp(pre_de->name, name)){
            return pre_de->ino;
        }
        if(++num == fd_inode->child_num)
            break;
    }  
    return -1;
}

/* get current inode , maybe used for the command ls, cd...*/
inode_t * get_inode_from_dir(char * path){
    inode_t *pre_ino = (inode_t *)kmalloc(ID_SZ);
    char path_two_dim[DEPTH][MAX_PATH_LENGTH] = {0}; 
    /* analysis path */
    int judge = analysis_path(path, path_two_dim);
    /* for the /... */ 
    int j;
    if (path_two_dim[0][0] == 0){
        /* get the 0 inode */
        sbi_sd_read(kva2pa((uintptr_t)tmp_inode), 1, SB_START + ID_BLOCK_OFFSET);
        // pre_ino = (inode_t *)tmp_inode;
        kmemcpy(pre_ino, tmp_inode, ID_SZ);
        if(judge == 0){
            return pre_ino;
        }
        j = 1;
    } 
    /*for the oslab/.....*/
    else {
        kmemcpy(pre_ino, &current_ino, ID_SZ);
        j = 0;
    }
    for (; j <= judge; j++){
        /* not a dir */
        if(pre_ino->mode == IS_FILE){
            prints("[Error] the %s is a file\n", path_two_dim[j]);
            return NULL;
        }
        if(path_two_dim[j][0] == NULL){
            prints("[Error] illegal input: %s\n", path);
            return NULL;
        }
        if(!kstrcmp(path_two_dim[j], "."));
        else{
            int find_ino = find_inode_from_name(path_two_dim[j], pre_ino);
            if(find_ino == -1){
                prints("[Error] No such file: %s\n",path_two_dim[j]);
                return NULL;
            }else{
                /* get inode from the ino number */
                sbi_sd_read(kva2pa((uintptr_t)tmp_inode), 1, SB_START + ID_BLOCK_OFFSET + find_ino);
                kmemcpy(pre_ino, tmp_inode, ID_SZ);
            } 
        }
    }
    return pre_ino;
}
/* alloc inode */
int alloc_inode(){
    /* read id_map */
	sbi_sd_read(kva2pa((uintptr_t)tmp_id_map)
            , SIZE_ID_MAP, SB_START + ID_MAP_OFFSET);
    int ino;
    for (ino = 0; ino < ID_MAP_NUM; ino++){
        if(tmp_id_map[ino] == 0) break;
    }
    tmp_id_map[ino] = 1;
	sbi_sd_write(kva2pa((uintptr_t)tmp_id_map)
            , SIZE_ID_MAP, SB_START + ID_MAP_OFFSET);
    /* stst of sb */
    kmemset(tmp_sb, 0, SIZE_SR);
    sbi_sd_read(kva2pa((uintptr_t)tmp_sb), 1, SB_START);
    superblock_t * sb_now = (superblock_t *)tmp_sb;    
    sb_now->inode_num ++;
    sbi_sd_write(kva2pa((uintptr_t)tmp_sb), 1, SB_START);     
    return ino;
}
/* alloc sector */
int alloc_sector(){
    sbi_sd_read(kva2pa((uintptr_t)tmp_block_map)
            , SIZE_SR_MAP, SB_START + SR_MAP_OFFSET); 
    int sro;
    for (sro = 0; sro < SR_MAP_NUM; sro++){
        if(tmp_block_map[sro] == 0) break;
    }
    tmp_block_map[sro] = 1;
    sbi_sd_write(kva2pa((uintptr_t)tmp_block_map)
            , SIZE_SR_MAP, SB_START + SR_MAP_OFFSET);  
    /* stst of sb */
    kmemset(tmp_sb, 0, SIZE_SR);
    sbi_sd_read(kva2pa((uintptr_t)tmp_sb), 1, SB_START);
    superblock_t * sb_now = (superblock_t *)tmp_sb;    
    sb_now->datablock_num ++;
    sbi_sd_write(kva2pa((uintptr_t)tmp_sb), 1, SB_START); 
    return sro;       
}
int alloc_sector_continue(int * sr_map, int sro){
    for (; sro < SR_MAP_NUM; sro++){
        if(sr_map[sro] == 0){
            sr_map[sro] = 1;
            return sro;
        }
    }
}


/* free inode */
void free_inode(int id){
    /* read id_map */
	sbi_sd_read(kva2pa((uintptr_t)tmp_id_map)
            , SIZE_ID_MAP, SB_START + ID_MAP_OFFSET);
    tmp_id_map[id] = 0;
	sbi_sd_write(kva2pa((uintptr_t)tmp_id_map)
            , SIZE_ID_MAP, SB_START + ID_MAP_OFFSET);
    /* stst of sb */
    kmemset(tmp_sb, 0, SIZE_SR);
    sbi_sd_read(kva2pa((uintptr_t)tmp_sb), 1, SB_START);
    superblock_t * sb_now = (superblock_t *)tmp_sb;    
    sb_now->inode_num --;
    sbi_sd_write(kva2pa((uintptr_t)tmp_sb), 1, SB_START);     
}
/* free sector */
void free_sector(int id){
    sbi_sd_read(kva2pa((uintptr_t)tmp_block_map)
            , SIZE_SR_MAP, SB_START + SR_MAP_OFFSET); 
    tmp_block_map[id] = 0;
    sbi_sd_write(kva2pa((uintptr_t)tmp_block_map)
            , SIZE_SR_MAP, SB_START + SR_MAP_OFFSET);  
    /* stst of sb */
    kmemset(tmp_sb, 0, SIZE_SR);
    sbi_sd_read(kva2pa((uintptr_t)tmp_sb), 1, SB_START);
    superblock_t * sb_now = (superblock_t *)tmp_sb;    
    sb_now->datablock_num --;
    sbi_sd_write(kva2pa((uintptr_t)tmp_sb), 1, SB_START); 
}

/* set the inode first for the dir or file */
int set_inode_first(int father_ino, int new_inode, int mode){
    if(mode == IS_DIR){
        kmemset(tmp_inode, 0, SIZE_SR);

        inode_t * all_ino = (inode_t *)tmp_inode;
        all_ino->access = O_RDWR;
        all_ino->ino = new_inode;
        all_ino->mode = IS_DIR;
        all_ino->block_num = 2;
        all_ino->child_num = 2;  
        all_ino->modify_time = get_timer();
        all_ino->create_time = get_timer();
        all_ino->link_num = 0;
        /* the "." and the ".." */    
        /* for "." */
        kmemset(tmp_dentry, 0, SIZE_SR);

        dentry_t * to_me_de = (dentry_t *)tmp_dentry;
        kstrcpy(to_me_de->name, "."); 
        to_me_de->ino = new_inode;
        to_me_de->access = O_RDWR;
    
        int to_me_sector = alloc_sector();
        set_sector_id(all_ino, 0, to_me_sector);
        sbi_sd_write(kva2pa((uintptr_t)tmp_dentry)
                , 1 , SB_START + DT_BLOCK_OFFSET + to_me_sector);   
        /* for ".." */
        kmemset(tmp_dentry, 0, SIZE_SR);
        dentry_t * to_father_de = (dentry_t *)tmp_dentry;
        kstrcpy(to_father_de->name, ".."); 
        to_father_de->ino = father_ino;
        to_father_de->access = O_RDWR;
        int to_father_sector = alloc_sector();
        set_sector_id(all_ino, 1, to_father_sector);
        sbi_sd_write(kva2pa((uintptr_t)tmp_dentry)
                , 1 , SB_START + DT_BLOCK_OFFSET + to_father_sector); 
        /* write new inode */
        sbi_sd_write(kva2pa((uintptr_t)tmp_inode), 
                1, SB_START + ID_BLOCK_OFFSET + all_ino->ino);         
    } else {
        kmemset(tmp_inode, 0, SIZE_SR);

        inode_t * all_ino = (inode_t *)tmp_inode;
        all_ino->access = O_RDWR;
        all_ino->ino = new_inode;
        all_ino->mode = IS_FILE;
        all_ino->modify_time = get_timer();
        all_ino->create_time = get_timer();
        all_ino->link_num = 1;
        all_ino->size = 0;
        /* no need for the "." and the ".." */    
     
        /* write new inode */
        sbi_sd_write(kva2pa((uintptr_t)tmp_inode), 
                1, SB_START + ID_BLOCK_OFFSET + all_ino->ino);             
    }
}


/* creat one dentry */
int creat_entry(inode_t * father_ino, char * name, int mode){
    /* read sb */

    kmemset(tmp_dentry, 0, SIZE_SR);
    kmemset(tmp_inode, 0, SIZE_SR);
    dentry_t * de = (dentry_t *)tmp_dentry;
    /* the father's son */
    int new_inode = alloc_inode();

    int new_sector = alloc_sector();

    kstrcpy(de->name, name);   
    de->access = O_RDWR;
    de->ino = new_inode;
	sbi_sd_write(kva2pa((uintptr_t)tmp_dentry)
            , 1 , SB_START + DT_BLOCK_OFFSET + new_sector);
    father_ino->child_num ++;
    father_ino->block_num ++;
    father_ino->modify_time = get_timer();
    /* add sector to the father */
    set_sector_id(father_ino, get_free_entry(father_ino), new_sector);
    /* write back father inode */
    if(father_ino->ino == current_ino.ino)
        kmemcpy(&current_ino, father_ino, ID_SZ);
    kmemcpy(tmp_inode, father_ino, ID_SZ);
    sbi_sd_write(kva2pa((uintptr_t)tmp_inode), 
            1, SB_START + ID_BLOCK_OFFSET + father_ino->ino); 

    /* init the inode */
    set_inode_first(father_ino->ino, new_inode, mode);
    return new_inode;
}
/* delete one entry */
void delete_entry(inode_t * father_ino, char * name){
    
    int son_inoid = get_entry_from_inode(father_ino, name);
    inode_t * son_inode = get_inode_from_inoid(son_inoid);
    if(son_inode->mode == IS_FILE){
        son_inode -> link_num --;
        kmemset(R_W_buffer, 0, SIZE_SR);
        kmemcpy(R_W_buffer, son_inode, ID_SZ);
        sbi_sd_write(kva2pa((uintptr_t)R_W_buffer),
                1, SB_START + ID_BLOCK_OFFSET + son_inode->ino); 
    }
        
    if(son_inode->link_num == 0 && son_inode->mode == IS_FILE){
        delete_entry_from_inode(father_ino, name);
        free_entry_from_inode(son_inode);
        free_inode(son_inoid);
    }else if(son_inode->child_num > 2){
        delete_entry_from_inode(father_ino, name);
        /* digui */
        int offset = 2;
        int num = 2;
        while(1){
            while(get_sector_id(son_inode, offset) == 0)
                offset ++;
            kmemset(tmp_dentry, 0, SIZE_SR);
            sbi_sd_read(kva2pa((uintptr_t)tmp_dentry), 
                    1, SB_START + DT_BLOCK_OFFSET + get_sector_id(son_inode, offset));
            offset++;
            dentry_t * de = (dentry_t *)kmalloc(DENTRY_SZ);
            kmemcpy(de, tmp_dentry, DENTRY_SZ);
            delete_entry(son_inode, de->name);
            if(num == son_inode->child_num)
                break;
            
        }   
        free_entry_from_inode(son_inode);             
               
        free_inode(son_inoid); 
    }else{
        delete_entry_from_inode(father_ino, name);
        
        free_inode(son_inoid); 
    }
}

/* get the father inode */
int get_father(inode_t * pre_inode){
    kmemset(tmp_dentry, 0 ,SIZE_SR);
    sbi_sd_read(kva2pa((uintptr_t)tmp_dentry), 
            1, SB_START + DT_BLOCK_OFFSET + get_sector_id(pre_inode, 1));
    dentry_t * father_de = (dentry_t *)tmp_dentry;
    return father_de->ino;
}

inode_t * get_inode_from_inoid(int inoid){
    inode_t * fd_inode = (inode_t *)kmalloc(ID_SZ);
    kmemset(tmp_inode, 0, SIZE_SR);
    sbi_sd_read(kva2pa((uintptr_t)tmp_inode), 
            1, SB_START + ID_BLOCK_OFFSET + inoid);
    kmemcpy(fd_inode, tmp_inode, ID_SZ);
    return fd_inode;  
}

int get_entry_from_inode(inode_t * fd_inode, char * name){
    int offset = 0;
    int num = 0;
    while(1){
        /* alloc */
        while(get_sector_id(fd_inode, offset) == 0){
            if(fd_inode->ino == 0 && offset == 0)
                break;
            else
                offset ++;
        }
        kmemset(tmp_dentry, 0, SIZE_SR);
        sbi_sd_read(kva2pa((uintptr_t)tmp_dentry), 
                1, SB_START + DT_BLOCK_OFFSET + get_sector_id(fd_inode, offset));
        offset ++;
        dentry_t * de = (dentry_t *)tmp_dentry;
        if(!kstrcmp(de->name, name))
            return de->ino;
    }
    return -1;
}

int delete_entry_from_inode(inode_t * fd_inode, char * name){
    int offset = 0;
    int num = 0;
    while(1){
        while(get_sector_id(fd_inode, offset) == 0){
            if(fd_inode->ino == 0 && offset == 0)
                break;
            else
                offset ++;
        }
        kmemset(tmp_dentry, 0, SIZE_SR);
        sbi_sd_read(kva2pa((uintptr_t)tmp_dentry), 
                1, SB_START + DT_BLOCK_OFFSET + get_sector_id(fd_inode, offset));
        offset++;
        dentry_t * de = (dentry_t *)tmp_dentry;
        if(!kstrcmp(de->name, name)){
            free_sector(get_sector_id(fd_inode, offset -1));
            sbi_sd_write(kva2pa((uintptr_t)tmp_sb), 1, SB_START); 
            set_sector_id(fd_inode, offset -1, 0);
            fd_inode->child_num--;
            fd_inode->block_num--;
            kmemset(tmp_inode, 0, SIZE_SR);
            kmemcpy(tmp_inode, fd_inode, ID_SZ);
            sbi_sd_write(kva2pa((uintptr_t)tmp_inode),
                1, SB_START + ID_BLOCK_OFFSET + fd_inode->ino);
            if(fd_inode->ino == current_ino.ino)
                kmemcpy(&current_ino, fd_inode, ID_SZ);
            return 1;
        }
    }
}

/* free all place */
int free_entry_from_inode(inode_t * fr_inode){
    sbi_sd_read(kva2pa((uintptr_t)tmp_block_map)
            , SIZE_SR_MAP, SB_START + SR_MAP_OFFSET);
   
    int j = 0;    
    for(int i = 0; i < fr_inode->block_num; i++){
        int free_sector;
        while((free_sector = get_sector_id(fr_inode, j)) == 0) j++;
        j++;
        tmp_block_map[free_sector] = 0;
    }
    sbi_sd_write(kva2pa((uintptr_t)tmp_block_map)
            , SIZE_SR_MAP, SB_START + SR_MAP_OFFSET); 
    kmemset(tmp_sb, 0, SIZE_SR);
    sbi_sd_read(kva2pa((uintptr_t)tmp_sb), 1, SB_START);
    superblock_t * sb_now = (superblock_t *)tmp_sb;    
    sb_now->datablock_num -= fr_inode->block_num;
    sbi_sd_write(kva2pa((uintptr_t)tmp_sb), 1, SB_START);     
}

/* get sector id from offset */
int get_sector_id(inode_t * fd_inode, int offset){
    if(offset < MAX_DIR_MAP)
        /* direct map */
        return fd_inode->direct[offset];
    else if(offset < MAX_FST_MAP){
        /* first map */
        int find_level_1 = (offset - MAX_DIR_MAP)/128;
        int find_final = offset - MAX_DIR_MAP - 128 * find_level_1;
        if(fd_inode->level_1[find_level_1] == 0){
            return 0;
        }else{
            sbi_sd_read(kva2pa((uintptr_t)first_map), 
                    1, SB_START + DT_BLOCK_OFFSET + fd_inode->level_1[find_level_1]);    
        }
        if(first_map[find_final] == 0){
            return 0 ;            
        }
        return first_map[find_final];
    }else if(offset < MAX_SEC_MAP){
        /* second map */
        int find_level_1 = (offset - MAX_FST_MAP) / (128 * 128);
        int find_level_2 = (offset - MAX_FST_MAP - (128 * 128) * find_level_1) / 128;
        int find_final =  offset - MAX_FST_MAP - (128 * 128) * find_level_1 - 128 * find_level_2;
        if(fd_inode->level_2[find_level_1] == 0){
            
            return 0;
        } else {
            sbi_sd_read(kva2pa((uintptr_t)first_map), 
                    1, SB_START + DT_BLOCK_OFFSET + fd_inode->level_2[find_level_1]); 
        }

        if(first_map[find_level_2] == 0){
            
            return 0;
        } else {
            sbi_sd_read(kva2pa((uintptr_t)second_map),
                    1, SB_START + DT_BLOCK_OFFSET + first_map[find_level_2]);                 
        }

        if(second_map[find_final] == 0){
            return 0;           
        }

        return second_map[find_final];            
    }else if(offset < MAX_THR_MAP){
        /* third map */
        int find_level_1 = (offset - MAX_SEC_MAP) / (128 * 128 * 128);
        int find_level_2 = (offset - MAX_SEC_MAP - (128 * 128 * 128) * find_level_1) / (128 * 128);
        int find_level_3 = (offset - MAX_SEC_MAP - (128 * 128 * 128) * find_level_1 - (128 * 128) * find_level_2) / 128;
        int find_final =  offset - MAX_SEC_MAP - (128 * 128 * 128) * find_level_1 - (128 * 128) * find_level_2 - 128 * find_level_3;     
        if(fd_inode->level_3[find_level_1] == 0){
            return 0;
        } else {
            sbi_sd_read(kva2pa((uintptr_t)first_map), 
                    1, SB_START + DT_BLOCK_OFFSET + fd_inode->level_3[find_level_1]); 
        }    

        if(first_map[find_level_2] == 0){
     
            return 0;
        } else {
            sbi_sd_read(kva2pa((uintptr_t)second_map),
                    1, SB_START + DT_BLOCK_OFFSET + first_map[find_level_2]);
        }

        if(second_map[find_level_3] == 0){
         
            return 0;
        } else {
            sbi_sd_read(kva2pa((uintptr_t)third_map),
                    1, SB_START + DT_BLOCK_OFFSET + second_map[find_level_3]);
        }

        if(third_map[find_final] == 0){
            
            return 0;
        }
        return third_map[find_final];
    }        
}

/* set sector id from offset */
int set_sector_id(inode_t * fd_inode, int offset, int sector_id){
    // if(fd_inode->mode == IS_DIR){
    
        if(offset < MAX_DIR_MAP){
            fd_inode->direct[offset] = sector_id;
            return fd_inode->direct[offset];
        }else if(offset < MAX_FST_MAP){
           
            int find_level_1 = (offset - MAX_DIR_MAP)/128;
            int find_final = offset - MAX_DIR_MAP - 128 * find_level_1;
            if(fd_inode->level_1[find_level_1] == 0){
                fd_inode->level_1[find_level_1] = alloc_sector();
                
                kmemset(first_map, 0 ,SIZE_SR);
            }else{
                sbi_sd_read(kva2pa((uintptr_t)first_map), 
                        1, SB_START + DT_BLOCK_OFFSET + fd_inode->level_1[find_level_1]);    
            }
            first_map[find_final] = sector_id;
            sbi_sd_write(kva2pa((uintptr_t)first_map), 
                    1, SB_START + DT_BLOCK_OFFSET + fd_inode->level_1[find_level_1]);
            return first_map[find_final];
        }else if(offset < MAX_SEC_MAP){
       
            int find_level_1 = (offset - MAX_FST_MAP) / (128 * 128);
            int find_level_2 = (offset - MAX_FST_MAP - (128 * 128) * find_level_1) / 128;
            int find_final =  offset - MAX_FST_MAP - (128 * 128) * find_level_1 - 128 * find_level_2;
            if(fd_inode->level_2[find_level_1] == 0){
                fd_inode->level_2[find_level_1] = alloc_sector();
                kmemset(first_map, 0, SIZE_SR);
            } else {
                sbi_sd_read(kva2pa((uintptr_t)first_map), 
                        1, SB_START + DT_BLOCK_OFFSET + fd_inode->level_2[find_level_1]); 
            }

            if(first_map[find_level_2] == 0){
                first_map[find_level_2] = alloc_sector();
                sbi_sd_write(kva2pa((uintptr_t)first_map),
                        1, SB_START + DT_BLOCK_OFFSET + fd_inode->level_2[find_level_1]);
                kmemset(second_map, 0, SIZE_SR);
            } else {
                sbi_sd_read(kva2pa((uintptr_t)second_map),
                        1, SB_START + DT_BLOCK_OFFSET + first_map[find_level_2]);                 
            }
            second_map[find_final] = sector_id;
            sbi_sd_write(kva2pa((uintptr_t)second_map),
                    1, SB_START + DT_BLOCK_OFFSET + first_map[find_level_2]);
            return second_map[find_final];            
        }else if(offset < MAX_THR_MAP){
            int find_level_1 = (offset - MAX_SEC_MAP) / (128 * 128 * 128);
            int find_level_2 = (offset - MAX_SEC_MAP - (128 * 128 * 128) * find_level_1) / (128 * 128);
            int find_level_3 = (offset - MAX_SEC_MAP - (128 * 128 * 128) * find_level_1 - (128 * 128) * find_level_2) / 128;
            int find_final =  offset - MAX_SEC_MAP - (128 * 128 * 128) * find_level_1 - (128 * 128) * find_level_2 - 128 * find_level_3; 
        
            if(fd_inode->level_3[find_level_1] == 0){
                fd_inode->level_3[find_level_1] = alloc_sector();
                kmemset(first_map, 0, SIZE_SR);
            }else {
                sbi_sd_read(kva2pa((uintptr_t)first_map), 
                        1, SB_START + DT_BLOCK_OFFSET + fd_inode->level_3[find_level_1]); 
            }       
            if(first_map[find_level_2] == 0){
                first_map[find_level_2] = alloc_sector();
                sbi_sd_write(kva2pa((uintptr_t)first_map),
                        1, SB_START + DT_BLOCK_OFFSET + fd_inode->level_3[find_level_1]);
                kmemset(second_map, 0, SIZE_SR);                
            } else {
                sbi_sd_read(kva2pa((uintptr_t)second_map),
                        1, SB_START + DT_BLOCK_OFFSET + first_map[find_level_2]);
            }
            if(second_map[find_level_3] == 0){
                second_map[find_level_3] = alloc_sector();
                sbi_sd_write(kva2pa((uintptr_t)second_map),
                        1, SB_START + DT_BLOCK_OFFSET + first_map[find_level_2]);
                kmemset(third_map, 0, SIZE_SR);
            } else {
                sbi_sd_read(kva2pa((uintptr_t)third_map),
                        1, SB_START + DT_BLOCK_OFFSET + second_map[find_level_3]);
            }
            third_map[find_final] = sector_id;
            sbi_sd_write(kva2pa((uintptr_t)third_map),
                    1, SB_START + DT_BLOCK_OFFSET + second_map[find_level_3]);
            return third_map[find_final];
        }        
    // }

}

int get_free_entry(inode_t * pre_ino){
    for (int i = 0;; i++){
        if(get_sector_id(pre_ino, i) == 0 && i!=0)
            return i;
    }
}

/* init fd */
void init_fd(){
    for (int i = 0; i < MAX_FILE_NUM; i++){
        openfile[i].used = 0;
    }
}

/* alloc_fd */
int alloc_fd(){
    int i;
    for ( i = 0; i < MAX_FILE_NUM; i++){
        if(openfile[i].used == 0){
            openfile[i].used = 1;
            return i;
        }
    }
    return -1;
}

uintptr_t load_elf_sd(                                  const int fd,
                                    unsigned length, uintptr_t pgdir,
    uintptr_t (*prepare_page_for_va)(uintptr_t va, uintptr_t pgdir))
{
    kmemset(load_buff, 0, NORMAL_PAGE_SIZE);
    do_fread(fd, load_buff, sizeof(Elf64_Ehdr));
    Elf64_Ehdr *ehdr = (Elf64_Ehdr *)kmalloc(sizeof(Elf64_Ehdr));
    kmemcpy(ehdr, load_buff, sizeof(Elf64_Ehdr));
    Elf64_Phdr *phdr = (Elf64_Phdr *)kmalloc(sizeof(Elf64_Phdr));//NULL;
    /* As a loader, we just care about segment,
     * so we just parse program headers.
     */
    Elf64_Off ptr_ph_table;
    Elf64_Half ph_entry_count;
    Elf64_Half ph_entry_size;
    int i = 0;

    // check whether `binary` is a ELF file.
    if (length < 4 || !is_elf_format(ehdr)) {
        return 0;  // return NULL when error!
    }

    ptr_ph_table   = ehdr->e_phoff;
    ph_entry_count = ehdr->e_phnum;
    ph_entry_size  = ehdr->e_phentsize;
    while (ph_entry_count--) {
        do_lseek(fd, ptr_ph_table, SEEK_SET);
        do_fread(fd, phdr, sizeof(Elf64_Phdr));
        if (phdr->p_type == PT_LOAD) {
            /* TODO: */
            for (i = 0; i < phdr->p_memsz; i += NORMAL_PAGE_SIZE) {
                if (i < phdr->p_filesz) {
                    unsigned char *bytes_of_page =
                        (unsigned char *)prepare_page_for_va(
                            (uintptr_t)(phdr->p_vaddr + i), pgdir);
                    do_lseek(fd, phdr->p_offset + i, SEEK_SET);
                    do_fread(fd, load_buff, MIN(phdr->p_filesz - i, NORMAL_PAGE_SIZE));
                    kmemcpy(
                        bytes_of_page,
                        load_buff,
                        MIN(phdr->p_filesz - i, NORMAL_PAGE_SIZE));
                    if (phdr->p_filesz - i < NORMAL_PAGE_SIZE) {
                        for (int j =
                                 phdr->p_filesz % NORMAL_PAGE_SIZE;
                             j < NORMAL_PAGE_SIZE; ++j) {
                            bytes_of_page[j] = 0;
                        }
                    }
                } else {
                    long *bytes_of_page =
                        (long *)prepare_page_for_va(
                            (uintptr_t)(phdr->p_vaddr + i), pgdir);
                    for (int j = 0;
                         j < NORMAL_PAGE_SIZE / sizeof(long);
                         ++j) {
                        bytes_of_page[j] = 0;
                    }
                }
            }
        }
        ptr_ph_table += ph_entry_size;
    }

    return ehdr->e_entry;
}

void re_load_elf_sd(
    const int fd, unsigned length, 
    uintptr_t pgdir_des, 
    uintptr_t pgdir_src,
    PTE * (*prepare_page_for_va)(uintptr_t va, uintptr_t pgdir, uint64_t kva, uint32_t mode)){
    kmemset(load_buff, 0, NORMAL_PAGE_SIZE);
    do_fread(fd, load_buff, sizeof(Elf64_Ehdr));
    Elf64_Ehdr *ehdr = (Elf64_Ehdr *)kmalloc(sizeof(Elf64_Ehdr));
    kmemcpy(ehdr, load_buff, sizeof(Elf64_Ehdr));
    Elf64_Phdr *phdr = (Elf64_Phdr *)kmalloc(sizeof(Elf64_Phdr));//NULL;
    /* As a loader, we just care about segment,
     * so we just parse program headers.
     */
    Elf64_Off ptr_ph_table;
    Elf64_Half ph_entry_count;
    Elf64_Half ph_entry_size;
    int i = 0;

    // check whether `binary` is a ELF file.
    if (length < 4 || !is_elf_format(ehdr)) {
        return 0;  // return NULL when error!
    }

    ptr_ph_table   = ehdr->e_phoff;
    ph_entry_count = ehdr->e_phnum;
    ph_entry_size  = ehdr->e_phentsize;

    while (ph_entry_count--) {
        do_lseek(fd, ptr_ph_table, SEEK_SET);
        do_fread(fd, phdr, sizeof(Elf64_Phdr));

        if (phdr->p_type == PT_LOAD) {
            /* TODO: */
            for (i = 0; i < phdr->p_memsz; i += NORMAL_PAGE_SIZE) {
                PTE *fk_pte_of_page = prepare_page_for_va(
                        (uintptr_t)(phdr->p_vaddr + i), pgdir_des , get_pfn_of(phdr->p_vaddr + i, pgdir_src),
                         MAP_USER);
                (*fk_pte_of_page) &= ~(_PAGE_WRITE);
                PTE *og_pte_of_page = get_PTE_of((uintptr_t)(phdr->p_vaddr + i), pgdir_src);
                (*og_pte_of_page) &= ~(_PAGE_WRITE);
            }
        }
        ptr_ph_table += ph_entry_size;
    }        
}