#include <stdio.h> 
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include "commun.h"
#include "parseur.h"

#define COLOR_RED "\x1B[31m"
#define COLOR_GREEN "\x1B[32m"
#define COLOR_ORANGE "\x1B[33m"
#define COLOR_BLUE "\x1B[34m"
#define COLOR_MAGENTA "\x1B[35m"
#define COLOR_CYAN "\x1B[36m"
#define COLOR_WHITE  "\x1B[37m"

/* Programme serveur */

int main(int argc, char *argv[]) {
    // Message d'erreur en cas de paramètres incorrects
    if (argc != 3) {
        printf("%s[SERVEUR] Erreur - Paramètres incorrects\n[SERVEUR] Utilisation : %s port_serveur nom_fichier \n%s", COLOR_RED, argv[0], COLOR_WHITE);
        exit(1);
    }

    // Récupération des résultats du parseur
    char *filename = argv[2];
    int port = atoi(argv[1]);
    struct graphe graph;
    graph = parser(filename);
    
    // Création et mise en écoute de la socket du serveur
    int socket_ecoute_noeuds = createSocket();
    nameSocket(socket_ecoute_noeuds,port);
    TCPListen(socket_ecoute_noeuds,graph.nombre_sommets-1);

    struct sockaddr_in* adNoeudsContact = (struct sockaddr_in*) malloc(graph.nombre_sommets*sizeof(struct sockaddr_in));
    struct sockaddr_in* adNoeudsVoisins = (struct sockaddr_in*) malloc(graph.nombre_sommets*sizeof(struct sockaddr_in));
    struct sockaddr_in adNoeud;
    socklen_t lgA = sizeof(struct sockaddr_in);
    int socket_client;

    // Récupération des adresses des noeuds
    printf("%s[SERVEUR] Réception des noeuds\n%s", COLOR_MAGENTA, COLOR_WHITE);
    for (int i=1; i < graph.nombre_sommets; i++) {
        socket_client = accept(socket_ecoute_noeuds, (struct sockaddr *) &adNoeud, &lgA);
        if (socket_client == -1) {
            close(socket_ecoute_noeuds);
            printf("%s[SERVEUR] Erreur - Acceptation du %ie noeud échouée : %s\n%s", COLOR_RED, i, strerror(errno), COLOR_WHITE);
            exit(1);
        }
        // Envoi du nombre de voisins
        if (sendTCP(socket_client, &graph.tab_degres[i], sizeof(int)) == -1) {
            printf("%s[SERVEUR] Erreur - Envoi du %ie nombre de voisins échoué : %s\n%s", COLOR_RED, i, strerror(errno), COLOR_WHITE);
            close(socket_client);
            close(socket_ecoute_noeuds);
            exit(1);
        }
        // Réception de l'adresse de contact
        if (recvTCP(socket_client, (void *) &adNoeudsContact[i], sizeof(struct sockaddr_in)) == -1) {
            printf("%s[SERVEUR] Erreur - Réception de la %ie adresse de contact : %s\n%s", COLOR_RED, i, strerror(errno), COLOR_WHITE);
            close(socket_client);
            close(socket_ecoute_noeuds);
            exit(1);
        }
        // Réception de l'adresse de voisin
        if (recvTCP(socket_client, (void *) &adNoeudsVoisins[i], sizeof(struct sockaddr_in)) == -1) {
            printf("%s[SERVEUR] Erreur - Réception de la %ie adresse de voisin : %s\n%s", COLOR_RED, i, strerror(errno), COLOR_WHITE);
            close(socket_client);
            close(socket_ecoute_noeuds);            
            exit(1);
        }
        close(socket_client);
    }
    close(socket_ecoute_noeuds);
    printf("%s[SERVEUR] Tous les noeuds reconnus\n%s", COLOR_MAGENTA, COLOR_WHITE);

    // Se connecte aux noeuds et leur envoie leurs voisins
    printf("%s[SERVEUR] Envoi des adresses\n%s", COLOR_MAGENTA, COLOR_WHITE);
    for (int i=1; i < graph.nombre_sommets; i++) {
        int socket_connexion_noeuds = createSocket();
        if (TCPConnect(socket_connexion_noeuds, &adNoeudsContact[i]) == -1) {
            printf("%s[SERVEUR] Erreur - Connexion au %ie noeud : %s\n%s", COLOR_RED, i, strerror(errno), COLOR_WHITE);
            close(socket_connexion_noeuds);
            exit(1);
        }
        for (int j = 0; j < graph.tab_degres[i]; j++) {
            struct sockaddr_in adresseVoisin = adNoeudsVoisins[graph.tab_voisins[i][j]];
            if (sendTCP(socket_connexion_noeuds, (void*) &adresseVoisin, sizeof(struct sockaddr_in)) == -1) {
                printf("%s[SERVEUR] Erreur - Envoi de la %ie adresse au %ie noeud : %s\n%s", COLOR_RED, j+1, i, strerror(errno), COLOR_WHITE);
                close(socket_connexion_noeuds);
                exit(1);
            }
        }
        close(socket_connexion_noeuds);
    }
    printf("%s[SERVEUR] Toutes les adresses envoyées\n%s", COLOR_MAGENTA, COLOR_WHITE);
    return 0;
}