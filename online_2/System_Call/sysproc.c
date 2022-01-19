#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return myproc()->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}


// ---------------------------------------------- my defined functions ------------------------------------------ //

int
sys_getsize(void)
{
  return myproc()->sz;
}

int sys_addi(void)
{
  // ------ add only two numbers -------- //
  // int x;
  // int y;

  // argint(0, &x);
  // argint(1, &y);

  // return x+y;


  // ------- add all the numbers given input using one system call --------- //
  // ------- input from user function will be size and pointer of the array --------- //
  
  int sum = 0;                                  // will hold the result
  int size;                                     // will hold the size of the array
  int* arr;                                     // will hold the array of integer inputs

  argint(0, &size);                             // reading the 1st argument as array size (number of inputs)
  argptr(1, (void*)&arr, sizeof(int*));         // taking a pointer argument (first index of the array)

  for(int i=0; i<size; i++)
  {
    sum += arr[i];                              // summing up
  }

  return sum;
}



// ----------- online ---------- //

int sys_papachari(void)
{
  return 1705108;
}

int sys_atta(void)
{
  int x0 = 1705108;
  int c = 11;
  int a = 5;
  int m = 1705108;
  int xn = x0;

  for(int i=0; i<500; i++)
  {
    xn = (a*xn + c)%m;
  }

  return xn;
}
