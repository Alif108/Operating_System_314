#include "types.h"
#include "stat.h"
#include "user.h"

// creates a new process by forking
// arguments: ticktets(optional) -> set the tickets of this process; default value = 1
int main(int argc, char* argv[])
{
	int fret = fork();

	if(fret>0)
	{
		printf(1, "Process with id %d created\n", fret);		// parent process exiting
		exit();
	}

	while(1);

	exit();
}

