
MODCFLAGS := -Wall

a.out:		ioscarrier.o ain330.o cnfg330.o drvr330.o mccd330.o rsts330.o

			cc -o a.out ioscarrier.o ain330.o cnfg330.o drvr330.o mccd330.o rsts330.o

ioscarrier.o:	../ioscarrier/ioscarrier.c ../ioscarrier/ioscarrier.h
			$(CC) $(MODCFLAGS) -c ../ioscarrier/ioscarrier.c

drvr330.o:		drvr330.c ios330.h
			$(CC) $(MODCFLAGS) -c drvr330.c

ain330.o:		ain330.c ios330.h
			$(CC) $(MODCFLAGS) -c ain330.c

cnfg330.o:		cnfg330.c ios330.h
			$(CC) $(MODCFLAGS) -c cnfg330.c

mccd330.o:		mccd330.c ios330.h
			$(CC) $(MODCFLAGS) -c mccd330.c

rsts330.o:		rsts330.c ios330.h
			$(CC) $(MODCFLAGS) -c rsts330.c

clean:
			\rm -f *.o *~ a.out

