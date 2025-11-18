#include "common_services.h"
#include <poll.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h> 
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>



/* This function is the main loop of the program dsmexec. Once all remote
 * processes are launched, it waits for data on all file descriptors (pipes of
 * children outputs, network, signals, ...).
 * Dsmexec stays in this function until all child processes are not finished.
 */


 void die(ssize_t status, const char* str){
  if (status==-1){
      perror(str); 
      exit(EXIT_FAILURE);
  }
}

//Read the size of the next message
void read_size(int fd, size_t * size_next_msg , int* closed) {
	int bytes_read = 0;
	*closed=0;
	int to_read = sizeof(size_t); 
	while (bytes_read < to_read) {
		int result = read(fd, (char *)size_next_msg + bytes_read, to_read - bytes_read);
		die(result, "read");
		if (result == 0) {
			*closed=1;
			int ret=close(fd);
			die(ret,"close");
			return;
		}
		bytes_read = bytes_read + result;
	}
}

//Read the actual message
void read_all(int fd, char *buf, size_t size,int* closed) {
	*closed=0;
	if (size == 0) return;
	size_t bytes_read = 0;
	while (bytes_read < size) {
		size_t result = read(fd, (char *)buf + bytes_read, size - bytes_read);
		die(result, "read");
		if (result == 0) {
			printf("Connection closed \n");
			int ret=close(fd);
			die(ret,"close");
			*closed=1;
			break;
		}
		bytes_read += result;
	}
}

//Send the actual message
void write_all(int fd, const char *buf, size_t count) {
	if (count == 0) return;
	size_t bytes_written = 0;
	while (bytes_written < count) {
		ssize_t result = write(fd, (const char *)buf + bytes_written, count - bytes_written);
		die(result, "write");
		bytes_written += result;
	}
}

int wait_for_all_io(int num_procs, remote_proc_t proc_array[] ,int master_fd)
{
  fprintf(stdout,"=== Call to wait_for_all_io\n");
  fflush(NULL);
  //gestion des sockets 
  char buffer[1024];
  int closed=0;
  int connected = 0; // nombre de clients connectés


  //gestion des pipes 
  int open_fds = num_procs * 2; // each proc has 2 pipes (stdout, stderr)
  struct pollfd *fds = malloc((open_fds+1) * sizeof(struct pollfd)); // +1 for master_fd + num_procs for potential client fds 
  if (!fds) {
      perror("malloc fds");
      return -1;
  }
  fds[0].fd = master_fd;
  fds[0].events = POLLIN;
  fds[0].revents = 0;
  for (int i = 0; i < num_procs; i++) {
    // stdout
    fds[1+2*i].fd = proc_array[i].stdout_pipe[0]; // stdout du processus i => les index impairs sont pour stdout
    fds[1+2*i].events = POLLIN;
    fds[1+2*i].revents = 0;

    // stderr
    fds[1+2*i+1].fd = proc_array[i].stderr_pipe[0]; // stderr du processus i => les index pairs sont pour stderr
    fds[1+2*i+1].events = POLLIN;
    fds[1+2*i+1].revents = 0;
  }

  
  while (1) {
    int ret = poll(fds, open_fds+1, -1);
    if (ret < 0) {
      if (errno == EINTR) continue; // interrupted by signal, retry
      die(ret, "poll");
    }
    //gérer le socket d'écoute 
    // Vérifier si le master_fd  détecte une nouvelle connexion
    if (fds[0].revents & POLLIN){
      int rank;
      struct sockaddr_in addr;
      socklen_t len = sizeof(addr);
      int newfd = accept(master_fd, (struct sockaddr*)&addr, &len);
      if (newfd <0){
        perror("accept");
        continue;
      }

      //1-Lecture du rang
      int lu = 0;
      int a_lire = sizeof(int);
      int lire_rank;
      while(lu<sizeof(int)){
        lire_rank = read(newfd, (char*)&rank + lu, a_lire-lu);
        if (lire_rank < 0) {
          perror("read rank");
          close(newfd);
          continue;
        }
        if (lire_rank == 0) {
          fprintf(stderr, "Client closed connection before sending rank\n");
          close(newfd);
          continue;
        }
        lu += lire_rank;
      }
      proc_array[rank].fd_client = newfd;
      // 2- lecture de la taille des infos de message 
      size_t info_size_network;
      read_size(newfd, &info_size_network, &closed);
      if (closed) {
        fprintf(stderr, "=== Client disconnected (fd=%d)\n", newfd);
        close(newfd);
        continue;
      }
      proc_array[rank].connect_info_size = info_size_network;
      fprintf(stdout, "=== Taille des infos de connexion reçues pour le proc %d : %zu octets\n",
              rank,
              proc_array[rank].connect_info_size); //%zu pour size_t
      
      // 3- lecture des infos de connexion
      proc_array[rank].connect_info = malloc(info_size_network);
      read_all(newfd,proc_array[rank].connect_info,info_size_network,&closed);
      if (closed) {
        fprintf(stderr, "=== Client disconnected (fd=%d)\n", newfd);
        close(newfd);
        continue;
      }
      connected++;

      //4- si toutes les connexions sont établies, on envoie la table complète aux clients
      if (connected == num_procs) {
        fprintf(stdout, "=== Tous les processus sont connectés, envoi de la table de connexions ===\n");
        for (int i = 0; i < num_procs; i++) {
          int fd = proc_array[i].fd_client;
          for (int j = 0; j < num_procs; j++) { 
            write_all(fd,proc_array[j].connect_info,proc_array[j].connect_info_size);              
          }
          close(fd);
        }
        // libération après envoi
        for (int i = 0; i < num_procs; i++) {
          free(proc_array[i].connect_info);
          proc_array[i].connect_info = NULL;
        }  
      }
      fds[0].revents = 0;
    }
    //gérer les pipes des enfants 
    for (int i = 1 ; i < open_fds+1; i++) {
      if (fds[i].fd == -1) continue; // skip closed fds
      int proc_idx = (i-1) / 2; // each proc has 2 fds
      int is_stdout = ((i-1) % 2 == 0);
      if (fds[i].revents & POLLIN) {
        ssize_t bytes = read(fds[i].fd, buffer, sizeof(buffer) - 1); // read est non atomique but since c'est un pipe ca devrait aller   
        if (bytes > 0) {
          buffer[bytes] = '\0'; // null terminate for printing
          fprintf(stdout, "[Proc %d : %s : %s] %s",
                  proc_array[proc_idx].rank,
                  proc_array[proc_idx].machine,
                  is_stdout ? "stdout" : "stderr",
                  buffer);
          fflush(stdout);
        } else if (bytes == 0) {
          // Fin de flux
          close(fds[i].fd);
          fds[i].fd = -1;
        } else {
          die(bytes, "read from pipe");
          close(fds[i].fd);
          fds[i].fd = -1;
        }
      }
      if (fds[i].revents & POLLHUP) {
          close(fds[i].fd);
          fds[i].fd = -1;
      }
      fds[i].revents = 0; // reset revents
    }

  
    // Vérifie si tous les fds sont fermés
    int remaining_fds = 0;
    for (int i = 1; i < open_fds+1 ; i++) {
        if (fds[i].fd != -1) {
            remaining_fds++;
        }
    }

    // Condition de sortie : tous les pipes et sockets fermés
    if (remaining_fds == 0) {
        break;
    }

  }

  free(fds);

  fprintf(stdout, "=== All child outputs processed ===\n");
  
  return 0;
}

