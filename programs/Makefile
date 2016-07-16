# 
#
#

include ${ANITA_FLIGHT_SOFT_DIR}/standard_definitions.mk

SUBDIRS = Acqd Archived Calibd Cmdd Eventd GPSd Hkd LOSd Monitord SIPd other Prioritizerd Playbackd LogWatchd Openportd NTUd RTLd 

all: subdirs

subdirs:
	@for aDir in $(SUBDIRS); do \
	( cd $$aDir ; make ) ; \
	done

clean:
	@for aDir in $(SUBDIRS); do \
	( cd $$aDir ; make clean ) ; \
	done



















