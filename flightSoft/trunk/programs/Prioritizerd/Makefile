# 
#
#

include ${ANITA_FLIGHT_SOFT_DIR}/standard_definitions.mk

FFTW3_INCDIR=$(FFTW3_DIR)/include
FFTW3_LIBDIR=-L $(FFTW3_DIR)/lib

#Targets = eventd fakeAcqd fakeCalibd fakePrioritizerd fakeGPSd
Targets = Prioritizerd
Objs= Filters.o AnitaInstrument.o fftwxcor.o GenerateScore.o

CFLAGS= -g -I ../../common/ 

#redefines standard variable
#INCLUDES= -I$(ANITA_FLIGHT_SOFT_DIR) -I$(ANITA_FLIGHT_SOFT_DIR)/common \
#-I$(ANITA_FLIGHT_SOFT_DIR)/common/includes -I$(ANITA_FLIGHT_SOFT_DIR)/outside/cr7/include -I$(ANITA_FLIGHT_SOFT_DIR)/outside/acromag -I$(ANITA_FLIGHT_SOFT_DIR)/common/sipcomLib -I$(ANITA_FLIGHT_SOFT_DIR)/outside/bzip2-1.0.3/ -I/usr/local/surfDriver/include -I$(FFTW3_INCDIR)

INCLUDES += -I$(FFTW3_INCDIR)

CCFLAGS += -D ANITA2


all: $(Targets) Pdebug xcorrtest

$(Targets).o : ${Objs}

$(Targets): % : %.o ${Objs}
	@echo "<**Linking**> $@ ..."
	@$(LD) $@.o $(Objs) $(LDFLAGS) $(ANITA_LIBS) $(FFTW3_LIBDIR) $(FFTW_LIB) -o $@
	@chmod 555 $@
	ln -sf $(shell pwd)/$@ ${ANITA_FLIGHT_SOFT_DIR}/bin

Pdebug:	Pdebug.o SubtractPedestals.o Unwrap.o BaselineSubtract.o
	$(LD) Pdebug.o SubtractPedestals.o Unwrap.o BaselineSubtract.o  $(LDFLAGS) $(ANITA_LIBS)  -o Pdebug

xcorrtest: xcorrtest.o fftwxcor.o
	$(LD) xcorrtest.o fftwxcor.o  $(LDFLAGS) $(ANITA_LIBS)$(FFTW3_LIBDIR) $(FFTW_LIB)  -o xcorrtest

clean: objclean
	@-rm -f $(Targets) Pdebug
