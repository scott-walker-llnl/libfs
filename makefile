CFLAGS= -Wall -g -O -fPIC -fomit-frame-pointer -std=c99
SOURCES=avx_functions.c firestarter_global.h fma4_functions.c fma_functions.c init_functions.c work.h init.c

libfs.so: init_functions.o init.o avx_functions.o fma4_functions.o fma_functions.o
	$(LINK.c) -shared $^ -o $@

init_functions.o: init_functions.c
	gcc $(CFLAGS) -O2 -c $< -o $@

init.o: init.c firestarter_global.h init.h work.h
	gcc $(CFLAGS) -pthread -O2 -c $< -o $@

avx_functions.o: avx_functions.c
	gcc $(CFLAGS) -O0 -c $< -o $@

fma4_functions.o: fma4_functions.c
	gcc $(CFLAGS) -O0 -c $< -o $@

fma_functions.o: fma_functions.c
	gcc $(CFLAGS) -O0 -c $< -o $@

test: libfs.so test.c
	gcc test.c -o test.exe -g -pthread -L. -lfs

clean:
	rm -f *.o
	rm -f *.so
	rm -f test.exe 
