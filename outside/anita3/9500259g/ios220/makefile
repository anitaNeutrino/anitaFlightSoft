
MODCFLAGS := -Wall

a.out:		ioscarrier.o cd220.o drvr220.o coef220.o mode220.o rccid220.o wro220.o

			cc -o a.out ioscarrier.o cd220.o drvr220.o coef220.o mode220.o rccid220.o wro220.o

ioscarrier.o:	../ioscarrier/ioscarrier.c ../ioscarrier/ioscarrier.h
			$(CC) $(MODCFLAGS) -c ../ioscarrier/ioscarrier.c

drvr220.o:		drvr220.c ios220.h
			$(CC) $(MODCFLAGS) -c drvr220.c

cd220.o:		cd220.c ios220.h
			$(CC) $(MODCFLAGS) -c cd220.c

coef220.o:		coef220.c ios220.h
			$(CC) $(MODCFLAGS) -c coef220.c

mode220.o:		mode220.c ios220.h
			$(CC) $(MODCFLAGS) -c mode220.c

rccid220.o:		rccid220.c ios220.h
			$(CC) $(MODCFLAGS) -c rccid220.c

wro220.o:		wro220.c ios220.h
			$(CC) $(MODCFLAGS) -c wro220.c

clean:
			\rm -f *.o *~ a.out

