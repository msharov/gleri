################ Source files ##########################################

DSRC	:= data/ter-d18b.psf $(wildcard data/sh/*)
DSRCB	:= $(subst data/,,${DSRC})
MKDCC	:= .o/data/mkdata
PAK	:= .o/data/resource.pak
DCC	:= .o/data/data.cc
DHH	:= .o/data/data.h
DCCDEPS	:= ${DCC:.cc=.d} ${MKDCC}.d

################ Compilation ###########################################

${MKDCC}:	${MKDCC}.o
	@echo "Linking $@ ..."
	@${LD} ${LDFLAGS} -o $@ $<

${PAK}:	${DSRC} ${MKDCC}.o
	@echo "    Collecting data files ..."
	@echo ${DSRCB}|xargs -n1 echo|(cd data; cpio -o 2>/dev/null)|gzip -9 > $@

${DHH}:	${PAK} ${MKDCC}
	@echo "    Compiling $< ..."
	@${MKDCC} $<

${DCC} gleris.cc:	${DHH}

${PAK} ${DCC} ${DHH} ${MKDCC}: Makefile data/Module.mk ${CONFS}

################ Maintenance ###########################################

clean:	data/clean
data/clean:
	@rm -f ${MKDCC} ${MKDCC}.o ${PAK} ${DCC} ${DHH} ${DCCDEPS}
	@rmdir $O/data &> /dev/null || true

-include ${DCCDEPS}
