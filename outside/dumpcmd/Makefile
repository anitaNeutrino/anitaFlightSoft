# 

include ${ANITA_FLIGHT_SOFT_DIR}/standard_definitions.mk


Targets =  dumpcmd 

all: $(Targets)


$(Targets): % : %.o
	@echo "<**Linking**> $@ ..."
	@$(LD) $@.o $(LDFLAGS)  -o $@
	@chmod 555 $@

clean: objclean
	@-rm -f $(Targets) 




















