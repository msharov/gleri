################ Source files ##########################################

LIBA	:= .o/gleri/libgleri.a
LIBSRC	:= $(wildcard gleri/*.cc)
LIBOBJ	:= $(addprefix .o/,$(LIBSRC:.cc=.o))
LIBDEPS	:= ${LIBOBJ:.o=.d}

################ Compilation ###########################################

${LIBA}: ${LIBOBJ}
	@echo "Linking $@ ..."
	@rm -f ${LIBA}
	@${AR} qc $@ ${LIBOBJ}

${LIBOBJ}:	Makefile gleri/Module.mk ${CONFS}

################ Maintenance ###########################################

clean:	gleri/clean
gleri/clean:
	@rm -f ${LIBA} ${LIBOBJ} ${LIBDEPS}
	@rmdir $O/gleri &> /dev/null || true

-include ${LIBDEPS}
