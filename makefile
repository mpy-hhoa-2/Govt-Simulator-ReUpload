CC = gcc
CFLAGS = -pthread -std=gnu99
DEPS = prensa.h rwoper.h
OBJ = exec.o legis.o judi.o

paisProcesos: prensa.c $(OBJ)
	$(CC) -o $@ $< $(CFLAGS)

exec.o: exec.c rwoper.c
	$(CC) -o $@ $^ $(CFLAGS)

legis.o: legis.c rwoper.c
	$(CC) -o $@ $^ $(CFLAGS)

judi.o: judi.c rwoper.c
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean

clean: 
	rm paisProcesos
	rm *.o

.PHONY: cleanPipes

cleanPipes:
	rm /tmp/GovtPress
	rm /tmp/Press*
	rm /tmp/Exec*
	rm /tmp/Jud*
	rm /tmp/Leg*