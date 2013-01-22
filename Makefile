-include Config.mk

######################################################################

EXE	:= gleris
CXXFLAGS+= -I.
CONFS	:= Config.mk config.h gleri/config.h config.status

include data/Module.mk
include gleri/Module.mk
include test/Module.mk

INC	:= $(wildcard *.h)
SRC	:= $(wildcard *.cc) ${DCC}
OBJ	:= $(addprefix .o/,$(SRC:.cc=.o))

######################################################################

.PHONY:	all run clean distclean maintainer-clean install uninstall

all:	${EXE}

${EXE}:	${OBJ} ${LIBA}
	@echo "Linking $@ ..."
	@${LD} ${LDFLAGS} -o $@ ${OBJ} ${LIBA} ${LIBS}

.o/%.o:	%.cc
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
	@echo "Removing ${EXEI} ..."
	@rm -f ${EXEI}
endif

################ Maintenance ###########################################

clean:
	@rm -f ${EXE}
	@rm -rf .o

distclean:	clean
	@rm -f ${CONFS}

maintainer-clean: distclean

${CONFS}:	configure Config.mk.in config.h.in gleri/config.h.in
	@if [ -x config.status ]; then echo "Reconfiguring ..."; ./config.status; \
	else echo "Running configure ..."; ./configure; fi

${OBJ}: Makefile ${CONFS}

-include ${OBJ:.o=.d}
