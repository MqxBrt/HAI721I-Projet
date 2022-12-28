#include "commun.h"
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <errno.h>

#define COLOR_RED "\x1B[31m"
#define COLOR_GREEN "\x1B[32m"
#define COLOR_ORANGE "\x1B[33m"
#define COLOR_BLUE "\x1B[34m"
#define COLOR_MAGENTA "\x1B[35m"
#define COLOR_CYAN "\x1B[36m"
#define COLOR_WHITE "\x1B[37m"

struct sInformations {
    int entrants;       // nombre de voisins entrants (à écouter)
    int sortants;       // nombre de voisins sortants (à se connecter)
    int flag;           // 1 si on a le token au début de la coloration, 0 sinon
};

struct sMessage {
    int demandeCouleur;     // 1 si on veut la couleur, 0 sinon
    int reponseCouleur;     // 1 si on donne notre couleur, 0 sinon
    int couleur;            // 0 si pas coloré, coloré sinon
    int demandeToken;       // 1 si on veut le token, 0 sinon
    int passageToken;       // 1 si on donne le token, 0 sinon
    int estColore;          // 1 si on a fini sa coloration, 0 sinon
};

struct sAdresse {
    int tableau;    // 0 si entrants, 1 si sortants
    int indice;     // indice dans le tableau
};

/* ##### Programme noeud ##### */

int main(int argc, char *argv[]) {
    // Message d'erreur en cas de paramètres incorrects
    if (argc != 4) {
        printf("%s[NOEUD] Erreur - Paramètres incorrects\n[NOEUD] Utilisation : %s ip_serveur port_serveur id_noeud\n%s", COLOR_RED, argv[0], COLOR_WHITE);
        exit(1);
    }

    /*
    ============== INITIALISATION ==============
    */

    // Création des sockets
    int socket_connexion_serveur = createSocket();
    int socket_attente_voisins = createSocket();

    char *id = argv[3];
    int port = atoi(argv[2]);

    int done = 0;
    int status = 0;
    int one = 1;

    struct sInformations infos;

    // Nommage des sockets
    if (nameSocketAddress(socket_attente_voisins, 0) == -1) {
        printf("%s[NOEUD %s] Erreur - Nommage de la socket d'écoute des voisins : %s\n%s", COLOR_RED, id, strerror(errno), COLOR_WHITE);
        close(socket_attente_voisins);
        exit(1);
    }
    struct sockaddr_in adEcouteVoisins;
    int taille_sockaddr_in;
    taille_sockaddr_in = sizeof(adEcouteVoisins);
    getsockname(socket_attente_voisins, (struct sockaddr *)&adEcouteVoisins, &taille_sockaddr_in);

    /*
    ============== CONNEXION AU SERVEUR ==============
    */

    // Connexion au serveur
    printf("%s[NOEUD %s] Connexion au serveur\n%s", COLOR_CYAN, id, COLOR_WHITE);
    struct sockaddr_in adresse_serveur = designateSocket(argv[1], port);
    socklen_t lgA = sizeof(struct sockaddr_in);
    if (TCPConnect(socket_connexion_serveur, &adresse_serveur) == -1) {
        printf("%s[NOEUD %s] Erreur - Connexion au serveur : %s\n%s", COLOR_RED, id, strerror(errno), COLOR_WHITE);
        close(socket_attente_voisins);
        close(socket_connexion_serveur);
        exit(1);
    }

    // Réception des informations du serveur
    if (recvTCP(socket_connexion_serveur, &infos, sizeof(struct sInformations)) == -1) {
        printf("%s[NOEUD %s] Erreur - Réception du nombre de voisins entrants : %s\n%s", COLOR_RED, id, strerror(errno), COLOR_WHITE);
        close(socket_connexion_serveur);
        close(socket_attente_voisins);
        exit(1);
    }
    int nombre_voisins_entrants = infos.entrants;
    int nombre_voisins_sortants = infos.sortants;
    int topFlag = infos.flag;

    // Mise en écoute de la socket des voisins
    if (nombre_voisins_entrants > 0) {
        if (TCPListen(socket_attente_voisins, nombre_voisins_entrants) == -1) {
            printf("%s[NOEUD %s] Erreur - Mise en écoute de la socket voisins : %s\n%s", COLOR_RED, id, strerror(errno), COLOR_WHITE);
            close(socket_connexion_serveur);
            close(socket_attente_voisins);
            exit(1);
        }
    }
    // Envoi de l'adresse au serveur
    if (sendTCP(socket_connexion_serveur, (void *)&adEcouteVoisins, sizeof(struct sockaddr_in)) == -1) {
        printf("%s[NOEUD %s] Erreur - Envoi de l'adresse de voisin au serveur : %s\n%s", COLOR_RED, id, strerror(errno), COLOR_WHITE);
        close(socket_connexion_serveur);
        close(socket_attente_voisins);
        exit(1);
    }

    /*
    ============== RECEPTION DES ADRESSES ==============
    */

    int *sockets_connexion_voisins;
    // Réception des adresses des voisins
    struct sockaddr_in *adresses_voisins_sortants = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in) * nombre_voisins_sortants);
    for (int i = 0; i < nombre_voisins_sortants; i++) {
        done = 0;
        while (done == 0) {
            ioctl(socket_connexion_serveur, FIONREAD, &status);
            if (status > 0) {
                if (recvTCP(socket_connexion_serveur, (void *)&adresses_voisins_sortants[i], sizeof(struct sockaddr_in)) == -1) {
                    printf("%s[NOEUD %s] Erreur - Réception de l'adresse %i/%i : %s\n%s", COLOR_RED, id, i + 1, nombre_voisins_sortants, strerror(errno), COLOR_WHITE);
                    close(socket_connexion_serveur);
                    close(socket_attente_voisins);
                    exit(1);
                }
                done = 1;
            }
        }
        if (i == 0) {
            printf("%s[NOEUD %s] Réception des adresses\n%s", COLOR_CYAN, id, COLOR_WHITE);
        }
    }

    /*
    ============== CONNEXION AUX VOISINS ==============
    */

    printf("%s[NOEUD %s] Adresses reçues, communication avec les voisins\n%s", COLOR_CYAN, id, COLOR_WHITE);
    // Connexion aux voisins
    if (nombre_voisins_sortants > 0) {
        // Création des sockets pour se connecter à ses voisins
        sockets_connexion_voisins = (int *)malloc(sizeof(int) * nombre_voisins_sortants);
        for (int i = 0; i < nombre_voisins_sortants; i++) {
            sockets_connexion_voisins[i] = createSocket();
        }

        for (int i = 0; i < nombre_voisins_sortants; i++) {
            if (TCPConnect(sockets_connexion_voisins[i], &adresses_voisins_sortants[i]) == -1) {
                printf("%s[NOEUD %s] Erreur - Connexion au voisin %i/%i : %s\n%s", COLOR_RED, id, i + 1, nombre_voisins_sortants, strerror(errno), COLOR_WHITE);
                close(socket_connexion_serveur);
                close(socket_attente_voisins);
                for (int j = 0; j < nombre_voisins_sortants; j++) {
                    close(sockets_connexion_voisins[j]);
                }
                exit(1);
            }
        }
    }

    // Acceptation des voisins
    struct sockaddr_in *adresses_voisins_entrants = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in) * nombre_voisins_entrants);
    int *sockets_ecoute_voisins = (int *)malloc(sizeof(int) * nombre_voisins_entrants);
    for (int i = 0; i < nombre_voisins_entrants; i++) {
        if ((sockets_ecoute_voisins[i] = accept(socket_attente_voisins, (struct sockaddr *)&adresses_voisins_entrants[i], &lgA)) == -1) {
            printf("%s[NOEUD %s] Erreur - Acceptation du voisin %i/%i : %s\n%s", COLOR_RED, id, i + 1, nombre_voisins_entrants, strerror(errno), COLOR_WHITE);
            close(socket_connexion_serveur);
            close(socket_attente_voisins);
            for (int j = 0; j < nombre_voisins_sortants; j++) {
                close(sockets_connexion_voisins[j]);
            }
            for (int j = 0; j < i; j++) {
                close(sockets_ecoute_voisins[j]);
            }
            exit(1);
        }
    }
    close(socket_attente_voisins);

    printf("%s[NOEUD %s] Connecté à tous les voisins\n%s", COLOR_GREEN, id, COLOR_WHITE);

    /*
    ============== VALIDATION ==============
    */

    // Validation de l'interconnexion côté noeud
    if (sendTCP(socket_connexion_serveur, (void *)&one, sizeof(int)) == -1) {
        printf("%s[NOEUD %s] Erreur - Validation interconnexion au serveur : %s\n%s", COLOR_RED, id, strerror(errno), COLOR_WHITE);
        close(socket_connexion_serveur);
        for (int i = 0; i < nombre_voisins_entrants; i++) {
            close(sockets_ecoute_voisins[i]);
        }
        for (int i = 0; i < nombre_voisins_sortants; i++) {
            close(sockets_connexion_voisins[i]);
        }
        exit(1);
    }
    // Validation de l'interconnexion côté serveur
    while (done == 0) {
        ioctl(socket_connexion_serveur, FIONREAD, &status);
        if (status > 0) {
            if (recvTCP(socket_connexion_serveur, (void *)&done, sizeof(int)) == -1) {
                printf("%s[NOEUD %s] Erreur - Validation interconnexion du serveur : %s\n%s", COLOR_RED, id, strerror(errno), COLOR_WHITE);
                close(socket_connexion_serveur);
                for (int i = 0; i < nombre_voisins_entrants; i++) {
                    close(sockets_ecoute_voisins[i]);
                }
                for (int i = 0; i < nombre_voisins_sortants; i++) {
                    close(sockets_connexion_voisins[i]);
                }
                exit(1);
            }
        }
    }

    /*
    ============== COLORATION ==============
    */

    // Messages génériques
    struct sMessage requestColors = {1, 0, 0, 0, 0, 0};
    struct sMessage requestToken = {0, 0, 0, 1, 0, 0};
    struct sMessage passToken = {0, 0, 0, 0, 1, 0};
    struct sMessage iAmColored = {0, 0, 0, 0, 0, 1};

    // Variables pour la coloration
    int hasToken = topFlag;     // 1 si on a le token, 0 sinon
    int voisins_colores = 0;    // nombre de voisins colorés
    int isColored = 0;          // 1 si on est coloré, 0 sinon
    int myColor = 0;            // valeur de notre couleur
    int receivedAll = 0;        // 1 si on a reçu toutes les couleurs de nos voisins, 0 sinon
    struct sAdresse suivants[nombre_voisins_sortants + nombre_voisins_entrants];        // tableau des suivants
    int indice_suivant = 0;     // indice du prochain suivant dans le tableau
    int max_suivant = 0;        // nombre maximal de suivants dans le tableau
    struct sAdresse colores[nombre_voisins_sortants + nombre_voisins_entrants];         // tableau des voisins colorés
    int indice_colore = 0;      // plus grand indice du voisin coloré
    int* couleurs_voisins = (int *)malloc(sizeof(int) * (nombre_voisins_sortants + nombre_voisins_entrants));   // tableau des couleurs de nos voisins
    int couleurs_recues = 0;    // nombre de couleurs reçues
    struct sAdresse predecesseur;   // prédécesseur (celui qui nous a donné le token en premier)
    int checkAll = 1;           // 1 si aucun voisin ne s'est coloré pendant ce tour de boucle, 0 sinon
    int backToSender = topFlag;     // 1 si on a renud le token au prédécesseur, 0 sinon

    // Début de la coloration en fonction du top flag
    if (hasToken == 1) {
        // Demande des couleurs
        for (int i = 0; i < nombre_voisins_entrants; i++) {
            if (sendTCP(sockets_ecoute_voisins[i], &requestColors, sizeof(struct sMessage)) == -1) {
                printf("%s[NOEUD %s] Erreur - Demande de couleur : %s\n%s", COLOR_RED, id, strerror(errno), COLOR_WHITE);
                close(socket_connexion_serveur);
                for (int j = 0; j < nombre_voisins_entrants; j++) {
                    close(sockets_ecoute_voisins[j]);
                }
                for (int j = 0; j < nombre_voisins_sortants; j++) {
                    close(sockets_connexion_voisins[j]);
                }
                exit(1);
            }
        }
        for (int i = 0; i < nombre_voisins_sortants; i++) {
            if (sendTCP(sockets_connexion_voisins[i], &requestColors, sizeof(struct sMessage)) == -1) {
                printf("%s[NOEUD %s] Erreur - Demande de couleur : %s\n%s", COLOR_RED, id, strerror(errno), COLOR_WHITE);
                close(socket_connexion_serveur);
                for (int j = 0; j < nombre_voisins_entrants; j++) {
                    close(sockets_ecoute_voisins[j]);
                }
                for (int j = 0; j < nombre_voisins_sortants; j++) {
                    close(sockets_connexion_voisins[j]);
                }
                exit(1);
            }
        }
    }
    else {
        // Demande du token
        for (int i=0; i<nombre_voisins_entrants; i++) {
            if (sendTCP(sockets_ecoute_voisins[i], &requestToken, sizeof(struct sMessage)) == -1) {
                printf("%s[NOEUD %s] Erreur - Demande de token : %s\n%s", COLOR_RED, id, strerror(errno), COLOR_WHITE);
                close(socket_connexion_serveur);
                for (int j = 0; j < nombre_voisins_entrants; j++) {
                    close(sockets_ecoute_voisins[j]);
                }
                for (int j = 0; j < nombre_voisins_sortants; j++) {
                    close(sockets_connexion_voisins[j]);
                }
                exit(1);
            }
        }
        for (int i=0; i<nombre_voisins_sortants; i++) {
            if (sendTCP(sockets_connexion_voisins[i], &requestToken, sizeof(struct sMessage)) == -1) {
                printf("%s[NOEUD %s] Erreur - Demande de token : %s\n%s", COLOR_RED, id, strerror(errno), COLOR_WHITE);
                close(socket_connexion_serveur);
                for (int j = 0; j < nombre_voisins_entrants; j++) {
                    close(sockets_ecoute_voisins[j]);
                }
                for (int j = 0; j < nombre_voisins_sortants; j++) {
                    close(sockets_connexion_voisins[j]);
                }
                exit(1);
            }
        }
    }

    // Boucle tant que finissant lorsqu'un sommet et tous ses voisins sont colorés
    while (voisins_colores < nombre_voisins_entrants+nombre_voisins_sortants || isColored == 0 || backToSender == 0) {
        
        /*
        ============== TRAITEMENT EXTERNE ==============
        */

        checkAll = 1;
        struct sMessage message_recu;
        for (int i = 0; i < nombre_voisins_entrants; i++) {
            status = 1;
            while (status > 0) { 
                ioctl(sockets_ecoute_voisins[i], FIONREAD, &status);
                if (status > 0) {
                    if (recvTCP(sockets_ecoute_voisins[i], (void *)&message_recu, sizeof(struct sMessage)) == -1) {
                        printf("%s[NOEUD %s] Erreur - Réception de message : %s\n%s", COLOR_RED, id, strerror(errno), COLOR_WHITE);
                        close(socket_connexion_serveur);
                        for (int j = 0; j < nombre_voisins_entrants; j++) {
                            close(sockets_ecoute_voisins[j]);
                        }
                        for (int j = 0; j < nombre_voisins_sortants; j++) {
                            close(sockets_connexion_voisins[j]);
                        }
                        exit(1);
                    }
                    // Si notre voisin s'est coloré (condition d'arrêt)
                    if (message_recu.estColore == 1) {
                        voisins_colores++;
                        struct sAdresse temp = {0, i};
                        colores[indice_colore] = temp;
                        indice_colore++;
                        continue;
                    }
                    // Si je reçois une demande de couleur
                    if (message_recu.demandeCouleur == 1) {
                        // Renvoie ma couleur
                        struct sMessage giveColor = {0, 1, myColor, 0, 0, 0};
                        if (sendTCP(sockets_ecoute_voisins[i], &giveColor, sizeof(struct sMessage)) == -1) {
                            printf("%s[NOEUD %s] Erreur - Envoi de la couleur : %s\n%s", COLOR_RED, id, strerror(errno), COLOR_WHITE);
                            close(socket_connexion_serveur);
                            for (int j = 0; j < nombre_voisins_entrants; j++) {
                                close(sockets_ecoute_voisins[j]);
                            }
                            for (int j = 0; j < nombre_voisins_sortants; j++) {
                                close(sockets_connexion_voisins[j]);
                            }
                            exit(1);
                        }
                        continue;
                    }
                    // Si je reçois une demande de token
                    if (message_recu.demandeToken == 1) {
                        // Ajoute le suivant à la liste
                        struct sAdresse temp = {0, i};
                        suivants[max_suivant] = temp;
                        max_suivant++;
                        continue;
                    }
                    // Si je reçois une couleur
                    if (message_recu.reponseCouleur == 1) {
                        couleurs_voisins[couleurs_recues] = message_recu.couleur;
                        couleurs_recues++;
                        // Actualisation du statut de réception des couleurs
                        if (couleurs_recues == nombre_voisins_entrants + nombre_voisins_sortants) {
                            receivedAll = 1;
                        }
                        continue;
                    }
                    // Si je reçois le token
                    if (message_recu.passageToken == 1) {
                        hasToken = 1;
                        // Si je ne suis pas coloré
                        if (isColored == 0) {
                            // Demande des couleurs
                            for (int i = 0; i < nombre_voisins_entrants; i++) {
                                if (sendTCP(sockets_ecoute_voisins[i], &requestColors, sizeof(struct sMessage)) == -1) {
                                    printf("%s[NOEUD %s] Erreur - Demande de couleur : %s\n%s", COLOR_RED, id, strerror(errno), COLOR_WHITE);
                                    close(socket_connexion_serveur);
                                    for (int j = 0; j < nombre_voisins_entrants; j++) {
                                        close(sockets_ecoute_voisins[j]);
                                    }
                                    for (int j = 0; j < nombre_voisins_sortants; j++) {
                                        close(sockets_connexion_voisins[j]);
                                    }
                                    exit(1);
                                }
                            }
                            for (int i = 0; i < nombre_voisins_sortants; i++) {
                                if (sendTCP(sockets_connexion_voisins[i], &requestColors, sizeof(struct sMessage)) == -1) {
                                    printf("%s[NOEUD %s] Erreur - Demande de couleur : %s\n%s", COLOR_RED, id, strerror(errno), COLOR_WHITE);
                                    close(socket_connexion_serveur);
                                    for (int j = 0; j < nombre_voisins_entrants; j++) {
                                        close(sockets_ecoute_voisins[j]);
                                    }
                                    for (int j = 0; j < nombre_voisins_sortants; j++) {
                                        close(sockets_connexion_voisins[j]);
                                    }
                                    exit(1);
                                }
                            }
                            struct sAdresse t = {0, i}; 
                            predecesseur = t;
                        }
                        // Si je suis coloré
                        else {
                            checkAll = 0;
                        } 
                        continue;
                    }
                }
            } 
        }
        for (int i = 0; i < nombre_voisins_sortants; i++) {
            status = 1;
            while (status > 0) { 
                ioctl(sockets_connexion_voisins[i], FIONREAD, &status);
                if (status > 0) {
                    if (recvTCP(sockets_connexion_voisins[i], (void *)&message_recu, sizeof(struct sMessage)) == -1) {
                        printf("%s[NOEUD %s] Erreur - Réception de message : %s\n%s", COLOR_RED, id, strerror(errno), COLOR_WHITE);
                        close(socket_connexion_serveur);
                        for (int j = 0; j < nombre_voisins_entrants; j++) {
                            close(sockets_ecoute_voisins[j]);
                        }
                        for (int j = 0; j < nombre_voisins_sortants; j++) {
                            close(sockets_connexion_voisins[j]);
                        }
                        exit(1);
                    }
                    // Si notre voisin s'est coloré (condition d'arrêt)
                    if (message_recu.estColore == 1) {
                        voisins_colores++;
                        struct sAdresse temp = {1, i};
                        colores[indice_colore] = temp;
                        indice_colore++;
                        continue;
                    }
                    // Si je reçois une demande de couleur
                    if (message_recu.demandeCouleur == 1) {
                        // Renvoie ma couleur
                        struct sMessage giveColor = {0, 1, myColor, 0, 0, 0};
                        if (sendTCP(sockets_connexion_voisins[i], &giveColor, sizeof(struct sMessage)) == -1) {
                            printf("%s[NOEUD %s] Erreur - Envoi de la couleur : %s\n%s", COLOR_RED, id, strerror(errno), COLOR_WHITE);
                            close(socket_connexion_serveur);
                            for (int j = 0; j < nombre_voisins_entrants; j++) {
                                close(sockets_ecoute_voisins[j]);
                            }
                            for (int j = 0; j < nombre_voisins_sortants; j++) {
                                close(sockets_connexion_voisins[j]);
                            }
                            exit(1);
                        }
                        continue;
                    }
                    // Si je reçois une demande de token
                    if (message_recu.demandeToken == 1) {
                        // Ajoute le suivant à la liste
                        struct sAdresse temp = {1, i};
                        suivants[max_suivant] = temp;
                        max_suivant++;
                        continue;
                    }
                    // Si je reçois une couleur
                    if (message_recu.reponseCouleur == 1) {
                        couleurs_voisins[couleurs_recues] = message_recu.couleur;
                        couleurs_recues++;
                        // Actualisation du statut de réception des couleurs
                        if (couleurs_recues == nombre_voisins_entrants + nombre_voisins_sortants) {
                            receivedAll = 1;
                        }
                        continue;
                    }
                    // Si je reçois le token
                    if (message_recu.passageToken == 1) {
                        hasToken = 1;
                        // Si je ne suis pas coloré
                        if (isColored == 0) {
                            // Demande des couleurs
                            for (int i = 0; i < nombre_voisins_entrants; i++) {
                                if (sendTCP(sockets_ecoute_voisins[i], &requestColors, sizeof(struct sMessage)) == -1) {
                                    printf("%s[NOEUD %s] Erreur - Demande de couleur : %s\n%s", COLOR_RED, id, strerror(errno), COLOR_WHITE);
                                    close(socket_connexion_serveur);
                                    for (int j = 0; j < nombre_voisins_entrants; j++) {
                                        close(sockets_ecoute_voisins[j]);
                                    }
                                    for (int j = 0; j < nombre_voisins_sortants; j++) {
                                        close(sockets_connexion_voisins[j]);
                                    }
                                    exit(1);
                                }
                            }
                            for (int i = 0; i < nombre_voisins_sortants; i++) {
                                if (sendTCP(sockets_connexion_voisins[i], &requestColors, sizeof(struct sMessage)) == -1) {
                                    printf("%s[NOEUD %s] Erreur - Demande de couleur : %s\n%s", COLOR_RED, id, strerror(errno), COLOR_WHITE);
                                    close(socket_connexion_serveur);
                                    for (int j = 0; j < nombre_voisins_entrants; j++) {
                                        close(sockets_ecoute_voisins[j]);
                                    }
                                    for (int j = 0; j < nombre_voisins_sortants; j++) {
                                        close(sockets_connexion_voisins[j]);
                                    }
                                    exit(1);
                                }
                            }
                            struct sAdresse t = {1, i}; 
                            predecesseur = t;
                        }
                        // Si je suis coloré
                        else {
                            checkAll = 0;
                        } 
                        continue;
                    }
                }
            } 
        }
        
        /*
        ============== TRAITEMENT INTERNE ==============
        */

        // Si des suivants sont à enlever
        if (indice_colore > 0) { 
            for (int i=indice_suivant; i<max_suivant; i++) {
                for (int j=0; j<indice_colore; j++) {
                    if (suivants[i].tableau == colores[j].tableau && suivants[i].indice == colores[j].indice) {
                        // On supprime le voisin des suivants
                        for (int k=i+1; k<max_suivant; k++) {
                            suivants[k-1] = suivants[k];
                        }
                        max_suivant--;
                    }
                }
            }
            indice_colore = 0;
        } 

        // J'ai tout reçu, je ne suis pas coloré et j'ai le token
        if (receivedAll == 1 && isColored == 0 && hasToken == 1) { 
            printf("%s[NOEUD %s] Token possédé, coloration en cours\n%s", COLOR_CYAN, id, COLOR_WHITE);
            int tempColor = 1;
            int over = 0;
            while (over == 0) {
                over = 1;
                for (int i=0; i < couleurs_recues; i++) {
                    if (couleurs_voisins[i] == tempColor) {
                        tempColor++;
                        over = 0;
                        continue;
                    }
                }
            }
            myColor = tempColor;
            isColored = 1;
            // Broadcast de ma coloration à tous mes voisins
            for (int i = 0; i < nombre_voisins_entrants; i++) {
                if (sendTCP(sockets_ecoute_voisins[i], &iAmColored, sizeof(struct sMessage)) == -1) {
                    printf("%s[NOEUD %s] Erreur - Broadcast de la coloration : %s\n%s", COLOR_RED, id, strerror(errno), COLOR_WHITE);
                    close(socket_connexion_serveur);
                    for (int j = 0; j < nombre_voisins_entrants; j++) {
                        close(sockets_ecoute_voisins[j]);
                    }
                    for (int j = 0; j < nombre_voisins_sortants; j++) {
                        close(sockets_connexion_voisins[j]);
                    }
                    exit(1);
                }
            }
            for (int i = 0; i < nombre_voisins_sortants; i++) {
                if (sendTCP(sockets_connexion_voisins[i], &iAmColored, sizeof(struct sMessage)) == -1) {
                    printf("%s[NOEUD %s] Erreur - Broadcast de la coloration : %s\n%s", COLOR_RED, id, strerror(errno), COLOR_WHITE);
                    close(socket_connexion_serveur);
                    for (int j = 0; j < nombre_voisins_entrants; j++) {
                        close(sockets_ecoute_voisins[j]);
                    }
                    for (int j = 0; j < nombre_voisins_sortants; j++) {
                        close(sockets_connexion_voisins[j]);
                    }
                    exit(1);
                }
            }
        }

        // J'ai le token et je suis déjà coloré et j'ai vérifié qu'aucun suivant a terminé durant ce tour de boucle
        if (hasToken == 1 && isColored == 1 && checkAll == 1) { 
            // Si j'ai un suivant
            if (indice_suivant < max_suivant) {
                // Donne le token au suivant
                if (suivants[indice_suivant].tableau == 0) {
                    if (sendTCP(sockets_ecoute_voisins[suivants[indice_suivant].indice], &passToken, sizeof(struct sMessage)) == -1) {
                        printf("%s[NOEUD %s] Erreur - Envoi du token : %s\n%s", COLOR_RED, id, strerror(errno), COLOR_WHITE);
                        close(socket_connexion_serveur);
                        for (int j = 0; j < nombre_voisins_entrants; j++) {
                            close(sockets_ecoute_voisins[j]);
                        }
                        for (int j = 0; j < nombre_voisins_sortants; j++) {
                            close(sockets_connexion_voisins[j]);
                        }
                        exit(1);
                    }
                }
                else {
                    if (sendTCP(sockets_connexion_voisins[suivants[indice_suivant].indice], &passToken, sizeof(struct sMessage)) == -1) {
                        printf("%s[NOEUD %s] Erreur - Envoi du token : %s\n%s", COLOR_RED, id, strerror(errno), COLOR_WHITE);
                        close(socket_connexion_serveur);
                        for (int j = 0; j < nombre_voisins_entrants; j++) {
                            close(sockets_ecoute_voisins[j]);
                        }
                        for (int j = 0; j < nombre_voisins_sortants; j++) {
                            close(sockets_connexion_voisins[j]);
                        }
                        exit(1);
                    }
                }
                indice_suivant++;
                hasToken = 0;
            }
            // Si je n'ai pas de suivant
            else {
                // Si j'ai un prédécesseur
                if (topFlag == 0) {
                    // Donne le token au prédécesseur
                    if (predecesseur.tableau == 0) {
                        if (sendTCP(sockets_ecoute_voisins[predecesseur.indice], &passToken, sizeof(struct sMessage)) == -1) {
                            printf("%s[NOEUD %s] Erreur - Envoi du token : %s\n%s", COLOR_RED, id, strerror(errno), COLOR_WHITE);
                            close(socket_connexion_serveur);
                            for (int j = 0; j < nombre_voisins_entrants; j++) {
                                close(sockets_ecoute_voisins[j]);
                            }
                            for (int j = 0; j < nombre_voisins_sortants; j++) {
                                close(sockets_connexion_voisins[j]);
                            }
                            exit(1);
                        }
                    }
                    else {
                        if (sendTCP(sockets_connexion_voisins[predecesseur.indice], &passToken, sizeof(struct sMessage)) == -1) {
                            printf("%s[NOEUD %s] Erreur - Envoi du token : %s\n%s", COLOR_RED, id, strerror(errno), COLOR_WHITE);
                            close(socket_connexion_serveur);
                            for (int j = 0; j < nombre_voisins_entrants; j++) {
                                close(sockets_ecoute_voisins[j]);
                            }
                            for (int j = 0; j < nombre_voisins_sortants; j++) {
                                close(sockets_connexion_voisins[j]);
                            }
                            exit(1);
                        }
                    }
                    hasToken = 0;
                    backToSender = 1;
                }
            }
        }
    }

    /*
    ============== RESULTATS ==============
    */

    // Affiche du résultat de la coloration et envoi au serveur
    printf("%s[NOEUD %s] Couleur = %i\n%s", COLOR_GREEN, id, myColor, COLOR_WHITE);
    if (sendTCP(socket_connexion_serveur, &myColor, sizeof(int)) == -1) {
        printf("%s[NOEUD %s] Erreur - Envoi de la couleur finale : %s\n%s", COLOR_RED, id, strerror(errno), COLOR_WHITE);
        close(socket_connexion_serveur);
        for (int j = 0; j < nombre_voisins_entrants; j++) {
            close(sockets_ecoute_voisins[j]);
        }
        for (int j = 0; j < nombre_voisins_sortants; j++) {
            close(sockets_connexion_voisins[j]);
        }
        exit(1);
    }

    close(socket_connexion_serveur);
    for (int j = 0; j < nombre_voisins_entrants; j++) {
        close(sockets_ecoute_voisins[j]);
    }
    for (int j = 0; j < nombre_voisins_sortants; j++) {
        close(sockets_connexion_voisins[j]);
    }

    return 0;
}
