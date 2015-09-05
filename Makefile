#
# When building a package or installing otherwise in the system, make
# sure that the variable PREFIX is defined, e.g. make PREFIX=/usr/local
#
PROGNAME=dump1090

ifndef DUMP1090_VERSION
DUMP1090_VERSION=$(shell git describe --tags)
endif

ifdef PREFIX
BINDIR=$(PREFIX)/bin
SHAREDIR=$(PREFIX)/share/$(PROGNAME)
EXTRACFLAGS=-DHTMLPATH=\"$(SHAREDIR)\"
endif

CPPFLAGS+=-DMODES_DUMP1090_VERSION=\"$(DUMP1090_VERSION)\"
CFLAGS+=-O2 -g -Wall -Werror -W
LIBS=-lpthread -lm -lrt
LIBS_RTL=`pkg-config --libs librtlsdr`
CC=gcc

all: dump1090 view1090

%.o: %.c *.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(EXTRACFLAGS) -c $< -o $@

dump1090.o: CFLAGS += `pkg-config --cflags librtlsdr`

dump1090: dump1090.o anet.o interactive.o mode_ac.o mode_s.o net_io.o crc.o demod_2000.o demod_2400.o stats.o cpr.o icao_filter.o track.o util.o convert.o
	$(CC) -g -o $@ $^ $(LIBS) $(LIBS_RTL) $(LDFLAGS)

view1090: view1090.o anet.o interactive.o mode_ac.o mode_s.o net_io.o crc.o stats.o cpr.o icao_filter.o track.o util.o
	$(CC) -g -o $@ $^ $(LIBS) $(LDFLAGS)

faup1090: faup1090.o anet.o mode_ac.o mode_s.o net_io.o crc.o stats.o cpr.o icao_filter.o track.o util.o
	$(CC) -g -o $@ $^ $(LIBS) $(LDFLAGS)

clean:
	rm -f *.o dump1090 view1090 faup1090 cprtests crctests

test: cprtests
	./cprtests

cprtests: cpr.o cprtests.o
	$(CC) $(CPPFLAGS) $(CFLAGS) $(EXTRACFLAGS) -g -o $@ $^ -lm

crctests: crc.c crc.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(EXTRACFLAGS) -g -DCRCDEBUG -o $@ $<
