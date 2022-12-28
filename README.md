
# [HAI721I] Projet Coloration de graphe distribuée

Jérémie BENTOLILA, Maxime BOURRET

## Téléchargement
1. Ouvrir un terminal dans le dossier de destination
2. Excécuter la commande suivante
```bash
git clone https://github.com/MqxBrt/HAI721I-Projet
```

## Compilation
1. Ouvrir un terminal dans le dossier téléchargé
2. Exécuter la commande suivante
```bash
make compile
```

## Exécution
1. Ouvrir un terminal dans le dossier téléchargé
2. Exécuter la commande suivante
```bash
make run
```
3. Choisir une vitesse d'exécution (0 = aucun sleep, 1 = sleep de 3 secondes entre chaque lancement de nœud)
4. Saisir un nom de fichier compatible contenu dans le dossier 'files'

## Erreur possible
```
eno1: erreur lors de la recherche d'infos sur l'interface: Périphérique non trouvé
```
Si l'erreur ci-dessus survient, remplacez les occurences de '**eno1**' dans les fichiers *serveur.c* et *commun.h* par la balise affichée lors de l'exécution de la commande ci-dessous
```bash
ifconfig
```