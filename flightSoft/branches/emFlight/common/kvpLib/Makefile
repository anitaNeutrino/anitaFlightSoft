#-------------------------------------------------------------------------
# Makefile for Key/Value pair library and test programs
#
# Author : G.J.Crone
# Stolen by : R. Nichol
#
#
#-------------------------------------------------------------------------

include ${ANITA_FLIGHT_SOFT_DIR}/standard_definitions.mk


LIB_OBJS         =  keyValuePair.o

Target       = $(ANITA_LIB_DIR)/libkvp.a


all: $(Target)

$(Target): $(LIB_OBJS)
	@/bin/rm -f $(Target)	
	@echo "Linking $(Target) ..."
	@ar -r $@ $^

clean: objclean
	@/bin/rm -f $(Target)

