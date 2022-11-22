compile :
	clear
	rm -f bin/*.o
	mkdir -p bin
	gcc serveur.c -o bin/serveur
	gcc noeud.c -o bin/noeud

run :
	make compile
	python3 script.py
