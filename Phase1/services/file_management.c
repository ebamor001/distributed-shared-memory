#include "common_services.h"
#include <ctype.h> // for isspace

/* This function reads the content of the file with the path in machinefile and
 * fills the parameters num_procs and proc_array accordingly. */
int file_management(const char *machinefile, int *num_procs, remote_proc_t **proc_array)
{
  //Open the file mode read only
  fprintf(stdout,"=== Opening file: %s\n",machinefile);
  FILE *f = fopen(machinefile, "r");
  if (!f) {
      perror("Error opening machinefile");
      return -1;
  }
  
  //counting number of non-empty lines
  char line[MAX_STR];  // buffer to store each line representing a machine name 
  int count = 0;
  while (fgets(line, sizeof(line), f)) {
      char *p = line;
      while (isspace((unsigned char)*p)) p++; // skip whitespaces at the beginning
      if (*p == '\0') continue; // ignore empty lines
      count++;
  }
  fseek(f, 0, SEEK_SET); //go back to the beginning of the file

  //allocating proc_array
  *num_procs = count;
  *proc_array = (remote_proc_t *)calloc(*num_procs,sizeof(remote_proc_t));
  if(!*proc_array){
    perror("calloc failed");
    fclose(f);
    return -1;
  }
  
  //filling proc_array information
  for(int proc_idx = 0 ; proc_idx < *num_procs ; proc_idx++){
    while (fgets(line, sizeof(line), f)) {
      //in case linux line ends with \n or in windows file with \r\n so we remove them and assure string is null-terminated
      size_t len = strlen(line);
      if (len > 0 && line[len-1] == '\n') {
          line[len-1] = '\0';  
      }
      if (len > 1 && line[len-2] == '\r') {
          line[len-2] = '\0';
      }
      
      char * head = line;
      while (isspace((unsigned char)*head)) head++;
      if (*head != '\0') { // non empty line so we keep it
          // define rank
          (*proc_array)[proc_idx].rank = proc_idx;

          // define machine
          strcpy((*proc_array)[proc_idx].machine, head);

          // initialize the remaining fields that will be set later
          (*proc_array)[proc_idx].fd_client = -1; 
          (*proc_array)[proc_idx].stdout_pipe[0] = -1; 
          (*proc_array)[proc_idx].stdout_pipe[1] = -1;
          (*proc_array)[proc_idx].stderr_pipe[0] = -1;
          (*proc_array)[proc_idx].stderr_pipe[1] = -1;
          (*proc_array)[proc_idx].local_pid = -1;
          (*proc_array)[proc_idx].connect_info = NULL;
          (*proc_array)[proc_idx].connect_info_size = 0;

          break; // on sort du while et on passe au proc_idx suivant
      }
    }
  }

  // Close the file and print read machines
  fclose(f);

  fprintf(stdout, "=== Read %d machines from %s ===\n", *num_procs, machinefile);
  for (int i = 0; i < *num_procs; i++) {
        fprintf(stdout, "Proc %d -> machine: %s\n", (*proc_array)[i].rank, (*proc_array)[i].machine);
  }

  return 0;
}

