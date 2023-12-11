VERSION=0.0

albatwid: main.c
	cc -std=c99 -pedantic -Wall -o $@ -DVERSION=$V $^

test: albatwid
	! ./albatwid --nonexistent-flag
	./albatwid
