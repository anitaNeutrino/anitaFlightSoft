#-----------------------------------------------------------------------------
#
#      File         :  Makefile
#      Abstract     :  The makefile for building an Application
#      Last Revision:  03-01-02
#      SDK Version  :  3.40
#      Usage        :  To Build Target:
#                          make
#
#                      To Cleanup Intermdiate files only:
#                          make clean
#
#                      To Cleanup All files:
#                          make cleanall
#
#-----------------------------------------------------------------------------


#=============================================================================
# Modify the following lines as needed:
#
# ImageName   = The final image name
# TGT_TYPE    = Type of Target image [App | Library | Driver]
# DEBUG       = Add/remove the comment symbol(#) to disable/enable debugging
#=============================================================================
ImageName   = ApiTest$(DBG)
TGT_TYPE    = App
#DEBUG       = 1


#=============================================================================
# Put all object files here
#=============================================================================
OBJECTS	= \
	$(OBJ_DIR)/ApiTest.o  \
	$(OBJ_DIR)/ConsFunc.o \
	$(OBJ_DIR)/PlxInit.o  \
	$(OBJ_DIR)/RegDefs.o  \
	$(OBJ_DIR)/Test9030.o \
	$(OBJ_DIR)/Test9050.o \
	$(OBJ_DIR)/Test9054.o \
	$(OBJ_DIR)/Test9656.o \
	$(OBJ_DIR)/Test9080.o



#=============================================================================
# Include standard common definitions
#=============================================================================
include $(PLX_SDK_DIR)/linux/makefiles/Common.def



#=============================================================================
# Target builds
#=============================================================================
all:    DisplayParameters Build$(TGT_TYPE)



#=============================================================================
# Include definitions for the tools
#=============================================================================
include $(PLX_SDK_DIR)/linux/makefiles/Gcc.def



#=============================================================================
# Include definitions for building targets
#=============================================================================
include $(PLX_SDK_DIR)/linux/makefiles/Targets.def
