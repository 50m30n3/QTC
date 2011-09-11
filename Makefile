BINARIES = qtvcomp qticomp
CC = gcc
CFLAGS = -Wall -O4 -march=native
LDFLAGS = -lm

.PHONY: all
all: $(BINARIES)


%: %.o
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@


%.o: %.c
	$(CC) $(CFLAGS) -c $<


qtvcomp: qtvcomp.o databuffer.o image.o ppm.o qtc.o qti.o qtv.o rangecode.o
qticomp: qticomp.o databuffer.o image.o ppm.o qtc.o qti.o rangecode.o


databuffer.o: databuffer.c databuffer.h
image.o: image.c image.h
ppm.o: ppm.c image.h ppm.h
qtc.o: qtc.c databuffer.h qti.h image.h qtc.h
qti.o: qti.c databuffer.h rangecode.h qti.h
qticomp.o: qticomp.c image.h qti.h qtc.h ppm.h
qtv.o: qtv.c databuffer.h rangecode.h qti.h qtv.h
qtvcomp.o: qtvcomp.c image.h qti.h qtc.h qtv.h ppm.h
rangecode.o: rangecode.c databuffer.h rangecode.h



.PHONY: clean
clean:
	rm -f $(BINARIES) *.o

