################ Source files ##########################################

DSRC	:= data/ter-d18b.psf $(wildcard data/sh/*)
DSRCB	:= $(subst data/,,${DSRC})
MKDCC	:= $Odata/mkdata
PAK	:= $Odata/resource.pak
DCC	:= $Odata/data.cc
DCO	:= $Odata/data.o
DHH	:= $Odata/data.h
DCCDEPS	:= ${DCC:.cc=.d} ${MKDCC}.d

################ Compilation ###########################################

${MKDCC}:	${MKDCC}.o
	@echo "Linking $@ ..."
ifdef USE_USTL
	@${LD} ${LDFLAGS} -o $@ $< -lustl
else
	@${LD} ${LDFLAGS} -o $@ $<
endif

${PAK}:	${DSRC} ${MKDCC}.o
	@echo "    Collecting data files ..."
	@echo ${DSRCB}|xargs -n1 echo|(cd data; cpio -o 2>/dev/null)|gzip -9 > $@

${DHH}:	${PAK} ${MKDCC}
	@echo "    Compiling $< ..."
	@${MKDCC} $<

${DCC} iconn.cc:	${DHH}

${PAK} ${DCC} ${DHH} ${MKDCC} ${MKDCC}.o ${DCO}: ${MKDEPS} data/Module.mk

${DCO}:	${DCC}
	@echo "    Compiling $< ..."
	@${CXX} ${CXXFLAGS} -MMD -MT "$(<:.cc=.s) $@" -o $@ -c $<

################ Maintenance ###########################################

clean:	data/clean
data/clean:
	@if [ -d $Odata ]; then\
	    rm -f ${MKDCC} ${MKDCC}.o ${PAK} ${DCC} ${DHH} ${DCO} ${DCCDEPS};\
	    rmdir $Odata;\
	fi

-include ${DCCDEPS}
