#
# When building a package or installing otherwise in the system, make
# sure that the variable PREFIX is defined, e.g. make PREFIX=/usr/local
#
PROGNAME=dump1090

ifdef PREFIX
	BINDIR=$(PREFIX)/bin
	SBINDIR=$(PREFIX)/sbin
	SHAREDIR=$(PREFIX)/share/$(PROGNAME)
	EXTRACFLAGS=-DHTMLPATH=\"$(SHAREDIR)\"
else
	BINDIR=$(DESTDIR)/usr/bin
	SBINDIR=$(DESTDIR)/usr/sbin
	SHAREDIR=$(DESTDIR)/usr/share/$(PROGNAME)
	EXTRACFLAGS+=-DHTMLPATH=\"$(SHAREDIR)/public_html\"
endif

CFLAGS+=-O2 -g -Wall -W `pkg-config --cflags librtlsdr`
LIBS=`pkg-config --libs librtlsdr` -lpthread -lm
CC=gcc

all: dump1090 view1090

%.o: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(EXTRACFLAGS) -c $<

dump1090: dump1090.o anet.o interactive.o mode_ac.o mode_s.o net_io.o
	$(CC) -g -o dump1090 dump1090.o anet.o interactive.o mode_ac.o mode_s.o net_io.o $(LIBS) $(LDFLAGS)

ppup1090: ppup1090.o anet.o interactive.o mode_ac.o mode_s.o net_io.o
	$(CC) -g -o ppup1090 ppup1090.o anet.o interactive.o mode_ac.o mode_s.o net_io.o coaa1090.obj $(LIBS) $(LDFLAGS)

view1090: view1090.o anet.o interactive.o mode_ac.o mode_s.o net_io.o
	$(CC) -g -o view1090 view1090.o anet.o interactive.o mode_ac.o mode_s.o net_io.o $(LIBS) $(LDFLAGS)

clean:
	rm -f *.o dump1090 ppup1090 view1090
