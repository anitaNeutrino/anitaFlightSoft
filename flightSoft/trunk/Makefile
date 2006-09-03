# 
#
#

include ${ANITA_FLIGHT_SOFT_DIR}/standard_definitions.mk

SUBDIRS = common outside/acromag programs #testing

all: subdirs scripts

configDocs:
	@script/makeHtmledConfigFiles.sh

docs: configDocs
	@doxygen doc/doxy.config

subdirs:
	@for aDir in $(SUBDIRS); do \
	( cd $$aDir ; make ) ; \
	done

clean:
	@for aDir in $(SUBDIRS); do \
	( cd $$aDir ; make clean ) ; \
	done


scripts:
	for file in ${ANITA_FLIGHT_SOFT_DIR}/script/*[^~]; do \
	( chmod a+x $$file; ln -sf $$file bin; ) ; \
	done	















