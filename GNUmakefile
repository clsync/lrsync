
DESTDIR ?= 
PREFIX  ?= /usr
COMPRESS_MAN ?= yes
STRIP_BINARY ?= yes
EXAMPLES ?= yes

CSECFLAGS ?= -fstack-protector-all -Wall --param ssp-buffer-size=4 -D_FORTIFY_SOURCE=2 -fstack-check -DPARANOID -std=gnu99
CFLAGS ?= -pipe -O2
CFLAGS += $(CSECFLAGS)
DEBUGCFLAGS ?= -pipe -Wall -Werror -ggdb3 -export-dynamic -Wno-error=unused-variable -O0 -pipe $(CSECFLAGS)

CARCHFLAGS ?= -march=native

LDSECFLAGS ?= -Xlinker -zrelro
LDFLAGS += $(LDSECFLAGS)

INSTDIR = $(DESTDIR)$(PREFIX)

objs=\
error.o\
malloc.o\
popen.o\
lrsync.o\
main.o\

binary=lrsync

all: $(objs)
	$(CC) $(CARCHFLAGS) $(CFLAGS) $(LDFLAGS) $(objs) $(LIBS) -o $(binary)

%.o: %.c
	$(CC) $(CARCHFLAGS) $(CFLAGS) $(INC) $< -c -o $@

debug:
	$(CC) $(CARCHFLAGS) $(DEBUGCFLAGS) $(INC) $(LDFLAGS) *.c $(LIBS) -o $(binary)


clean:
	rm -f $(binary) *.o

distclean: clean

install:
	install -d "$(INSTDIR)/bin" "$(INSTDIR)/share/man/man1"
ifeq ($(STRIP_BINARY),yes)
	strip --strip-unneeded -R .comment -R .GCC.command.line -R .note.gnu.gold-version $(binary)
endif
	install -m 755 $(binary) "$(INSTDIR)"/bin/
	install -m 644 man/man1/$(binary).1 "$(INSTDIR)"/share/man/man1/
ifeq ($(COMPRESS_MAN),yes)
	rm -f "$(INSTDIR)"/share/man/man1/$(binary).1.gz
	gzip "$(INSTDIR)"/share/man/man1/$(binary).1
endif

deinstall:
	rm -f "$(INSTDIR)"/bin/$(binary) "$(INSTDIR)"/share/man/man1/$(binary).1.gz "$(INSTDIR)"/share/man/man1/$(binary).1

