
MODCFLAGS := -Wall

LIBS :=./library/PENTIUMgnu/Acr1553_IPx86lnx.a -lpthread

a.out:		carrier.o drvr570.o acetest.o bcdbuf.o bcdemo.o \
		looptest.o mtirq.o mtpoll.o rtdbuf.o rtirq.o rtmode.o \
		rtmtCmbdemo.o rtmtdemo.o rtpoll.o

		cc -o a.out carrier.o drvr570.o acetest.o bcdbuf.o bcdemo.o \
		looptest.o mtirq.o mtpoll.o rtdbuf.o rtirq.o rtmode.o \
		rtmtCmbdemo.o rtmtdemo.o rtpoll.o $(LIBS)

carrier.o:	../carrier/carrier.c ../carrier/carrier.h
		$(CC) $(MODCFLAGS) -c ../carrier/carrier.c

drvr570.o:	drvr570.c ip570.h
		$(CC) $(MODCFLAGS) -c drvr570.c

acetest.o:	acetest.c ip570.h
		$(CC) $(MODCFLAGS) -c acetest.c

bcdbuf.o:	bcdbuf.c ip570.h
		$(CC) $(MODCFLAGS) -c bcdbuf.c

bcdemo.o:	bcdemo.c ip570.h
		$(CC) $(MODCFLAGS) -c bcdemo.c

looptest.o:	looptest.c ip570.h
		$(CC) $(MODCFLAGS) -c looptest.c

mtirq.o:	mtirq.c ip570.h
		$(CC) $(MODCFLAGS) -c mtirq.c

mtpoll.o:	mtpoll.c ip570.h
		$(CC) $(MODCFLAGS) -c mtpoll.c

rtdbuf.o:	rtdbuf.c ip570.h
		$(CC) $(MODCFLAGS) -c rtdbuf.c

rtirq.o:	rtirq.c ip570.h
		$(CC) $(MODCFLAGS) -c rtirq.c

rtmode.o:	rtmode.c ip570.h
		$(CC) $(MODCFLAGS) -c rtmode.c

rtmtCmbdemo.o:	rtmtCmbdemo.c ip570.h
		$(CC) $(MODCFLAGS) -c rtmtCmbdemo.c

rtmtdemo.o:	rtmtdemo.c ip570.h
		$(CC) $(MODCFLAGS) -c rtmtdemo.c

rtpoll.o:	rtpoll.c ip570.h
		$(CC) $(MODCFLAGS) -c rtpoll.c

clean:
		\rm -f *.o *~ a.out

