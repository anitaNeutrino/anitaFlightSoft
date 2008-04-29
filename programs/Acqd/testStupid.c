#include <stdio.h>
#include <stdlib.h>

int main()
{
  int size = 0x2000;

  printf("size: %8.8X -size: %8.8X\n",
	 (unsigned int) size, (unsigned int) -size);
}
