export prefix = /usr

export INSTALL  = /usr/bin/install
export PANDOC   = /usr/bin/pandoc
export DH_CLEAN = /usr/bin/dh_clean
export GIT      = /usr/bin/git
export ENVSUBST = /usr/bin/envsubst
export XXD      = /usr/bin/xxd

.PHONY: all package install clean distclean

all:
	$(MAKE) -C src

package:
	dpkg-buildpackage -us -uc

install:
	$(MAKE) -C src install

clean:
	$(MAKE) -C src clean

distclean:
	$(MAKE) -C src distclean
	-$(DH_CLEAN)
