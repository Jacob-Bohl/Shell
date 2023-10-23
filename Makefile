cssh: cssh.c
	gcc -std=c99 -Wall -g -o cssh cssh.c

clean:
	rm -rf cssh
