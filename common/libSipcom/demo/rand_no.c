/* rand_no.c - return a random number in a range
 * Marty Olevitch, Apr '03
 */

#include <stdlib.h>

int
rand_no(int lim)
{
    float a;
    a = (float)rand() / RAND_MAX;
    a *= lim;
    return ((int) a);
}

void
rand_no_seed(unsigned int seed)
{
    srand(seed);
}

#ifdef TESTING

#include <stdio.h>
#include <sys/types.h>	/* getpid() */
#include <unistd.h>	/* getpid() */

int
main(int argc, char *argv[])
{
    int i;
    int r;
    int val;

    if (argc != 2) {
	fprintf(stderr, "USAGE: %s val\n", argv[0]);
	exit(1);
    }

    val = atoi(argv[1]);
    srand(getpid());

    while (1) {
	r = rand_no(val);
	printf("%d\n", r);
    }
}
#endif /* TESTING */
