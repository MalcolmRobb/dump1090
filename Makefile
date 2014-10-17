#
# When building a package or installing otherwise in the system, make
# sure that the variable PREFIX is defined, e.g. make PREFIX=/usr/local
#
PROGNAME=dump1090

BINDIR=$(DESTDIR)/usr/bin
SBINDIR=$(DESTDIR)/usr/sbin
SHAREDIR=$(DESTDIR)/usr/share/$(PROGNAME)
EXTRACFLAGS=-DHTMLPATH=\"$(SHAREDIR)/public_html\"

CFLAGS=-O2 -g -Wall -W $(shell pkg-config --cflags librtlsdr)
CFLAGS += $(shell dpkg-buildflags --get CFLAGS)
CFLAGS += $(shell dpkg-buildflags --get CPPFLAGS)
CFLAGS += $(shell dpkg-buildflags --get CXXFLAGS)
LIBS=$(shell pkg-config --libs librtlsdr) -lpthread -lm
LDFLAGS=$(shell dpkg-buildflags --get LDFLAGS)
CC=gcc

all: dump1090 view1090
install: installbin installhtml

%.o: %.c
	$(CC) $(CFLAGS) $(EXTRACFLAGS) -c $<

dump1090: dump1090.o anet.o interactive.o mode_ac.o mode_s.o net_io.o
	$(CC) -g -o dump1090 dump1090.o anet.o interactive.o mode_ac.o mode_s.o net_io.o $(LIBS) $(LDFLAGS)

view1090: view1090.o anet.o interactive.o mode_ac.o mode_s.o net_io.o
	$(CC) -g -o view1090 view1090.o anet.o interactive.o mode_ac.o mode_s.o net_io.o $(LIBS) $(LDFLAGS)

clean:
	rm -f *.o dump1090 view1090

installbin:
	install -m755 dump1090 $(SBINDIR)
	install -m755 view1090 $(BINDIR)

installhtml:
	install -d -m 755 $(SHAREDIR)/public_html
	cp -R public_html/* $(SHAREDIR)/public_html

