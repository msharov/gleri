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
	@PATH="." ./${tut/rgliv/EXE}

${tut/rgliv/EXE}: tut/rgliv/%: ${tut/rgliv/OBJS} ${EXE} ${LIBA}
	@echo "Linking $@ ..."
	@${LD} ${LDFLAGS} -o $@ ${tut/rgliv/OBJS} ${tut/rgliv/LIBS}

################ Maintenance ###########################################

clean:	tut/rgliv/clean
tut/rgliv/clean:
	@if [ -d $Otut/rgliv ]; then\
	    rm -f ${tut/rgliv/EXE} ${tut/rgliv/OBJS} ${tut/rgliv/DEPS};\
	    rmdir $Otut/rgliv;\
	fi

${tut/rgliv/OBJS}: Makefile tut/rgliv/Module.mk ${CONFS}

-include ${tut/rgliv/DEPS}
