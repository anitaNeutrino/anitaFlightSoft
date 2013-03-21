
MODCFLAGS := -Wall

a.out:		ioscarrier.o drvr520.o portio.o uart.o rsts520.o
			cc -o a.out ioscarrier.o drvr520.o portio.o uart.o rsts520.o

ioscarrier.o:	../ioscarrier/ioscarrier.c ../ioscarrier/ioscarrier.h
			$(CC) $(MODCFLAGS) -c ../ioscarrier/ioscarrier.c

drvr520.o:		drvr520.c portio.h
			$(CC) $(MODCFLAGS) -c drvr520.c

portio.o:		portio.c portio.h
			$(CC) $(MODCFLAGS) -c portio.c

uart.o:		uart.c portio.h
			$(CC) $(MODCFLAGS) -c uart.c

rsts520.o:		rsts520.c portio.h
			$(CC) $(MODCFLAGS) -c rsts520.c
clean:
			\rm -f *.o *~ a.out

