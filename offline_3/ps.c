#include "types.h"
#include "stat.h"
#include "user.h"
#include "pstat.h"
#include "param.h"

// shows statistics of the current running processes
int main()
{
	struct pstat stat;

	if(getpinfo(&stat)==-1)
	{
		printf(1, "Get Statistics Failed\n");
		exit();
	}

	printf(1, "pid \t ticks \t tickets \n");
	
	for(int i=0; i<NPROC; i++)
	{
		if(stat.inuse[i])
			printf(1, "%d \t %d \t %d \n", stat.pid[i], stat.ticks[i], stat.tickets[i]);
	}
	exit();
}