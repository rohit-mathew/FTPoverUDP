#include<stdio.h>
#include<stdlib.h>
#include<time.h>

int shallisend()
{
	srand ( time(NULL) );
	int random = rand();
	random = random%100;
	printf ("Some random number: %d\n", random);	
	if(random <= 20)
		return 0;
	else
		return 1;
}
