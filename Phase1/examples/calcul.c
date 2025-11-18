#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
  if(argc > 1){
    int sum = 0;
    while(*(++argv)){
      sum += atoi(*argv);
    }
    fprintf(stderr,"somme = %i\n",sum);
  }
  
  exit(EXIT_SUCCESS);
}
