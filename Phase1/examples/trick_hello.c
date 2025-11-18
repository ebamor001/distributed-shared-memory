#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
  fprintf(stdout,"Hello ");
  fflush(stdout);
  
  sleep(2);

  fprintf(stdout,"World\n");
  exit(EXIT_SUCCESS);
}
