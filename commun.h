#include <stdio.h> 
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>

// Fonctions utilisées par le serveur central et les noeuds

int true = 1;

// Création de socket
int createSocket() {
    int ds = socket(PF_INET, SOCK_STREAM, 0);
    if (ds == -1) {
        perror("[COMMUN] Erreur - Création de la socket échouée\n");
        exit(1);
    }
    // Fermeture des sockets propre
    if (setsockopt(ds,SOL_SOCKET,SO_REUSEADDR,&true,sizeof(int)) == -1) {
        perror("[COMMUN] Erreur - Setsockopt échoué\n");
        exit(1);
    }
    return ds;
}

// Nommage de socket
int nameSocket(int ds, int port) {
    struct sockaddr_in ad;
    ad.sin_family=AF_INET;
    ad.sin_addr.s_addr=INADDR_ANY;
    ad.sin_port=htons((short) port);

    int res = bind(ds,(struct sockaddr*) &ad,sizeof(ad));
    return res;
}

// Désignation de la socket avec des char*
struct sockaddr_in designateSocket(char* ip, char* port) {
    struct sockaddr_in sock;
    sock.sin_addr.s_addr = inet_addr(ip) ; 
    sock.sin_port = htons((short) atoi(port));
    return sock;
}

// Passage en mode écoute
int TCPListen(int ds, int queueLength) {
    return listen(ds, queueLength);
}

// Connexion
int TCPConnect(int ds, const struct sockaddr_in* sockAddr) {
    socklen_t lgA = sizeof(struct sockaddr_in);
    return connect(ds, (struct sockaddr*)sockAddr, lgA);
}

// Envoi
int sendTCP(int sock, void* msg, int sizeMsg) {
    int res;
    int sent = 0;
    while(sent < sizeMsg) {
        res = send(sock, msg+sent, sizeMsg-sent, 0);
        sent += res;
        if (res == -1) {
            return -1;
        }
    }
    return sent;
}

// Réception
int recvTCP(int sock, void* msg, int sizeMsg) {
    int res;
    int received = 0;
    while(received < sizeMsg) {
        res = recv(sock, msg+received, sizeMsg-received, 0);
        received += res;
        if (res == -1) {
            return -1;
        } else if (res == 0) {
            return 0;
        }
    }
    return received;
}
