#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netdb.h> 
#include <arpa/inet.h>

#include "common_services.h"

/* This function:
 * - launches the local processes which will execute the program through SSH
 * - setup the output redirection of child processes to dsmexec */
int spawn_local_procs(int num_procs, remote_proc_t proc_array[], int master_port, int argc, char *argv[])
{
  // Récupérer le nom de la machine
  char nick[MAX_STR-14];
  if (gethostname(nick, sizeof(nick)) == -1) {
    perror("gethostname");
    return -1;
  }

  /*char  hostname[MAX_STR];
  snprintf(hostname, sizeof(hostname), "%s.pedago.ipb.fr", nick);*/
  
  // Résolution du FQDN portable 
  char hostname[MAX_STR];
  struct addrinfo hints, *res;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  int ret = getaddrinfo(nick, NULL, &hints, &res);
  if (ret == 0 && res) {
      // getnameinfo reconstruit le nom complet (FQDN)
      int r = getnameinfo(res->ai_addr, res->ai_addrlen, hostname, sizeof(hostname),
                          NULL, 0, NI_NAMEREQD); //  NI_NAMEREQD:If set, then an error is returned if the hostname cannot be determined.
      if (r != 0) {
          // si échec → on garde que le préfixe
          strncpy(hostname, nick, sizeof(hostname));
          hostname[sizeof(hostname)-1] = '\0';
      }
      freeaddrinfo(res);
  } else {
      strncpy(hostname, nick, sizeof(hostname));
      hostname[sizeof(hostname)-1] = '\0';
  }


  fprintf(stdout,"=== Listener socket port is %i on machine: %s\n", master_port, hostname);
  
  for(int proc_idx = 0 ; proc_idx < num_procs ; proc_idx++){

    fprintf(stdout,"=== Launching process %i/%i on machine: %s for command: %s\n",
	    proc_idx,
	    num_procs,
	    proc_array[proc_idx].machine,
	    argv[2]);

    //création des tubes 
    if (pipe(proc_array[proc_idx].stdout_pipe) == -1 ) {
      fprintf(stderr, "pipe failed for stdout of the child process number %d\n", proc_idx);
      perror("pipe");
      return -1;
    }
    if (pipe(proc_array[proc_idx].stderr_pipe) == -1 ) {
      fprintf(stderr, "pipe failed for stderr of the child process number %d\n", proc_idx);
      perror("pipe");
      return -1;
    }

    //création du processus enfant
    pid_t pid = fork();
    if (pid == -1) {
      perror("fork");
      return -1;
    } else if (pid == 0){
      // Processus enfant
      // Fermer les extrémités en lecture
      close(proc_array[proc_idx].stdout_pipe[0]);
      close(proc_array[proc_idx].stderr_pipe[0]);

      // Rediriger stdout et stderr
      dup2(proc_array[proc_idx].stdout_pipe[1], STDOUT_FILENO);
      dup2(proc_array[proc_idx].stderr_pipe[1], STDERR_FILENO);

      //Plus besoin des anciens fds
      close(proc_array[proc_idx].stdout_pipe[1]);
      close(proc_array[proc_idx].stderr_pipe[1]);

      //variables d'environnement  à transmettre 
      char rankid_str[64];
      char ranknum_str[64];
      char hostname_str[128+18];
      char portnum_str[64];

      snprintf(rankid_str, sizeof(rankid_str), "DSMEXEC_RANKID=%d", proc_idx);
      snprintf(ranknum_str, sizeof(ranknum_str), "DSMEXEC_RANKNUM=%d", num_procs);
      snprintf(hostname_str, sizeof(hostname_str), "DSMEXEC_HOSTNAME=%s", hostname);
      snprintf(portnum_str, sizeof(portnum_str), "DSMEXEC_PORTNUM=%d", master_port);

      //Récupération de DSM_BIN 
      char *dsm_bin = getenv("DSM_BIN");
      if (!dsm_bin) {
          fprintf(stderr, "Error: DSM_BIN not defined in environment\n");
          exit(EXIT_FAILURE);
      }
    
  
      // Construction du chemin complet de l’exécutable 
      char program_path[MAX_STR];
      snprintf(program_path, sizeof(program_path), "%s/%s", dsm_bin ,argv[2]);
      // Préparer les arguments pour ssh
      // ssh machine $DSM_BIN/program arg1 arg2 ...
      char **ssh_argv = malloc((argc + 9) * sizeof(char *)); // +3 pour les arguments +5 pour les variables d'environnement et  +1 pour NULL final
      if (ssh_argv == NULL) {
        perror("malloc failed");
        exit(EXIT_FAILURE);
      }
      
      int idx=0;
      ssh_argv[idx++] = "ssh";
      ssh_argv[idx++] = proc_array[proc_idx].machine;
      ssh_argv[idx++] = "env"; // pour définir les variables d'environnement sur la machine distante
      ssh_argv[idx++] = rankid_str;
      ssh_argv[idx++] = ranknum_str;
      ssh_argv[idx++] = hostname_str;
      ssh_argv[idx++] = portnum_str;
      ssh_argv[idx++] = program_path;  // chemin complet de l’exécutable et non juste le nom


      for (int j = 3; j < argc; j++) {  // mettre les arguments du programme
        ssh_argv[idx++] = argv[j];
      }
      ssh_argv[idx] = NULL;


      execvp("ssh", ssh_argv); //execution du programme passé en argument sur une machine distante avec ssh

      // execvp n'a pas réussi si on arrive ici
      perror("execvp ssh");
      free(ssh_argv);
      exit(EXIT_FAILURE);

    } else { 
      // Père mode lecture
      close(proc_array[proc_idx].stdout_pipe[1]);
      close(proc_array[proc_idx].stderr_pipe[1]);
      proc_array[proc_idx].local_pid = pid; // PID du processus ssh
      //on fait pas de wait ici, car les enfants exécutent execvp et deviennent des vrais processus ssh donc ca sert à rien d'attendre ici
    }
  
  }
      
  return 0;
}
