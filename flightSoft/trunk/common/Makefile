# 
#
#

include ${ANITA_FLIGHT_SOFT_DIR}/standard_definitions.mk


SUBDIRS = configLib kvpLib socketLib utilLib serialLib losLib sipcomLib compressLib pedestalLib mapLib slowLib linkWatchLib ftpLib sbsTempLib

all: subdirs

subdirs:
	@for aDir in $(SUBDIRS); do \
	( cd $$aDir ; make ) ; \
	done

clean:
	@for aDir in $(SUBDIRS); do \
	( cd $$aDir ; make clean ) ; \
	done



















