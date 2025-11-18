#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <stdint.h>

#define ERROR_EXIT(str){						\
    fprintf(stderr,"%s error @ [%s:line %i] : %s\n", str,		\
	    __FILE__, __LINE__, strerror(errno));			\
    fflush(stderr);							\
    exit(EXIT_FAILURE);}

//////////////////////////////////////
// This constant CANNOT be modified //
//////////////////////////////////////
#define MAX_STR (128) 
//////////////////////////////////////
//////////////////////////////////////

typedef char maxstr_t[MAX_STR];

struct remote_proc {
  ////////////// DO NOT CHANGE FIELD ORDER and NUMBER 
  int              rank;              /* Store the rank of the remote process */
                                      /* This is determined by dsmexec */ 
  maxstr_t         machine;           /* hostname of the remote machine to launch process on*/
  int              fd_client;         /* file descriptor of the socket of  */
                                      /* the remote proc used to exchange connection info */
  int              stdout_pipe[2];    /* pipe to redirect all stdout flow */
  int              stderr_pipe[2];    /* pipe to redirect all stderr flow */
  pid_t            local_pid;         /* the pid of the child process issuing the ssh command */                 
                                      /* that actually launches the remote process */
  void            *connect_info;      /* A pointer to the space needed to store the connect_info */
                                      /* Note that this information is opaque to dsmexec */
  size_t           connect_info_size; /* Size of the connection info */

  ///////////////// Add new fields here if needed
  ///////////////// and reduce padding value accordingly.
  ///////////////// Take into account the alignment of newly
  ///////////////// introduced fields.
  uint8_t          padding[568];          
};
typedef struct remote_proc remote_proc_t;


/* Implemented in file_management.c */
int file_management(const char *machinefile, int *num_procs, remote_proc_t **proc_array);

/* Implemented in process_management.c */
int childprocs_mgmt_setup(void);

/* Implemented in network_management.c */
int socket_creation(int *master_port);

/* Implemented in spawn_management.c */
int spawn_local_procs(int num_procs, remote_proc_t proc_array[], int master_port, int argc, char *argv[]);

/* Implemented in io_management.c */
int wait_for_all_io(int num_procs, remote_proc_t proc_array[], int master_fd);

/* Implemented in cleanup_management.c */
int cleanup(int num_procs, remote_proc_t proc_array[], void *extra);
