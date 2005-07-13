# 
#
#

include ${ANITA_FLIGHT_SOFT_DIR}/standard_definitions.mk

SUBDIRS = common outside/acromag outside/sipcom programs #testing

all: subdirs scripts

docs:
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
	@chmod u+x ${ANITA_FLIGHT_SOFT_DIR}/script/getLogWindow.sh 
	@chmod u+x ${ANITA_FLIGHT_SOFT_DIR}/script/startProgsInXterms.sh 
	@chmod u+x ${ANITA_FLIGHT_SOFT_DIR}/script/killAllProgs.sh 
	@chmod u+x ${ANITA_FLIGHT_SOFT_DIR}/script/turnRFCMOff
	@chmod u+x ${ANITA_FLIGHT_SOFT_DIR}/script/turnRFCMOn
	@chmod u+x ${ANITA_FLIGHT_SOFT_DIR}/script/turnNDOff
	@chmod u+x ${ANITA_FLIGHT_SOFT_DIR}/script/turnNDOn
	@chmod u+x ${ANITA_FLIGHT_SOFT_DIR}/script/turnGPSOff
	@chmod u+x ${ANITA_FLIGHT_SOFT_DIR}/script/turnGPSOn
	@chmod u+x ${ANITA_FLIGHT_SOFT_DIR}/script/turnCalPulserOff
	@chmod u+x ${ANITA_FLIGHT_SOFT_DIR}/script/turnCalPulserOn
	@chmod u+x ${ANITA_FLIGHT_SOFT_DIR}/script/triggerCalPulser
	@chmod u+x ${ANITA_FLIGHT_SOFT_DIR}/script/setSSGain
	@chmod u+x ${ANITA_FLIGHT_SOFT_DIR}/script/setCalPulserAddr
	@ln -sf ${ANITA_FLIGHT_SOFT_DIR}/script/getLogWindow.sh bin
	@ln -sf ${ANITA_FLIGHT_SOFT_DIR}/script/startProgsInXterms.sh bin
	@ln -sf ${ANITA_FLIGHT_SOFT_DIR}/script/killAllProgs.sh bin
	@ln -sf ${ANITA_FLIGHT_SOFT_DIR}/script/turnRFCMOff bin
	@ln -sf ${ANITA_FLIGHT_SOFT_DIR}/script/turnRFCMOn bin
	@ln -sf ${ANITA_FLIGHT_SOFT_DIR}/script/turnNDOff bin
	@ln -sf ${ANITA_FLIGHT_SOFT_DIR}/script/turnNDOn bin
	@ln -sf ${ANITA_FLIGHT_SOFT_DIR}/script/turnGPSOff bin
	@ln -sf ${ANITA_FLIGHT_SOFT_DIR}/script/turnGPSOn bin
	@ln -sf ${ANITA_FLIGHT_SOFT_DIR}/script/turnCalPulserOff bin
	@ln -sf ${ANITA_FLIGHT_SOFT_DIR}/script/turnCalPulserOn bin
	@ln -sf ${ANITA_FLIGHT_SOFT_DIR}/script/triggerCalPulser bin
	@ln -sf ${ANITA_FLIGHT_SOFT_DIR}/script/setSSGain bin
	@ln -sf ${ANITA_FLIGHT_SOFT_DIR}/script/setCalPulserAddr bin
















