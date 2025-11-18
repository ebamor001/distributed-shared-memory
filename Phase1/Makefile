
CC = gcc
export CC
CLINKER	= $(CC)
TOPDIR = $(CURDIR)
export TOPDIR

CFLAGS = -D_GNU_SOURCE -DREENTRANT -Wunused -Wall -Werror -Wvla -Og -g 
export CFLAGS
OPTFLAGS  = -I$(TOPDIR)/include 
export OPTFLAGS

LIBS = -lpthread -lhelpers

default: dsmexec examples

dsmexec.o: dsmexec.c $(wildcard include/*.h) 
	$(CC) $(CFLAGS) $(OPTFLAGS) -o $@ -c $<

dsmexec: dsmexec.o services
	$(CLINKER) $(CFLAGS) -o $@ $< $(wildcard services/*.o) -L$(TOPDIR)/binaries/ $(LIBS)

SUBDIRS = services examples 

subdirs: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@ $(MAKECMDGOALS)

.PHONY: $(SUBDIRS) subdirs clean distclean install

install: dsmexec examples 
	@-/bin/mkdir -p ./bin
	@-/bin/mv dsmexec ./bin/
	@-/bin/cp binaries/data_exchange ./bin/

clean: subdirs
	@-\rm -rf *~ *.o  dsmexec

distclean: clean
	@-/bin/rm -f $(TOPDIR)/bin/*
	@-/bin/rmdir $(TOPDIR)/bin/

