#include "commun.h"
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

#define COLOR_RED "\x1B[31m"
#define COLOR_GREEN "\x1B[32m"
#define COLOR_ORANGE "\x1B[33m"
#define COLOR_BLUE "\x1B[34m"
#define COLOR_MAGENTA "\x1B[35m"
#define COLOR_CYAN "\x1B[36m"
#define COLOR_WHITE  "\x1B[37m"

/* Programme noeud */

int main(int argc, char *argv[]) {
    // Message d'erreur en cas de paramètres incorrects
    if (argc != 4) {
        printf("%s[NOEUD] Erreur - Paramètres incorrects\n[NOEUD] Utilisation : %s ip_serveur port_serveur id_noeud\n%s", COLOR_RED, argv[0], COLOR_WHITE);
        exit(1);
    }

    // Création des sockets
    int socket_connexion_serveur = createSocket();
    int socket_ecoute_serveur = createSocket();
    int socket_ecoute_voisins = createSocket();

    char *id = argv[3];

    // Nommage des sockets
    nameSocket(socket_ecoute_serveur, 0);
    struct sockaddr_in adEcouteServeur;
    int taille_sockaddr_in;
    taille_sockaddr_in = sizeof(adEcouteServeur);
    getsockname(socket_ecoute_serveur, (struct sockaddr *) &adEcouteServeur, &taille_sockaddr_in);

    nameSocket(socket_ecoute_voisins, 0);
    struct sockaddr_in adEcouteVoisins;
    getsockname(socket_ecoute_voisins, (struct sockaddr *) &adEcouteVoisins, &taille_sockaddr_in);

    // Mise en écoute de la socket du serveur
    if (TCPListen(socket_ecoute_serveur, 1) == -1) {
        printf("%s[NOEUD %s] Erreur - Mise en écoute de la socket serveur : %s\n%s", COLOR_RED, id, strerror(errno), COLOR_WHITE);
        close(socket_ecoute_serveur);
        close(socket_ecoute_voisins);
        exit(1);
    }

    // Connexion au serveur
    printf("%s[NOEUD %s] Communication avec le serveur\n%s", COLOR_CYAN, id, COLOR_WHITE);
    struct sockaddr_in adresse_serveur = designateSocket(argv[1], argv[2]);
    socklen_t lgA = sizeof(struct sockaddr_in);
    if (TCPConnect(socket_connexion_serveur, &adresse_serveur) ==-1) {
        printf("%s[NOEUD %s] Erreur - Connexion au serveur : %s\n%s", COLOR_RED, id, strerror(errno), COLOR_WHITE);
        exit(1);
    }

    // Réception du nombre de voisins
    int nombre_voisins;
    if (recvTCP(socket_connexion_serveur, &nombre_voisins, sizeof(int)) == -1) {
        printf("%s[NOEUD %s] Erreur - Réception du nombre de voisins : %s\n%s", COLOR_RED, id, strerror(errno), COLOR_WHITE);
        close(socket_connexion_serveur);
        close(socket_ecoute_serveur);
        close(socket_ecoute_voisins);
        exit(1);
    }

    // Mise en écoute de la socket des voisins
    if (TCPListen(socket_ecoute_voisins, nombre_voisins)) {
        printf("%s[NOEUD %s] Erreur - Mise en écoute de la socket voisins : %s\n%s", COLOR_RED, id, strerror(errno), COLOR_WHITE);
        close(socket_ecoute_serveur);
        close(socket_ecoute_voisins);
        exit(1);
    }

    // Envoi des adresses au serveur
    if (sendTCP(socket_connexion_serveur, (void*) &adEcouteServeur, sizeof(struct sockaddr_in)) == -1) {
        printf("%s[NOEUD %s] Erreur - Envoi de l'adresse de contact au serveur : %s\n%s", COLOR_RED, id, strerror(errno), COLOR_WHITE);
        close(socket_connexion_serveur);
        close(socket_ecoute_serveur);
        close(socket_ecoute_voisins);
        exit(1);
    }
    if (sendTCP(socket_connexion_serveur, (void*) &adEcouteVoisins, sizeof(struct sockaddr_in)) == -1) {
        printf("%s[NOEUD %s] Erreur - Envoi de l'adresse de voisin au serveur : %s\n%s", COLOR_RED, id, strerror(errno), COLOR_WHITE);
        close(socket_connexion_serveur);
        close(socket_ecoute_serveur);
        close(socket_ecoute_voisins);
        exit(1);
    }
    close(socket_connexion_serveur);

    // Réception des adresses des voisins
    struct sockaddr_in adresseServeur;
    int socket_serveur = accept(socket_ecoute_serveur, (struct sockaddr *)&adresseServeur, &lgA);
    if (socket_serveur == -1) {
        printf("%s[NOEUD %s] Erreur - Acceptation du serveur : %s\n%s", COLOR_RED, id, strerror(errno), COLOR_WHITE);
        close(socket_ecoute_serveur);
        close(socket_ecoute_voisins);
        exit(1);
    }
    struct sockaddr_in * adressesVoisins = (struct sockaddr_in *) malloc(sizeof(struct sockaddr_in) * nombre_voisins);
    for (int i = 0; i < nombre_voisins; i++) {
        if (recvTCP(socket_serveur, (void *) &adressesVoisins[i], sizeof(struct sockaddr_in)) == -1) {
            printf("%s[NOEUD %s] Erreur - Réception de la %ie adresse : %s\n%s", COLOR_RED, id, i+1, strerror(errno), COLOR_WHITE);
            close(socket_ecoute_serveur);
            close(socket_ecoute_voisins);
            close(socket_serveur);
            exit(1);
        }
    }
    close(socket_serveur);

    printf("%s[NOEUD %s] Adresses reçues, communication avec les voisins\n%s", COLOR_CYAN, id, COLOR_WHITE);    
    // Création des sockets pour se connecter à ses voisins
    int * sockets_connexion_voisins = (int *) malloc(sizeof(int) * nombre_voisins);
    for (int i = 0; i < nombre_voisins; i++) {
        sockets_connexion_voisins[i] = createSocket();
    }

    // Connexion aux voisins
    for (int i = 0; i < nombre_voisins; i++) {
        if (TCPConnect(sockets_connexion_voisins[i], &adressesVoisins[i]) == -1) {
            printf("%s[NOEUD %s] Erreur - Connexion au voisin %i/%i : %s\n%s", COLOR_RED, id, i+1, nombre_voisins, strerror(errno), COLOR_WHITE);
            close(socket_ecoute_serveur);
            close(socket_ecoute_voisins);
            exit(1);
        }
    }

    // Acceptation des voisins
    struct sockaddr_in * tab_adresses_voisins = (struct sockaddr_in *) malloc(sizeof(struct sockaddr_in) * nombre_voisins);
    int * tab_sockets_voisins = (int *) malloc(sizeof(int) * nombre_voisins);
    for (int i = 0; i < nombre_voisins; i++) {
        if ((tab_sockets_voisins[i] = accept(socket_ecoute_voisins, (struct sockaddr *)&tab_adresses_voisins[i], &lgA)) == -1) {   
            printf("%s[NOEUD %s] Erreur - Acceptation du %ie voisin : %s\n%s", COLOR_RED, id, i+1, strerror(errno), COLOR_WHITE);
            close(socket_ecoute_serveur);
            close(socket_ecoute_voisins);
            for (int i = 0; i < nombre_voisins; i++) {
                close(sockets_connexion_voisins[i]);
            } 
            exit(1);
        }
    }
    printf("%s[NOEUD %s] Connecté à tous les voisins\n%s", COLOR_GREEN, id, COLOR_WHITE);
    return 0;
}
