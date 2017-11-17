################ Source files ##########################################

tut/hello/EXE	:= $Otut/hello/hello
tut/hello/SRCS	:= $(wildcard tut/hello/*.cc)
tut/hello/OBJS	:= $(addprefix $O,$(tut/hello/SRCS:.cc=.o))
tut/hello/DEPS	:= $(tut/hello/OBJS:.o=.d)

################ Compilation ###########################################

.PHONY:	tut/hello/all tut/hello/run tut/hello/clean

all:		tut/hello/all

tut/hello/all:	${tut/hello/EXE}

tut/hello/run:	${tut/hello/EXE} ${EXE}
	@PATH="$O" ${tut/hello/EXE}

${tut/hello/EXE}:	${tut/hello/OBJS} ${LIBA}
	@echo "Linking $@ ..."
	@${LD} ${LDFLAGS} -o $@ ${tut/hello/OBJS} ${RGLLIBS}

################ Maintenance ###########################################

clean:	tut/hello/clean
tut/hello/clean:
	@if [ -d $Otut/hello ]; then\
	    rm -f ${tut/hello/EXE} ${tut/hello/OBJS} ${tut/hello/DEPS} $Otut/hello/.d;\
	    rmdir $Otut/hello;\
	fi

$Otut/hello/.d:	$Otut/.d
	@[ -d $(dir $@) ] || mkdir $(dir $@)
	@touch $@

${tut/hello/OBJS}: ${MKDEPS} tut/hello/Module.mk $Otut/hello/.d

-include ${tut/hello/DEPS}
