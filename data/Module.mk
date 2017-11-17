################ Source files ##########################################

DSRC	:= data/ter-d18b.psf $(wildcard data/sh/*)
GLPAK	:= $Odata/gleripak
DCC	:= $Odata/data.cc
DCO	:= $Odata/data.o
DHH	:= $Odata/data.h
DCCDEPS	:= ${DCC:.cc=.d} ${GLPAK}.d

################ Compilation ###########################################

all:	data/all
data/all:	${GLPAK}

${GLPAK}:	${GLPAK}.o ${LIBA}
	@echo "Linking $@ ..."
	@${LD} ${LDFLAGS} -o $@ $< -lz ${RGLLIBS}

${DCC}:	${DSRC} ${GLPAK}
	@echo "    Creating $@ ..."
	@${GLPAK} -c -b data/ -v File_resource $@ ${DSRC}

${DHH}:		${DCC}
iconn.cc:	${DHH}

${DCO}:	${DCC}
	@echo "    Compiling $< ..."
	@${CXX} ${CXXFLAGS} -MMD -MT "$(<:.cc=.s) $@" -O0 -o $@ -c $<

################ Installation ##########################################

ifdef BINDIR
GLPAKI	:= ${BINDIR}/$(notdir ${GLPAK})

install:	data/install
data/install:	${GLPAKI}
${GLPAKI}:	${GLPAK}
	@echo "Installing $< as $@ ..."
	@${INSTALLEXE} $< $@

uninstall:	data/uninstall
data/uninstall:
	@if [ -f ${GLPAKI} ]; then\
	    echo "Removing ${GLPAKI} ...";\
	    rm -f ${GLPAKI};\
	fi
endif

################ Maintenance ###########################################

clean:	data/clean
data/clean:
	@if [ -d $Odata ]; then\
	    rm -f ${GLPAK} ${GLPAK}.o ${DCC} ${DHH} ${DCO} ${DCCDEPS} $Odata/.d;\
	    rmdir $Odata;\
	fi

$Odata/.d:	$O.d
	@[ -d $(dir $@) ] || mkdir $(dir $@)
	@touch $@

${DCC} ${DHH} ${GLPAK} ${GLPAK}.o ${DCO}: ${MKDEPS} data/Module.mk $Odata/.d

-include ${DCCDEPS}
