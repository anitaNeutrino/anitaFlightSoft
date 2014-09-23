# 
#
#

include ${ANITA_FLIGHT_SOFT_DIR}/standard_definitions.mk

LIBS = -L.. $(ANITA_LIBS)  -lpthread

#Targets = eventd fakeAcqd fakeCalibd fakePrioritizerd fakeGPSd
Targets = Archived

all: $(Targets)


$(Targets): % : %.o
	@echo "<**Linking**> $@ ..."
	@$(LD) $@.o $(LDFLAGS) $(ANITA_COMP_LIBS) $(LIBS) -o $@
	@chmod 555 $@
	@ln -sf $(shell pwd)/$@ ${ANITA_FLIGHT_SOFT_DIR}/bin

clean: objclean
	@-rm -f $(Targets) 




















