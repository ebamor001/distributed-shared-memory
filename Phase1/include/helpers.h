/* Prototypes of functions implemented in binaries/libhelpers.a.
 * Do not change anything in this file.
 *
 * These functions can be used, but just temporarily, to ease the development.
 */


//Step 1
int file_management_correction(const char *machinefile, int *num_procs, remote_proc_t **proc_array);

//Step2 
int spawn_local_procs_correction(int num_procs, remote_proc_t proc_array[], int master_port, int argc, char *argv[]);

//Step 3
int wait_for_all_io_correction(int num_procs, remote_proc_t proc_array[], int master_fd);

//Network
int socket_creation_correction(int *master_port);

// This function Can be used to test Step #3 (reading on pipes) 
// It launches a thread that writes messages randomly on processes stdout and/or stderr
// just like remote processes would when doing an fprintf on stdout or stderr
// A file name write_manifest.txt is also written and can be used to compare
// what your wait_for_all_io function outputs and what was really written to pipes
// (Note that the order of messages can be different)
pthread_t write_to_pipes(int num_procs, remote_proc_t proc_array[], int num_msgs); 

// This function tests if a command uses the libdsm or not
// It will return a flag set to 1 in the case of data_exchange
int check_dsm_use_status(const char *command, int *flag);
