#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "dsm.h"

int main(int argc, char **argv)
{
  char *pointer = dsm_init(argc,argv);
  char *current = pointer;
  int   value;
  
  fprintf(stdout,"[Proc %i] : base address for shared mapping is: %p\n", dsm_node_id, pointer);
  fflush(stdout);

  if(0 == dsm_node_id){
    //uncomment to see a different behaviour
    //sleep(2);
    *((int *)current) += 4;
    value = *((int *)current);
    printf("[Proc %i] : integer value: %i\n", dsm_node_id, value);
  } else if(1 == dsm_node_id){
    *((int *)current) += 8;
    value = *((int *)current);
    printf("[Proc %i] : integer value: %i\n", dsm_node_id, value);
  } else {
    printf("[Proc %i] : I'm not part of this \n", dsm_node_id);
  }
  
  fprintf(stdout,"[Proc %i] : Finalizing ...\n", dsm_node_id);
  fflush(stdout);

  dsm_finalize();
  fprintf(stdout,"[Proc %i] : done \n", dsm_node_id);
  fflush(stdout);

  
  return 1;
}
