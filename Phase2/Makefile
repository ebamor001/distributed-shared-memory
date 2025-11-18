
TOPDIR = $(CURDIR)

CC = gcc
CLINKER	= $(CC)

AR = ar
ARFLAGS = -r

LIBNAME = dsm
LIB = lib$(LIBNAME).a

OBJS = $(patsubst %.c,%.o,$(wildcard *.c)) 
EXECS = $(patsubst examples/%.c,examples/%,$(wildcard examples/*.c)) 

CFLAGS = -D_GNU_SOURCE  -DREENTRANT  -Werror -Wvla -Og -g
LIBS = -l$(LIBNAME) -lpthread

default: $(EXECS) $(LIB) 

$(LIB): $(OBJS)
	$(AR) $(ARFLAGS) $@ $^
	@-\rm -rf $(OBJS)

%.o: %.c  $(TOPDIR)/include/dsm_impl.h  
	$(CC) $(CFLAGS) -I$(TOPDIR)/include/ -o $@ -c $< 

examples/%: $(LIB) 
	$(CC) $(CFLAGS) -I$(TOPDIR)/include/ -I$(TOPDIR) -L$(TOPDIR)  -o $@ $@.c $(LIBS)
	@-/bin/mv $@ $(TOPDIR)/../Phase1/bin/

.PHONY:  examples clean

clean: 
	@-\rm -rf *~  examples/*~ include/*~  lib$(LIBNAME).a 
