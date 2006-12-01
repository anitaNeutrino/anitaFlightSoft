#-------------------------------------------------------------------------
# Makefile for config library and test programs
#
# Author : G.J.Crone
# Stolen by : R. Nichol
#
#
#-------------------------------------------------------------------------

include ${ANITA_FLIGHT_SOFT_DIR}/standard_definitions.mk
SOFLAGS       = -shared

LIB_OBJS =  configLoad.o configStore.o configError.o configFileSpec.o

Name = libConfig
DynLib  = $(ANITA_LIB_DIR)/$(Name).so
Library = $(ANITA_LIB_DIR)/$(Name).a

all: $(Library) $(DynLib)

$(Library): $(LIB_OBJS)
	@/bin/rm -f $(Library)	
	@echo "Linking $(Library) ..."
	@ar -r $@ $^

$(DynLib): $(LIB_OBJS)
	@/bin/rm -f $(DynLib)	
	@echo "Creating $(DynLib) ..."
	@$(LD) $(LDFLAGS) $(LIBS) $(SOFLAGS) $(LIB_OBJS) -o $(DynLib)
	@chmod 555 $(DynLib)

clean: objclean
	@/bin/rm -f $(Library) $(DynLib)

