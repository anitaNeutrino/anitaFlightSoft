

include ${ANITA_FLIGHT_SOFT_DIR}/standard_definitions.mk

FFTW3_INCDIR=$(FFTW3_DIR)/include
FFTW3_LIBDIR=-L $(FFTW3_DIR)/lib

#Targets = eventd fakeAcqd fakeCalibd fakePrioritizerd fakeGPSd
Targets = Prioritizerd 
Objs= Filters.o AnitaInstrument.o fftwxcor.o GenerateScore.o

CFLAGS= -g -I ../../common/ 

INCLUDES += -I$(FFTW3_INCDIR)

CCFLAGS += -D ANITA2

all: $(Targets) xcorrtest

$(Targets).o : ${Objs}

$(Targets): % : %.o ${Objs}
	@echo "<**Linking**> $@ ..."
	@$(LD) $@.o $(Objs) $(LDFLAGS) $(ANITA_LIBS) $(FFTW3_LIBDIR) $(FFTW_LIB) -o $@
	@chmod 555 $@
	ln -sf $(shell pwd)/$@ ${ANITA_FLIGHT_SOFT_DIR}/bin

xcorrtest: xcorrtest.o fftwxcor.o
	$(LD) xcorrtest.o fftwxcor.o  $(LDFLAGS) $(ANITA_LIBS)$(FFTW3_LIBDIR) $(FFTW_LIB)  -o xcorrtest

clean: objclean
	@-rm -f $(Targets) Pdebug
