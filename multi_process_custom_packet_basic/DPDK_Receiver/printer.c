#include <stdio.h>

#define INC 100

/*
	Increment a counter and print it every second.
*/
int main()
{

	const char clr[] = { 27, '[', '2', 'J', '\0' };
	const char topLeft[] = { 27, '[', '1', ';', '1', 'H','\0' };
	int counter = 0;

	for(;;)
	{
		/* Clear screen and move to top left */
		printf("%s%s", clr, topLeft);

		printf("\nPort statistics ====================================");
		printf("\nPackets received: %d", counter);
		printf("\nAggregate statistics ===============================");
		printf("\nTotal packets received: %d", counter);
		printf("\n====================================================\n");

		counter += INC;

		sleep(1);
	}

	return 0;
}