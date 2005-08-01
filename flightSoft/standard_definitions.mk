#
#
#


ANITA_LIB_DIR=${ANITA_FLIGHT_SOFT_DIR}/lib
ANITA_BIN_DIR=${ANITA_FLIGHT_SOFT_DIR}/bin

ObjSuf	      = o
SrcSuf        = c
ExeSuf        =
OPT           = -O2 -Wall #-DGCC #-g --debug --pedantic-errors

NOOPT         =
EXCEPTION     = 

CC           = gcc
INCLUDES      = -I$(ANITA_FLIGHT_SOFT_DIR) -I$(ANITA_FLIGHT_SOFT_DIR)/common \
-I$(ANITA_FLIGHT_SOFT_DIR)/common/includes -I$(ANITA_FLIGHT_SOFT_DIR)/outside/cr7/include -I$(ANITA_FLIGHT_SOFT_DIR)/outside/acromag -I$(ANITA_FLIGHT_SOFT_DIR)/outside/sipcom
CCFLAGS      = $(EXCEPTION) $(OPT) -fPIC $(INCLUDES) -D_BSD_SOURCE
LD            = gcc
LDFLAGS       = $(EXCEPTION) -L$(ANITA_LIB_DIR)
ANITA_LIBS    =  -lkvp -lConfig -lUtil -lcr7 -lm #-lSocket
ANITA_HKD_LIBS = -lAcromag
ANITA_GPS_LIBS = -lSerial
ANITA_SIP_LIBS = -lSipcom


all: $(Target)

%.$(ObjSuf) : %.$(SrcSuf)
	@echo "<**Compiling**> "$<
	@$(CC) $(CCFLAGS) -c $< -o  $@

objclean:
	@-rm -f *.o




















