
MODCFLAGS := -Wall

a.out:		ioscarrier.o drvr560p.o sja1000p.o
			cc -o a.out ioscarrier.o drvr560p.o sja1000p.o

ioscarrier.o:		../../ioscarrier/ioscarrier.c ../../ioscarrier/ioscarrier.h
			$(CC) $(MODCFLAGS) -c ../../ioscarrier/ioscarrier.c

drvr560p.o:		drvr560p.c ios560p.h
			$(CC) $(MODCFLAGS) -c drvr560p.c

sja1000p.o:		sja1000p.c ios560p.h
			$(CC) $(MODCFLAGS) -c sja1000p.c

clean:
			\rm -f *.o *~ a.out

