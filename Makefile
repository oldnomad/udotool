export prefix = /usr

export INSTALL  = /usr/bin/install
export PANDOC   = /usr/bin/pandoc
export DEBUILD  = /usr/bin/debuild
export DH_CLEAN = /usr/bin/dh_clean

.PHONY: all package install clean distclean

all:
	$(MAKE) -C src

package:
	$(DEBUILD)

install:
	$(MAKE) -C src install

clean:
	$(MAKE) -C src clean

distclean: clean
	-$(DH_CLEAN)
