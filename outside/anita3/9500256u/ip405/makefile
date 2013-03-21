
MODCFLAGS := -Wall

a.out:			carrier.o drvr405.o rsts405.o rpnt405.o rprt405.o wpnt405.o wprt405.o
			cc -o a.out carrier.o drvr405.o rsts405.o rpnt405.o rprt405.o wpnt405.o wprt405.o

carrier.o:		../carrier/carrier.c ../carrier/carrier.h
			$(CC) $(MODCFLAGS) -c ../carrier/carrier.c

drvr405.o:		drvr405.c ip405.h
			$(CC) $(MODCFLAGS) -c drvr405.c

rsts405.o:		rsts405.c ip405.h
			$(CC) $(MODCFLAGS) -c rsts405.c

rpnt405.o:		rpnt405.c ip405.h
			$(CC) $(MODCFLAGS) -c rpnt405.c

rprt405.o:		rprt405.c ip405.h
			$(CC) $(MODCFLAGS) -c rprt405.c

wpnt405.o:		wpnt405.c ip405.h
			$(CC) $(MODCFLAGS) -c wpnt405.c

wprt405.o:		wprt405.c ip405.h
			$(CC) $(MODCFLAGS) -c wprt405.c

clean:
			\rm -f *.o *~ a.out

