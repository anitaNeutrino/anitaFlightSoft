
MODCFLAGS := -Wall

LIBS :=./library/PENTIUMgnu/Acr1553_IOSx86lnx.a -lpthread

a.out:		ioscarrier.o drvr570.o acetest.o bcdbuf.o bcdemo.o \
		looptest.o mtirq.o mtpoll.o rtdbuf.o rtirq.o rtmode.o \
		rtmtCmbdemo.o rtmtdemo.o rtpoll.o

		cc -o a.out ioscarrier.o drvr570.o acetest.o bcdbuf.o bcdemo.o \
		looptest.o mtirq.o mtpoll.o rtdbuf.o rtirq.o rtmode.o \
		rtmtCmbdemo.o rtmtdemo.o rtpoll.o $(LIBS)

ioscarrier.o:	../ioscarrier/ioscarrier.c ../ioscarrier/ioscarrier.h
		$(CC) $(MODCFLAGS) -c ../ioscarrier/ioscarrier.c

drvr570.o:	drvr570.c ios570.h
		$(CC) $(MODCFLAGS) -c drvr570.c

acetest.o:	acetest.c ios570.h
		$(CC) $(MODCFLAGS) -c acetest.c

bcdbuf.o:	bcdbuf.c ios570.h
		$(CC) $(MODCFLAGS) -c bcdbuf.c

bcdemo.o:	bcdemo.c ios570.h
		$(CC) $(MODCFLAGS) -c bcdemo.c

looptest.o:	looptest.c ios570.h
		$(CC) $(MODCFLAGS) -c looptest.c

mtirq.o:	mtirq.c ios570.h
		$(CC) $(MODCFLAGS) -c mtirq.c

mtpoll.o:	mtpoll.c ios570.h
		$(CC) $(MODCFLAGS) -c mtpoll.c

rtdbuf.o:	rtdbuf.c ios570.h
		$(CC) $(MODCFLAGS) -c rtdbuf.c

rtirq.o:	rtirq.c ios570.h
		$(CC) $(MODCFLAGS) -c rtirq.c

rtmode.o:	rtmode.c ios570.h
		$(CC) $(MODCFLAGS) -c rtmode.c

rtmtCmbdemo.o:	rtmtCmbdemo.c ios570.h
		$(CC) $(MODCFLAGS) -c rtmtCmbdemo.c

rtmtdemo.o:	rtmtdemo.c ios570.h
		$(CC) $(MODCFLAGS) -c rtmtdemo.c

rtpoll.o:	rtpoll.c ios570.h
		$(CC) $(MODCFLAGS) -c rtpoll.c

clean:
		\rm -f *.o *~ a.out

