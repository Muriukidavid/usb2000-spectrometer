all:  spectroread

spectroread: spectroread.c
	gcc -Wall -Wno-unused-variable -O3 -o spectroread spectroread.c

clean:
	rm -f *~
	rm -f spectroread
