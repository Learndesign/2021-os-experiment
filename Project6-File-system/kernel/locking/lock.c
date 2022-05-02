#include <os/lock.h>
#include <os/sched.h>
#include <os/mm.h>
#include <atomic.h>
#include <screen.h> 
#include <os/string.h>
/*初始化互斥锁队列*/
void do_mutex_lock_init(mutex_lock_t *lock)
{
    /* TODO */
    init_list(&lock->block_queue);//初始化互斥锁队列
    lock->lock.status = UNLOCKED;
}
/*锁的申请*/
void do_mutex_lock_acquire(mutex_lock_t *lock)
{
    /* TODO */
    current_running = get_current_cpu_id() == 0 ? current_running_master :current_running_slave; 
    if(lock->lock.status == LOCKED){//如果锁住则入队
        do_block(&current_running->list, &lock->block_queue);      
    }else{//否则上锁
        lock->lock.status = LOCKED;
        kernel_lock_t * use_lock = get_entry_from_list(lock,kernel_lock_t,kernel_lock);
        for (int i = 0; i < MAX_LOCK_NUM; i++){//用户判定持有这把锁
            if(current_running->handle_mutex[i] == -1){
                current_running->handle_mutex[i] = use_lock->id;
                break;
            }        
        }       
        use_lock->status = USED;
    }
}

/*锁的释放*/
void do_mutex_lock_release(mutex_lock_t *lock)
{
    /* TODO */
    int i;
    current_running = get_current_cpu_id() == 0 ? current_running_master :current_running_slave; 
    kernel_lock_t * use_lock = get_entry_from_list(lock,kernel_lock_t,kernel_lock);
    for (i = 0; i < MAX_LOCK_NUM; i++){
        if(use_lock->id == current_running->handle_mutex[i]){
            current_running->handle_mutex[i] = -1;//标记该进程没有持有这把锁。
            break;
        }         
    }
    if(i >= MAX_LOCK_NUM){
        // screen_move_cursor(1,4);
        prints("> [Error] the pracess() dosen't handle this mutex!\n"/*, current_running->pid*/);
        return ;
    }
    if(!Is_empty_list(&lock->block_queue)){
        pcb_t * unblock = get_entry_from_list(lock->block_queue.prev, pcb_t, list);
        do_unblock(lock->block_queue.prev);//释放一个锁进程        
        for (int i = 0; i < MAX_LOCK_NUM; i++){
            if(unblock->handle_mutex[i]==-1){
                unblock->handle_mutex[i] = use_lock->id;//该进程持有锁
                break;
            }
        }
    }
    else{
        lock->lock.status=UNLOCKED;
        use_lock->status = UNSED;
    }
}

/*所有的锁都进行初始化*/
void init_lock_array(){
    for (int i = 0; i < MAX_LOCK_NUM; i++){ 
        /*初始化锁*/       
        User_Lock[i].status = UNSED;
        User_Lock[i].id = i;
        User_Lock[i].key = -1;
        /*初始化信号量*/
        User_Sem_Wait[i].status = UNSED;
        /*初始化屏障*/
        User_Barrier_Wait[i].status =UNSED;
        /*初始化信箱*/
        User_Mailbox[i].visited = -1;
        User_Mailbox[i].name = 0;
    }
}

/*返回一个锁，遍历，找到一个没有用过的锁，将这个数值返回给用户*/
int do_mutex_init(int * mutex_id, int key){
    int i = 0;
    for (i = 0; i < MAX_LOCK_NUM; i++){
        if(User_Lock[i].key == key)
            return i;
    }
    for (i = 0; i < MAX_LOCK_NUM; i++){
        if(User_Lock[i].kernel_lock.lock.status != LOCKED && User_Lock[i].status != USED){
            User_Lock[i].status = USED;
            User_Lock[i].key = key;
            do_mutex_lock_init(&User_Lock[i].kernel_lock);
            *mutex_id = i;
            return i;
        }
    }
    if(i>= MAX_LOCK_NUM){
        // screen_move_cursor(1,4);
        prints("> [Error] : not find one free lock\n");
        return -1;
        while (1) ;         
    }     
}
/* op==1 上锁
 * op==0 解锁
*/
void do_mutex_op(int mutex_id, int op){
    if(op == USER_LOCK)
        //上锁
        do_mutex_lock_acquire(&User_Lock[mutex_id].kernel_lock);
    else
        //解锁
        do_mutex_lock_release(&User_Lock[mutex_id].kernel_lock);
}

/*信号量操作*/
void do_semphore_init(int * id, int * value, int val){
    /*初始化信号量即为直接初始化锁结构就行*/
    int i;
    for(i = 0; i<MAX_WAIT_QUEUE; i++){
        if(User_Sem_Wait[i].status == UNSED){
            User_Sem_Wait[i].status = USED;
            User_Sem_Wait[i].wt_now = 0;
            User_Sem_Wait[i].wt_num = val;
            init_list(&User_Sem_Wait[i].wait_queue);
            break;
        }
    }
    if(i>MAX_WAIT_QUEUE){
        // screen_move_cursor(1,4);
        prints("> [Error] no more resoure!\n");
        while (1) ; 
    }
    (*id) = i;
    (*value) =val;
}

void do_semphore_up(int mutex_id, int * semphore){
    if(mutex_id == -1){
        // screen_move_cursor(1,4);
        prints("> [Error]: no this semphore, it has been destory\n");
        return ;
    }
    list_node_t * wt = &User_Sem_Wait[mutex_id].wait_queue;
    if(!Is_empty_list(wt)){
        do_unblock(wt->prev);
    }
    (*semphore)++;
}

void do_semphore_down(int mutex_id,int * semphore){
    current_running = get_current_cpu_id() == 0 ? current_running_master :current_running_slave; 
    if(mutex_id == -1){
        // screen_move_cursor(1,4);
        prints("> [Error]: no this semphore, it has been destory\n");
        return ;
    }
    (*semphore)--;
    if((*semphore) < 0){
        do_block(&current_running->list,&User_Sem_Wait[mutex_id].wait_queue);
    }
}

void do_semphore_destory(int * mutex_id,int * semphore){
    kernel_semwait_t des_sem = User_Sem_Wait[*mutex_id];
    /*删除该信号量上的所有的阻塞进程*/
    list_node_t * des_list = &User_Sem_Wait[*mutex_id].wait_queue;
    while(!Is_empty_list(des_list)){
        /*重复的释放锁*/
        do_unblock(des_list->prev);
    }        
    (*mutex_id) = -1;
    (*semphore) = 0;
    des_sem.status = UNSED;
}
/*屏障*/
void do_barrier_init(int * barrier_id, int * wt_now, int * wt_num, unsigned count){
    int i;
    for (i = 0; i < MAX_BARRIER_NUM; i++){
        if(User_Barrier_Wait[i].status == UNSED){
            init_list(&User_Barrier_Wait[i].wait_queue);
            *barrier_id = i;
            *wt_now = 0;
            *wt_num = count;
            break;
        }
    }
    if(i>MAX_BARRIER_NUM){
        // screen_move_cursor(1,4);
        prints("> [Error]: no more resoure!\n");
        while (1) ; 
    }
}
void do_barrier_wait(int barrier_id, int * wt_now, int wt_num){
    current_running = get_current_cpu_id() == 0 ? current_running_master :current_running_slave; 
    if(barrier_id == -1){
        // screen_move_cursor(1,4);
        prints("> [Error]: no this Barrier, it has been deleted!\n");
        return;
    }
    if((++*wt_now) == wt_num){
        while(!Is_empty_list(&User_Barrier_Wait[barrier_id].wait_queue)){
            do_unblock(User_Barrier_Wait[barrier_id].wait_queue.prev);
        }
        *wt_now = 0;
    }else{
        do_block(&current_running->list, &User_Barrier_Wait[barrier_id].wait_queue);
    }
}

void do_barrier_destory(int * barrier_id, int * wt_now, int * wt_num){
    User_Barrier_Wait[*barrier_id].status = UNSED;
    *barrier_id = -1;
    *wt_now = 0;
    *wt_num = 0;
}


/*mailbox通信*/
void do_mbox_open(int *mailbox_id, char * name){
    int i= 0;
    for ( i = 0; i < MAX_MAILBOX_NUM; i++){
        if (User_Mailbox[i].visited!=-1 && kstrcmp(User_Mailbox[i].name,name)==0){
            User_Mailbox[i].visited ++;
            *mailbox_id = i;
            return;
        }       
    }
    for ( i = 0; i < MAX_MAILBOX_NUM; i++){
        if(User_Mailbox[i].visited == -1){
            User_Mailbox[i].head = 0;
            User_Mailbox[i].tail = 0;
            User_Mailbox[i].msg_num = 0;
            User_Mailbox[i].status = MBOX_OPEN;
            User_Mailbox[i].visited = 1; 
            /*初始化msg*/
            int mutex_id;
            do_mutex_init(&mutex_id, 5);
            User_Mailbox[i].mbox_mutex = &User_Lock[mutex_id].kernel_lock;  
            init_list(&User_Mailbox[i].empty);
            init_list(&User_Mailbox[i].full);
            int j = 0;
            User_Mailbox[i].name = (char*)kmalloc(sizeof(char)*kstrlen(name));
            while(*name){
                User_Mailbox[i].name[j++]=*name;
                name++;
            }
            User_Mailbox[i].name[j]='\0';
            *mailbox_id = i;
            return;
        }
    }
    prints(">[Error]: falied to get one mailbox");
    return;
}
void do_mbox_close(int mailbox_id){
    User_Mailbox[mailbox_id].status = MBOX_CLOSE;
    User_Mailbox[mailbox_id].visited--;
    if(User_Mailbox[mailbox_id].visited == 0){
        User_Mailbox[mailbox_id].name = 0;
        kmemset(User_Mailbox[mailbox_id].msg, 0, MAX_MSG_LENGTH);
        User_Mailbox[mailbox_id].visited = -1;
        User_Mailbox[mailbox_id].head = 0;
        User_Mailbox[mailbox_id].tail = 0;
        User_Mailbox[mailbox_id].msg_num = 0;
        User_Mailbox[mailbox_id].mbox_mutex->lock.status = UNLOCKED;
        get_entry_from_list(User_Mailbox[mailbox_id].mbox_mutex, kernel_lock_t, kernel_lock)->status = UNSED;
        while (!Is_empty_list(&User_Mailbox[mailbox_id].mbox_mutex->block_queue)){
            do_unblock(User_Mailbox[mailbox_id].mbox_mutex->block_queue.prev);
        }
        while (!Is_empty_list(&User_Mailbox[mailbox_id].empty)){
            do_unblock(User_Mailbox[mailbox_id].empty.prev);
        }
        while (!Is_empty_list(&User_Mailbox[mailbox_id].full)){
            do_unblock(User_Mailbox[mailbox_id].full.prev);
        }
    }
}


int do_mbox_send(int mailbox_id, void *msg, int msg_length){
    current_running = get_current_cpu_id() == 0 ? current_running_master :current_running_slave; 
    int send_block_time = 0;
    /*互斥访问*/
    // do_mutex_lock_acquire(User_Mailbox[mailbox_id].mbox_mutex);
    while (User_Mailbox[mailbox_id].msg_num + msg_length > MAX_MSG_LENGTH){
        //暂时释放锁
        // do_mutex_lock_release(User_Mailbox[mailbox_id].mbox_mutex);   
        //将发送进程阻塞到full队列上
        send_block_time ++;
        do_block(&current_running->list, &User_Mailbox[mailbox_id].full);
        //重新申请锁
        // do_mutex_lock_acquire(User_Mailbox[mailbox_id].mbox_mutex);
    }
    
    //put msg in mailbox
    int i;
    User_Mailbox[mailbox_id].msg_num += msg_length;
    for ( i = 0; i < msg_length ; i++){
        User_Mailbox[mailbox_id].msg[(User_Mailbox[mailbox_id].head++)%MAX_MSG_LENGTH] = ((char *)msg)[i];
    } 
    User_Mailbox[mailbox_id].head = (User_Mailbox[mailbox_id].head) % MAX_MSG_LENGTH;

    while (!Is_empty_list(&User_Mailbox[mailbox_id].empty)){
        //唤醒所有的接收进程
        do_unblock(User_Mailbox[mailbox_id].empty.prev);   
    }
    // do_mutex_lock_release(User_Mailbox[mailbox_id].mbox_mutex);
    return send_block_time;
}


int do_mbox_recv(int mailbox_id, void *msg, int msg_length){
    current_running = get_current_cpu_id() == 0 ? current_running_master :current_running_slave; 
    int recv_block_time = 0;
    /*互斥访问*/
    // do_mutex_lock_acquire(User_Mailbox[mailbox_id].mbox_mutex);
    while (User_Mailbox[mailbox_id].msg_num < msg_length){
        //暂时释放锁
        // do_mutex_lock_release(User_Mailbox[mailbox_id].mbox_mutex);   
        //将接收进程阻塞到empty队列上
        recv_block_time ++;
        do_block(&current_running->list, &User_Mailbox[mailbox_id].empty);
        //重新申请锁
        // do_mutex_lock_acquire(User_Mailbox[mailbox_id].mbox_mutex);
    }
    
    int i;
    User_Mailbox[mailbox_id].msg_num -= msg_length;
    for(i = 0; i < msg_length; i++){
        ((char*)msg)[i] = User_Mailbox[mailbox_id].msg[(User_Mailbox[mailbox_id].tail++)%MAX_MSG_LENGTH];
    }
    User_Mailbox[mailbox_id].tail = User_Mailbox[mailbox_id].tail%MAX_MSG_LENGTH;

    while (!Is_empty_list(&User_Mailbox[mailbox_id].full)){
        //唤醒所有的发送进程
        do_unblock(User_Mailbox[mailbox_id].full.prev);   
    }
    // do_mutex_lock_release(User_Mailbox[mailbox_id].mbox_mutex);
    return recv_block_time;
}

int do_mbox_send_recv(int *mailbox_id, void *send_msg, void * recv_msg, int *length){
    current_running = get_current_cpu_id() == 0 ? current_running_master :current_running_slave; 
    int type = 0;
    int block_time = 0;    
    /*互斥访问*/
    int send_length = length[0];
    int recv_length = length[1];

    int send_mailbox_id = mailbox_id[0];
    int recv_mailbox_id = mailbox_id[1];

    // do_mutex_lock_acquire(User_Mailbox[mailbox_id].mbox_mutex);
    while (User_Mailbox[recv_mailbox_id].msg_num < recv_length && User_Mailbox[send_mailbox_id].msg_num + send_length > MAX_MSG_LENGTH){
        //暂时释放锁
        // do_mutex_lock_release(User_Mailbox[mailbox_id].mbox_mutex);   
        //将接收进程阻塞到empty队列上
        block_time ++;
        do_block(&current_running->list, &User_Mailbox[recv_mailbox_id].empty);
        //重新申请锁
        // do_mutex_lock_acquire(User_Mailbox[mailbox_id].mbox_mutex);
    }
    if(User_Mailbox[recv_mailbox_id].msg_num >= recv_length){//recv
        int i;
        type = 0x1;
        User_Mailbox[recv_mailbox_id].msg_num -= recv_length;
        for(i = 0; i < recv_length; i++){
            ((char*)recv_msg)[i] = User_Mailbox[recv_mailbox_id].msg[(User_Mailbox[recv_mailbox_id].tail++)%MAX_MSG_LENGTH];
        }
        User_Mailbox[recv_mailbox_id].tail = User_Mailbox[recv_mailbox_id].tail%MAX_MSG_LENGTH;        
    }
    if(User_Mailbox[send_mailbox_id].msg_num + send_length <= MAX_MSG_LENGTH){//send
        if(type == 0x1)
            type = 0x3;
        else if(type = 0)
            type = 0x2;
        int i;
        User_Mailbox[send_mailbox_id].msg_num += send_length;
        for ( i = 0; i < send_length ; i++){
            User_Mailbox[send_mailbox_id].msg[(User_Mailbox[send_mailbox_id].head++)%MAX_MSG_LENGTH] = ((char *)send_msg)[i];
        } 
        User_Mailbox[send_mailbox_id].head = (User_Mailbox[send_mailbox_id].head) % MAX_MSG_LENGTH;
    }
    while (!Is_empty_list(&User_Mailbox[recv_mailbox_id].empty)){
        //唤醒所有的发送进程
        do_unblock(User_Mailbox[recv_mailbox_id].empty.prev);   
    }
    // do_mutex_lock_release(User_Mailbox[mailbox_id].mbox_mutex);
    return type;//返回1为send，返回0为recv
}
