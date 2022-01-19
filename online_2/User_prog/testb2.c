#include "types.h"
#include "stat.h"
#include "user.h"

int get_sum(int id)
{
	int temp = id;
	int sum = 0;

	while(temp>0)
	{
		sum += temp%10;
		temp = temp/10;
	}
	return sum;
}

int main(void)
{
	int id = papachari();
	int random_number = atta();
	
	int sum = get_sum(id);

	printf(1, "id: %d\n", id);
	printf(1, "sum: %d\n", sum);
	printf(1, "random number: %d\n", random_number);
	printf(1, "remainder: %d\n", sum%random_number);

	exit();
}