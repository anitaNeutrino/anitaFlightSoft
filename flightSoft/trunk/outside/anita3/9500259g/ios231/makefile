
MODCFLAGS := -Wall

a.out:		ioscarrier.o cd231.o drvr231.o coef231.o mode231.o rccid231.o wro231.o rdo231.o program.o

			cc -o a.out ioscarrier.o cd231.o drvr231.o coef231.o mode231.o rccid231.o wro231.o rdo231.o program.o


ioscarrier.o:	../ioscarrier/ioscarrier.c ../ioscarrier/ioscarrier.h
			$(CC) $(MODCFLAGS) -c ../ioscarrier/ioscarrier.c

drvr231.o:		drvr231.c ios231.h
			$(CC) $(MODCFLAGS) -c drvr231.c

cd231.o:		cd231.c ios231.h
			$(CC) $(MODCFLAGS) -c cd231.c

coef231.o:		coef231.c ios231.h
			$(CC) $(MODCFLAGS) -c coef231.c

mode231.o:		mode231.c ios231.h
			$(CC) $(MODCFLAGS) -c mode231.c

rccid231.o:		rccid231.c ios231.h
			$(CC) $(MODCFLAGS) -c rccid231.c

wro231.o:		wro231.c ios231.h
			$(CC) $(MODCFLAGS) -c wro231.c

rdo231.o:		rdo231.c ios231.h
			$(CC) $(MODCFLAGS) -c rdo231.c

program.o:		program.c ios231.h
			$(CC) $(MODCFLAGS) -c program.c

clean:
			\rm -f *.o *~ a.out
