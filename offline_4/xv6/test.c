#include "types.h"
#include "stat.h"
#include "user.h"

int main(int argc, char* argv[])
{
	printf(1, "Increase Memory\n");

	for(int i=0; i<20; i++)
	{
		sbrk(4096);
		sleep(5);
	}
	exit();
}

