
MODCFLAGS := -Wall

a.out:		carrier.o cd236.o cnfg236.o rmid236.o wro236.o rwcc236.o drvr236.o

			cc -o a.out carrier.o cd236.o drvr236.o cnfg236.o rmid236.o wro236.o rwcc236.o -lm

carrier.o:		../carrier/carrier.c ../carrier/carrier.h
			$(CC) $(MODCFLAGS) -c ../carrier/carrier.c

drvr236.o:		drvr236.c ip236.h
			$(CC) $(MODCFLAGS) -c drvr236.c

cd236.o:		cd236.c ip236.h
			$(CC) $(MODCFLAGS) -c cd236.c

cnfg236.o:		cnfg236.c ip236.h
			$(CC) $(MODCFLAGS) -c cnfg236.c

rmid236.o:		rmid236.c ip236.h
			$(CC) $(MODCFLAGS) -c rmid236.c

wro236.o:		wro236.c ip236.h
			$(CC) $(MODCFLAGS) -c wro236.c

rwcc236.o:		rwcc236.c ip236.h
			$(CC) $(MODCFLAGS) -c rwcc236.c
clean:
			\rm -f *.o *~ a.out

