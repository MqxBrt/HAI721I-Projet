compile :
	clear
	rm -f bin/*.o
	mkdir -p bin
	gcc serveur.c -o bin/serveur
	gcc noeud.c -o bin/noeud

run :
	make compile
	python3 script.py

server :
	clear
	./bin/serveur 10000 anneau3.txt 1

node :
	clear
	./bin/noeud 