
MODCFLAGS := -Wall

a.out:		ioscarrier.o drvr521.o portio.o uart.o rsts521.o
			cc -o a.out ioscarrier.o drvr521.o portio.o uart.o rsts521.o

ioscarrier.o:	../ioscarrier/ioscarrier.c ../ioscarrier/ioscarrier.h
			$(CC) $(MODCFLAGS) -c ../ioscarrier/ioscarrier.c

drvr521.o:		drvr521.c portio.h
			$(CC) $(MODCFLAGS) -c drvr521.c

portio.o:		portio.c portio.h
			$(CC) $(MODCFLAGS) -c portio.c

uart.o:		uart.c portio.h
			$(CC) $(MODCFLAGS) -c uart.c

rsts521.o:		rsts521.c portio.h
			$(CC) $(MODCFLAGS) -c rsts521.c
clean:
			\rm -f *.o *~ a.out

