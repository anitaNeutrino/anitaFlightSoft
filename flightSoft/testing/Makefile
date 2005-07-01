# 
#
#

include ${ANITA_FLIGHT_SOFT_DIR}/standard_definitions.mk


Targets = clientSocket serverSocket sillyTest

all: $(Targets)


$(Targets): % : %.o
	@echo "<**Linking**> $@ ..."
	@$(LD)  $@.o $(LDFLAGS) $(ANITA_LIBS) -o $@
	@chmod 555 $@

clean: objclean
	@-rm -f $(Targets) 




















