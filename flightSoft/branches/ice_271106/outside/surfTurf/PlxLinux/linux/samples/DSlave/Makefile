#-----------------------------------------------------------------------------
#
#      File         :  Makefile
#      Abstract     :  The makefile for building an Application
#      Last Revision:  03-01-02
#      SDK Version  :  3.50
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
ImageName   = DSlave$(DBG)
TGT_TYPE    = App
#DEBUG       = 1


#=============================================================================
# Put all object files here
#=============================================================================
OBJECTS	= \
	$(OBJ_DIR)/DSlave.o   \
	$(OBJ_DIR)/ConsFunc.o \
	$(OBJ_DIR)/PlxInit.o



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
