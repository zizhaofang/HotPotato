all:ringmaster player

player:player.c potato.h
	g++ -o player player.c potato.h

ringmaster:ringmaster.c potato.h
	g++ -o  ringmaster ringmaster.c potato.h

.PHONY:
clean:
	rm -f *.o *~ player ringmaster