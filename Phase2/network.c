#include "dsm_impl.h"
#include <errno.h>

/* Mettre dans ce fichier les fonctions reseau */
int function_send(int sockfd, char *buff, int size){
	int sent = 0;
	int to_send = size;
	int ret;
	while(sent != to_send){
		ret = send(sockfd, buff + sent, to_send - sent, 0);
		if (ret == -1) {
			if (errno == EINTR) continue;
			return -1;
		}

		else if(ret == 0){
			return -1;
		}
		sent = sent + ret;
	}
	return 1;
}

int function_recv(int sockfd, char *buff, int size){
	int received = 0;
	int to_receive = size;
	int ret;
	while(received != to_receive){
		ret = recv(sockfd, buff + received, to_receive - received, 0);
		if (ret == -1) {
			if (errno == EINTR) continue;
			return -1;
		}
		else if(ret == 0){
			return 0;
		}
		received = received + ret;
	}
	return 1;

}

int listen_socket_creation(int *port) 
{
    struct addrinfo hints, *result, *rp;
	int sfd;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;	   
	hints.ai_socktype = SOCK_STREAM;	
	hints.ai_flags = AI_PASSIVE;		
	// Lier la socket à une adresse locale et un port dynamique
	if (getaddrinfo(NULL, "0", &hints, &result) != 0) {
		perror("getaddrinfo()");
		exit(EXIT_FAILURE);
	}
	for (rp = result; rp != NULL; rp = rp->ai_next) {
		sfd = socket(rp->ai_family, rp->ai_socktype,rp->ai_protocol);
		if (sfd == -1) continue;
		if (bind(sfd, rp->ai_addr, rp->ai_addrlen) == 0) {
			break;
		}
		close(sfd);
	}
	if (rp == NULL) {
		fprintf(stderr, "Could not bind\n");
		freeaddrinfo(result);
		exit(EXIT_FAILURE);	
	}
	freeaddrinfo(result);

	// Récupérer le port choisi par le système
	struct sockaddr_storage addr;
	socklen_t addrlen = sizeof(addr);
    if (getsockname(sfd, (struct sockaddr *)&addr, &addrlen) == -1) {
        perror("getsockname()");
        close(sfd);
        exit(EXIT_FAILURE);
    }

    // Extraire le port (selon IPv4 / IPv6)
    if (addr.ss_family == AF_INET) {
        *port = ntohs(((struct sockaddr_in *)&addr)->sin_port);
    } else if (addr.ss_family == AF_INET6) {
        *port = ntohs(((struct sockaddr_in6 *)&addr)->sin6_port);
    } else {
        *port = -1;
    }
	if (listen(sfd, SOMAXCONN) == -1) {
		perror("listen");
		close(sfd);
		exit(EXIT_FAILURE);
	}
	return sfd;
}

int dsm_socket_creation(char * hostname, char *dsm_port){
	struct addrinfo hints, *result, *rp;
	int sockfd;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	if (getaddrinfo(hostname, dsm_port, &hints, &result) != 0) {
		perror("getaddrinfo()");
		exit(EXIT_FAILURE);
	}
	for (rp = result; rp != NULL; rp = rp->ai_next) {
		sockfd = socket(rp->ai_family, rp->ai_socktype,rp->ai_protocol);
		if (sockfd == -1) continue;
		if (connect(sockfd, rp->ai_addr, rp->ai_addrlen) != -1) {
			break;
		}
		close(sockfd);
	}
	if (rp == NULL) {
		fprintf(stderr, "Could not connect to %s:%s\n", hostname, dsm_port);
		freeaddrinfo(result);
		exit(EXIT_FAILURE);
	}
	freeaddrinfo(result);
	return sockfd;
} 


int send_info_to_dsmexec(int sfd, proc_conn_info_t connect_infos){
   int ret;
   size_t size = sizeof(proc_conn_info_t);

   //envoyer le rang
   ret = function_send(sfd, (char *)&connect_infos.rank, sizeof(int));
   if (ret == -1) {
		perror("send()");
		return -1; 
	}
	else if (ret == 0) {
		printf("dsmexec a fermé la connexion\n");
		close(sfd);
		return -1; 
	}

   //envoyer la taille de infos
   ret = function_send(sfd, (char *)&size, sizeof(size_t));
   if (ret == -1) {
		perror("send()");
		return -1; 
	}
	else if (ret == 0) {
		printf("dsmexec a fermé la connexion\n");
		close(sfd);
		return -1;
	}

   //envoyer infos
   ret = function_send(sfd, (char *)&connect_infos, size);
   if (ret == -1) {
		perror("send()");
		return -1; 
	}
	else if (ret == 0) {
		printf("dsmexec a fermé la connexion\n");
		close(sfd);
		return -1; 
	}
	return 0;
}


int recv_info_from_dsmexec(int sfd, int num_procs, dsm_proc_t *procs, int node_id){
	for (int i=0; i<num_procs; i++){
		proc_conn_info_t tmp;
		int ret=function_recv(sfd, (char *)&tmp, sizeof(proc_conn_info_t));
		if (ret <= 0) {
			fprintf(stderr, "[%d] erreur ou déconnexion dsmexec pendant réception table\n", node_id);
			close(sfd);
			return -1;
		}
		int rank = tmp.rank;
		procs[rank].connect_info = tmp;
		procs[rank].fd = -1;  // init
	}
	return 0;
}