DSRC	:= data/ter-d18b.psf $(wildcard data/sh/*)
DSRCB	:= $(subst data/,,${DSRC})
MKDCC	:= .o/data/mkdata
PAK	:= .o/data/resource.pak
DCC	:= .o/data/data.cc
DHH	:= .o/data/data.h

########################################################################

${MKDCC}:	${MKDCC}.o
	@echo "Linking $@ ..."
	@${LD} ${LDFLAGS} -o $@ $^

${PAK}:	${DSRC} ${MKDCC}.o
	@echo "    Collecting data files ..."
	@echo ${DSRCB}|xargs -n1 echo|(cd data; cpio -o 2>/dev/null)|gzip -9 > $@

${DCC}:	${PAK} ${MKDCC}
	@echo "    Compiling $< ..."
	@${MKDCC} $<

${DHH}:		${DCC}
gleris.cc:	${DHH}

${PAK} ${DCC} ${DHH}: Makefile Config.mk config.h ${NAME}/config.h
