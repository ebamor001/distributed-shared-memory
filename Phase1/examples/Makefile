EXECS = $(patsubst %.c,%,$(wildcard *.c))

all: $(EXECS)

.PHONY = all clean install distclean

%: %.c
	$(CC) $(CFLAGS) -o $@ $< 

install: all
	@-/bin/mkdir -p  $(TOPDIR)/bin
	@-/bin/cp $(EXECS) $(TOPDIR)/bin

clean:
	@-\rm -f *.o *~  *.out core* $(EXECS)

distclean: clean
