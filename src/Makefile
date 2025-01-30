INSTALL   = /usr/bin/install
PANDOC    = /usr/bin/pandoc
GIT       = /usr/bin/git
ENVSUBST  = /usr/bin/envsubst
XXD       = /usr/bin/xxd

QUIRKS    = LIBINPUT

CFLAGS   += -D_GNU_SOURCE \
    -Wall -Wextra -pedantic -Wshadow -Wunused -Wuninitialized \
    -Wpointer-arith -Wstrict-prototypes -Wmissing-prototypes \
    -Wformat=2 -Wformat-overflow=2 -Wformat-truncation=2 -Wformat-signedness
CFLAGS   += $(foreach quirk,$(QUIRKS),-DUDOTOOL_$(quirk)_QUIRK)
LDLIBS   += -ljim

SRC_FILES  = $(wildcard *.c)
GEN_FILES  = config.h exec-tcl.h
HDR_FILES  = $(wildcard *.h) $(GEN_FILES)
OBJ_FILES  = $(patsubst %.c,%.o,$(SRC_FILES))
EXE_FILE   = udotool
MAN_FILE   = udotool.man

all: $(EXE_FILE) $(MAN_FILE)

$(EXE_FILE): $(OBJ_FILES)
	$(CC) $(LDFLAGS) $(OBJ_FILES) $(LDLIBS) -o $@

$(OBJ_FILES): $(HDR_FILES)

config.h: config.h.template ../.git/HEAD
	GIT_VERSION=`$(GIT) describe --tags --match 'v[0-9]*' --always --dirty` $(ENVSUBST) <$< >$@

exec-tcl.h: exec-tcl.tcl
	$(XXD) -i <$< >$@

.SUFFIXES: .md .man

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<

.md.man:
	$(PANDOC) -s -f markdown -t man -o $@ $<

.PHONY: all install clean distclean

install: $(EXE_FILE) $(MAN_FILE)
	$(INSTALL) -D -t $(DESTDIR)$(prefix)/bin $(EXE_FILE)

clean:
	-$(RM) *.o $(EXE_FILE) $(MAN_FILE)

distclean: clean
	-$(RM) $(GEN_FILES)
