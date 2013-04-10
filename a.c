#include <stdio.h>
#include <stdbool.h>

//typedef enum { no = 0, false = 0, yes = 1, true = 1 } bool;



int main(int argc, char *argv[]) {
	bool maybe = true;

	printf("sizeof (unsigned long long) = %d\n", sizeof(unsigned long long));
	printf("sizeof (unsigned long) = %d\n", sizeof(unsigned long));

	printf("sizeof int = %d\n", sizeof(int));
	printf("sizeof short = %d\n", sizeof(short));
	printf("sizeof bool = %d\n", sizeof(maybe));

}
