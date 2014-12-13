

include ${ANITA_FLIGHT_SOFT_DIR}/standard_definitions.mk

FFTW3_INCDIR=$(FFTW3_DIR)/include
FFTW3_LIBDIR=-L $(FFTW3_DIR)/lib

# Where the opencl headers and libraries live
CL=/opt/AMDAPP
CL_INCDIR = $(CL)/include
CL_LIBDIR = $(CL)/lib

#DEBUG=1
#CALIBRATION=1
#TIME_STAMP=1
#PROFILER

LDFLAGS +=-lOpenCL -lgsl 
CCFLAGS +=-g

Targets = Prioritizerd fakePrioritizerd
Objs= openCLwrapper.o anitaGpu.o anitaTimingCalib.c prioritizerdFunctions.o


ifdef PROFILER
LDFLAGS += -ltcmalloc -lprofiler
endif

ifdef TIME_STAMP
Objs +=rcc_time_stamp.o
CFLAGS= +=-DTSTAMP
endif

ifdef CALIBRATION
CCFLAGS +=-DCALIBRATION
endif

ifdef DEBUG
CCFLAGS +=-DDEBUG_MODE
endif

INCLUDES +=-I$(FFTW3_INCDIR) -I$(CL_INCDIR)

all: $(Targets)

$(Targets).o : ${Objs}

$(Targets): % : %.o ${Objs}
	@echo "<**Linking**> $@ ..."
	@$(LD) $@.o $(Objs) $(CCFLAGS) $(INCLUDES) $(LDFLAGS) $(ANITA_LIBS) $(FFTW3_LIBDIR) $(FFTW_LIB) $(CL_LIB) -o $@
	@chmod 555 $@
	ln -sf $(shell pwd)/$@ ${ANITA_FLIGHT_SOFT_DIR}/bin

clean: objclean
	@-rm -f $(Targets) Pdebug
