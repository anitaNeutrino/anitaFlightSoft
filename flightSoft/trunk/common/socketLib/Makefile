# 
#
#

include ${ANITA_FLIGHT_SOFT_DIR}/standard_definitions.mk


LIB_OBJS         =  socketLib.o

Target       = $(ANITA_LIB_DIR)/libSocket.a


all: $(Target)

$(Target): $(LIB_OBJS)
	@/bin/rm -f $(Target)	
#	@echo "Creating $(Target) ..."
	@ar -r $@ $^
#	@$(LD) $(LDFLAGS) $(LIBS) $(SOFLAGS) $(Objects) -o $(Target)
#	@chmod 555 $(Target)

clean: objclean
	@/bin/rm -f $(Target)





















