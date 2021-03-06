-include Config.mk

######################################################################

EXE	:= $Ogleris
CXXFLAGS+= -I.
CONFS	:= Config.mk config.h gleri/config.h config.status
MKDEPS	:= Makefile ${CONFS} $O.d

INC	:= $(wildcard *.h)
SRC	:= $(wildcard *.cc)
OBJ	:= $(addprefix $O,$(SRC:.cc=.o))
DEP	:= ${OBJ:.o=.d}
ONAME	:= $(notdir $(abspath $O))

######################################################################

.PHONY:	all run clean distclean maintainer-clean install uninstall

all:	${EXE}

include gleri/Module.mk
include data/Module.mk

${EXE}:	${OBJ} ${DCO} ${LIBA}
	@echo "Linking $@ ..."
	@${LD} ${LDFLAGS} -o $@ ${OBJ} ${DCO} ${LIBS} ${RGLLIBS}

$O%.o:	%.cc
	@echo "    Compiling $< ..."
	@${CXX} ${CXXFLAGS} -MMD -MT "$(<:.cc=.s) $@" -o $@ -c $<

%.s:	%.cc
	@echo "    Assembling $< ..."
	@${CXX} ${CXXFLAGS} -S -o $@ -c $<

################ Installation ##########################################

ifdef BINDIR
EXEI	:= ${BINDIR}/$(notdir ${EXE})

install:	${EXEI}
${EXEI}:	${EXE}
	@echo "Installing $< as $@ ..."
	@${INSTALLEXE} $< $@

uninstall:
	@if [ -f ${EXEI} ]; then\
	    echo "Removing ${EXEI} ...";\
	    rm -f ${EXEI};\
	fi
endif

################ Test and tutorials ####################################

include test/Module.mk
include tut/hello/Module.mk
include tut/rgliv/Module.mk

################ Maintenance ###########################################

clean:
	@if [ -h ${ONAME} ]; then\
	    rm -f ${EXE} ${OBJ} ${DEP} $Otut/.d $O.d ${ONAME};\
	    [ ! -d ${BUILDDIR}/tut ] || rmdir ${BUILDDIR}/tut;\
	    ${RMPATH} ${BUILDDIR};\
	fi

distclean:	clean
	@rm -f ${CONFS}

maintainer-clean: distclean

${BUILDDIR}/.d:	Makefile
	@[ -d $(dir $@) ] || mkdir -p $(dir $@)
	@touch $@
$O.d:	${BUILDDIR}/.d
	@[ -h ${ONAME} ] || ln -sf ${BUILDDIR} ${ONAME}
$O%/.d:	$O.d
	@[ -d $(dir $@) ] || mkdir $(dir $@)
	@touch $@

${CONFS}:	configure Config.mk.in config.h.in gleri/config.h.in
	@if [ -x config.status ]; then\
	    echo "Reconfiguring ...";\
	    ./config.status;\
	else\
	    echo "Running configure ...";\
	    ./configure;\
	fi

${OBJ}:	${MKDEPS}

-include ${DEP}
