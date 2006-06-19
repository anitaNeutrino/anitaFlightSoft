#-----------------------------------------------------------------------------
#
#      File         :  Makefile
#      Abstract     :  The makefile for building the API library
#      Last Revision:  09-01-03
#      SDK Version  :  4.20
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
# DEBUG       = Add/remove the comment symbol(#) to disable/enable debugging.
#=============================================================================
ImageName   = PciDrvApi$(DBG)
TGT_TYPE    = Library
#DEBUG       = 1


#=============================================================================
# Put all object files here
#=============================================================================
OBJECTS	= \
	$(OBJ_DIR)/PciDrvApi.o



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
