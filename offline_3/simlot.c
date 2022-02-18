#include "types.h"
#include "stat.h"
#include "user.h"
#include "pstat.h"
#include "param.h"


// this function simulates "lottery scheduler"
// args: "time" -> unit of time of simulation
// shows process cpu usage status in the given time
int main(int argc, char* argv[])
{
	if(argc!=2)
	{
		printf(1, "Please provide simulation time\n");
		exit();
	}

	int simulation_time = atoi(argv[1]);

	struct pstat stat_before;
	struct pstat stat_after;

	if(getpinfo(&stat_before)==-1)
	{
		printf(1, "Get Statistics Failed\n");
		exit();
	}

	printf(1, "\nSimulation running. Please don't create any new process\n\n");

	sleep(simulation_time * 1000);

	if(getpinfo(&stat_after)==-1)
	{
		printf(1, "Get Statistics Failed\n");
		exit();
	}

	int total_ticks = 0;
	int total_tickets = 0;

	for(int i=0; i<NPROC; i++)
	{
		if(stat_after.inuse[i])
		{
			total_ticks += stat_after.ticks[i] - stat_before.ticks[i];
			total_tickets += stat_after.tickets[i];
		}
	}

	printf(1, "pid \t ticks_before \t ticks_after \t ticks_diff \t ticks_percentage \t tickets \t ticket_percentage\n");
	for(int i=0; i<NPROC; i++)
	{
		if(stat_after.inuse[i])
		{
			printf(1, "%d \t\t %d \t\t %d \t\t %d \t\t %d \t\t %d \t\t %d\n", stat_after.pid[i], stat_before.ticks[i], stat_after.ticks[i], stat_after.ticks[i] - stat_before.ticks[i], (stat_after.ticks[i] - stat_before.ticks[i])*100/total_ticks, stat_after.tickets[i], stat_after.tickets[i]*100/total_tickets);
		}
	}

	exit();
}