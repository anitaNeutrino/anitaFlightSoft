#
#
#


ANITA_LIB_DIR=${ANITA_FLIGHT_SOFT_DIR}/lib
ANITA_BIN_DIR=${ANITA_FLIGHT_SOFT_DIR}/bin

CC           = gcc
LD            = gcc
ObjSuf	      = o
SrcSuf        = c
ExeSuf        =
OPT           = -O3 -Wall --debug  # --debug --pedantic-errors
SOFLAGS       = -shared
DllSuf        = so

#Hack so that the software compiles on MAC OS X
SYSTEM:= $(shell uname -s)
CHIP:= $(shell uname -m)      # only used by Solaris at this time...

ifeq ($(SYSTEM),Darwin)
# MacOS X with cc (GNU cc 2.95.2 and gcc 3.3)
MACOSX_MINOR := $(shell sw_vers | sed -n 's/ProductVersion://p' | cut -d . -f 2)
MACOSXTARGET := MACOSX_DEPLOYMENT_TARGET=10.$(MACOSX_MINOR)
ifeq ($(MACOSX_MINOR),5)
MACOSX_MINOR  = 4
endif
CXX           = gcc
CXXFLAGS      = $(OPT2) -pipe -Wall -W -Woverloaded-virtual -I/usr/include/sys/
#SYSCCFLAGS        = -I/usr/include/sys/
LD            = gcc
LDFLAGS       = $(OPT2) -bind_at_load
# The SOFLAGS will be used to create the .dylib,
# the .so will be created separately
DllSuf       = dylib
UNDEFOPT      = dynamic_lookup
ifneq ($(MACOSX_MINOR),4)
ifneq ($(MACOSX_MINOR),3)
UNDEFOPT      = suppress
LD            = gcc
endif
endif
SOFLAGS       = -dynamiclib -single_module -undefined $(UNDEFOPT)
endif




NOOPT         =
EXCEPTION     = 


ifdef USE_FAKE_DATA_DIR
FAKEFLAG = -DUSE_FAKE_DATA_DIR
else
FAKEFLAG =
endif


INCLUDES      = -I$(ANITA_FLIGHT_SOFT_DIR) -I$(ANITA_FLIGHT_SOFT_DIR)/common \
-I$(ANITA_FLIGHT_SOFT_DIR)/common/includes -I$(ANITA_FLIGHT_SOFT_DIR)/outside/cr7/include -I$(ANITA_FLIGHT_SOFT_DIR)/outside/acromag -I$(ANITA_FLIGHT_SOFT_DIR)/common/sipcomLib -I$(ANITA_FLIGHT_SOFT_DIR)/outside/bzip2-1.0.3/
CCFLAGS      = $(EXCEPTION) $(OPT) -fPIC $(INCLUDES) -D_BSD_SOURCE $(FAKEFLAG) $(SYSCCFLAGS)
LDFLAGS       = $(EXCEPTION) -L$(ANITA_LIB_DIR) 
ANITA_LIBS    =  -lkvp -lConfig  -lPedestal -lUtil -lSlow -lm -lz#-lSocket
ANITA_HKD_LIBS = -lAcromag #-lcr7
ANITA_GPS_LIBS = -lSerial
ANITA_SIP_LIBS = -lSipcom
ANITA_LOS_LIBS = -lLos
ANITA_COMP_LIBS = -lCompress
ANITA_MAP_LIBS = -lAnitaMap
BZ_LIB = -L$(ANITA_FLIGHT_SOFT_DIR)/outside/bzip2-1.0.3/ -lbz2
FFTW_LIB = -L$(ANITA_FLIGHT_SOFT_DIR)/outside/fftw/lib -lfftw3

INSTALL=install
GROUP=anita
OWNER=anita
BINDIR=/usr/bin
LIBDIR=/usr/lib

all: $(Target)

%.$(ObjSuf) : %.$(SrcSuf)
	@echo "<**Compiling****> "$<
	$(CC) $(CCFLAGS) -c $< -o  $@

objclean:
	@-rm -f *.o




















