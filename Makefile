#
# When building a package or installing otherwise in the system, make
# sure that the variable PREFIX is defined, e.g. make PREFIX=/usr/local
#
PROGNAME=dump1090

ifdef PREFIX
BINDIR=$(PREFIX)/bin
SHAREDIR=$(PREFIX)/share/$(PROGNAME)
EXTRACFLAGS=-DHTMLPATH=\"$(SHAREDIR)\"
endif

INSTALL = /usr/bin/install -c
INSTALL_PROGRAM = ${INSTALL}

CFLAGS=-O2 -g -Wall -W `pkg-config --cflags librtlsdr`
LIBS=`pkg-config --libs librtlsdr` -lpthread -lm
CC=gcc


all: dump1090 view1090

%.o: %.c
	$(CC) $(CFLAGS) $(EXTRACFLAGS) -c $<

dump1090: dump1090.o anet.o interactive.o mode_ac.o mode_s.o net_io.o
	$(CC) -g -o dump1090 dump1090.o anet.o interactive.o mode_ac.o mode_s.o net_io.o $(LIBS) $(LDFLAGS)

view1090: view1090.o anet.o interactive.o mode_ac.o mode_s.o net_io.o
	$(CC) -g -o view1090 view1090.o anet.o interactive.o mode_ac.o mode_s.o net_io.o $(LIBS) $(LDFLAGS)

clean:
	rm -f *.o dump1090 view1090

install: all
	$(INSTALL_PROGRAM) -D dump1090 $(BINDIR)/dump1090
	$(INSTALL_PROGRAM) -D view1090 $(BINDIR)/view1090
	$(INSTALL_PROGRAM) -D init/dump1090 /etc/init.d/dump1090

uninstall:
	rm -fr $(BINDIR)/dump1090
	rm -fr $(BINDIR)/view1090
	rm -fr /etc/init.d/dump1090	
