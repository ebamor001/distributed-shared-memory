#include "common_services.h"
#include "helpers.h"

int main(int argc, char *argv[])
{
  if(argc < 3){
    fprintf(stdout,"Usage : dsmexec <files of machines> <command to launch> <arg1> <arg2> ...\n");
    exit(EXIT_FAILURE);
  }
  
  remote_proc_t *proc_array = NULL;
  int            num_procs  = 0;

  char *machine_file = argv[1];
  
  /* Step #1- Machine file management */
  int ret = file_management(machine_file, &num_procs, &proc_array);
  if(ret == -1) ERROR_EXIT("file_management");

  /* Children processes management  */
  /* This call can be placed here or later */
  /* depending on the chosen method */
  ret = childprocs_mgmt_setup();
  if(ret == -1) ERROR_EXIT("childprocs_mgmt_setup");

  /* Master server socket creation */
  int master_port = 0;
  int master_socket_fd = socket_creation(&master_port);
  if(master_socket_fd == -1) ERROR_EXIT("socket_creation");
  
  /* Step #2- Spawn local processes and launch remote processes */
  ret = spawn_local_procs(num_procs, proc_array, master_port, argc, argv);
  if(ret == -1) ERROR_EXIT("spawn_local_procs");

  //Uncomment to simulate random writes on pipes.
  //This is to test step #3 when step #2 is not
  //fully implemented  
  //If you want to use this function, you need to change the last parameter of
  //the call to the cleanup() function below.
  //pthread_t tid = write_to_pipes(num_procs, proc_array, 50);

 
  /* Step #3- Read all info on file descriptors */
  ret = wait_for_all_io(num_procs, proc_array, master_socket_fd);
  if(ret == -1) ERROR_EXIT("wait_for_all_io");

  /* Clean up used resources */
  /* if write_to_pipes is not used, replace last argument with NULL */
  ret = cleanup(num_procs, proc_array, NULL); //&tid);
  if(ret == -1) ERROR_EXIT("cleanup");

  exit(EXIT_SUCCESS);  
}
