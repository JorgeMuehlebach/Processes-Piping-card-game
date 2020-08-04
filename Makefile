
.PHONY: all clean
.DEFAULT_GOAL := all

OBJECTS = tester.c

all: 2310hub 2310alice 2310bob

2310hub: 2310hub.c tester.c
	gcc -Wall -pedantic -std=gnu99 $(OBJECTS) 2310hub.c -o 2310hub

2310alice: 2310alice.c tester.c
	gcc -Wall -pedantic -std=gnu99 $(OBJECTS) 2310alice.c -o 2310alice
	  
2310bob: 2310bob.c tester.c
	gcc -Wall -pedantic -std=gnu99 $(OBJECTS) 2310bob.c -o 2310bob

clean:
	rm 2310alice 2310bob 2310hub










