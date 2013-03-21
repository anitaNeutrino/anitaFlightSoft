
MODCFLAGS := -Wall

a.out:			carrier.o cnfg1k110.o drvr1k110.o rpnt1k110.o rprt1k110.o\
				rsts1k110.o wpnt1k110.o wprt1k110.o pldcfg1k110.o
			cc -o a.out carrier.o cnfg1k110.o drvr1k110.o rpnt1k110.o rprt1k110.o\
				rsts1k110.o wpnt1k110.o wprt1k110.o pldcfg1k110.o

carrier.o:		../carrier/carrier.c ../carrier/carrier.h
			$(CC) $(MODCFLAGS) -c ../carrier/carrier.c

drvr1k110.o:		drvr1k110.c ip1k110.h
			$(CC) $(MODCFLAGS) -c drvr1k110.c

cnfg1k110.o:		cnfg1k110.c ip1k110.h
			$(CC) $(MODCFLAGS) -c cnfg1k110.c

pldcfg1k110.o:		pldcfg1k110.c ip1k110.h
			$(CC) $(MODCFLAGS) -c pldcfg1k110.c

rpnt1k110.o:		rpnt1k110.c ip1k110.h
			$(CC) $(MODCFLAGS) -c rpnt1k110.c

rprt1k110.o:		rprt1k110.c ip1k110.h
			$(CC) $(MODCFLAGS) -c rprt1k110.c

rsts1k110.o:		rsts1k110.c ip1k110.h
			$(CC) $(MODCFLAGS) -c rsts1k110.c

wpnt1k110.o:		wpnt1k110.c ip1k110.h
			$(CC) $(MODCFLAGS) -c wpnt1k110.c

wprt1k110.o:		wprt1k110.c ip1k110.h
			$(CC) $(MODCFLAGS) -c wprt1k110.c

clean:
			\rm -f *.o *~ a.out

