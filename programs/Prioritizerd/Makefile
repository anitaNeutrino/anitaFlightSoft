# 
#
#

include ${ANITA_FLIGHT_SOFT_DIR}/standard_definitions.mk


#Targets = eventd fakeAcqd fakeCalibd fakePrioritizerd fakeGPSd
Targets = Prioritizerd
Objs= writePacketsAndHeader.o

all: $(Targets)

$(Targets): % : %.o ${Objs}
	@echo "<**Linking**> $@ ..."
	@$(LD) $@.o $(Objs) $(LDFLAGS) $(ANITA_LIBS)  -o $@
	@chmod 555 $@
	ln -sf $(shell pwd)/$@ ${ANITA_FLIGHT_SOFT_DIR}/bin

clean: objclean
	@-rm -f $(Targets) 




















