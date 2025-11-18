#include "dsm_impl.h"
#include <arpa/inet.h>
#include <poll.h>

static dsm_proc_t *procs = NULL;
static dsm_page_info_t table_page[PAGE_NUMBER];
static pthread_t comm_daemon;
static int finalize_count = 0;        // nombre de processus arrivés à la barrière
static pthread_mutex_t finalize_mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  finalize_cv  = PTHREAD_COND_INITIALIZER;


/* Ces deux variables sont utilisables  */
/* dans les programmes utilisateurs     */
int dsm_node_num;
int dsm_node_id;


void read_all(int fd, char *buf, size_t size,int* closed) {
	*closed=0;
	if (size == 0) return;
	size_t bytes_read = 0;
	while (bytes_read < size) {
		size_t result = read(fd, (char *)buf + bytes_read, size - bytes_read);
		if (result == -1) {
         if (errno == EINTR) {
               // appel interrompu par un signal alors on recommence
               continue;
         }
         perror("read");
         *closed = 1;
         return;
      }
      if (result == 0) {
         // la connexion est fermée proprement
         printf("Connection closed\n");
         *closed = 1;
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
		if (result == -1) {
         if (errno == EINTR) {
               continue;    
         }
         perror("write");
         return;        
      }
      if (result == 0) {
         return;
      }
		bytes_written += result;
	}
}


/* indique l'adresse de debut de la page de numero numpage */
static char *num2address( int numpage )
{ 
   char *pointer = (char *)(BASE_ADDR+(numpage*(PAGE_SIZE)));
   
   if( pointer >= (char *)TOP_ADDR ){
      fprintf(stderr,"[Proc %i] : Invalid address !\n", dsm_node_id);
      return NULL;
   }
   else return pointer;
}


static void mark_fd_closed(int fd)
{
   if (fd < 0) return;
   for (int r = 0; r < dsm_node_num; r++) {
      if (procs[r].fd == fd) {
         procs[r].fd = -1;
         break;
      }
   }
}

/* cette fonction permet de recuperer un numero de page */
/* a partir  d'une adresse  quelconque */
static int address2num( char *addr )
{
  return (((intptr_t)(addr - BASE_ADDR))/(PAGE_SIZE));
}

/* fonctions pouvant etre utiles */
static void dsm_change_info( int numpage,  dsm_page_owner_t owner)
{
   if ((numpage >= 0) && (numpage < PAGE_NUMBER)) {	
     if (owner >= 0 )
      table_page[numpage].owner = owner;
     return;
   }
   else {
     fprintf(stderr,"[Proc %i] : Invalid page number !\n", dsm_node_id);
     return;
   }
}

static dsm_page_owner_t get_owner( int numpage)
{
   return table_page[numpage].owner;
}


/* Allocation (= mapping) d'une nouvelle page */
static void dsm_map_page( int numpage )
{
   char *page_addr = num2address( numpage );
   mmap(page_addr, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
   return ;  
}

/* Changement de la protection d'une page */
static void dsm_protect_page( int numpage , int prot)
{
   char *page_addr = num2address( numpage );
   mprotect(page_addr, PAGE_SIZE, prot);
   return;
}

static void dsm_unmap_page( int numpage )
{
   char *page_addr = num2address( numpage );
   munmap(page_addr, PAGE_SIZE);
   return;
}
static void dsm_reset_page(int numpage)
{
   char *page_addr = (char *)num2address( numpage );
   memset(page_addr,0,PAGE_SIZE);
   return ;
}


// Thread de communication
static void *dsm_comm_daemon( void *arg)
{  
   struct pollfd *fds = malloc((dsm_node_num-1) * sizeof(struct pollfd)); 
   if (!fds) {
      perror("malloc fds");
      return NULL;
   }
   int n=0;
   for(int r=0;r<dsm_node_num;r++){
      if(r==dsm_node_id) continue;
      if(procs[r].fd >= 0){
         fds[n].fd = procs[r].fd;
         fds[n].events = POLLIN;
         fds[n].revents = 0;
         n++;
      }
   }
   while(1){
      int ret = poll (fds, n, -1);
      if(ret < 0){ 
         if(errno==EINTR) continue;// interrupted by signal, retry
         perror("poll");
         break; 
      }
      for(int i=0;i<n;i++){
         if (fds[i].revents & POLLHUP){
            int deadfd = fds[i].fd;
            fds[i].fd = -1;
            mark_fd_closed(deadfd);
            continue;
         }
         if ((fds[i].revents & POLLIN) == POLLIN){
            fds[i].revents=0; 
            dsm_req_t recieved_req;
            int closed;
            read_all(fds[i].fd, (char*)&recieved_req, sizeof(recieved_req), &closed);
            if (closed) {
               int deadfd = fds[i].fd;
               close(deadfd);
               fds[i].fd = -1;
               mark_fd_closed(deadfd);
               continue;
            }
            // traitement de la requête
            char *page = num2address(recieved_req.page_num);
            switch(recieved_req.type){
               case DSM_PAGE:
                  /* on reçoit la page */
                  dsm_protect_page(recieved_req.page_num, PROT_WRITE); // autoriser l'écriture temporairement
                  read_all(fds[i].fd, page, PAGE_SIZE, &closed);
                  if (closed) {
                     int deadfd = fds[i].fd;
                     close(deadfd);
                     fds[i].fd = -1;
                     mark_fd_closed(deadfd);
                     continue;
                  }

                  dsm_protect_page(recieved_req.page_num, PROT_NONE); // interdire l'accès
                  
                  /* on devient le propriétaire */
                  pthread_mutex_lock(&table_page[recieved_req.page_num].mtx);
                  table_page[recieved_req.page_num].owner = dsm_node_id;
                  table_page[recieved_req.page_num].state = DSM_OWNED;
                  dsm_protect_page(recieved_req.page_num, PROT_READ | PROT_WRITE);
                  pthread_cond_broadcast(&table_page[recieved_req.page_num].cv);  //réveiller les processus en attente 
                  pthread_mutex_unlock(&table_page[recieved_req.page_num].mtx);
                  break;

               case DSM_REQ:
                  /* on envoie la page*/
                  if (table_page[recieved_req.page_num].owner != dsm_node_id) break;
                  dsm_req_t req = { DSM_PAGE, dsm_node_id, recieved_req.page_num};
                  if (fds[i].fd < 0) break;     // si le fd est fermé entre temps on skip
                  write_all(fds[i].fd, (char*)&req, sizeof(req)); /*pour notifier le destinataire*/
                  write_all(fds[i].fd, page, PAGE_SIZE); /*envoi de la page*/
                  /* On perd localement la page */
                  pthread_mutex_lock(&table_page[recieved_req.page_num].mtx);
                  dsm_protect_page(recieved_req.page_num, PROT_NONE);
                  table_page[recieved_req.page_num].state = DSM_NOACCESS;
                  table_page[recieved_req.page_num].owner = recieved_req.source;
                  pthread_mutex_unlock(&table_page[recieved_req.page_num].mtx);
                  
                  // notifier les autres processus du changement de propriétaire
                  dsm_req_t upd = { DSM_UPDATE, recieved_req.source, recieved_req.page_num };
                  for (int r = 0; r < dsm_node_num; r++) {
                     if (r == dsm_node_id || r == recieved_req.source) continue; //ils savent déjà
                     if (procs[r].fd < 0) continue;       // on continue
                     write_all(procs[r].fd, (char*)&upd, sizeof(upd));
                  }
                  break;

               case DSM_UPDATE:
                  int new_owner = recieved_req.source;
                  int p = recieved_req.page_num;

                  pthread_mutex_lock(&table_page[p].mtx);
                  table_page[p].owner = new_owner;
                  // Si cette page n’est plus possédée par ce processus
                  if (table_page[p].state == DSM_REQUESTED && new_owner != dsm_node_id) {
                     table_page[p].state = DSM_NOACCESS;
                     dsm_protect_page(p, PROT_NONE);
                     pthread_cond_broadcast(&table_page[p].cv);
                  }
                  pthread_mutex_unlock(&table_page[p].mtx);
                  
                  fprintf(stdout,"[Proc %d] : NEW owner page %d -> %d\n",dsm_node_id, p, new_owner);
                  break;

               case DSM_FINALIZE:
                  int ended = recieved_req.source;
                  fprintf(stdout, "[Proc %d] : %d est arrivé à la barrière de fin\n",
                           dsm_node_id, ended);
                  pthread_mutex_lock(&finalize_mtx);
                  finalize_count++; 
                  pthread_cond_signal(&finalize_cv);//réveiller, le thread principal (dans dsm_finalize())
                  pthread_mutex_unlock(&finalize_mtx);
                  break;
            } 
         }
      }
      
   }
   free(fds);
   return NULL;
}

// Gestionnaire d'impossibilité d'accès aux pages
static void dsm_handler(void* addr) {  
    int num = address2num((char*)addr);

    pthread_mutex_lock(&table_page[num].mtx);

    while (table_page[num].state != DSM_OWNED) {
         if (table_page[num].state == DSM_NOACCESS) {
            int owner = table_page[num].owner;
            dsm_req_t req = { DSM_REQ, dsm_node_id, num };

            if (procs[owner].fd >= 0) {
                write_all(procs[owner].fd, (char *)&req, sizeof(req));
                table_page[num].state = DSM_REQUESTED;
                fprintf(stdout,"[Proc %d] : demande page %d à %d\n", dsm_node_id, num, owner);
            } else {
                fprintf(stderr, "[Proc %d] : ERREUR: fd du propriétaire %d invalide\n", dsm_node_id, owner);
                pthread_mutex_unlock(&table_page[num].mtx);
                return;
            }
         }
        pthread_cond_wait(&table_page[num].cv, &table_page[num].mtx);
    }

    pthread_mutex_unlock(&table_page[num].mtx);
}

/* traitant de signal adequat */
static void segv_handler(int sig, siginfo_t *info, void *context)
{
  /* adresse qui a provoque une erreur */
   void  *addr = info->si_addr;
   
   if ((addr >= (void *)BASE_ADDR) && (addr < (void *)TOP_ADDR)){
     dsm_handler(addr);
   } else {
     /* SIGSEGV normal : ne rien faire*/
     abort();
   }
}


/* Seules ces deux dernieres fonctions sont visibles et utilisables */
/* dans les programmes utilisateurs de la DSM                          */
char *dsm_init(int argc, char *argv[])
{   
   /* Recuperation de la valeur des variables d'environnement */
   /* DSMEXEC_RANKNUM et DSMEXEC_RANKID */
   /* et affectation des variables dsm_node_num et dsm_node_id */
   char *rankid = getenv("DSMEXEC_RANKID");
   char *ranknum = getenv("DSMEXEC_RANKNUM");
   dsm_node_num = atoi(ranknum);
   dsm_node_id = atoi(rankid);
   
   /* Recuperation de l'adresse IP ou du nom de machine */
   /* et du numero de port */
   /* Ces informations sont aussi passees sous forme */
   /* de variables d'environnement */
   char *dsm_port = getenv("DSMEXEC_PORTNUM");
   char *hostname = getenv("DSMEXEC_HOSTNAME");

   /* Connexion au processus dsmexec  */
   int dsm_fd;
   dsm_fd = dsm_socket_creation(hostname, dsm_port);

   /* Creation de la socket d'ecoute pour etablir les connexions */
   /* avec les autres processus applicatifs utilisant la dsm */
   /* On ecoute sur la socket d'ecoute */

   int port;
   int listenfd = listen_socket_creation(&port);
   
   /* Envoi des infos de connexion au processus dsmexec */
   /* (structure de type proc_conn_info_t) */
   /* Cette structure est opaque pour dsmexec */
   /* donc il faut (aprés envoyer le rang): 1- envoyer la taille */
   /*                2- envoyer la structure proprement dite */ 
   char hostname_local[MAX_STR];
   gethostname(hostname_local, sizeof(hostname_local));
   hostname_local[MAX_STR - 1] = '\0';  // sécurité

   proc_conn_info_t connect_infos;
   connect_infos.rank = dsm_node_id;
   connect_infos.port_num = port;
   strcpy(connect_infos.machine, hostname_local);


   int ret;
   ret = send_info_to_dsmexec(dsm_fd, connect_infos);

   /* Allocation du tableau procs */
   procs = malloc(dsm_node_num * sizeof(dsm_proc_t));
   
   /* Recuperation des infos de connexion des autres processus */
   /* Ces informations sont envoyees par dsmexec */
   /* ce sont des structures de type proc_conn_info_t */
   ret = recv_info_from_dsmexec(dsm_fd, dsm_node_num, procs, dsm_node_id);
   close(dsm_fd);

   /* initialisation des connexions */ 
   /* avec les autres processus : sequence connect/accept */
   // 1- Accepter les connexions des rangs < moi
   int to_accept = dsm_node_id;  // nombre de processus qui vont se connecter à moi
   int accepted = 0;

   while (accepted < to_accept) {
      struct sockaddr_in addr;
      socklen_t len = sizeof(addr);
      int newfd = accept(listenfd, (struct sockaddr*)&addr, &len);
      if (newfd < 0) {
         if (errno == EINTR) continue; // interruption signal -> recommencer
         perror("accept");
         exit(EXIT_FAILURE);
      }

      // Handshake : le client m’envoie son rank
      int peer_rank;
      int ret = function_recv(newfd, (char*)&peer_rank, sizeof(peer_rank));
      if (ret <= 0) {
         close(newfd);
         continue;
      }
      procs[peer_rank].fd = newfd;
   
      accepted++;
   }
   close(listenfd);

   // 2- Se connecter aux rangs > moi
   for (int j=dsm_node_id+1; j<dsm_node_num; j++){
      char port_str[16];
      snprintf(port_str, sizeof(port_str), "%d", procs[j].connect_info.port_num);
      int fd_other_proc = dsm_socket_creation(procs[j].connect_info.machine, port_str);
      if (fd_other_proc < 0) {
         fprintf(stderr, "[Proc %d] erreur connexion à %d\n", dsm_node_id, j);
         exit(EXIT_FAILURE);
      }
      function_send(fd_other_proc, (char*)&dsm_node_id, sizeof(dsm_node_id));      // Handshake : j’envoie mon rank
      procs[j].fd = fd_other_proc;
   }

   
   /* Allocation des pages en tourniquet */
   for(int index = 0; index < PAGE_NUMBER; index ++){	
      //création d'un moniteur pour chaque page
      pthread_mutex_init(&table_page[index].mtx, NULL);
      pthread_cond_init (&table_page[index].cv,  NULL);
      dsm_change_info(index, index % dsm_node_num); //définir les propriétaires initiaux
      dsm_map_page(index);
      dsm_reset_page(index);   
      if ((index % dsm_node_num) == dsm_node_id){
         dsm_protect_page(index, PROT_READ | PROT_WRITE);
         table_page[index].state = DSM_OWNED;
      }else {
         table_page[index].state = DSM_NOACCESS;
         dsm_protect_page(index, PROT_NONE);
      }
   }

   /* mise en place du traitant de SIGSEGV */
   struct sigaction act;
   act.sa_flags = SA_SIGINFO; 
   act.sa_sigaction = segv_handler; //envoyer une requête de page au propriétaire
   sigaction(SIGSEGV, &act, NULL);
   
   /* creation du thread de communication           */
   /* ce thread va attendre et traiter les requetes */
   /* des autres processus                          */
   pthread_create(&comm_daemon, NULL, dsm_comm_daemon, NULL);
   
   /* Adresse de début de la zone de mémoire partagée */
   return ((char *)BASE_ADDR);
}

void dsm_finalize( void )
{
   /* fermer proprement les connexions avec les autres processus */

   /* terminer *correctement* le thread de communication */
   /* pour le moment :  */

   // notifier tous les autres qu’on se termine
   for (int r = 0; r < dsm_node_num; r++) {
      if (r == dsm_node_id) continue;
      if (procs[r].fd < 0) continue;   // éviter d'écrire sur un fd fermé
      dsm_req_t fin = { DSM_FINALIZE, dsm_node_id, -1 }; 
      write_all(procs[r].fd, (char*)&fin, sizeof(fin));
   }
   //attendre que tout le monde soit arrivé
   pthread_mutex_lock(&finalize_mtx);
   while (finalize_count < dsm_node_num - 1) {  
      pthread_cond_wait(&finalize_cv, &finalize_mtx);
   }
   pthread_mutex_unlock(&finalize_mtx);

   fprintf(stdout, "[Proc %d] : barrière atteinte : tout le monde a fini\n", dsm_node_id);

   // fermer le thread de communication

   pthread_cancel(comm_daemon);
   
   pthread_join(comm_daemon,NULL);
   for (int i = 0; i < PAGE_NUMBER; i++) {
      dsm_unmap_page(i);
      pthread_mutex_destroy(&table_page[i].mtx);
      pthread_cond_destroy(&table_page[i].cv);
   }
   for (int r = 0; r < dsm_node_num; r++) {
      if (r != dsm_node_id && procs[r].fd >= 0) {
         close(procs[r].fd);   // réveille poll() → POLLHUP
         procs[r].fd = -1;
      }
   }
   //pas besoin de libérer finalize_mtx et finalize_cv car statiques ca sera fait automatiquement

   free(procs);
   fprintf(stdout,"[Proc %d] : DSM finalized proprement\n", dsm_node_id);
   
  return;
}

