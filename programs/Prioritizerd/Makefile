# 
#
#

include ${ANITA_FLIGHT_SOFT_DIR}/standard_definitions.mk


#Targets = eventd fakeAcqd fakeCalibd fakePrioritizerd fakeGPSd
Targets = Prioritizerd
Objs= 

all: $(Targets) Pdebug

$(Targets): % : %.o ${Objs}
	@echo "<**Linking**> $@ ..."
	@$(LD) $@.o $(Objs) $(LDFLAGS) $(ANITA_LIBS)  -o $@
	@chmod 555 $@
	ln -sf $(shell pwd)/$@ ${ANITA_FLIGHT_SOFT_DIR}/bin

Pdebug:	Pdebug.o SubtractPedestals.o Unwrap.o BaselineSubtract.o
	@$(LD) Pdebug.o SubtractPedestals.o Unwrap.o BaselineSubtract.o -static $(LDFLAGS) $(ANITA_LIBS)  -o Pdebug

clean: objclean
	@-rm -f $(Targets) Pdebug




















