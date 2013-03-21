
MODCFLAGS := -Wall

a.out:		ioscarrier.o drvrlst.o
			cc -o a.out ioscarrier.o drvrlst.o

ioscarrier.o:	../ioscarrier/ioscarrier.c ../ioscarrier/ioscarrier.h
			$(CC) $(MODCFLAGS) -c ../ioscarrier/ioscarrier.c

drvrlst.o:		drvrlst.c ioslst.h
			$(CC) $(MODCFLAGS) -c drvrlst.c

clean:
			\rm -f *.o *~ a.out

