BINARIES = qtvcomp qticomp qtwcomp qtvcap
CC = gcc
CFLAGS = -Wall -Wextra -O4 -march=native
LDFLAGS = -lm
X11FLAGS = -lX11 -lXext -lXfixes

.PHONY: all
all: $(BINARIES)


%: %.o
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@


%.o: %.c
	$(CC) $(CFLAGS) -c $<


qticomp: qticomp.o databuffer.o image.o ppm.o qtc.o qti.o rangecode.o
qtvcomp: qtvcomp.o databuffer.o image.o ppm.o qtc.o qti.o qtv.o rangecode.o
qtwcomp: qtwcomp.o databuffer.o image.o ppm.o qtc.o qti.o qtw.o rangecode.o

qtvcap: qtvcap.o x11grab.o databuffer.o image.o qtc.o qti.o qtv.o rangecode.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(X11FLAGS) $^ -o $@


databuffer.o: databuffer.c databuffer.h
image.o: image.c image.h
ppm.o: ppm.c image.h ppm.h
qtc.o: qtc.c databuffer.h qti.h image.h qtc.h
qti.o: qti.c databuffer.h rangecode.h qti.h
qticomp.o: qticomp.c image.h qti.h qtc.h ppm.h
qtv.o: qtv.c databuffer.h rangecode.h qti.h qtv.h
qtvcap.o: qtvcap.c x11grab.h image.h qti.h qtc.h qtv.h
qtvcomp.o: qtvcomp.c image.h qti.h qtc.h qtv.h ppm.h
qtw.o: qtw.c databuffer.h rangecode.h qti.h qtw.h
qtwcomp.o: qtwcomp.c image.h qti.h qtc.h qtw.h ppm.h
rangecode.o: rangecode.c databuffer.h rangecode.h
x11grab.o: x11grab.c image.h x11grab.h


.PHONY: clean
clean:
	rm -f $(BINARIES) *.o

