################ Source files ##########################################

tut/rgliv/EXE	:= tut/rgliv/rgliv
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
	@./${tut/rgliv/EXE}

${tut/rgliv/EXE}: tut/rgliv/%: ${tut/rgliv/OBJS} ${EXE} ${LIBA}
	@echo "Linking $@ ..."
	@${LD} ${LDFLAGS} -o $@ ${tut/rgliv/OBJS} ${tut/rgliv/LIBS}

################ Maintenance ###########################################

clean:	tut/rgliv/clean
tut/rgliv/clean:
	@rm -f ${tut/rgliv/EXE} ${tut/rgliv/OBJS} ${tut/rgliv/DEPS}
	@rmdir $O/tut/rgliv &> /dev/null || true

${tut/rgliv/OBJS}: Makefile tut/rgliv/Module.mk ${CONFS}

-include ${tut/rgliv/DEPS}
