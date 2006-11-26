#
#
#


ANITA_LIB_DIR=${ANITA_FLIGHT_SOFT_DIR}/lib
ANITA_BIN_DIR=${ANITA_FLIGHT_SOFT_DIR}/bin

ObjSuf	      = o
SrcSuf        = c
ExeSuf        =
OPT           = -O3 -Wall  # --debug --pedantic-errors

NOOPT         =
EXCEPTION     = 


ifdef USE_FAKE_DATA_DIR
FAKEFLAG = -DUSE_FAKE_DATA_DIR
else
FAKEFLAG =
endif

CC           = gcc
INCLUDES      = -I$(ANITA_FLIGHT_SOFT_DIR) -I$(ANITA_FLIGHT_SOFT_DIR)/common \
-I$(ANITA_FLIGHT_SOFT_DIR)/common/includes -I$(ANITA_FLIGHT_SOFT_DIR)/outside/cr7/include -I$(ANITA_FLIGHT_SOFT_DIR)/outside/acromag -I$(ANITA_FLIGHT_SOFT_DIR)/common/sipcomLib -I$(ANITA_FLIGHT_SOFT_DIR)/outside/bzip2-1.0.3/ -I$(ANITA_FLIGHT_SOFT_DIR)/outside/fftw/include
CCFLAGS      = $(EXCEPTION) $(OPT) -fPIC $(INCLUDES) -D_BSD_SOURCE $(FAKEFLAG)
LD            = gcc
LDFLAGS       = $(EXCEPTION) -L$(ANITA_LIB_DIR) 
ANITA_LIBS    =  -lkvp -lConfig  -lcr7 -lPedestal -lUtil -lSlow -lm -lz#-lSocket
ANITA_HKD_LIBS = -lAcromag
ANITA_GPS_LIBS = -lSerial
ANITA_SIP_LIBS = -lSipcom
ANITA_LOS_LIBS = -lLos
ANITA_COMP_LIBS = -lCompress
ANITA_MAP_LIBS = -lAnitaMap
BZ_LIB = -L$(ANITA_FLIGHT_SOFT_DIR)/outside/bzip2-1.0.3/ -lbz2
FFTW_LIB = -L$(ANITA_FLIGHT_SOFT_DIR)/outside/fftw/lib -lfftw3

all: $(Target)

%.$(ObjSuf) : %.$(SrcSuf)
	@echo "<**Compiling**> "$<
	@$(CC) $(CCFLAGS) -c $< -o  $@

objclean:
	@-rm -f *.o




















