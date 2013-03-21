
MODCFLAGS := -Wall

a.out:			carrier.o cnfg409.o drvr409.o rpnt409.o rprt409.o\
				rsts409.o wpnt409.o wprt409.o
			cc -o a.out carrier.o cnfg409.o drvr409.o\
				rpnt409.o rprt409.o rsts409.o wpnt409.o wprt409.o

carrier.o:		../carrier/carrier.c ../carrier/carrier.h
			$(CC) $(MODCFLAGS) -c ../carrier/carrier.c

drvr409.o:		drvr409.c ip409.h
			$(CC) $(MODCFLAGS) -c drvr409.c

cnfg409.o:		cnfg409.c ip409.h
			$(CC) $(MODCFLAGS) -c cnfg409.c

rpnt409.o:		rpnt409.c ip409.h
			$(CC) $(MODCFLAGS) -c rpnt409.c

rprt409.o:		rprt409.c ip409.h
			$(CC) $(MODCFLAGS) -c rprt409.c

rsts409.o:		rsts409.c ip409.h
			$(CC) $(MODCFLAGS) -c rsts409.c

wpnt409.o:		wpnt409.c ip409.h
			$(CC) $(MODCFLAGS) -c wpnt409.c

wprt409.o:		wprt409.c ip409.h
			$(CC) $(MODCFLAGS) -c wprt409.c

clean:
			\rm -f *.o *~ a.out

