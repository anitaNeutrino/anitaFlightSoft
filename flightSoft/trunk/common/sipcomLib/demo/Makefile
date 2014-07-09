CFLAGS = -g -I..
LIBS = -L.. -lsipcom -lpthread

OBJS = rand_no.o
PROGS = rddir rdstdin driver

all: $(PROGS)

rddir:	rddir.c ../libsipcom.a
	gcc $(CFLAGS) -o rddir rddir.c $(LIBS)

rdstdin:	rdstdin.c ../libsipcom.a
	gcc $(CFLAGS) -o rdstdin rdstdin.c $(LIBS)

driver:	driver.c ../libsipcom.a $(OBJS)
	gcc $(CFLAGS) -o driver driver.c $(OBJS) $(LIBS)


rand_no.o:	rand_no.h

clean:
	rm -f $(PROGS) $(OBJS)
