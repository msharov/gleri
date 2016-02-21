################ Source files ##########################################

test/NAME	:= gltest
test/EXE	:= $Otest/${test/NAME}
test/SRCS	:= $(wildcard test/*.cc)
test/OBJS	:= $(addprefix $O,$(test/SRCS:.cc=.o))
test/DEPS	:= $(test/OBJS:.o=.d)
test/LIBS	:= ${LIBA}
ifdef USE_USTL
test/LIBS	+= -lustl -lsupc++
endif

################ Compilation ###########################################

.PHONY:	test/all test/clean test/check

test/all:	${test/EXE}

# The correct output of a test is stored in testXX.std
# When the test runs, its output is compared to .std
#
check:		test/check
test/check:	${test/EXE} ${EXE}
	@echo "Running $<"; \
	PATH="$O" ./${test/EXE} > ${test/EXE}.out 2>&1; \
	diff test/${test/NAME}.std ${test/EXE}.out && rm -f ${test/EXE}.out

${test/EXE}:	${test/OBJS} ${EXE} ${LIBA}
	@echo "Linking $@ ..."
	@${LD} ${LDFLAGS} -o $@ ${test/OBJS} ${test/LIBS}

################ Maintenance ###########################################

clean:	test/clean
test/clean:
	@if [ -d $O/test ]; then\
	    rm -f ${test/EXE} ${test/OBJS} ${test/DEPS} ${test/EXE}.out $Otest/.d;\
	    rmdir $O/test;\
	fi

$Otest/.d:	$O.d
	@mkdir $Otest && touch $Otest/.d

${test/OBJS}: ${MKDEPS} test/Module.mk $Otest/.d

-include ${test/DEPS}
