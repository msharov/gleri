################ Source files ##########################################

LIBSRC	:= $(wildcard gleri/*.cc)
LIBOBJ	:= $(addprefix .o/,$(LIBSRC:.cc=.o))
LIBINC	:= gleri.h $(wildcard gleri/*.h)
LIBDEPS	:= ${LIBOBJ:.o=.d}
LIBA_R	:= $Olib${NAME}.a
LIBA_D	:= $Olib${NAME}_d.a
ifdef DEBUG
LIBA	:= ${LIBA_D}
else
LIBA	:= ${LIBA_R}
endif

################ Compilation ###########################################

all:	gleri/all

gleri/all:	${LIBA}

${LIBA}: ${LIBOBJ}
	@echo "Linking $@ ..."
	@rm -f ${LIBA}
	@${AR} qc $@ ${LIBOBJ}

${LIBOBJ}:	${MKDEPS} gleri/Module.mk

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
	    ${RMPATH} ${INCDIR}/gleri;\
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
	    rm -f ${LIBA_D} ${LIBA_R} ${LIBOBJ} ${LIBDEPS};\
	    rmdir $O/gleri;\
	fi

-include ${LIBDEPS}
