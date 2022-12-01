import os
import time

port = 10000
print("=================================================================")
print("Vitesse d'exécution ? [rapide (0) / lente (1)] :")
mode = str(input())
if (mode == "0") :
    sleeptime = 0.005
else :
    sleeptime = 5
print("Nom du fichier (à placer dans le dossier 'files') :")
filename = str(input())

file = open("files/" + filename, "r")
nSommets = 0
nAretes = 0
for line in file:
  if line.startswith("p"):
        nSommets = int(line.split(" ")[2])
        nAretes = int(line.split(" ")[3])
        break
if (nAretes > 65000) :
  print("Erreur - trop d'arêtes dans ce graphe (" + str(nAretes) + " > 65000)")
  print("=================================================================")
  exit()
print("\nGraphe à " + str(nSommets) +" sommets - Temps estimé = " + str(2*sleeptime*nSommets) + " secondes (~" + str(2*sleeptime) + "s/sommet)\n")
print("=================================================================")

file = '"' + filename + '"'
call = "./bin/serveur " + str(port) + " " + file +" " + mode + " &"
os.system(call)
time.sleep(sleeptime*nAretes/600)

for i in range(0, nSommets) :
  time.sleep(sleeptime)
  call = "./bin/noeud 127.0.0.1  " + str(port) + " " + str(i+1) + " &"
  os.system(call)

if (mode == "0") :
    time.sleep(nSommets*sleeptime*3)
else :
    time.sleep(nSommets*sleeptime+0.5)
