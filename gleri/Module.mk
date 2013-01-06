LIBA	:= .o/${NAME}/lib${NAME}.a
LIBSRC	:= $(wildcard ${NAME}/*.cc)
LIBOBJ	:= $(addprefix .o/,$(LIBSRC:.cc=.o))

${LIBA}: ${LIBOBJ}
	@echo "Linking $@ ..."
	@rm -f ${LIBA}
	@${AR} qc $@ ${LIBOBJ}
