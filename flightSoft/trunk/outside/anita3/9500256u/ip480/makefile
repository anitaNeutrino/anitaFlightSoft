
MODCFLAGS := -Wall

a.out:			carrier.o cnfg480.o drvr480.o rmid480.o
			cc -o a.out carrier.o cnfg480.o drvr480.o rmid480.o

carrier.o:		../carrier/carrier.c ../carrier/carrier.h
			$(CC) $(MODCFLAGS) -c ../carrier/carrier.c

drvr480.o:		drvr480.c ip480.h
			$(CC) $(MODCFLAGS) -c drvr480.c

cnfg480.o:		cnfg480.c ip480.h
			$(CC) $(MODCFLAGS) -c cnfg480.c

rmid480.o:		rmid480.c ip480.h
			$(CC) $(MODCFLAGS) -c rmid480.c

clean:
			\rm -f *.o *~ a.out

