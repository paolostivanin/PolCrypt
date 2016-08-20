CC = gcc
#CC = clang

CFLAGS = -Wall -Wextra -std=c11 -O2 -Wformat=2 -ftrapv -fstack-protector-all -fstack-protector-strong -pie -fpie -fdiagnostics-color=always -Wstrict-prototypes -Wunreachable-code  -Wwrite-strings -Wpointer-arith -Wbad-function-cast -Wcast-qual -Wcast-align $(shell pkg-config --cflags gtk+-3.0)

DFLAGS = -D_FILE_OFFSET_BITS=64 -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2

NOFLAGS = -Wno-missing-field-initializers -Wno-return-type -Wno-cast-qual -Wno-sign-compare

LDFLAGS = -Wl,-O1,--sort-common,--as-needed,-z,relro

LIBS = -lgcrypt -lgpgme $(shell pkg-config --libs gtk+-3.0)

SOURCES = $(wildcard src/*.c src/hash/*.c)
OBJS = ${SOURCES:.c=.o}

PROG = gtkcrypto

.SUFFIXES:.c .o

.c.o:
	$(CC) -c $(CFLAGS) $(NOFLAGS) $(DFLAGS) $(LDFLAGS) $< -o $@

all: $(PROG)


$(PROG) : $(OBJS)
	$(CC) $(CFLAGS) $(NOFLAGS) $(DFLAGS) $(LDFLAGS) $(OBJS) -o $@ $(LIBS)


.PHONY: clean

clean :
	rm -f $(PROG) $(OBJS)


install:
	mkdir -v /usr/share/gtkcrypto
	test -s gtkcrypto.desktop && cp -v gtkcrypto.desktop /usr/share/applications/ || echo "Desktop file not copied"
	test -s gtkcrypto && cp -v gtkcrypto /usr/bin/ || echo "--> GUI not built, please type make before make install"
	test -s po/it.mo && cp -v po/it.mo /usr/share/locale/it/LC_MESSAGES/gtkcrypto.mo || echo "--> Italian language not installed"
	test -s COPYING && cp -v COPYING /usr/share/gtkcrypto/
	cp -v gtkcrypto.png /usr/share/pixmaps/


uninstall:
	rm -vr /usr/share/gtkcrypto
	rm -v /usr/bin/gtkcrypto
	rm -v /usr/share/applications/gtkcrypto.desktop
