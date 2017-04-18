# 
#
#
include ${ANITA_FLIGHT_SOFT_DIR}/standard_definitions.mk


LIB_OBJS         =  magneticLib.o geomag70_mod.o 

Target       = $(ANITA_LIB_DIR)/libMagnetic.a
SharedTarget       = $(ANITA_LIB_DIR)/libMagnetic.so


all: $(Target) $(SharedTarget)

$(Target): $(LIB_OBJS)
	@/bin/rm -f $(Target)	
#	@echo "Creating $(Target) ..."
	@ar -r $@ $^

$(SharedTarget): $(LIB_OBJS)
	@$(LD) $(LDFLAGS) $(LIBS) $(SOFLAGS) $(LIB_OBJS) -o $(SharedTarget)
#	@chmod 555 $(Target)

clean: objclean
	@/bin/rm -f $(Target) $(SharedTarget)





















