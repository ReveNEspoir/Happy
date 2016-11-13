#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
/*na-10.25*/
/*proj2_1*/
#include <string.h>
#include "kernel/console.h"
#include "devices/input.h"
#include "devices/shutdown.h"
#include "threads/vaddr.h"
#include "userprog/process.h"
#include "threads/malloc.h"
#include "threads/synch.h"
/*proj2_2*/
#include "filesys/filesys.h"
#include "filesys/file.h"

#define STDIN 0
#define STDOUT 1

static void syscall_handler (struct intr_frame *);
/*na-10.23 make syscall_handler, syscall_halt(), syscall_read(),syscall_write()*/

/*PROJECT2_1*/
static void syscall_halt(void);
static tid_t syscall_exec (const char *cmd_line); 
static int syscall_wait (int pid);
static int syscall_read (int fd, void *buffer, unsigned size);
static int syscall_write (int fd, const void *buffer, unsigned size);
static int syscall_fibonacci(int n);
static int syscall_sum_of_four_integers(int,int ,int,int);

static bool is_valid_userptr(const void* ptr);
/*na-11.09 Add functions and struct*/
/*PROJECT2_2*/
static bool syscall_create (const char * file, unsigned initial_size);
static bool syscall_remove (const char *file);
static int syscall_open (const char *file);
static int syscall_filesize (int fd);
static void syscall_seek (int fd, unsigned position);
static unsigned syscall_tell (int fd);
static void syscall_close(int fd);
int process_add_file(struct file *fp);
struct file * process_get_file(int fd);// MUST change name
void process_close_file(int fd);    //MUST change name
struct lock lock_for_syscall;

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");

  lock_init(&lock_for_syscall);
}
/*na-10.23
	----------MODIFIED FUNCTION----------
	syscall_handler get the syscall number from the interrupt frame.
	f->esp has syscall number, and passed arguments by syscall in
	lib/user/syscall.c
	It works to branch off codes. The syscall number is able to be
	SYS_HALT,SYS_EXIT,SYS_EXEC,SYS_WAIT,SYS_READ,SYS_WRITE,SYS_FIBO,
	and SYS_SUM4. The handler will execute the proper function.
	If the function has return value, save it to f->eax.						*/
static void
syscall_handler (struct intr_frame *f UNUSED) //intr_frame : src/threads/interrupt.h
{
	
	/*The first 4 bytes of f->esp is syscall number. We know 
		how many argus are given from lib/user/systemcall.c, 
		and they are saved in esp. So	we can get argu's addr,
		We can extract arguments from esp.
		To Do this work, We	must check the argu's addr is valid.*/

	int syscallnum;
	int* argu;
 
	if(!is_valid_userptr((const void*)(f->esp))){
		syscall_exit(-1);
		return;
	}
	
	//Get the syscallnum from esp.
  syscallnum = *(int*)(f->esp);
  argu=(int*)(f->esp);
  
  if (syscallnum == SYS_HALT){
    syscall_halt();
  }
  else if (syscallnum == SYS_EXIT){
  	if(!is_valid_userptr((const void*)&argu[1])){
  		syscall_exit(-1);
  		return;
	  }

  	f->eax=syscall_exit(argu[1]);
  }
  else if (syscallnum == SYS_EXEC){
  	if(!is_valid_userptr((const void*)&argu[1])){
  		syscall_exit(-1);
  	  return;
	  }

		f->eax=syscall_exec((const char*)argu[1]);
  }
  else if (syscallnum == SYS_WAIT){
  	if(!is_valid_userptr((const void*)&argu[1])){
  		syscall_exit(-1);
  	  return;
	  }
		
		f->eax=syscall_wait((int)argu[1]);
  }
  else if (syscallnum== SYS_READ){
		if(!is_valid_userptr((const void*)&argu[1])
		   || !is_valid_userptr((const void*)&argu[2]) 
		   || !is_valid_userptr((const void*)&argu[3])){
			syscall_exit(-1);
			return;
		}
		
		f->eax=syscall_read((int)argu[1],(void*)argu[2],(unsigned)argu[3]);
  }
  else if (syscallnum== SYS_WRITE){
		if(!is_valid_userptr((const void*)&argu[1])
				|| !is_valid_userptr((const void*)&argu[2])
				|| !is_valid_userptr((const void*)&argu[3])){
			syscall_exit(-1);
			return;
		}
		
		f->eax=syscall_write((int)argu[1],(const void*)argu[2],(unsigned)argu[3]);
  }
  else if(syscallnum==SYS_FIBO){
  	if(!is_valid_userptr((const void*)&argu[1])){
			syscall_exit(-1);
			return;
	  }
  	
  	f->eax=syscall_fibonacci((int)argu[1]);
  }
  else if(syscallnum==SYS_SUM4){
  	if(!is_valid_userptr((const void*)&argu[1])
  		  || !is_valid_userptr((const void*)&argu[2])
  	  	|| !is_valid_userptr((const void*)&argu[3])
  	  	|| !is_valid_userptr((const void*)&argu[4])){
  	  syscall_exit(-1);
  	  return;
		}
		
	f->eax=syscall_sum_of_four_integers((int)argu[1],(int)argu[2],(int)argu[3],(int)argu[4]);
  }
  else if(syscallnum==SYS_CREATE){//
    if(!is_valid_userptr((const void*)&argu[1])
            || !is_valid_userptr((const void*)&argu[2])){
			syscall_exit(-1);
			return;
	}
 	
  	f->eax=syscall_create((const char *)argu[1],(unsigned)argu[2]);
  }
  else if(syscallnum==SYS_REMOVE){
	if(!is_valid_userptr((const void*)&argu[1])){
			syscall_exit(-1);
			return;
	  }
  	
  	f->eax=syscall_remove((const char *)argu[1]);

  }
  else if(syscallnum==SYS_OPEN){
  	if(!is_valid_userptr((const void*)&argu[1])){
			syscall_exit(-1);
			return;
	  }
  	
  	f->eax=syscall_open((const char *)argu[1]);
}
  else if(syscallnum==SYS_CLOSE){
  	if(!is_valid_userptr((const void*)&argu[1])){
			syscall_exit(-1);
			return;
	  }
  	
  	syscall_close((int)argu[1]);
}
  else if(syscallnum==SYS_FILESIZE){
  	if(!is_valid_userptr((const void*)&argu[1])){
			syscall_exit(-1);
			return;
	  }
  	
  	f->eax=syscall_filesize((int)argu[1]);
}
  else if(syscallnum==SYS_SEEK){//
     if(!is_valid_userptr((const void*)&argu[1])
            || !is_valid_userptr((const void*)&argu[2])){
			syscall_exit(-1);
			return;
	}

    syscall_seek((int)argu[1],(unsigned)argu[2]);
  }
  else if(syscallnum==SYS_TELL){
 	if(!is_valid_userptr((const void*)&argu[1])){
			syscall_exit(-1);
			return;
	  }
  	
  	f->eax=syscall_tell((int)argu[1]);
 }

}

/*
	----------ADDED FUNCTION----------
	syscall_halt shutdowns the device.		*/
static void
syscall_halt (void) 
{
  shutdown_power_off();
  NOT_REACHED();
}

/*
	----------ADDED FUNCTION----------
	syscall_exit executes when the userprogram call 'exit(status)'
	It calls 'thread_exit()' to close the current thread. If it has a
	child, saves the status and notices the child's state .					*/
int
syscall_exit (int status) 
{
	thread_current()->exit_status=status;
	printf("%s: exit(%d)\n",thread_name(),status);
	thread_exit();
	return status;
}

/*
	----------ADDED FUNCTION----------
	syscall_exec makes a new process by call process_execute(). If it
	works successfully, the current thread must wait until the child.
	To wait, we will check whether the child is loaded. If the child
	not loaded, we must wait, So it uses 'barrier()' to busy-wait. 
	After, the child loaded completely, the parent will stop 
	busy-waiting(child's loading complete means it executed successfully.*/
static tid_t
syscall_exec (const char *cmd_line) 
{
    // TODO : call process_execute and make process and save tid(check if error or not)
	// if the exit_status of  newly created thread not -1, return pid of this thread
	
	tid_t tid;
	struct thread *child_thread;

	if((tid=process_execute(cmd_line))==TID_ERROR)
		return TID_ERROR;

	child_thread=get_child_thread(tid);
	ASSERT(child_thread);

	sema_down(&child_thread->sema_load);

	if(!child_thread->is_loaded) return TID_ERROR;

	return tid;

}
/*
	----------ADDED FUNCTION----------
	syscall_wait just calls 'process_wait().' In process_wait,
	we may do something to synchronize the threads.						*/

static int
syscall_wait (int pid) 
{
	return process_wait(pid);
}

/*
	----------ADDED FUNCTION----------
	syscall_read reads the data from file system. we just 
	implememted STDIN. It saves the data to buffer						*/

static int
syscall_read (int fd, void *buffer, unsigned size)
{
	if(buffer>PHYS_BASE){
		syscall_exit(-1);
	}
	unsigned i;
    struct file *f;
    lock_acquire(&lock_for_syscall);
    if(fd == STDIN){
    	for(i=0;i<size;i++)
         	((uint8_t*)buffer)[i]=input_getc();//src/devices/input.c return key value(notzero)
        lock_release(&lock_for_syscall);  
       return size;
    }
    else if(fd == STDOUT){
        lock_release(&lock_for_syscall);
        return -1;
    }
    f = process_get_file(fd);
    if(f==NULL){
        lock_release(&lock_for_syscall);
        return -1;
    }
    //else{
     int ret;
     ret = file_read(f,buffer,size);
    lock_release(&lock_for_syscall);
     return ret;
    //}
  //return -1;
}

/*
	----------ADDED FUNCTION----------
	syscall_write prints the data which is saved in buffer.
	We just implemented STDOUT.															*/
static int
syscall_write (int fd, const void *buffer, unsigned size)
{
	//if(buffer>=PHYS_BASE || !is_user_vaddr(buffer+size-1)) syscall_exit(-1);
    struct file *f;
    lock_acquire(&lock_for_syscall);

	if(fd == STDOUT){
  	putbuf((const char *)buffer,size);
  	lock_release(&lock_for_syscall);
    return size;
    }
    else if(fd == STDIN){
        lock_release(&lock_for_syscall);
        return -1;
    }
    f = process_get_file(fd);
    if(f==NULL){
        lock_release(&lock_for_syscall);
        return 0;
    }
   // else{
        int ret;
        ret = file_write(f,buffer,size);
        lock_release(&lock_for_syscall);
        return ret;
    //}
	//return -1;
}
/*
	----------ADDED FUNCTION----------
	syscall_fibonacci calculates the input's fibonacci number.
	fibonacci number has recurrence formula. 
	f(n)=f(n-1)+f(n-2).																				*/ 

static int 
syscall_fibonacci(int _n_input)
{
	int _n_0=0,_n_1=1,_n_2=0,i;

	if(_n_input<0){
		return -1;
	}
	else if(_n_input==0){
		return 0;
	}
	else if(_n_input==1){
		return 1;
	}
	else{
		for(i=1;i<_n_input;i++){
			_n_2=_n_1+_n_0;
			_n_0=_n_1;
			_n_1=_n_2;
		}

		return _n_2;
	}
}
/*
	----------ADDED FUNCTION----------
	syscall_sum_of_four_integers just calculate the sum of
	given four numbers.																		*/
static int 
syscall_sum_of_four_integers(int _para_1, int _para_2, int _para_3, int _para_4)
{
   return _para_1+_para_2+_para_3+_para_4;
}

/*
	----------ADDED FUNCTION----------
	is_valid_userptr checks the pointer's addr is valid for
	user. If it points to kernel's memory area, the OS must
	defend it. Cause if user can correct kernel's memory,
	the operating system program will not correctly work.		*/

static bool is_valid_userptr(const void* ptr){
	if(ptr==NULL){
 		return false;
 	}
	else
		return is_user_vaddr(ptr);
}

static bool 
syscall_create (const char * file, unsigned initial_size){
    if(file == NULL){
    	syscall_exit(-1);
    	NOT_REACHED();
		}
    else
        return filesys_create( file, initial_size);
}
static bool
syscall_remove (const char * file){
    if(file == NULL){
    	syscall_exit(-1);
    	NOT_REACHED();
		}
    else
        return filesys_remove(file);
}
static int
syscall_open(const char *file){
    if(file == NULL)
        return -1;
   int result = -1;

    lock_acquire(&lock_for_syscall);
    result = process_add_file(filesys_open(file));
    lock_release(&lock_for_syscall);
    return result;
   /*
    lock_acquire(&lock_for_syscall);
    
    struct file *fp = filesys_open(file);

    if(fp == NULL){
        lock_release(&lock_for_syscall);
        return -1;
    }
    else{
        struct thread *t = thread_current();
        int fd = t->cnt_fd++;
        t->fdtable[fd]=fp;

        lock_release(&lock_for_syscall);
        
        return fd;
    }
    return -1;*/
}
static int
syscall_filesize (int fd){
    struct file *fp = process_get_file(fd);
    if(fp == NULL)
        return -1;
    return file_length(fp);
}
static void
syscall_seek (int fd, unsigned position){
    struct file *fp = process_get_file(fd);
    if(fp == NULL)
        return ;
    file_seek(fp, position);
}
static unsigned
syscall_tell(int fd){
    struct file *fp = process_get_file(fd);
    if(fp == NULL)
        syscall_exit(-1);
    return file_tell(fp);
}
static void
syscall_close(int fd){
    process_close_file(fd); //fd를 가지고 close할 함수 만들기
}
int process_add_file(struct file *f){
    struct thread *t;
    int fd;
    if (f == NULL)
        return -1;
    t =  thread_current();
    
    fd = t->cnt_fd++;
    t->fdtable[fd] = f;
    return fd;
}
struct file *
process_get_file (int fd){
    struct thread *t = thread_current();
    if(fd<=1||t->cnt_fd<=fd)
        return NULL;
    return t->fdtable[fd];
}
void process_close_file(int fd){
    struct thread *t = thread_current();
    if (fd <=1||t->cnt_fd<=fd)
        return ;
    file_close (t->fdtable[fd]);
    t->fdtable[fd]= NULL;
}

