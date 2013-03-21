
MODCFLAGS := -Wall

a.out:		ioscarrier.o ain341.o cnfg341.o drvr341.o mccd341.o rsts341.o rwcc341.o

			cc -o a.out ioscarrier.o ain341.o cnfg341.o drvr341.o mccd341.o rsts341.o rwcc341.o

ioscarrier.o:	../ioscarrier/ioscarrier.c ../ioscarrier/ioscarrier.h
			$(CC) $(MODCFLAGS) -c ../ioscarrier/ioscarrier.c

drvr341.o:		drvr341.c ios341.h
			$(CC) $(MODCFLAGS) -c drvr341.c

ain341.o:		ain341.c ios341.h
			$(CC) $(MODCFLAGS) -c ain341.c

cnfg341.o:		cnfg341.c ios341.h
			$(CC) $(MODCFLAGS) -c cnfg341.c

mccd341.o:		mccd341.c ios341.h
			$(CC) $(MODCFLAGS) -c mccd341.c

rsts341.o:		rsts341.c ios341.h
			$(CC) $(MODCFLAGS) -c rsts341.c

rwcc341.o:		rwcc341.c ios341.h
			$(CC) $(MODCFLAGS) -c rwcc341.c
clean:
			\rm -f *.o *~ a.out
