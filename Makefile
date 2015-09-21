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

# Set NORTLSDR to compile without librtlsdr e.g. "make NORTLSDR=1"
ifndef NORTLSDR
ifeq ($(shell pkg-config --exists librtlsdr || echo "F"), F)
$(warning librtlsdr not found - please install it, or build with "NORTLSDR=1" to build with RTL SDR support.)
endif
RTLSDR_CFLAGS=$(shell pkg-config --cflags librtlsdr)
RTLSDR_LDFLAGS=$(shell pkg-config --libs librtlsdr)
else
RTLSDR_CFLAGS=-DNORTLSDR
endif

CFLAGS=-O2 -g -Wall -W $(RTLSDR_CFLAGS)
LDFLAGS+=$(RTLSDR_LDFLAGS) -lpthread -lm
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
