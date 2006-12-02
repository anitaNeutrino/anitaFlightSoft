# 
#
#

include ${ANITA_FLIGHT_SOFT_DIR}/standard_definitions.mk


Targets = configTest validate

all: $(Targets)
	@echo "done"


$(Targets): % : %.o
	@echo "<**Linking**> $@ ..."
	@$(LD)  $@.o $(LDFLAGS) $(ANITA_LIBS) -o $@
	@chmod 555 $@

clean: objclean
	@-rm -f $(Targets) 




















