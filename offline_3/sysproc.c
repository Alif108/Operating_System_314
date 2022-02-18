#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "pstat.h"

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


// ---------------------------------------------------- custom system calls ------------------------------------------ //


// gets all the pstat infos of all the INUSE processes
int sys_getpinfo()
{
  struct pstat* temp;
  struct proc* p;
  int index;

  argptr(0, (void*)&temp, sizeof(*temp));                     // the first argument (pstat pointer) is stored in temp

  if(temp == 0)                                               // if passed pointer is null
    return -1;

  acquire(&ptable.lock);                                      // ptable is shared resource, so locking it before accessing
  for(p=ptable.proc;p != &(ptable.proc[NPROC]); p++)          // populating each process with its info
  {
    index = p - ptable.proc;                                  

    if(p->state != UNUSED)
    {
      temp->pid[index] = p->pid;
      temp->ticks[index] = p->ticks;
      temp->tickets[index] = p->tickets;
      temp->inuse[index] = (p->state != UNUSED);
    }
  }
  release(&ptable.lock);                                      // releasing the ptable lock
  return 0;
}

// sets tickets(priority) for a process
int sys_settickets(void)
{
  int pid;
  int tickets;
  struct proc* p;
  int found = 0;

  argint(0, &pid);                                          // first argument: pid
  argint(1, &tickets);                                      // second argument: number of tickets

  if(pid < 0)
    return -1;

  if(tickets < 0)
    return -1;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
  {
    if(p->pid == pid)
    {
      found = 1;
      p->tickets = tickets;
      break;
    }
  }
  release(&ptable.lock);

  if(!found)
    return -2;

  return 0;
}