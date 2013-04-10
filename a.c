#include <stdio.h>
#include <stdbool.h>

//typedef enum { no = 0, false = 0, yes = 1, true = 1 } bool;

int main(int argc, char *argv[]) {
	bool maybe = true;

	printf("sizeof (unsigned long long) = %d\n", sizeof(unsigned long long));

	printf("sizeof bool = %d\n", sizeof(maybe));

}
