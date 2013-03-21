
MODCFLAGS := -Wall

a.out:		carrier.o drvr560p.o sja1000p.o
			cc -o a.out carrier.o drvr560p.o sja1000p.o

carrier.o:		../../carrier/carrier.c ../../carrier/carrier.h
			$(CC) $(MODCFLAGS) -c ../../carrier/carrier.c

drvr560p.o:		drvr560p.c ip560p.h
			$(CC) $(MODCFLAGS) -c drvr560p.c

sja1000p.o:		sja1000p.c ip560p.h
			$(CC) $(MODCFLAGS) -c sja1000p.c

clean:
			\rm -f *.o *~ a.out


