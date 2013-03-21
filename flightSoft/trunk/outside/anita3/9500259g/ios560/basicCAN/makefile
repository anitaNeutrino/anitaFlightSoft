
MODCFLAGS := -Wall

a.out:		ioscarrier.o drvr560b.o sja1000b.o
			cc -o a.out ioscarrier.o drvr560b.o sja1000b.o

ioscarrier.o:		../../ioscarrier/ioscarrier.c ../../ioscarrier/ioscarrier.h
			$(CC) $(MODCFLAGS) -c ../../ioscarrier/ioscarrier.c

drvr560b.o:		drvr560b.c ios560b.h
			$(CC) $(MODCFLAGS) -c drvr560b.c

sja1000b.o:		sja1000b.c ios560b.h
			$(CC) $(MODCFLAGS) -c sja1000b.c

clean:
			\rm -f *.o *~ a.out

