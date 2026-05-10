#
# Makefile for ampr-ripd
#

CONFDIR = $(DESTDIR)/etc
SBINDIR = $(DESTDIR)/usr/sbin
SCACHEDIR = $(DESTDIR)/var/lib/amprd

CC = gcc
CFLAGS = -Wall -O2
LD = gcc
LDFLAGS =

OBJFILES = amprd.o tunnel.o minIni.o list.o rip.o encap.o cache.o route.o callhome.o

.c.o:
	$(CC) $(CFLAGS) -c $<

amprd:	$(OBJFILES)
	$(LD) $(LDFLAGS) -o amprd $(OBJFILES)

all:	amprd

clean:
	rm -f amprd *.o

install:	amprd
	install -m 755 -p -d -D                      $(SCACHEDIR)
	install -m 644 -p -b -D amprd.conf.example   $(CONFDIR)/amprd.conf.example
	install -m 755 -p -s -D amprd                $(SBINDIR)/amprd
