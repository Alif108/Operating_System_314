#include "types.h"
#include "stat.h"
#include "user.h"

/// ----------- this user function adds two number using a system call --------------- ///

// int char_to_int(const char* s)
// {
//   int n;

//   n = 0;
//   while('0' <= *s && *s <= '9')
//     n = n*10 + *s++ - '0';
//   return n;
// }

int main(int argc, char* argv[])
{
	// if(argc<2)
	// {
	// 	printf(1, "Please provide integer arguments\n");
	// 	exit();
	// }

	// int sum = 0;

	// for(int i=1; i<argc; i++)
	// {
	// 	sum = addi(sum, atoi(argv[i]));
	// }

	// printf(1, "Sum is: %d\n", sum);
	// exit();



	// ------------- summing up all the argument inputs via one system call ------------- //

	if(argc<2)
	{
		printf(1, "Please provide integer arguments\n");
		exit();
	}

	int* arr = (int*) malloc((argc-1) * sizeof(int));						// allocating an array to hold all the arguments

	for(int i=1; i<argc; i++)
		arr[i-1] = atoi(argv[i]);																	// storing the arguments in the array

	int sum = addi(argc-1, arr);																// ** system_call ** //		passing (int, int*) as argument to system_call

	printf(1, "%d\n", sum);

	free(arr);																									// deallocating

	exit();
}