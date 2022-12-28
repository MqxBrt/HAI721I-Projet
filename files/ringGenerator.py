n = int(input())
f = open("anneau" + str(n) + ".txt", "a")
f.write("p edge " + str(n) + " " + str(2*n)+ "\n")
for i in range (1, n) :
	f.write("e " + str(i) + " " + str(i+1) + "\n")
f.write("e " + str(n) + " " + str(1))
f.close()