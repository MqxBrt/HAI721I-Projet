import os
import time

sleeptime = 0.005
port = 10000
print("=================================================================")
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
call = "./bin/serveur " + str(port) + " " + file +" &"
os.system(call)
time.sleep(sleeptime*nAretes/600)

for i in range(0, nSommets) :
  time.sleep(sleeptime)
  call = "./bin/noeud 127.0.0.1  " + str(port) + " " + str(i+1) + " &"
  os.system(call)

time.sleep(nSommets*sleeptime*3)
