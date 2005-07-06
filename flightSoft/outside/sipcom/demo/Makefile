CFLAGS = -g -I..
LIBS = -L.. -lsipcom

OBJS = rand_no.o

driver:	driver.c ../libsipcom.a $(OBJS)
	gcc $(CFLAGS) -o driver driver.c $(OBJS) $(LIBS)


rand_no.o:	rand_no.h

clean:
	rm -f driver *.o
