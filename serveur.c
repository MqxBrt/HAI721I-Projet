#include <stdio.h> 
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
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

struct sInformations {
    int entrants;
    int sortants;
    int flag;
};

/* ##### Programme serveur ##### */

int main(int argc, char *argv[]) {

    // Démarrage du timer
    time_t start_t, end_t;
    double diff_t;
    time(&start_t);

    // Message d'erreur en cas de paramètres incorrects
    if (argc != 4) {
        printf("%s[SERVEUR] Erreur - Paramètres incorrects\n[SERVEUR] Utilisation : %s port_serveur nom_fichier sleep_time\n%s", COLOR_RED, argv[0], COLOR_WHITE);
        exit(1);
    }

    /*
    ============== INITIALISATION ==============
    */

    // Récupération des résultats du parseur
    char *filename = argv[2];
    int port = atoi(argv[1]);
    int sleep_time = atoi(argv[3]);
    struct graphe graph;
    graph = parser(filename);
    // Détection du sommet avec le plus haut degré
    int plus_grand_sommet = 1;
    int plus_grand_degre = 0;
    for (int i=1; i<graph.nombre_sommets; i++) {
        int tempInt = graph.tab_degres_entrants[i] + graph.tab_degres_sortants[i];
        if (tempInt > plus_grand_degre) {
            plus_grand_sommet = i;
            plus_grand_degre = tempInt;
        }
    }

    int done = 0;
    int status = 0;
    int one = 1;

    // Création et mise en écoute de la socket du serveur
    int socket_attente_noeuds = createSocket();
    if (nameSocketDefault(socket_attente_noeuds,port) == -1) {
        printf("%s[SERVEUR] Erreur - Nommage de la socket d'écoute des noeuds : %s\n%s", COLOR_RED, strerror(errno), COLOR_WHITE);   
        close(socket_attente_noeuds);
        exit(1); 
    }
    if (TCPListen(socket_attente_noeuds,graph.nombre_sommets-1) == -1) {
        printf("%s[SERVEUR] Erreur - Mise en écoute de la socket d'écoute des noeuds : %s\n%s", COLOR_RED, strerror(errno), COLOR_WHITE);
        close(socket_attente_noeuds);
        exit(1);
    }

    /*
    ============== ATTENTE DES NOEUDS ==============
    */

    int *sockets_ecoute_noeuds = (int *)malloc(sizeof(int) * graph.nombre_sommets);
    struct sockaddr_in* adNoeuds = (struct sockaddr_in*) malloc(graph.nombre_sommets*sizeof(struct sockaddr_in));
    socklen_t lgA = sizeof(struct sockaddr_in);
    struct sockaddr_in* adVoisins = (struct sockaddr_in*) malloc(graph.nombre_sommets*sizeof(struct sockaddr_in));

    // Récupération de l'adresse IP publique
    FILE *fichier = popen("ifconfig eno1 | grep 'inet' | cut -d: -f2 | awk '{ print $2}'", "r"); // Remplacer 'eno1' par 'ens160' pour exécuter sur x2Go
    char * ip = NULL;
    size_t len = 0;
    getline(&ip, &len, fichier);
    pclose(fichier);
    ip[strcspn(ip, "\n")] = 0;

    // Récupération des adresses des noeuds
    printf("%s[SERVEUR] En attente des noeuds [IP: %s / Port: %i]\n%s", COLOR_MAGENTA, ip, port, COLOR_WHITE);
    for (int i=1; i < graph.nombre_sommets; i++) {
        if ((sockets_ecoute_noeuds[i] = accept(socket_attente_noeuds, (struct sockaddr *) &adNoeuds[i], &lgA)) == -1) {
            printf("%s[SERVEUR] Erreur - Acceptation noeud %i/%i : %s\n%s", COLOR_RED, i, graph.nombre_sommets-1, strerror(errno), COLOR_WHITE);
            close(socket_attente_noeuds);
            for (int j=1; j<i; j++) {
                close(sockets_ecoute_noeuds[j]);
            }
            exit(1);
        }

        // Récupération de l'IP + port du noeud pour l'affichage
        struct sockaddr_in adBuf;
        int taille_sockaddr_in;
        taille_sockaddr_in = sizeof(adBuf);
        getpeername(sockets_ecoute_noeuds[i], (struct sockaddr *) &adBuf, &taille_sockaddr_in);
        char buffAddr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(adBuf.sin_addr), buffAddr, INET_ADDRSTRLEN);
        int buffPort = (int) ntohs(adBuf.sin_port);
        printf("%s[SERVEUR] Réception des noeuds (%i/%i) [%s:%i]\n%s", COLOR_MAGENTA, i, graph.nombre_sommets-1, buffAddr, buffPort, COLOR_WHITE);

        struct sInformations infos;
        if (i == plus_grand_sommet) { // Si c'est le plus grand sommet
            struct sInformations temp = {graph.tab_degres_entrants[i], graph.tab_degres_sortants[i], 1};
            infos = temp;
        }
        else { // Sinon
            struct sInformations temp = {graph.tab_degres_entrants[i], graph.tab_degres_sortants[i], 0};
            infos = temp;
        }

        // Envoi des informations au noeud
        if (sendTCP(sockets_ecoute_noeuds[i], &infos, sizeof(struct sInformations)) == -1) {
            printf("%s[SERVEUR] Erreur - Envoi des informations  %i/%i : %s\n%s", COLOR_RED, i, graph.nombre_sommets-1, strerror(errno), COLOR_WHITE);
            close(socket_attente_noeuds);
            for (int j=1; j<i; j++) {
                close(sockets_ecoute_noeuds[j]);
            }
            exit(1);
        }

        // Réception des adresses de voisin
        if (recvTCP(sockets_ecoute_noeuds[i], (void *) &adVoisins[i], sizeof(struct sockaddr_in)) == -1) {
            printf("%s[SERVEUR] Erreur - Réception de l'adresse de voisin %i/%i : %s\n%s", COLOR_RED, i, graph.nombre_sommets-1, strerror(errno), COLOR_WHITE);
            close(socket_attente_noeuds);
            for (int j=1; j<i; j++) {
                close(sockets_ecoute_noeuds[j]);
            }
            exit(1);
        }
    }
    close(socket_attente_noeuds);

    printf("%s[SERVEUR] Tous les noeuds reconnus\n%s", COLOR_MAGENTA, COLOR_WHITE);
    if (sleep_time != 0) {
        sleep(3);
    }

    /*
    ============== ENVOI DES ADRESSES ==============
    */
    
    // Envoie les voisins aux noeuds
    for (int i=1; i < graph.nombre_sommets; i++) {
        // Récupération de l'IP + port du noeud pour l'affichage
        char buffAddr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(adNoeuds[i].sin_addr), buffAddr, INET_ADDRSTRLEN);
        int buffPort = (int) ntohs(adNoeuds[i].sin_port);
        printf("%s[SERVEUR] Envoi des adresses (%i/%i) [%s:%i]\n%s", COLOR_MAGENTA, i, graph.nombre_sommets-1, buffAddr, buffPort, COLOR_WHITE);
        
        // Envoi des adresses
        for (int j = 0; j < graph.tab_degres_sortants[i]; j++) {
            struct sockaddr_in adresseVoisin = adVoisins[graph.tab_voisins_sortants[i][j]];
            if (sendTCP(sockets_ecoute_noeuds[i], (void*) &adresseVoisin, sizeof(struct sockaddr_in)) == -1) {
                printf("%s[SERVEUR] Erreur - Envoi de la %ie adresse au %ie noeud : %s\n%s", COLOR_RED, j+1, i, strerror(errno), COLOR_WHITE);
                for (int k=1; k<graph.nombre_sommets; k++) {
                    close(sockets_ecoute_noeuds[k]);
                };
                exit(1);
            }
        }
        if (i!= graph.nombre_sommets - 1 && sleep_time != 0 && graph.tab_degres_sortants[i] > 0) {
            sleep(3);
        }
    }
    printf("%s[SERVEUR] Toutes les adresses envoyées\n%s", COLOR_MAGENTA, COLOR_WHITE);


    /*
    ============== VALIDATION ==============
    */

    for (int i=1; i < graph.nombre_sommets; i++) {
        done = 0;
        while (done == 0) {
            ioctl(sockets_ecoute_noeuds[i], FIONREAD, &status);
            if (status > 0) {
                if (recvTCP(sockets_ecoute_noeuds[i], (void *)&done, sizeof(int)) == -1) {
                    printf("%s[SERVEUR] Erreur - Validation interconnexion des noeuds : %s\n%s", COLOR_RED, strerror(errno), COLOR_WHITE);
                    for (int k=1; k<graph.nombre_sommets; k++) {
                        close(sockets_ecoute_noeuds[k]);
                    };
                    exit(1);
                }
            }
        }
    }
    if (sleep_time != 0) {
        sleep(3);
    }
    else {
        sleep(1);
    }
    for (int i=1; i < graph.nombre_sommets; i++) {
        if (sendTCP(sockets_ecoute_noeuds[i], (void*) &one, sizeof(int)) == -1) {
            printf("%s[SERVEUR] Erreur - Validation interconnexion aux noeuds : %s\n%s", COLOR_RED, strerror(errno), COLOR_WHITE);
            for (int k=1; k<graph.nombre_sommets; k++) {
                close(sockets_ecoute_noeuds[k]);
            };
            exit(1);
        }
    }

    /*
    ============== RESULTATS COLORATION ==============
    */

    int receivedColors = 0;
    int maxColor = 0;
    while (receivedColors != graph.nombre_sommets-1) {
        int color;
        for (int i = 1; i < graph.nombre_sommets; i++) {
            ioctl(sockets_ecoute_noeuds[i], FIONREAD, &status);
            if (status > 0) {
                if (recvTCP(sockets_ecoute_noeuds[i], (void *)&color, sizeof(int)) == -1) {
                    printf("%s[SERVEUR] Erreur - Réception couleur finale : %s\n%s", COLOR_RED, strerror(errno), COLOR_WHITE);
                    for (int j = 0; j < graph.nombre_sommets; j++) {
                        close(sockets_ecoute_noeuds[j]);
                    }
                    exit(1);
                }
                if (color > maxColor){
                    maxColor = color;
                }
                receivedColors++;
            }
        }  
    }

    printf("%s[SERVEUR] Tous les noeuds colorés, couleur maximale : %i\n%s", COLOR_MAGENTA, maxColor, COLOR_WHITE);

    for (int i=1; i<graph.nombre_sommets; i++) {
        close(sockets_ecoute_noeuds[i]);
    };

    // Affichage du timer
    time(&end_t);
    diff_t = difftime(end_t, start_t);
    printf("%s[SERVEUR] Temps d'exécution : %i secondes\n%s", COLOR_MAGENTA, (int)diff_t, COLOR_WHITE);

    return 0;
}
