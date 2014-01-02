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
	@if [ -d ${INCDIR}/gleri -o -f ${INCDIR}/gleri.h ]; then\
	    echo "Removing gleri headers ...";\
	    rm -f ${INCSI};\
	    rmdir ${INCDIR}/gleri;\
	fi
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
	@if [ -f ${LIBAI} ]; then\
	    echo "Removing library from ${LIBDIR} ...";\
	    rm -f ${LIBAI};\
	fi
endif

################ Maintenance ###########################################

clean:	gleri/clean
gleri/clean:
	@if [ -d $O/gleri ]; then\
	    rm -f ${LIBA} ${LIBOBJ} ${LIBDEPS};\
	    rmdir $O/gleri;\
	fi

-include ${LIBDEPS}
