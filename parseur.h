#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define COLOR_RED "\x1B[31m"
#define COLOR_GREEN "\x1B[32m"
#define COLOR_ORANGE "\x1B[33m"
#define COLOR_BLUE "\x1B[34m"
#define COLOR_MAGENTA "\x1B[35m"
#define COLOR_CYAN "\x1B[36m"
#define COLOR_WHITE  "\x1B[37m"

// Structure de retour
struct graphe {
    int ** tab_voisins_entrants;
    int ** tab_voisins_sortants;
    int * tab_degres_entrants;
    int * tab_degres_sortants;
    int nombre_sommets;
};

// Fonction de découpage de string
char** str_split(char* a_str, const char a_delim) {
    char** result = 0;
    size_t count = 0;
    char* tmp = a_str;
    char* last_comma = 0;
    char delim[2];
    delim[0] = a_delim;
    delim[1] = 0;
    while (*tmp) {
        if (a_delim == *tmp) {
            count++;
            last_comma = tmp;
        }
        tmp++;
    }
    count += last_comma < (a_str + strlen(a_str) - 1);
    count++;
    result = malloc(sizeof(char*) * count);
    if (result) {
        size_t idx  = 0;
        char* token = strtok(a_str, delim);
        while (token) {
            assert(idx < count);
            *(result + idx++) = strdup(token);
            token = strtok(0, delim);
        }
        assert(idx == count - 1);
        *(result + idx) = 0;
    }
    return result;
}

// Fonction d'affichage de tableau
void printTab(int* tab, int size){
  printf("[");
  for (int i = 0; i < size-1 ; i++){
    printf(" %i ,",tab[i]);
  }
  printf(" %i ]\n",tab[size-1]);
}

// Parseur
struct graphe parser(char * filename) {
    // Définition des variables utilisées par la suite
    char * folder = malloc(strlen(filename) + 16);
    folder[0] = '\0';
    strcat(folder, "./files/");
    filename = strcat(folder, filename); 
    FILE *input_file = fopen(filename, "r");
    char * line = NULL;
    size_t len = 0;
    ssize_t read;

    // Vérification de l'existence du fichier
    if (input_file == NULL) {
        printf("%s[PARSEUR] Erreur - Impossible d'ouvrir le fichier %s\n%s", COLOR_RED, filename, COLOR_WHITE);
        exit(1);
    }
    // Ouverture réalisée avec succès
    else {
        printf("%s[PARSEUR] Ouverture de %s réussie, parsage en cours\n%s", COLOR_ORANGE, filename, COLOR_WHITE);
        int tab_size;
        char ** infos;
        int * tailles_entrant; // nb voisins à écouter
        int * tailles_sortant; // nb voisins à se connecter
        // Lecture ligne par ligne
        // 1er tour pour récupérer les tailles des listes d'aretes
        while ((read = getline(&line, &len, input_file)) != -1) {
            if (line[0] == 'c'){ 
                continue;
            } 
            else if (line[0] == 'p'){
                infos = str_split(line, ' ');
                tab_size = atoi(infos[2]) + 1;
                tailles_entrant = (int*) malloc(sizeof(int) * tab_size);
                tailles_sortant = (int*) malloc(sizeof(int) * tab_size);
            } 
            else if (line[0] == 'e'){
                infos = str_split(line, ' ');
                tailles_sortant[atoi(infos[1])]++;
                tailles_entrant[atoi(infos[2])]++;

            } 
        }
        rewind(input_file);
        // Déclarer le tableau de tableaux
        int ** graph_entrant = (int**) malloc(sizeof(int*) * tab_size);
        int ** graph_sortant = (int**) malloc(sizeof(int*) * tab_size);
        for (int i = 0; i < tab_size; i++){
            graph_entrant[i]= (int*) malloc(sizeof(int) * tailles_entrant[i]);
            graph_sortant[i]= (int*) malloc(sizeof(int) * tailles_sortant[i]);
        } 

        int index1 = 0;
        int index2 = 0;
        int vertex1 = 0;
        int vertex2 = 0;
        // 2ème tour pour assigner les aretes
        while ((read = getline(&line, &len, input_file)) != -1) {
            if (line[0] == 'e'){
                infos = str_split(line, ' ');
                if (atoi(infos[1]) != vertex1){ // on passe à un nouveau sommet
                    index1 = 0;
                }
                vertex1 = atoi(infos[1]);
                if (atoi(infos[2]) != vertex2){ // on passe à un nouveau sommet
                    index2 = 0;
                }
                vertex2 = atoi(infos[2]);

                while (graph_sortant[vertex1][index1] != 0) index1++;
                graph_sortant[vertex1][index1] = vertex2;
                while (graph_entrant[vertex2][index2] != 0) index2++;
                graph_entrant[vertex2][index2] = vertex1;    
            }
        }
        
        // Fermeture et libération
        if (infos) {
            free(infos);
        } 
        fclose(input_file);
        if (line) {
            free(line);
        }
        printf("%s[PARSEUR] Parsage terminé\n%s", COLOR_ORANGE, COLOR_WHITE);
        struct graphe resStruct = {graph_entrant, graph_sortant, tailles_entrant, tailles_sortant, tab_size}; 
        return resStruct;
    }
}