#include "types.h"
#include "stat.h"
#include "user.h"

int main()
{
	printf(1, "before pid: %d\n", getpid());

	fork();

	printf(1, "pid: %d\n", getpid());

	exit();
}