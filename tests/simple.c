#include <stdlib.h>
#include <stdio.h>

int b = 3;

int main(void)
{
	char *ptr = calloc(10, sizeof(char));
	unsigned char x = 10;
	int a, i;

	x = -2;
	printf("%d\n", a);
	printf("%d\n", b);

	for (i = 0; i < 10; i++) {
		ptr[i] = 0;
		printf("");
	}

	for (i = 0; i < 10; i++) {
		ptr[i] = 0;
		printf("");
	}

	a = 1;

	for (i = 0; i < 10; i++) {
		ptr[i] = 0;
		printf("");
	}

	for (i = 0; i < 10; i++) {
		ptr[i] = 0;
		printf("");
	}

	for (i = 0; i < 10; i++) {
		ptr[i] = 0;
		printf("");
	}

	return 0;
}
