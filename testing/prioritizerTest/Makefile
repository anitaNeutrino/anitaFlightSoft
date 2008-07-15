#############################################################################
####


#Global Thingies
CC 	= g++
LD	= g++
SOFLAGS	= -shared
OBJSUF	= o
SRCSUF	= cxx
OPT	= -O2 -g --debug  -Wall -fPIC

ROOTINCLUDES      = -I$(ROOTSYS)/include
ifdef EVENT_READER_DIR
   INCLUDES	= -I$(EVENT_READER_DIR)
endif
CXXFLAGS	= $(EXCEPTION) $(OPT) $(CXXOPT) -fPIC $(INCLUDES) $(ROOTINCLUDES)

ROOTLIBS      = -L$(ROOTSYS)/lib $(shell $(ROOTSYS)/bin/root-config --libs) -lMinuit -lMathMore
ROOTGLIBS     = -L$(ROOTSYS)/lib $(shell $(ROOTSYS)/bin/root-config --glibs)
LIBS		= -L/unix/anita/software/install/lib/ -L/home/rjn/install/lib -lz -lm $(ROOTLIBS) -L$(EVENT_READER_DIR) -lAnitaEvent -lfftw3 -lRootFftwWrapper -lgsl



#ROOT stuff




TREE_MAKER = waveformToPrioritizer prioritizerToRoot



all : $(TREE_MAKER)

% :  %.$(SRCSUF)
	@echo "<**Linking**> "  
	$(LD)  $(CXXFLAGS) $(LDFLAGS) $<  $(LIBS) -o $@


clean:
	@rm -f *Dict*
	@rm -f *.${OBJSUF}
	@rm -f $(TREE_MAKER)
