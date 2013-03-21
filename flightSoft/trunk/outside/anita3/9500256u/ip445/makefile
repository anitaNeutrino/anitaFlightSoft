
MODCFLAGS := -Wall

a.out:			carrier.o drvr445.o rsts445.o rpnt445.o rprt445.o wpnt445.o wprt445.o wcntl445.o
			cc -o a.out carrier.o drvr445.o rsts445.o rpnt445.o rprt445.o wpnt445.o wprt445.o wcntl445.o

carrier.o:		../carrier/carrier.c ../carrier/carrier.h
			$(CC) $(MODCFLAGS) -c ../carrier/carrier.c

drvr445.o:		drvr445.c ip445.h
			$(CC) $(MODCFLAGS) -c drvr445.c

rsts445.o:		rsts445.c ip445.h
			$(CC) $(MODCFLAGS) -c rsts445.c

rpnt445.o:		rpnt445.c ip445.h
			$(CC) $(MODCFLAGS) -c rpnt445.c

rprt445.o:		rprt445.c ip445.h
			$(CC) $(MODCFLAGS) -c rprt445.c

wpnt445.o:		wpnt445.c ip445.h
			$(CC) $(MODCFLAGS) -c wpnt445.c

wprt445.o:		wprt445.c ip445.h
			$(CC) $(MODCFLAGS) -c wprt445.c

wcntl445.o:		wcntl445.c ip445.h
			$(CC) $(MODCFLAGS) -c wcntl445.c

clean:
			\rm -f *.o *~ a.out



