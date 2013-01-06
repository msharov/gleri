################ Source files ##########################################

bvt/EXE		:= bvt/gltest
bvt/SRCS	:= $(wildcard bvt/*.cc)
bvt/OBJS	:= $(addprefix $O,$(bvt/SRCS:.cc=.o))
bvt/DEPS	:= $(bvt/OBJS:.o=.d)
bvt/LIBS	:= ${LIBA}

################ Compilation ###########################################

.PHONY:	bvt/all bvt/run bvt/clean bvt/check

bvt/all:	${bvt/EXE}

# The correct output of a bvt is stored in bvtXX.std
# When the bvt runs, its output is compared to .std
#
check:		bvt/check
bvt/check:	${bvt/EXE} ${EXE}
	@echo "Running $<"; \
	./${bvt/EXE} &> ${bvt/EXE}.out; \
	diff ${bvt/EXE}.std ${bvt/EXE}.out && rm -f ${bvt/EXE}.out

${bvt/EXE}: bvt/%: ${bvt/OBJS} ${EXE} ${LIBA}
	@echo "Linking $@ ..."
	@${LD} ${LDFLAGS} -o $@ ${bvt/OBJS} ${bvt/LIBS}

################ Maintenance ###########################################

clean:	bvt/clean
bvt/clean:
	@rm -f ${bvt/EXE} ${bvt/OBJS} ${bvt/DEPS}
	@rmdir $O/bvt &> /dev/null || true

${bvt/OBJS}: Makefile bvt/Module.mk ${CONFS}

-include ${bvt/DEPS}
