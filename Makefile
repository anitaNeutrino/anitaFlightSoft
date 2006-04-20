# 
#
#

include ${ANITA_FLIGHT_SOFT_DIR}/standard_definitions.mk

SUBDIRS = common outside/acromag outside/sipcom programs #testing

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
	@chmod u+x ${ANITA_FLIGHT_SOFT_DIR}/script/getLogWindow.sh 
	@chmod u+x ${ANITA_FLIGHT_SOFT_DIR}/script/startProgsInXterms.sh 
	@chmod u+x ${ANITA_FLIGHT_SOFT_DIR}/script/killAllProgs.sh 
	@chmod u+x ${ANITA_FLIGHT_SOFT_DIR}/script/turnRFCM1Off
	@chmod u+x ${ANITA_FLIGHT_SOFT_DIR}/script/turnRFCM1On 
	@chmod u+x ${ANITA_FLIGHT_SOFT_DIR}/script/turnRFCM2Off
	@chmod u+x ${ANITA_FLIGHT_SOFT_DIR}/script/turnRFCM2On 
	@chmod u+x ${ANITA_FLIGHT_SOFT_DIR}/script/turnRFCM3Off
	@chmod u+x ${ANITA_FLIGHT_SOFT_DIR}/script/turnRFCM3On
	@chmod u+x ${ANITA_FLIGHT_SOFT_DIR}/script/turnRFCM4Off
	@chmod u+x ${ANITA_FLIGHT_SOFT_DIR}/script/turnRFCM4On
	@chmod u+x ${ANITA_FLIGHT_SOFT_DIR}/script/turnVetoOff
	@chmod u+x ${ANITA_FLIGHT_SOFT_DIR}/script/turnVetoOn
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
	@ln -sf ${ANITA_FLIGHT_SOFT_DIR}/script/turnRFCM1Off bin
	@ln -sf ${ANITA_FLIGHT_SOFT_DIR}/script/turnRFCM1On bin
	@ln -sf ${ANITA_FLIGHT_SOFT_DIR}/script/turnRFCM2Off bin
	@ln -sf ${ANITA_FLIGHT_SOFT_DIR}/script/turnRFCM2On bin
	@ln -sf ${ANITA_FLIGHT_SOFT_DIR}/script/turnRFCM3Off bin
	@ln -sf ${ANITA_FLIGHT_SOFT_DIR}/script/turnRFCM3On bin
	@ln -sf ${ANITA_FLIGHT_SOFT_DIR}/script/turnRFCM4Off bin
	@ln -sf ${ANITA_FLIGHT_SOFT_DIR}/script/turnRFCM4On bin
	@ln -sf ${ANITA_FLIGHT_SOFT_DIR}/script/turnVetoOff bin
	@ln -sf ${ANITA_FLIGHT_SOFT_DIR}/script/turnVetoOn bin
	@ln -sf ${ANITA_FLIGHT_SOFT_DIR}/script/turnGPSOff bin
	@ln -sf ${ANITA_FLIGHT_SOFT_DIR}/script/turnGPSOn bin
	@ln -sf ${ANITA_FLIGHT_SOFT_DIR}/script/turnAllOff bin
	@ln -sf ${ANITA_FLIGHT_SOFT_DIR}/script/turnAllOn bin
	@ln -sf ${ANITA_FLIGHT_SOFT_DIR}/script/turnCalPulserOff bin
	@ln -sf ${ANITA_FLIGHT_SOFT_DIR}/script/turnCalPulserOn bin
	@ln -sf ${ANITA_FLIGHT_SOFT_DIR}/script/triggerCalPulser bin
	@ln -sf ${ANITA_FLIGHT_SOFT_DIR}/script/setSSGain bin
	@ln -sf ${ANITA_FLIGHT_SOFT_DIR}/script/setCalPulserAddr bin
	@ln -sf ${ANITA_FLIGHT_SOFT_DIR}/script/runAcqdStandalone.sh bin














