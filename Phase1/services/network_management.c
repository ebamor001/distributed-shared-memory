#include "common_services.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>       
#include <arpa/inet.h>


/* This function creates the master socket and fills the port number in the
* port_num parameter. Maybe its return value isn't only a success/error
* code... */
int socket_creation(int *port_num){
    struct addrinfo hints, *result, *rp;
    int listen_fd;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_INET6;      // IPv6 (permet aussi IPv4 si V6ONLY=0)
    hints.ai_socktype = SOCK_STREAM;   // TCP
    hints.ai_flags    = AI_PASSIVE;   

    // service = NULL → port 0 → l'OS choisit un port libre
    if (getaddrinfo(NULL, "0", &hints, &result) != 0) {
        perror("getaddrinfo");
        return -1;
    }

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        listen_fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (listen_fd == -1)continue;

        // Autoriser IPv4 et IPv6 sur la même socket
        int no = 0;
        setsockopt(listen_fd, IPPROTO_IPV6, IPV6_V6ONLY, &no, sizeof(no)); // Autoriser IPv4 sur une socket IPv6

        // Adresse réutilisable
        int opt = 1;
        setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        // Essayer de binder 
        if (bind(listen_fd, rp->ai_addr, rp->ai_addrlen) == 0) {
            break; // succès
        }

        close(listen_fd);
    }

    if (rp == NULL) {
        fprintf(stderr, "Could not bind\n");
        freeaddrinfo(result);
        return -1;
    }
    freeaddrinfo(result);


    // Écoute ; on peut mettre num_procs comme backlog mais j'ai choisi SOMAXCONN pour plus de flexibilité pour le moment
    if (listen(listen_fd, SOMAXCONN) < 0) {
        perror("listen");
        close(listen_fd);
        return -1;
    }

    // Récupérer le port choisi automatiquement 
    struct sockaddr_storage addr; // Peut être IPv4 ou IPv6 
    socklen_t addrlen = sizeof(addr);
    if (getsockname(listen_fd, (struct sockaddr *)&addr, &addrlen) == -1) {
        perror("getsockname");
        close(listen_fd);
        return -1;
    }

    // Extraire le numéro de port
    if (addr.ss_family == AF_INET) {
        *port_num = ntohs(((struct sockaddr_in *)&addr)->sin_port);
    } else if (addr.ss_family == AF_INET6) {
        *port_num = ntohs(((struct sockaddr_in6 *)&addr)->sin6_port);
    }

    return listen_fd;
}