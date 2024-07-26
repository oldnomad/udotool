INSTALL   = /usr/bin/install
PANDOC    = /usr/bin/pandoc

QUIRKS    = LIBINPUT

CFLAGS   += -D_GNU_SOURCE \
    -Wall -Wextra -pedantic -Wshadow -Wunused -Wuninitialized \
    -Wpointer-arith -Wstrict-prototypes -Wmissing-prototypes \
    -Wformat=2 -Wformat-overflow=2 -Wformat-truncation=2 -Wformat-signedness
CFLAGS   += $(foreach quirk,$(QUIRKS),-DUDOTOOL_$(quirk)_QUIRK)

SRC_FILES = $(wildcard *.c)
HDR_FILES = $(wildcard *.h)
OBJ_FILES = $(patsubst %.c,%.o,$(SRC_FILES))
EXE_FILE  = udotool
MAN_FILE  = udotool.man

all: $(EXE_FILE) $(MAN_FILE)

$(EXE_FILE): $(OBJ_FILES)
	$(CC) $(LDFLAGS) $(OBJ_FILES) -o $@

$(OBJ_FILES): $(HDR_FILES)

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
