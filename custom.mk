# Custom Makefile snippets.

PALMSDK = "C:\Program Files\PalmSource\Palm OS Developer Suite\PalmSDK\Incs"

# function for converting resources into resource header files
define RESOURCE_LIST_TO_HEADERS
	$(addprefix src/, $(addsuffix _res.h, $(foreach file, $(RESOURCES), $(basename $(notdir $(file))))))
endef

RESOURCE_HEADERS = src/BlueRemote_res.h
#RESOURCE_HEADERS = $(RESOURCE_LIST_TO_HEADERS)

rsc/%_tweaked.xrd: rsc/%.xrd custom.mk xrd_tweak.xml
	xsltproc "xrd_tweak.xml" $< > $@

# XRD header file generation
src/%_res.h: rsc/%_tweaked.xrd custom.mk xrd2header.xml
	xsltproc --param guard "'$*'" "xrd2header.xml" $< > $@

$(SOURCE_LIST_TO_OBJS): $(RESOURCE_HEADERS) src/auto/Magic.g src/auto/Magic.i src/auto/Config.h

src/auto/Magic.g: src/auto $(wildcard src/*.h)
	cpp -D DEFUN_DEFINES src/CoreHeaders.h \
	| grep "^@define" \
	| sed -e 's/@/#/g' -e 's/%/_/g' -e 's/;//g' > $@

src/auto/Magic.i: src/auto $(wildcard src/*.h)
	cpp -D INITIALIZER_INCLUDE -o $@ src/CoreHeaders.h 

release:
	$(MAKE) DEBUG_OR_RELEASE=Release clean all
	-rm -f BlueRemote-1.0p.zip
	zip -j BlueRemote-1.0p.zip Release/BlueRemote.prc

src/auto:
	mkdir src/auto

clean::
	-rm -rf src/auto

# TODO: autoconf support
src/auto/Config.h: src/auto src/Config.h.in custom.mk
	b=$$(cat .build-number); cat src/Config.h.in > $@.t
	if [ ! -e $@ ] || cmp -s $@.t $@; then mv $@.t $@; else rm $@.t; fi


PROJECT_DIR = $(shell pwd)

doc:
	mkdir -p $(OBJ_DIR)/doc
	cd $(OBJ_DIR)/doc && xsltproc /usr/share/xml/docbook/stylesheet/nwalsh/html/chunk.xsl  $(PROJECT_DIR)/doc/BlueRemote.xml
	#xsltproc /usr/share/xml/docbook/stylesheet/nwalsh/html/docbook.xsl  $(PROJECT_DIR)/doc/BlueRemote.xml > $(OBJ_DIR)/doc/BlueRemote.html
	(cd doc; find images -name "*.png" | xargs tar -cf -) | tar -C $(OBJ_DIR)/doc -xf -
	dbcontext -p doc/param.xsl -o $(OBJ_DIR)/doc/BlueRemote.pdf doc/BlueRemote.xml

.PHONY: doc

#.PRECIOUS: rsc/%_tweaked.xrd
