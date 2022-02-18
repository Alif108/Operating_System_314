#include "types.h"
#include "stat.h"
#include "user.h"

// this function takes 2 args from user
// "pid" -> process id
// "tickets" -> number of tickets to assign
// Assigns number of tickets (priority) to a process

// a process with higher tickets has higher chance of getting allocated

int main(int argc, char* argv[])
{
	if(argc != 3)
	{
		printf(1, "Please provide pid and no_of_tickets\n");
		exit();
	}

	int pid = atoi(argv[1]);
	int tickets = atoi(argv[2]);

	int ret_value = settickets(pid, tickets);

	if(ret_value == -1)
	{
		printf(1, "Set tickets failed\n");
		exit();
	}
	else if(ret_value == -2)
	{
		printf(1, "No process with id %d found\n", pid);
		exit();
	}

	printf(1, "%d tickets given to process %d\n", tickets, pid);
	exit();
}