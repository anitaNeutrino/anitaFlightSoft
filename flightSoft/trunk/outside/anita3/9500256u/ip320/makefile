
MODCFLAGS := -Wall

a.out:		carrier.o ainmc320.o ainsc320.o drvr320.o mccd320.o sccd320.o rsts320.o

			cc -o a.out carrier.o ainmc320.o ainsc320.o drvr320.o mccd320.o rsts320.o sccd320.o

carrier.o:		../carrier/carrier.c ../carrier/carrier.h
			$(CC) $(MODCFLAGS) -c ../carrier/carrier.c

drvr320.o:		drvr320.c ip320.h
			$(CC) $(MODCFLAGS) -c drvr320.c

ainmc320.o:		ainmc320.c ip320.h
			$(CC) $(MODCFLAGS) -c ainmc320.c

ainsc320.o:		ainsc320.c ip320.h
			$(CC) $(MODCFLAGS) -c ainsc320.c

mccd320.o:		mccd320.c ip320.h
			$(CC) $(MODCFLAGS) -c mccd320.c

sccd320.o:		sccd320.c ip320.h
			$(CC) $(MODCFLAGS) -c sccd320.c

rsts320.o:		rsts320.c ip320.h
			$(CC) $(MODCFLAGS) -c rsts320.c
clean:
			\rm -f *.o *~ a.out

