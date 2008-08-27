# 
#
#

include ${ANITA_FLIGHT_SOFT_DIR}/standard_definitions.mk

#SOFLAGS       = -shared


LIB_OBJS         =  carrier/apc8620.o ip320/ainsc320.o ip320/drvr320.o \
	ip320/rsts320.o ip320/mccd320.o ip320/sccd320.o ip320/ainmc320.o \
	ip470/drvr470.o ip470/rprt470.o  ip470/wpnt470.o \
	ip470/cnfg470.o  ip470/rpnt470.o  ip470/rsts470.o  ip470/wprt470.o



Target       = $(ANITA_LIB_DIR)/libAcromag.a


all: $(Target)

$(Target): $(LIB_OBJS)
	@/bin/rm -f $(Target)	
#	@echo "Creating $(Target) ..."
	@ar -r $@ $^
#	@$(LD) $(LDFLAGS) $(LIBS) $(SOFLAGS) $(Objects) -o $(Target)
#	@chmod 555 $(Target)

clean: objclean
	@/bin/rm -f ${LIB_OBJS}
	@/bin/rm -f $(Target)





















