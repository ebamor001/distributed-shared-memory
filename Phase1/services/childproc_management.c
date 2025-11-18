#include "common_services.h"
#include <signal.h>
#include <sys/wait.h>

void traitant_sigchild(int signum){
  pid_t pid;
  while ((pid = waitpid(-1, NULL, WNOHANG)) > 0) {
    char buffer[256];
    int len;
    len = snprintf(buffer, sizeof(buffer), "=== Enfant %d terminé\n", pid);
    if (len > 0) {
        // async-signal-safe contrairement à printf 
        ssize_t w = write(STDOUT_FILENO, buffer, len);
        (void)w;


    }  
  }
}

/* Function to handle (or setup the handle of) the terminaison of child
 * processes. At the beginning of the project, you may not see what this
 * function should do, but you will (hopefuly) realize it later during the
 * project. */
int childprocs_mgmt_setup(void)
{
  int ret = 0;

  struct sigaction act;
  sigemptyset(&act.sa_mask);
  act.sa_flags = SA_RESTART; // ne pas interrompre les appels système
  act.sa_handler = traitant_sigchild;
  
  fprintf(stdout,"=== Call to childprocs_mgmt_setup\n");

  ret = sigaction(SIGCHLD, &act, NULL);  
  return ret;
}

