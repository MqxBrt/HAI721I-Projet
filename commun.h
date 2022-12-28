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
        perror("[COMMUN] Erreur - Création de la socket\n");
        exit(1);
    }
    // Fermeture des sockets propre
    if (setsockopt(ds, SOL_SOCKET, SO_REUSEADDR, &true, sizeof(int)) == -1) {
        perror("[COMMUN] Erreur - Setsockopt échoué\n");
        exit(1);
    }
    return ds;
}

// Nommage de socket par défaut
int nameSocketDefault(int ds, int port) {
    struct sockaddr_in ad;
    ad.sin_family = AF_INET;
    ad.sin_addr.s_addr = INADDR_ANY;
    ad.sin_port = htons((short)port);

    return bind(ds, (struct sockaddr *)&ad, sizeof(ad));
}

// Nommage de socket sur l'adresse publique
int nameSocketAddress(int ds, int port) {
    // Récupération de l'adresse IP publique
    FILE *fichier = popen("ifconfig eno1 | grep 'inet' | cut -d: -f2 | awk '{ print $2}'", "r"); // Remplacer 'eno1' par 'ens160' pour exécuter sur x2Go
    char *ip = NULL;
    size_t len = 0;
    getline(&ip, &len, fichier);
    pclose(fichier);

    struct sockaddr_in ad;
    ad.sin_family = AF_INET;
    ad.sin_addr.s_addr = inet_addr(ip);
    ad.sin_port = htons((short)port);

    return bind(ds, (struct sockaddr *)&ad, sizeof(ad));
}

// Désignation de la socket avec des char*
struct sockaddr_in designateSocket(char *ip, int port) {
    struct sockaddr_in sock;
    sock.sin_family = AF_INET;
    sock.sin_addr.s_addr = inet_addr(ip);
    sock.sin_port = htons((short)port);
    return sock;
}

// Passage en mode écoute
int TCPListen(int ds, int queueLength) {
    return listen(ds, queueLength);
}

// Connexion
int TCPConnect(int ds, const struct sockaddr_in *sockAddr) {
    socklen_t lgA = sizeof(struct sockaddr_in);
    return connect(ds, (struct sockaddr *)sockAddr, lgA);
}

// Envoi
int sendTCP(int sock, void *msg, int sizeMsg) {
    int res;
    int sent = 0;
    while (sent < sizeMsg) {
        res = send(sock, msg + sent, sizeMsg - sent, 0);
        sent += res;
        if (res == -1) {
            return -1;
        }
    }
    return sent;
}

// Réception
int recvTCP(int sock, void *msg, int sizeMsg) {
    int res;
    int received = 0;
    while (received < sizeMsg) {
        res = recv(sock, msg + received, sizeMsg - received, 0);
        received += res;
        if (res == -1) {
            return -1;
        }
        else if (res == 0) {
            return 0;
        }
    }
    return received;
}
