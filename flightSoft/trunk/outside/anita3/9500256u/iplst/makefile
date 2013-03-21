
MODCFLAGS := -Wall

a.out:		carrier.o drvrlst.o
			cc -o a.out carrier.o drvrlst.o

carrier.o:		../carrier/carrier.c ../carrier/carrier.h
			$(CC) $(MODCFLAGS) -c ../carrier/carrier.c

drvrlst.o:		drvrlst.c iplst.h
			$(CC) $(MODCFLAGS) -c drvrlst.c

clean:
			\rm -f *.o *~ a.out

