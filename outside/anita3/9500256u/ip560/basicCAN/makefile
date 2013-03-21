
MODCFLAGS := -Wall

a.out:		carrier.o drvr560b.o sja1000b.o
			cc -o a.out carrier.o drvr560b.o sja1000b.o

carrier.o:		../../carrier/carrier.c ../../carrier/carrier.h
			$(CC) $(MODCFLAGS) -c ../../carrier/carrier.c

drvr560b.o:		drvr560b.c ip560b.h
			$(CC) $(MODCFLAGS) -c drvr560b.c

sja1000b.o:		sja1000b.c ip560b.h
			$(CC) $(MODCFLAGS) -c sja1000b.c

clean:
			\rm -f *.o *~ a.out

