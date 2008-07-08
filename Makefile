# 
#
#

include ${ANITA_FLIGHT_SOFT_DIR}/standard_definitions.mk

SUBDIRS = common outside/acromag programs #testing

ANITA_INSTALL_LIBS=libAcromag.a libAnitaMap.a libAnitaMap.so libCompress.a libCompress.so libConfig.a libConfig.so libkvp.a libkvp.so libPedestal.a libPedestal.so libSerial.a libSipcom.a libSlow.a libSlow.so libUtil.a libUtil.so libLinkWatch.so libAnitaFtp.so libAnitaFtp.a

ANITA_INSTALL_BINS=SIPd LogWatchd getConfigValue getConfigString

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
	@for file in ${ANITA_FLIGHT_SOFT_DIR}/script/*[^~]; do \
	( chmod a+x $$file; ln -sf $$file bin; ) ; \
	done	

install:
#	$(INSTALL) -c -o $(OWNER) -g $(GROUP) -m 755 programs/SIPd/SIPd $(BINDIR)
	@for aBin in $(ANITA_INSTALL_BINS); do \
	( echo "Installing $(BINDIR)/$$aBin"; $(INSTALL) -c -o $(OWNER) -g $(GROUP) -m 755 bin/$$aBin $(BINDIR) ) ; \
	done

	@for aLib in $(ANITA_INSTALL_LIBS); do \
	( echo "Installing $(LIBDIR)/$$aLib"; $(INSTALL) -c -o $(OWNER) -g $(GROUP) -m 755 lib/$$aLib $(LIBDIR) ) ; \
	done

uninstall:
	rm -f $(BINDIR)/SIPd
	@for aLib in $(ANITA_INSTALL_LIBS); do \
	( echo "Deleting $(LIBDIR)/$$aLib"; rm -f $(LIBDIR)/$$aLib ) ; \
	done











