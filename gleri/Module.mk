################ Source files ##########################################

LIBA	:= .o/gleri/libgleri.a
LIBSRC	:= $(wildcard gleri/*.cc)
LIBOBJ	:= $(addprefix .o/,$(LIBSRC:.cc=.o))
LIBINC	:= gleri.h $(wildcard gleri/*.h)
LIBDEPS	:= ${LIBOBJ:.o=.d}

################ Compilation ###########################################

${LIBA}: ${LIBOBJ}
	@echo "Linking $@ ..."
	@rm -f ${LIBA}
	@${AR} qc $@ ${LIBOBJ}

${LIBOBJ}:	Makefile gleri/Module.mk ${CONFS}

################ Installation ##########################################

.PHONY:	uninstall-headers uninstall-lib

##### Install headers

ifdef INCDIR	# These ifdefs allow cold bootstrap
INCSI	:= $(addprefix ${INCDIR}/,${LIBINC})

install:	${INCSI}
${INCSI}: ${INCDIR}/%.h: %.h
	@echo "Installing $@ ..."
	@${INSTALLDATA} $< $@
uninstall:	uninstall-headers
uninstall-headers:
	@echo "Removing gleri headers ..."
	@(cd ${INCDIR}; rm -f ${INCSI}; [ ! -d gleri ] || rm -rf gleri)
endif

##### Install library

ifdef LIBDIR
LIBAI	:= ${LIBDIR}/$(notdir ${LIBA})
install:	${LIBAI}
${LIBAI}:	${LIBA}
	@echo "Installing $@ ..."
	@${INSTALLLIB} $< $@

uninstall:	uninstall-lib
uninstall-lib:
	@echo "Removing library from ${LIBDIR} ..."
	@rm -f ${LIBAI}
endif

################ Maintenance ###########################################

clean:	gleri/clean
gleri/clean:
	@rm -f ${LIBA} ${LIBOBJ} ${LIBDEPS}
	@rmdir $O/gleri &> /dev/null || true

-include ${LIBDEPS}
