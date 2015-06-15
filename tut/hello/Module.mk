################ Source files ##########################################

tut/hello/EXE	:= $Otut/hello/hello
tut/hello/SRCS	:= $(wildcard tut/hello/*.cc)
tut/hello/OBJS	:= $(addprefix $O,$(tut/hello/SRCS:.cc=.o))
tut/hello/DEPS	:= $(tut/hello/OBJS:.o=.d)
tut/hello/LIBS	:= ${LIBA}
ifdef USE_USTL
tut/hello/LIBS	+= -lustl -lsupc++
endif

################ Compilation ###########################################

.PHONY:	tut/hello/all tut/hello/run tut/hello/clean

all:		tut/hello/all

tut/hello/all:	${tut/hello/EXE}

tut/hello/run:	${tut/hello/EXE} ${EXE}
	@PATH="$O" ${tut/hello/EXE}

${tut/hello/EXE}:	${tut/hello/OBJS} ${EXE} ${LIBA}
	@echo "Linking $@ ..."
	@${LD} ${LDFLAGS} -o $@ ${tut/hello/OBJS} ${tut/hello/LIBS}

################ Maintenance ###########################################

clean:	tut/hello/clean
tut/hello/clean:
	@if [ -d $Otut/hello ]; then\
	    rm -f ${tut/hello/EXE} ${tut/hello/OBJS} ${tut/hello/DEPS};\
	    rmdir $Otut/hello;\
	fi

${tut/hello/OBJS}: ${MKDEPS} tut/hello/Module.mk

-include ${tut/hello/DEPS}
