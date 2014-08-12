#
# When building a package or installing otherwise in the system, make
# sure that the variable PREFIX is defined, e.g. make PREFIX=/usr/local
#

PROGNAME=dump1090

ifndef PREFIX
PREFIX=/usr/local
endif

BINDIR=$(PREFIX)/bin
SHAREDIR=$(PREFIX)/share/$(PROGNAME)
EXTRACFLAGS=-DHTMLPATH=\"$(SHAREDIR)\"

CFLAGS=-O2 -g -Wall -W `pkg-config --cflags librtlsdr`
LIBS=`pkg-config --libs librtlsdr` -lpthread -lm
CC=gcc

all: dump1090 ppup1090 view1090

%.o: %.c
	$(CC) $(CFLAGS) $(EXTRACFLAGS) -c $<

dump1090: dump1090.o anet.o interactive.o mode_ac.o mode_s.o net_io.o
	$(CC) -g -o dump1090 dump1090.o anet.o interactive.o mode_ac.o mode_s.o net_io.o $(LIBS) $(LDFLAGS)

ppup1090: ppup1090.o anet.o interactive.o mode_ac.o mode_s.o net_io.o
	$(CC) -g -o ppup1090 ppup1090.o anet.o interactive.o mode_ac.o mode_s.o net_io.o coaa1090.obj $(LIBS) $(LDFLAGS)

view1090: view1090.o anet.o interactive.o mode_ac.o mode_s.o net_io.o
	$(CC) -g -o view1090 view1090.o anet.o interactive.o mode_ac.o mode_s.o net_io.o $(LIBS) $(LDFLAGS)

clean:
	rm -f *.o dump1090 ppup1090 view1090

install: installbin installhtml

installbin:
	install -m755 dump1090 $(BINDIR)
	install -m755 ppup1090 $(BINDIR)
	install -m755 view1090 $(BINDIR)

installhtml:
	install -d -m 755 $(SHAREDIR)
	cp -R public_html/* $(SHAREDIR)

