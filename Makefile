BINARIES = qtienc qtidec qtvenc qtvdec qtvplay qtvcap
CC = gcc
LD = gcc
CFLAGS = -g -Wall -Wextra -O4 -march=native
LDFLAGS =
X11FLAGS = -lX11 -lXext -lXfixes
SDLFLAGS = -lSDL

.PHONY: all
all: $(BINARIES)

qtvcap: qtvcap.o databuffer.o image.o qtc.o qti.o qtv.o rangecode.o tilecache.o utils.o x11grab.o
	$(LD) $^ $(LDFLAGS) $(X11FLAGS) -o $@

qtvplay: qtvplay.o databuffer.o image.o qtc.o qti.o qtv.o rangecode.o tilecache.o utils.o
	$(LD) $^ $(LDFLAGS) $(SDLFLAGS) -o $@



%: %.o
	$(LD) $^ $(LDFLAGS) -o $@


%.o: %.c
	$(CC) $(CFLAGS) -c $<


qtienc: qtienc.o databuffer.o image.o ppm.o qtc.o qti.o rangecode.o tilecache.o
qtidec: qtidec.o databuffer.o image.o ppm.o qtc.o qti.o rangecode.o tilecache.o
qtvenc: qtvenc.o databuffer.o image.o ppm.o qtc.o qti.o qtv.o rangecode.o tilecache.o utils.o
qtvdec: qtvdec.o databuffer.o image.o ppm.o qtc.o qti.o qtv.o rangecode.o tilecache.o utils.o


databuffer.o: databuffer.c databuffer.h
image.o: image.c image.h
ppm.o: ppm.c image.h ppm.h
qtc.o: qtc.c databuffer.h qti.h tilecache.h image.h qtc.h
qti.o: qti.c databuffer.h rangecode.h tilecache.h qti.h
qtidec.o: qtidec.c image.h qti.h qtc.h ppm.h
qtienc.o: qtienc.c image.h qti.h qtc.h ppm.h tilecache.h
qtv.o: qtv.c databuffer.h rangecode.h qti.h qtv.h
qtvcap.o: qtvcap.c utils.h image.h x11grab.h qti.h qtc.h qtv.h
qtvdec.o: qtvdec.c utils.h image.h qti.h qtc.h qtv.h ppm.h
qtvenc.o: qtvenc.c utils.h image.h qti.h qtc.h qtv.h ppm.h
qtvplay.o: qtvplay.c utils.h image.h databuffer.h qti.h qtc.h qtv.h ppm.h
rangecode.o: rangecode.c databuffer.h rangecode.h
tilecache.o: tilecache.c tilecache.h
utils.o: utils.c
x11grab.o: x11grab.c image.h x11grab.h



.PHONY: clean
clean:
	rm -f $(BINARIES) *.o

