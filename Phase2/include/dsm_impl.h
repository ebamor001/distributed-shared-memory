#ifndef __DSM_IMPL_H__
#define __DSM_IMPL_H__

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <poll.h>
#include <assert.h>

//This constant can be modified
#define PAGE_NUMBER (100)

#define TOP_ADDR  (0x40000000)
#define BASE_ADDR (TOP_ADDR - (PAGE_NUMBER * PAGE_SIZE))
#define PAGE_SIZE (sysconf(_SC_PAGE_SIZE))


#define MAX_STR (128) 

typedef char maxstr_t[MAX_STR];

typedef int dsm_page_owner_t;

/* États locaux d'une page */
typedef enum {
  DSM_NOACCESS = 0,
  DSM_OWNED,
  DSM_REQUESTED
} dsm_page_state_t;


typedef struct
{ 
  dsm_page_owner_t owner;
  dsm_page_state_t state;     
  pthread_mutex_t  mtx;
  pthread_cond_t   cv; /* variable de condition pour l'attente des requêtes */
} dsm_page_info_t;


typedef enum
{
  DSM_NO_TYPE = -1,
  DSM_REQ,        // demande d'une page
  DSM_PAGE,      // envoi d'une page
  DSM_UPDATE,
  DSM_FINALIZE, 
} dsm_req_type_t;

/* En-tête générique pour les requetes DSM */
typedef struct
{
  int type;      /* dsm_req_type_t : DSM_REQ, DSM_PAGE, DSM_UPDATE, DSM_FINALIZE */
  int source;    /* rang émetteur */
  int page_num;   /* numéro de page */
} dsm_req_t;


struct proc_conn_info {
  int          rank;
  int          port_num;
  maxstr_t     machine; 
};
typedef struct proc_conn_info proc_conn_info_t;

struct dsm_proc {
  proc_conn_info_t connect_info;
  int              fd;
};
typedef struct dsm_proc dsm_proc_t;

int function_send(int sockfd, char *buff, int size);
int function_recv(int sockfd, char *buff, int size);
int listen_socket_creation(int *port);
int dsm_socket_creation(char *hostname, char *dsm_port);
int send_info_to_dsmexec(int sfd, proc_conn_info_t connect_infos);
int recv_info_from_dsmexec(int sfd, int num_procs, dsm_proc_t *procs,int node_id);



#endif //__DSM_IMPL_H__
