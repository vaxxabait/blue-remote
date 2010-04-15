# Custom Makefile snippets.

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

OBJS += $(OBJ_DIR)/Version.o

$(LINKER_OUTPUT): $(OBJ_DIR)/Version.o

$(OBJ_DIR)/Version.o: src/auto/Version.c
	$(CC) -c $< $(INCLUDES) $(CFLAGS) -o $@

src/auto/Magic.g: src/auto $(wildcard src/*.h)
	cpp -D DEFUN_DEFINES -o - src/CoreHeaders.h \
	| grep "^@define" \
	| sed -e 's/@/#/g' -e 's/%/_/g' -e 's/;//g' > $@

src/auto/Magic.i: src/auto $(wildcard src/*.h)
	cpp -D INITIALIZER_INCLUDE -o $@ src/CoreHeaders.h 

release:
	$(MAKE) DEBUG_OR_RELEASE=Release clean all
	-rm -f BlueRemote1.0p.zip
	zip -j BlueRemote1.0p.zip Release/BlueRemote.prc

build-number:
	if [ ! -r .build-number ]; then echo 0 > .build-number; fi
	b=$$(echo $$(cat .build-number) 1+p | dc); echo $$b > .build-number

src/auto:
	mkdir src/auto

clean::
	-rm -rf src/auto

src/auto/Version.c: src/auto src/Version.c.in custom.mk build-number
	b=$$(cat .build-number); cat src/Version.c.in | sed "s/@BUILD@/$$b/g" > $@

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
