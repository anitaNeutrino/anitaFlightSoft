
MODCFLAGS := -Wall

a.out:			ioscarrier.o cnfgep20x.o drvrep20x.o rpntep20x.o rprtep20x.o\
				rstsep20x.o wpntep20x.o wprtep20x.o pldcfgep20x.o
			cc -o a.out ioscarrier.o cnfgep20x.o drvrep20x.o rpntep20x.o rprtep20x.o\
				rstsep20x.o wpntep20x.o wprtep20x.o pldcfgep20x.o

ioscarrier.o:	../ioscarrier/ioscarrier.c ../ioscarrier/ioscarrier.h
			$(CC) $(MODCFLAGS) -c ../ioscarrier/ioscarrier.c

drvrep20x.o:		drvrep20x.c iosep20x.h
			$(CC) $(MODCFLAGS) -c drvrep20x.c

cnfgep20x.o:		cnfgep20x.c iosep20x.h
			$(CC) $(MODCFLAGS) -c cnfgep20x.c

pldcfgep20x.o:		pldcfgep20x.c iosep20x.h
			$(CC) $(MODCFLAGS) -c pldcfgep20x.c

rpntep20x.o:		rpntep20x.c iosep20x.h
			$(CC) $(MODCFLAGS) -c rpntep20x.c

rprtep20x.o:		rprtep20x.c iosep20x.h
			$(CC) $(MODCFLAGS) -c rprtep20x.c

rstsep20x.o:		rstsep20x.c iosep20x.h
			$(CC) $(MODCFLAGS) -c rstsep20x.c

wpntep20x.o:		wpntep20x.c iosep20x.h
			$(CC) $(MODCFLAGS) -c wpntep20x.c

wprtep20x.o:		wprtep20x.c iosep20x.h
			$(CC) $(MODCFLAGS) -c wprtep20x.c

clean:
			\rm -f *.o *~ a.out

