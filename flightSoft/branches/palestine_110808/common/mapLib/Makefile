# 
#
#

include ${ANITA_FLIGHT_SOFT_DIR}/standard_definitions.mk

LIB_OBJS         =  mapLib.o



Name = libAnitaMap
Library  = $(ANITA_LIB_DIR)/$(Name).a
DynLib  = $(ANITA_LIB_DIR)/$(Name).${DllSuf}


all: $(Library) $(DynLib)

$(Library): $(LIB_OBJS)
	@/bin/rm -f $(Library)	
	@ar -r $@ $^

$(DynLib): $(LIB_OBJS)
	@/bin/rm -f $(DynLib)	
	@echo "Creating $(DynLib) ..."
	@$(LD) $(LDFLAGS) $(LIBS) $(SOFLAGS) $(LIB_OBJS) -o $(DynLib)
	@chmod 555 $(DynLib)

clean: objclean
	@/bin/rm -f $(Library) $(DynLib)





















