OBJS = $(patsubst %.c,%.o,$(wildcard *.c))

default: $(OBJS)

spawn_management.o: spawn_management.c $(TOPDIR)/include/common_services.h
	$(CC) $(CFLAGS) $(OPTFLAGS) -DCONNECTION_METHOD=\"\ssh\" -o $@ -c $<

%.o: %.c $(TOPDIR)/include/common_services.h
	$(CC) $(CFLAGS) $(OPTFLAGS) -o $@ -c $< 

.PHONY: all clean install distclean

clean:
	@-/bin/rm -f $(OBJS) *~  *.out core* 

distclean: clean

install: default
