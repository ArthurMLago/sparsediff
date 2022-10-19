all: sparsediff

sparsediff: sparsediff.c
	gcc sparsediff.c -O3 -o sparsediff
