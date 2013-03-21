
a.out:		hfilegenerator.o
			cc -o a.out hfilegenerator.o

hfilegenerator.o:	hfilegenerator.c
			$(CC) $(MODCFLAGS) -c hfilegenerator.c

clean:
		\rm -f *.o *~ a.out
