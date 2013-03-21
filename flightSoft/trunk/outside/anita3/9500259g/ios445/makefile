
MODCFLAGS := -Wall

a.out:		ioscarrier.o drvr445.o rsts445.o rpnt445.o rprt445.o wpnt445.o wprt445.o wcntl445.o
			cc -o a.out ioscarrier.o drvr445.o rsts445.o rpnt445.o rprt445.o wpnt445.o wprt445.o wcntl445.o

ioscarrier.o:	../ioscarrier/ioscarrier.c ../ioscarrier/ioscarrier.h
			$(CC) $(MODCFLAGS) -c ../ioscarrier/ioscarrier.c

drvr445.o:		drvr445.c ios445.h
			$(CC) $(MODCFLAGS) -c drvr445.c

rsts445.o:		rsts445.c ios445.h
			$(CC) $(MODCFLAGS) -c rsts445.c

rpnt445.o:		rpnt445.c ios445.h
			$(CC) $(MODCFLAGS) -c rpnt445.c

rprt445.o:		rprt445.c ios445.h
			$(CC) $(MODCFLAGS) -c rprt445.c

wpnt445.o:		wpnt445.c ios445.h
			$(CC) $(MODCFLAGS) -c wpnt445.c

wprt445.o:		wprt445.c ios445.h
			$(CC) $(MODCFLAGS) -c wprt445.c

wcntl445.o:		wcntl445.c ios445.h
			$(CC) $(MODCFLAGS) -c wcntl445.c

clean:
			\rm -f *.o *~ a.out
