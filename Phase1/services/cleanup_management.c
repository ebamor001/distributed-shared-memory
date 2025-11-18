#include "common_services.h"

/* This function is called at the end of the program. It frees and releases all
 * allocated resources. */
int cleanup(int num_procs, remote_proc_t proc_array[], void *extra)
{

  fprintf(stdout,"=== dsmexec cleanup ...\n");
  if(extra){
    /* If we have a thread (probably because we enabled write_to_pipes()), join it */
    pthread_join(*(pthread_t *)extra, NULL);
  }

  for (int i = 0; i < num_procs; i++) {
        if (proc_array[i].connect_info != NULL) {
            free(proc_array[i].connect_info);
        }
  }

  free(proc_array);


  fprintf(stdout,"=== Done\n");
  return 0;
}

