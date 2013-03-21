
MODCFLAGS :=

a.out:		ioscarrier.o cnfg482.o drvr482.o rmid482.o rpnt482.o\
			rprt482.o wpnt482.o wprt482.o

			cc -o a.out ioscarrier.o cnfg482.o drvr482.o rmid482.o\
			rpnt482.o rprt482.o wpnt482.o wprt482.o

ioscarrier.o:	../ioscarrier/ioscarrier.c ../ioscarrier/ioscarrier.h
			$(CC) $(MODCFLAGS) -c ../ioscarrier/ioscarrier.c

drvr482.o:		drvr482.c ios482.h
			$(CC) $(MODCFLAGS) -c drvr482.c

cnfg482.o:		cnfg482.c ios482.h
			$(CC) $(MODCFLAGS) -c cnfg482.c

rmid482.o:		rmid482.c ios482.h
			$(CC) $(MODCFLAGS) -c rmid482.c

rpnt482.o:		rpnt482.c ios482.h
			$(CC) $(MODCFLAGS) -c rpnt482.c

rprt482.o:		rprt482.c ios482.h
			$(CC) $(MODCFLAGS) -c rprt482.c

wpnt482.o:		wpnt482.c ios482.h
			$(CC) $(MODCFLAGS) -c wpnt482.c

wprt482.o:		wprt482.c ios482.h
			$(CC) $(MODCFLAGS) -c wprt482.c
clean:
			\rm -f *.o *~ a.out
