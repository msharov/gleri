-include Config.mk

######################################################################

EXE	:= ${NAME}s
CXXFLAGS+= -I.

DSRC	:= $(filter-out data/mkdata.cc,$(shell find data/ -type f))
MKDCC	:= .o/data/mkdata
PAK	:= .o/data/resource.pak
DCC	:= .o/data/data.cc
DHH	:= .o/data/data.h

INC	:= $(wildcard *.h)
SRC	:= $(wildcard *.cc) ${DCC}
OBJ	:= $(addprefix .o/,$(SRC:.cc=.o))

LIBA	:= .o/lib${NAME}.a
LIBSRC	:= app.cc rglp.cc
LIBOBJ	:= $(addprefix .o/,$(LIBSRC:.cc=.o))

######################################################################

.PHONY:	all run clean distclean maintainer-clean install uninstall

all:	${EXE}

run:	${EXE}
	@./${EXE}

${EXE}:	${OBJ}
	@echo "Linking $@ ..."
	@${LD} ${LDFLAGS} -o $@ ${OBJ} ${LIBS}

${LIBA}: ${LIBOBJ}
	@echo "Linking $@ ..."
	@rm -f ${LIBA}
	@${AR} qc $@ ${LIBOBJ}

.o/%.o:	%.cc
	@echo "    Compiling $< ..."
	@[ -d $(dir $@) ] || mkdir -p $(dir $@)
	@${CXX} ${CXXFLAGS} -MMD -MT "$(<:.cc=.s) $@" -o $@ -c $<

${MKDCC}:	${MKDCC}.o
	@echo "Linking $@ ..."
	@${LD} ${LDFLAGS} -o $@ $^

${PAK}:	${DSRC} ${MKDCC}.o
	@echo "    Collecting data files ..."
	@echo $(subst data/,,${DSRC})|xargs -n1 echo|(cd data; cpio -o 2>/dev/null)|gzip -9 > $@

${DCC}:	${PAK} ${MKDCC}
	@echo "    Compiling $< ..."
	@${MKDCC} $<

${DHH}:		${DCC}
gleris.cc:	${DHH}

%.s:	%.cc
	@echo "    Assembling $< ..."
	@${CXX} ${CXXFLAGS} -S -o $@ -c $<

include bvt/Module.mk

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
	@rm -rf .o ${EXE}

distclean:	clean
	@rm -f Config.mk config.h config.status ${NAME}

maintainer-clean: distclean

Config.mk:		Config.mk.in
config.h:		config.h.in
Config.mk config.h:	configure
	@if [ -x config.status ]; then echo "Reconfiguring ..."; ./config.status; \
	else echo "Running configure ..."; ./configure; fi
${EXE}:		${NAME}/config.h
${NAME}/config.h:	config.h
	@echo "    Linking inplace header location ..."
	@rm -f ${NAME}; ln -s . ${NAME}

${OBJ} ${PAK} ${DCC} ${DHH}: Makefile Config.mk config.h

-include ${OBJ:.o=.d}
