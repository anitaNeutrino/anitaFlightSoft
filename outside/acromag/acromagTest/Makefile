# 
#
#

include ${ANITA_FLIGHT_SOFT_DIR}/standard_definitions.mk

#SOFLAGS       = -shared


Objects         =  acromagTest.o
LIBS	     = -lAcromag


Target       = acromagTest


all: $(Target)

$(Target): $(Objects)
	@/bin/rm -f $(Target)	
	@echo "Creating $(Target) ..."
#	@ar -r $@ $^
	@$(LD) $(Objects) $(LDFLAGS) $(LIBS) $(SOFLAGS) -o $(Target)
	@chmod 555 $(Target)

clean: objclean
	@/bin/rm -f ${LIB_OBJS}
	@/bin/rm -f $(Target)





















