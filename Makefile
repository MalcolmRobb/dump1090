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

CFLAGS+=-O2 -g -Wall -W `pkg-config --cflags librtlsdr`
LIBS=-lpthread -lm
LIBS_RTL=`pkg-config --libs librtlsdr`
CC=gcc


all: dump1090 view1090

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(EXTRACFLAGS) -c $<

dump1090: dump1090.o anet.o interactive.o mode_ac.o mode_s.o net_io.o
	$(CC) -g -o dump1090 dump1090.o anet.o interactive.o mode_ac.o mode_s.o net_io.o $(LIBS) $(LIBS_RTL) $(LDFLAGS)

view1090: view1090.o anet.o interactive.o mode_ac.o mode_s.o net_io.o
	$(CC) -g -o view1090 view1090.o anet.o interactive.o mode_ac.o mode_s.o net_io.o $(LIBS) $(LDFLAGS)

clean:
	rm -f *.o dump1090 view1090
