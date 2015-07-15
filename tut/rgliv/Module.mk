################ Source files ##########################################

tut/rgliv/EXE	:= $Otut/rgliv/rgliv
tut/rgliv/SRCS	:= $(wildcard tut/rgliv/*.cc)
tut/rgliv/OBJS	:= $(addprefix $O,$(tut/rgliv/SRCS:.cc=.o))
tut/rgliv/DEPS	:= $(tut/rgliv/OBJS:.o=.d)
tut/rgliv/LIBS	:= ${LIBA}
ifdef USE_USTL
tut/rgliv/LIBS	+= -lustl -lsupc++
endif

################ Compilation ###########################################

.PHONY:	tut/rgliv/all tut/rgliv/run tut/rgliv/clean

all:		tut/rgliv/all

tut/rgliv/all:	${tut/rgliv/EXE}

tut/rgliv/run:	${tut/rgliv/EXE} ${EXE}
	@PATH="." ./${tut/rgliv/EXE}

${tut/rgliv/EXE}:	${tut/rgliv/OBJS} ${EXE} ${LIBA}
	@echo "Linking $@ ..."
	@${LD} ${LDFLAGS} -o $@ ${tut/rgliv/OBJS} ${tut/rgliv/LIBS}

################ Installation ##########################################

ifdef BINDIR
tut/rgliv/EXEI	:= ${BINDIR}/$(notdir ${tut/rgliv/EXE})

install:		tut/rgliv/install
tut/rgliv/install:	${tut/rgliv/EXEI}
${tut/rgliv/EXEI}:	${tut/rgliv/EXE}
	@echo "Installing $< as $@ ..."
	@${INSTALLEXE} $< $@

uninstall:		tut/rgliv/uninstall
tut/rgliv/uninstall:
	@if [ -f ${tut/rgliv/EXEI} ]; then\
	    echo "Removing ${tut/rgliv/EXEI} ...";\
	    rm -f ${tut/rgliv/EXEI};\
	fi
endif

################ Maintenance ###########################################

clean:	tut/rgliv/clean
tut/rgliv/clean:
	@if [ -d $Otut/rgliv ]; then\
	    rm -f ${tut/rgliv/EXE} ${tut/rgliv/OBJS} ${tut/rgliv/DEPS};\
	    rmdir $Otut/rgliv;\
	fi

${tut/rgliv/OBJS}: ${MKDEPS} tut/rgliv/Module.mk

-include ${tut/rgliv/DEPS}
