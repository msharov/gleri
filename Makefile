-include Config.mk

######################################################################

EXE	:= gleris
CXXFLAGS+= -I.
CONFS	:= Config.mk config.h gleri/config.h config.status

include data/Module.mk
include gleri/Module.mk

INC	:= $(wildcard *.h)
SRC	:= $(wildcard *.cc)
OBJ	:= $(addprefix $O,$(SRC:.cc=.o))
DEP	:= ${OBJ:.o=.d}

######################################################################

.PHONY:	all run clean distclean maintainer-clean install uninstall

all:	${EXE}

${EXE}:	${OBJ} ${DCO} ${LIBA}
	@echo "Linking $@ ..."
	@${LD} ${LDFLAGS} -o $@ ${OBJ} ${DCO} ${LIBA} ${LIBS}

$O%.o:	%.cc
	@echo "    Compiling $< ..."
	@[ -d $(dir $@) ] || mkdir -p $(dir $@)
	@${CXX} ${CXXFLAGS} -MMD -MT "$(<:.cc=.s) $@" -o $@ -c $<

%.s:	%.cc
	@echo "    Assembling $< ..."
	@${CXX} ${CXXFLAGS} -S -o $@ -c $<

################ Installation ##########################################

ifdef BINDIR
EXEI	:= $(addprefix ${BINDIR}/,${EXE})

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
	@if [ -d $O ]; then\
	    rm -f ${EXE} ${OBJ} ${DEP};\
	    rmdir $Otut $O;\
	fi

distclean:	clean
	@rm -f ${CONFS}

maintainer-clean: distclean

${CONFS}:	configure Config.mk.in config.h.in gleri/config.h.in
	@if [ -x config.status ]; then\
	    echo "Reconfiguring ...";\
	    ./config.status;\
	else\
	    echo "Running configure ...";\
	    ./configure;\
	fi

${OBJ}: Makefile ${CONFS}

-include ${DEP}
