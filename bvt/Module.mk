################ Source files ##########################################

bvt/SRCS	:= $(wildcard bvt/*.cc)
bvt/BVTS	:= $(bvt/SRCS:.cc=)
bvt/OBJS	:= $(addprefix $O,$(bvt/SRCS:.cc=.o))
bvt/DEPS	:= $(bvt/OBJS:.o=.d)
bvt/LIBS	:= ${LIBA}

################ Compilation ###########################################

.PHONY:	bvt/all bvt/run bvt/clean bvt/check

bvt/all:	${bvt/BVTS}

# The correct output of a bvt is stored in bvtXX.std
# When the bvt runs, its output is compared to .std
#
check:		bvt/check
bvt/check:	${bvt/BVTS}
	@for i in ${bvt/BVTS}; do \
	    echo "Running $$i"; \
	    ./$$i &> $$i.out; \
	    diff $$i.std $$i.out && rm -f $$i.out; \
	done

${bvt/BVTS}: bvt/%: ${bvt/OBJS} ${EXE} ${LIBA}
	@echo "Linking $@ ..."
	@${LD} ${LDFLAGS} -o $@ ${bvt/OBJS} ${bvt/LIBS}

################ Maintenance ###########################################

clean:	bvt/clean
bvt/clean:
	@rm -f ${bvt/BVTS} ${bvt/OBJS} ${bvt/DEPS}
	@rmdir $O/bvt &> /dev/null || true

${bvt/OBJS}: Makefile bvt/Module.mk Config.mk config.h configure

-include ${bvt/DEPS}
