#include <stdio.h>
#include <stdlib.h>

inline volatile unsigned long ReadTSC();

int main() {
  unsigned long before, after;
  
  before = ReadTSC();
  printf("I am here");
  after = ReadTSC();
  printf("took %d cycles\n", after-before);
}

inline volatile unsigned long ReadTSC() {
  unsigned long tsc;
  asm("rdtsc":"=A"(tsc));
  return tsc;
}
