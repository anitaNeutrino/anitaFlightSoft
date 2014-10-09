

include ${ANITA_FLIGHT_SOFT_DIR}/standard_definitions.mk

FFTW3_INCDIR=$(FFTW3_DIR)/include
FFTW3_LIBDIR=-L $(FFTW3_DIR)/lib

# Where the opencl headers and libraries live
CL=/opt/AMDAPP
CL_INCDIR = $(CL)/include
CL_LIBDIR = $(CL)/lib

LDFLAGS += -lOpenCL

CFLAGS= -g -I ../../common/

#Targets = eventd fakeAcqd fakeCalibd fakePrioritizerd fakeGPSd
Targets = Prioritizerd 
#Objs= Filters.o AnitaInstrument.o fftwxcor.o GenerateScore.o
Objs= openCLwrapper.o anitaGpu.o 

#Objs += rcc_time_stamp.o
#CFLAGS= += -DTSTAMP


INCLUDES += -I$(FFTW3_INCDIR) -I$(CL_INCDIR)

all: $(Targets)

$(Targets).o : ${Objs}

$(Targets): % : %.o ${Objs}
	@echo "<**Linking**> $@ ..."
	@$(LD) $@.o $(Objs) $(CCFLAGS) $(INCLUDES) $(LDFLAGS) $(ANITA_LIBS) $(FFTW3_LIBDIR) $(FFTW_LIB) $(CL_LIB) -o $@
	@chmod 555 $@
	ln -sf $(shell pwd)/$@ ${ANITA_FLIGHT_SOFT_DIR}/bin

#xcorrtest: xcorrtest.o fftwxcor.o
#	$(LD) xcorrtest.o fftwxcor.o  $(LDFLAGS) $(ANITA_LIBS)$(FFTW3_LIBDIR) $(FFTW_LIB)  -o xcorrtest

clean: objclean
	@-rm -f $(Targets) Pdebug
